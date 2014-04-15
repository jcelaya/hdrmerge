/*****************************************************************************/
// Copyright 2006-2012 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in
// accordance with the terms of the Adobe license agreement accompanying it.
/*****************************************************************************/

/* $Id: //mondo/dng_sdk_1_4/dng_sdk/source/dng_image_writer.cpp#4 $ */ 
/* $DateTime: 2012/06/14 20:24:41 $ */
/* $Change: 835078 $ */
/* $Author: tknoll $ */

/*****************************************************************************/

#include "dng_image_writer.h"

#include "dng_abort_sniffer.h"
#include "dng_area_task.h"
#include "dng_bottlenecks.h"
#include "dng_camera_profile.h"
#include "dng_color_space.h"
#include "dng_exif.h"
#include "dng_flags.h"
#include "dng_exceptions.h"
#include "dng_host.h"
#include "dng_ifd.h"
#include "dng_image.h"
#include "dng_jpeg_image.h"
#include "dng_lossless_jpeg.h"
#include "dng_memory_stream.h"
#include "dng_negative.h"
#include "dng_pixel_buffer.h"
#include "dng_preview.h"
#include "dng_read_image.h"
#include "dng_stream.h"
#include "dng_string_list.h"
#include "dng_tag_codes.h"
#include "dng_tag_values.h"
#include "dng_utils.h"
#include "dng_xmp.h"

#include "zlib.h"

#if qDNGUseLibJPEG
#include "jpeglib.h"
#include "jerror.h"
#endif
	
/*****************************************************************************/

// Defines for testing DNG 1.2 features.

//#define qTestRowInterleave 2

//#define qTestSubTileBlockRows 2
//#define qTestSubTileBlockCols 2

/*****************************************************************************/

dng_resolution::dng_resolution ()

	:	fXResolution ()
	,	fYResolution ()
	
	,	fResolutionUnit (0)
	
	{
	
	}

/******************************************************************************/

static void SpoolAdobeData (dng_stream &stream,
							const dng_metadata *metadata,
							const dng_jpeg_preview *preview,
							const dng_memory_block *imageResources)
	{
	
	TempBigEndian tempEndian (stream);
	
	if (metadata && metadata->GetXMP ())
		{
		
		bool marked = false;
		
		if (metadata->GetXMP ()->GetBoolean (XMP_NS_XAP_RIGHTS,
											 "Marked",
											 marked))
			{
			
			stream.Put_uint32 (DNG_CHAR4 ('8','B','I','M'));
			stream.Put_uint16 (1034);
			stream.Put_uint16 (0);
			
			stream.Put_uint32 (1);
			
			stream.Put_uint8 (marked ? 1 : 0);
			
			stream.Put_uint8 (0);
			
			}
			
		dng_string webStatement;
		
		if (metadata->GetXMP ()->GetString (XMP_NS_XAP_RIGHTS,
											"WebStatement",
											webStatement))
			{
			
			dng_memory_data buffer;
			
			uint32 size = webStatement.Get_SystemEncoding (buffer);
			
			if (size > 0)
				{
				
				stream.Put_uint32 (DNG_CHAR4 ('8','B','I','M'));
				stream.Put_uint16 (1035);
				stream.Put_uint16 (0);
				
				stream.Put_uint32 (size);
				
				stream.Put (buffer.Buffer (), size);
				
				if (size & 1)
					stream.Put_uint8 (0);
					
				}
			
			}
		
		}
	
	if (preview)
		{
		
		preview->SpoolAdobeThumbnail (stream);
				
		}
		
	if (metadata && metadata->IPTCLength ())
		{
		
		dng_fingerprint iptcDigest = metadata->IPTCDigest ();

		if (iptcDigest.IsValid ())
			{
			
			stream.Put_uint32 (DNG_CHAR4 ('8','B','I','M'));
			stream.Put_uint16 (1061);
			stream.Put_uint16 (0);
			
			stream.Put_uint32 (16);
			
			stream.Put (iptcDigest.data, 16);
			
			}
			
		}
		
	if (imageResources)
		{
		
		uint32 size = imageResources->LogicalSize ();
		
		stream.Put (imageResources->Buffer (), size);
		
		if (size & 1)
			stream.Put_uint8 (0);
		
		}
		
	}

/******************************************************************************/

static dng_memory_block * BuildAdobeData (dng_host &host,
										  const dng_metadata *metadata,
										  const dng_jpeg_preview *preview,
										  const dng_memory_block *imageResources)
	{
	
	dng_memory_stream stream (host.Allocator ());
	
	SpoolAdobeData (stream,
					metadata,
					preview,
					imageResources);
					
	return stream.AsMemoryBlock (host.Allocator ());
	
	}

/*****************************************************************************/

tag_string::tag_string (uint16 code,
				    	const dng_string &s,
				    	bool forceASCII)
				    	
	:	tiff_tag (code, ttAscii, 0)
	
	,	fString (s)
	
	{
			
	if (forceASCII)
		{
		
		// Metadata working group recommendation - go ahead
		// write UTF-8 into ASCII tag strings, rather than
		// actually force the strings to ASCII.  There is a matching
		// change on the reading side to assume UTF-8 if the string
		// contains a valid UTF-8 string.
		//
		// fString.ForceASCII ();
		
		}
		
	else if (!fString.IsASCII ())
		{
		
		fType = ttByte;
		
		}
		
	fCount = fString.Length () + 1;
	
	}

/*****************************************************************************/

void tag_string::Put (dng_stream &stream) const
	{
	
	stream.Put (fString.Get (), Size ());
	
	}

/*****************************************************************************/

tag_encoded_text::tag_encoded_text (uint16 code,
									const dng_string &text)
	
	:	tiff_tag (code, ttUndefined, 0)
	
	,	fText (text)
	
	,	fUTF16 ()
	
	{
			
	if (fText.IsASCII ())
		{
	
		fCount = 8 + fText.Length ();
		
		}
		
	else
		{
		
		fCount = 8 + fText.Get_UTF16 (fUTF16) * 2;
		
		}
	
	}

/*****************************************************************************/

void tag_encoded_text::Put (dng_stream &stream) const
	{
	
	if (fUTF16.Buffer ())
		{
		
		stream.Put ("UNICODE\000", 8);
		
		uint32 chars = (fCount - 8) >> 1;
		
		const uint16 *buf = fUTF16.Buffer_uint16 ();
		
		for (uint32 j = 0; j < chars; j++)
			{
			
			stream.Put_uint16 (buf [j]);
			
			}
		
		}
		
	else
		{
		
		stream.Put ("ASCII\000\000\000", 8);
		
		stream.Put (fText.Get (), fCount - 8);
		
		}
		
	}
	
/*****************************************************************************/

void tag_data_ptr::Put (dng_stream &stream) const
	{
	
	// If we are swapping bytes, we need to swap with the right size
	// entries.
	
	if (stream.SwapBytes ())
		{
	
		switch (Type ())
			{
			
			// Two byte entries.
			
			case ttShort:
			case ttSShort:
			case ttUnicode:
				{
				
				const uint16 *p = (const uint16 *) fData;
				
				uint32 entries = (Size () >> 1);
				
				for (uint32 j = 0; j < entries; j++)
					{
					
					stream.Put_uint16 (p [j]);
					
					}
				
				return;
				
				}
			
			// Four byte entries.
			
			case ttLong:
			case ttSLong:
			case ttRational:
			case ttSRational:
			case ttIFD:
			case ttFloat:
			case ttComplex:
				{
				
				const uint32 *p = (const uint32 *) fData;
				
				uint32 entries = (Size () >> 2);
				
				for (uint32 j = 0; j < entries; j++)
					{
					
					stream.Put_uint32 (p [j]);
					
					}
				
				return;
				
				}
				
			// Eight byte entries.
			
			case ttDouble:
				{
				
				const real64 *p = (const real64 *) fData;
				
				uint32 entries = (Size () >> 3);
				
				for (uint32 j = 0; j < entries; j++)
					{
					
					stream.Put_real64 (p [j]);
					
					}
				
				return;
				
				}
			
			// Entries don't need to be byte swapped.  Fall through
			// to non-byte swapped case.
				
			default:
				{
				
				break;
				
				}

			}
			
		}
		
	// Non-byte swapped case.
		
	stream.Put (fData, Size ());
				
	}

/******************************************************************************/

tag_matrix::tag_matrix (uint16 code,
		    			const dng_matrix &m)
		    	   
	:	tag_srational_ptr (code, fEntry, m.Rows () * m.Cols ())
	
	{
	
	uint32 index = 0;
	
	for (uint32 r = 0; r < m.Rows (); r++)
		for (uint32 c = 0; c < m.Cols (); c++)
			{
			
			fEntry [index].Set_real64 (m [r] [c], 10000);
			
			index++;
			
			}
	
	}

/******************************************************************************/

tag_icc_profile::tag_icc_profile (const void *profileData,
								  uint32 profileSize)
			
	:	tag_data_ptr (tcICCProfile, 
					  ttUndefined,
					  0,
					  NULL)
	
	{
	
	if (profileData && profileSize)
		{
		
		SetCount (profileSize);
		SetData  (profileData);
		
		}
	
	}
			
/******************************************************************************/

void tag_cfa_pattern::Put (dng_stream &stream) const
	{
	
	stream.Put_uint16 ((uint16) fCols);
	stream.Put_uint16 ((uint16) fRows);
	
	for (uint32 col = 0; col < fCols; col++)
		for (uint32 row = 0; row < fRows; row++)
			{
			
			stream.Put_uint8 (fPattern [row * kMaxCFAPattern + col]);
			
			}

	}

/******************************************************************************/

tag_exif_date_time::tag_exif_date_time (uint16 code,
		            					const dng_date_time &dt)
		    	   
	:	tag_data_ptr (code, ttAscii, 20, fData)
	
	{
	
	if (dt.IsValid ())
		{
	
		sprintf (fData,
				 "%04d:%02d:%02d %02d:%02d:%02d",
				 (int) dt.fYear,
				 (int) dt.fMonth,
				 (int) dt.fDay,
				 (int) dt.fHour,
				 (int) dt.fMinute,
				 (int) dt.fSecond);
				 
		}
	
	}

/******************************************************************************/

tag_iptc::tag_iptc (const void *data,
		  			uint32 length)
	
	:	tiff_tag (tcIPTC_NAA, ttLong, (length + 3) >> 2)
	
	,	fData   (data  )
	,	fLength (length)
	
	{
	
	}

/******************************************************************************/

void tag_iptc::Put (dng_stream &stream) const
	{
	
	// Note: For historical compatiblity reasons, the standard TIFF data 
	// type for IPTC data is ttLong, but without byte swapping.  This really
	// should be ttUndefined, but doing the right thing would break some
	// existing readers.
	
	stream.Put (fData, fLength);
	
	// Pad with zeros to get to long word boundary.
	
	uint32 extra = fCount * 4 - fLength;
	
	while (extra--)
		{
		stream.Put_uint8 (0);
		}

	}

/******************************************************************************/

tag_xmp::tag_xmp (const dng_xmp *xmp)
	
	:	tag_uint8_ptr (tcXMP, NULL, 0)
	
	,	fBuffer ()
	
	{
	
	if (xmp)
		{
		
		fBuffer.Reset (xmp->Serialize (true));
		
		if (fBuffer.Get ())
			{
			
			SetData (fBuffer->Buffer_uint8 ());
			
			SetCount (fBuffer->LogicalSize ());
			
			}
		
		}
	
	}

/******************************************************************************/

void dng_tiff_directory::Add (const tiff_tag *tag)
	{
	
	if (fEntries >= kMaxEntries)
		{
		ThrowProgramError ();
		}
	
	// Tags must be sorted in increasing order of tag code.
	
	uint32 index = fEntries;
	
	for (uint32 j = 0; j < fEntries; j++)
		{
		
		if (tag->Code () < fTag [j]->Code ())
			{
			index = j;
			break;
			}
		
		}
		
	for (uint32 k = fEntries; k > index; k--)
		{
		
		fTag [k] = fTag [k - 1];
		
		}
	
	fTag [index] = tag;
	
	fEntries++;
	
	}
		
/******************************************************************************/

uint32 dng_tiff_directory::Size () const
	{
	
	if (!fEntries) return 0;
	
	uint32 size = fEntries * 12 + 6;
	
	for (uint32 index = 0; index < fEntries; index++)
		{
		
		uint32 tagSize = fTag [index]->Size ();
		
		if (tagSize > 4)
			{
			
			size += (tagSize + 1) & ~1;
			
			}
		
		}
		
	return size;
	
	}
		
/******************************************************************************/

void dng_tiff_directory::Put (dng_stream &stream,
						      OffsetsBase offsetsBase,
						      uint32 explicitBase) const
	{
	
	if (!fEntries) return;
	
	uint32 index;
	
	uint32 bigData = fEntries * 12 + 6;
	
	if (offsetsBase == offsetsRelativeToStream)
		bigData += (uint32) stream.Position ();

	else if (offsetsBase == offsetsRelativeToExplicitBase)
		bigData += explicitBase;

	stream.Put_uint16 ((uint16) fEntries);
	
	for (index = 0; index < fEntries; index++)
		{
		
		const tiff_tag &tag = *fTag [index];
		
		stream.Put_uint16 (tag.Code  ());
		stream.Put_uint16 (tag.Type  ());
		stream.Put_uint32 (tag.Count ());
		
		uint32 size = tag.Size ();
		
		if (size <= 4)
			{
			
			tag.Put (stream);
			
			while (size < 4)
				{
				stream.Put_uint8 (0);
				size++;
				}
				
			}
			
		else
			{
			
			stream.Put_uint32 (bigData);
			
			bigData += (size + 1) & ~1;
						
			}
		
		}
		
	stream.Put_uint32 (fChained);		// Next IFD offset
	
	for (index = 0; index < fEntries; index++)
		{
		
		const tiff_tag &tag = *fTag [index];
		
		uint32 size = tag.Size ();
		
		if (size > 4)
			{
			
			tag.Put (stream);
			
			if (size & 1)
				stream.Put_uint8 (0);
			
			}
		
		}
	
	}

/******************************************************************************/

dng_basic_tag_set::dng_basic_tag_set (dng_tiff_directory &directory,
									  const dng_ifd &info)
					 	  
	:	fNewSubFileType (tcNewSubFileType, info.fNewSubFileType)
	
	,	fImageWidth  (tcImageWidth , info.fImageWidth )
	,	fImageLength (tcImageLength, info.fImageLength)
	
	,	fPhotoInterpretation (tcPhotometricInterpretation,
							  (uint16) info.fPhotometricInterpretation)
	
	,	fFillOrder (tcFillOrder, 1)
	
	,	fSamplesPerPixel (tcSamplesPerPixel, (uint16) info.fSamplesPerPixel)
	
	,	fBitsPerSample (tcBitsPerSample,
						fBitsPerSampleData,
						info.fSamplesPerPixel)
						
	,	fStrips (info.fUsesStrips)
						
	,	fTileWidth (tcTileWidth, info.fTileWidth)
	
	,	fTileLength (fStrips ? tcRowsPerStrip : tcTileLength, 
					 info.fTileLength)
	
	,	fTileInfoBuffer (info.TilesPerImage () * 8)
	
	,	fTileOffsetData (fTileInfoBuffer.Buffer_uint32 ())
	
	,	fTileOffsets (fStrips ? tcStripOffsets : tcTileOffsets,
					  fTileOffsetData,
					  info.TilesPerImage ())
					   
	,	fTileByteCountData (fTileOffsetData + info.TilesPerImage ())
	
	,	fTileByteCounts (fStrips ? tcStripByteCounts : tcTileByteCounts,
						 fTileByteCountData,
						 info.TilesPerImage ())
						  
	,	fPlanarConfiguration (tcPlanarConfiguration, pcInterleaved)
	
	,	fCompression (tcCompression, (uint16) info.fCompression)
	,	fPredictor   (tcPredictor  , (uint16) info.fPredictor  )
	
	,	fExtraSamples (tcExtraSamples,
					   fExtraSamplesData,
					   info.fExtraSamplesCount)
					   
	,	fSampleFormat (tcSampleFormat,
					   fSampleFormatData,
					   info.fSamplesPerPixel)
					   
	,	fRowInterleaveFactor (tcRowInterleaveFactor,
							  (uint16) info.fRowInterleaveFactor)
							  
	,	fSubTileBlockSize (tcSubTileBlockSize,
						   fSubTileBlockSizeData,
						   2)
								 
	{
	
	uint32 j;
	
	for (j = 0; j < info.fSamplesPerPixel; j++)
		{
	
		fBitsPerSampleData [j] = (uint16) info.fBitsPerSample [0];
		
		}
	
	directory.Add (&fNewSubFileType);
	
	directory.Add (&fImageWidth);
	directory.Add (&fImageLength);
	
	directory.Add (&fPhotoInterpretation);
	
	directory.Add (&fSamplesPerPixel);
	
	directory.Add (&fBitsPerSample);
	
	if (info.fBitsPerSample [0] !=  8 &&
	    info.fBitsPerSample [0] != 16 &&
	    info.fBitsPerSample [0] != 32)
		{
	
		directory.Add (&fFillOrder);
		
		}
		
	if (!fStrips)
		{
		
		directory.Add (&fTileWidth);
	
		}

	directory.Add (&fTileLength);
	
	directory.Add (&fTileOffsets);
	directory.Add (&fTileByteCounts);
	
	directory.Add (&fPlanarConfiguration);
	
	directory.Add (&fCompression);
	
	if (info.fPredictor != cpNullPredictor)
		{
		
		directory.Add (&fPredictor);
		
		}
		
	if (info.fExtraSamplesCount != 0)
		{
		
		for (j = 0; j < info.fExtraSamplesCount; j++)
			{
			fExtraSamplesData [j] = (uint16) info.fExtraSamples [j];
			}
			
		directory.Add (&fExtraSamples);
		
		}
		
	if (info.fSampleFormat [0] != sfUnsignedInteger)
		{
		
		for (j = 0; j < info.fSamplesPerPixel; j++)
			{
			fSampleFormatData [j] = (uint16) info.fSampleFormat [j];
			}
			
		directory.Add (&fSampleFormat);
		
		}
		
	if (info.fRowInterleaveFactor != 1)
		{
		
		directory.Add (&fRowInterleaveFactor);
		
		}
		
	if (info.fSubTileBlockRows != 1 ||
		info.fSubTileBlockCols != 1)
		{
		
		fSubTileBlockSizeData [0] = (uint16) info.fSubTileBlockRows;
		fSubTileBlockSizeData [1] = (uint16) info.fSubTileBlockCols;
		
		directory.Add (&fSubTileBlockSize);
		
		}
	
	}

/******************************************************************************/

exif_tag_set::exif_tag_set (dng_tiff_directory &directory,
					  		const dng_exif &exif,
							bool makerNoteSafe,
							const void *makerNoteData,
							uint32 makerNoteLength,
					 	    bool insideDNG)

	:	fExifIFD ()
	,	fGPSIFD  ()
	
	,	fExifLink (tcExifIFD, 0)
	,	fGPSLink  (tcGPSInfo, 0)
	
	,	fAddedExifLink (false)
	,	fAddedGPSLink  (false)
	
	,	fExifVersion (tcExifVersion, ttUndefined, 4, fExifVersionData)
	
	,	fExposureTime      (tcExposureTime     , exif.fExposureTime     )
	,	fShutterSpeedValue (tcShutterSpeedValue, exif.fShutterSpeedValue)
	
	,	fFNumber 	   (tcFNumber      , exif.fFNumber      )
	,	fApertureValue (tcApertureValue, exif.fApertureValue)
	
	,	fBrightnessValue (tcBrightnessValue, exif.fBrightnessValue)
	
	,	fExposureBiasValue (tcExposureBiasValue, exif.fExposureBiasValue)
	
	,	fMaxApertureValue (tcMaxApertureValue , exif.fMaxApertureValue)
	
	,	fSubjectDistance (tcSubjectDistance, exif.fSubjectDistance)
	
	,	fFocalLength (tcFocalLength, exif.fFocalLength)
	
	// Special case: the EXIF 2.2 standard represents ISO speed ratings with 2 bytes,
	// which cannot hold ISO speed ratings above 65535 (e.g., 102400). In these
	// cases, we write the maximum representable ISO speed rating value in the EXIF
	// tag, i.e., 65535.

	,	fISOSpeedRatings (tcISOSpeedRatings, 
						  (uint16) Min_uint32 (65535, 
											   exif.fISOSpeedRatings [0]))

	,	fSensitivityType (tcSensitivityType, (uint16) exif.fSensitivityType)

	,	fStandardOutputSensitivity (tcStandardOutputSensitivity, exif.fStandardOutputSensitivity)
	
	,	fRecommendedExposureIndex (tcRecommendedExposureIndex, exif.fRecommendedExposureIndex)

	,	fISOSpeed (tcISOSpeed, exif.fISOSpeed)

	,	fISOSpeedLatitudeyyy (tcISOSpeedLatitudeyyy, exif.fISOSpeedLatitudeyyy)

	,	fISOSpeedLatitudezzz (tcISOSpeedLatitudezzz, exif.fISOSpeedLatitudezzz)

	,	fFlash (tcFlash, (uint16) exif.fFlash)
	
	,	fExposureProgram (tcExposureProgram, (uint16) exif.fExposureProgram)
	
	,	fMeteringMode (tcMeteringMode, (uint16) exif.fMeteringMode)
	
	,	fLightSource (tcLightSource, (uint16) exif.fLightSource)
	
	,	fSensingMethod (tcSensingMethodExif, (uint16) exif.fSensingMethod)
	
	,	fFocalLength35mm (tcFocalLengthIn35mmFilm, (uint16) exif.fFocalLengthIn35mmFilm)
	
	,	fFileSourceData ((uint8) exif.fFileSource)
	,	fFileSource     (tcFileSource, ttUndefined, 1, &fFileSourceData)

	,	fSceneTypeData ((uint8) exif.fSceneType)
	,	fSceneType     (tcSceneType, ttUndefined, 1, &fSceneTypeData)
	
	,	fCFAPattern (tcCFAPatternExif,
					 exif.fCFARepeatPatternRows,
					 exif.fCFARepeatPatternCols,
					 &exif.fCFAPattern [0] [0])
	
	,	fCustomRendered 	  (tcCustomRendered		 , (uint16) exif.fCustomRendered	  )
	,	fExposureMode 		  (tcExposureMode		 , (uint16) exif.fExposureMode		  )
	,	fWhiteBalance 		  (tcWhiteBalance		 , (uint16) exif.fWhiteBalance		  )
	,	fSceneCaptureType 	  (tcSceneCaptureType	 , (uint16) exif.fSceneCaptureType	  )
	,	fGainControl 		  (tcGainControl		 , (uint16) exif.fGainControl		  )
	,	fContrast 			  (tcContrast			 , (uint16) exif.fContrast			  )
	,	fSaturation 		  (tcSaturation			 , (uint16) exif.fSaturation		  )
	,	fSharpness 			  (tcSharpness			 , (uint16) exif.fSharpness			  )
	,	fSubjectDistanceRange (tcSubjectDistanceRange, (uint16) exif.fSubjectDistanceRange)
		
	,	fDigitalZoomRatio (tcDigitalZoomRatio, exif.fDigitalZoomRatio)
	
	,	fExposureIndex (tcExposureIndexExif, exif.fExposureIndex)
	
	,	fImageNumber (tcImageNumber, exif.fImageNumber)
	
	,	fSelfTimerMode (tcSelfTimerMode, (uint16) exif.fSelfTimerMode)
	
	,	fBatteryLevelA (tcBatteryLevel, exif.fBatteryLevelA)
	,	fBatteryLevelR (tcBatteryLevel, exif.fBatteryLevelR)
	
	,	fFocalPlaneXResolution (tcFocalPlaneXResolutionExif, exif.fFocalPlaneXResolution)
	,	fFocalPlaneYResolution (tcFocalPlaneYResolutionExif, exif.fFocalPlaneYResolution)
	
	,	fFocalPlaneResolutionUnit (tcFocalPlaneResolutionUnitExif, (uint16) exif.fFocalPlaneResolutionUnit)
	
	,	fSubjectArea (tcSubjectArea, fSubjectAreaData, exif.fSubjectAreaCount)

	,	fLensInfo (tcLensInfo, fLensInfoData, 4)
	
	,	fDateTime		   (tcDateTime		   , exif.fDateTime         .DateTime ())
	,	fDateTimeOriginal  (tcDateTimeOriginal , exif.fDateTimeOriginal .DateTime ())
	,	fDateTimeDigitized (tcDateTimeDigitized, exif.fDateTimeDigitized.DateTime ())
	
	,	fSubsecTime			 (tcSubsecTime, 		 exif.fDateTime         .Subseconds ())
	,	fSubsecTimeOriginal  (tcSubsecTimeOriginal,  exif.fDateTimeOriginal .Subseconds ())
	,	fSubsecTimeDigitized (tcSubsecTimeDigitized, exif.fDateTimeDigitized.Subseconds ())
	
	,	fMake (tcMake, exif.fMake)

	,	fModel (tcModel, exif.fModel)
	
	,	fArtist (tcArtist, exif.fArtist)
	
	,	fSoftware (tcSoftware, exif.fSoftware)
	
	,	fCopyright (tcCopyright, exif.fCopyright)
	
	,	fMakerNoteSafety (tcMakerNoteSafety, makerNoteSafe ? 1 : 0)
	
	,	fMakerNote (tcMakerNote, ttUndefined, makerNoteLength, makerNoteData)
					
	,	fImageDescription (tcImageDescription, exif.fImageDescription)
	
	,	fSerialNumber (tcCameraSerialNumber, exif.fCameraSerialNumber)
	
	,	fUserComment (tcUserComment, exif.fUserComment)
	
	,	fImageUniqueID (tcImageUniqueID, ttAscii, 33, fImageUniqueIDData)

	// EXIF 2.3 tags.

	,	fCameraOwnerName   (tcCameraOwnerNameExif,	  exif.fOwnerName		  )
	,	fBodySerialNumber  (tcCameraSerialNumberExif, exif.fCameraSerialNumber)
	,	fLensSpecification (tcLensSpecificationExif,  fLensInfoData, 4		  )
	,	fLensMake		   (tcLensMakeExif,			  exif.fLensMake		  )
	,	fLensModel		   (tcLensModelExif,		  exif.fLensName		  )
	,	fLensSerialNumber  (tcLensSerialNumberExif,	  exif.fLensSerialNumber  )

	,	fGPSVersionID (tcGPSVersionID, fGPSVersionData, 4)
	
	,	fGPSLatitudeRef (tcGPSLatitudeRef, exif.fGPSLatitudeRef)
	,	fGPSLatitude    (tcGPSLatitude,    exif.fGPSLatitude, 3)
	
	,	fGPSLongitudeRef (tcGPSLongitudeRef, exif.fGPSLongitudeRef)
	,	fGPSLongitude    (tcGPSLongitude,    exif.fGPSLongitude, 3)
	
	,	fGPSAltitudeRef (tcGPSAltitudeRef, (uint8) exif.fGPSAltitudeRef)
	,	fGPSAltitude    (tcGPSAltitude,            exif.fGPSAltitude   )
	
	,	fGPSTimeStamp (tcGPSTimeStamp, exif.fGPSTimeStamp, 3)
		
	,	fGPSSatellites  (tcGPSSatellites , exif.fGPSSatellites )
	,	fGPSStatus      (tcGPSStatus     , exif.fGPSStatus     )
	,	fGPSMeasureMode (tcGPSMeasureMode, exif.fGPSMeasureMode)
	
	,	fGPSDOP (tcGPSDOP, exif.fGPSDOP)
		
	,	fGPSSpeedRef (tcGPSSpeedRef, exif.fGPSSpeedRef)
	,	fGPSSpeed    (tcGPSSpeed   , exif.fGPSSpeed   )
		
	,	fGPSTrackRef (tcGPSTrackRef, exif.fGPSTrackRef)
	,	fGPSTrack    (tcGPSTrack   , exif.fGPSTrack   )
		
	,	fGPSImgDirectionRef (tcGPSImgDirectionRef, exif.fGPSImgDirectionRef)
	,	fGPSImgDirection    (tcGPSImgDirection   , exif.fGPSImgDirection   )
	
	,	fGPSMapDatum (tcGPSMapDatum, exif.fGPSMapDatum)
		
	,	fGPSDestLatitudeRef (tcGPSDestLatitudeRef, exif.fGPSDestLatitudeRef)
	,	fGPSDestLatitude    (tcGPSDestLatitude,    exif.fGPSDestLatitude, 3)
	
	,	fGPSDestLongitudeRef (tcGPSDestLongitudeRef, exif.fGPSDestLongitudeRef)
	,	fGPSDestLongitude    (tcGPSDestLongitude,    exif.fGPSDestLongitude, 3)
	
	,	fGPSDestBearingRef (tcGPSDestBearingRef, exif.fGPSDestBearingRef)
	,	fGPSDestBearing    (tcGPSDestBearing   , exif.fGPSDestBearing   )
		
	,	fGPSDestDistanceRef (tcGPSDestDistanceRef, exif.fGPSDestDistanceRef)
	,	fGPSDestDistance    (tcGPSDestDistance   , exif.fGPSDestDistance   )
		
	,	fGPSProcessingMethod (tcGPSProcessingMethod, exif.fGPSProcessingMethod)
	,	fGPSAreaInformation  (tcGPSAreaInformation , exif.fGPSAreaInformation )
	
	,	fGPSDateStamp (tcGPSDateStamp, exif.fGPSDateStamp)
	
	,	fGPSDifferential (tcGPSDifferential, (uint16) exif.fGPSDifferential)
		
	,	fGPSHPositioningError (tcGPSHPositioningError, exif.fGPSHPositioningError)
		
	{
	
	if (exif.fExifVersion)
		{
		
		fExifVersionData [0] = (uint8) (exif.fExifVersion >> 24);
		fExifVersionData [1] = (uint8) (exif.fExifVersion >> 16);
		fExifVersionData [2] = (uint8) (exif.fExifVersion >>  8);
		fExifVersionData [3] = (uint8) (exif.fExifVersion      );
		
		fExifIFD.Add (&fExifVersion);

		}
	
	if (exif.fExposureTime.IsValid ())
		{
		fExifIFD.Add (&fExposureTime);
		}
		
	if (exif.fShutterSpeedValue.IsValid ())
		{
		fExifIFD.Add (&fShutterSpeedValue);
		}
		
	if (exif.fFNumber.IsValid ())
		{
		fExifIFD.Add (&fFNumber);
		}
		
	if (exif.fApertureValue.IsValid ())
		{
		fExifIFD.Add (&fApertureValue);
		}
		
	if (exif.fBrightnessValue.IsValid ())
		{
		fExifIFD.Add (&fBrightnessValue);
		}
		
	if (exif.fExposureBiasValue.IsValid ())
		{
		fExifIFD.Add (&fExposureBiasValue);
		}
		
	if (exif.fMaxApertureValue.IsValid ())
		{
		fExifIFD.Add (&fMaxApertureValue);
		}
		
	if (exif.fSubjectDistance.IsValid ())
		{
		fExifIFD.Add (&fSubjectDistance);
		}
		
	if (exif.fFocalLength.IsValid ())
		{
		fExifIFD.Add (&fFocalLength);
		}
	
	if (exif.fISOSpeedRatings [0] != 0)
		{
		fExifIFD.Add (&fISOSpeedRatings);
		}
		
	if (exif.fFlash <= 0x0FFFF)
		{
		fExifIFD.Add (&fFlash);
		}
		
	if (exif.fExposureProgram <= 0x0FFFF)
		{
		fExifIFD.Add (&fExposureProgram);
		}
		
	if (exif.fMeteringMode <= 0x0FFFF)
		{
		fExifIFD.Add (&fMeteringMode);
		}
		
	if (exif.fLightSource <= 0x0FFFF)
		{
		fExifIFD.Add (&fLightSource);
		}
		
	if (exif.fSensingMethod <= 0x0FFFF)
		{
		fExifIFD.Add (&fSensingMethod);
		}
		
	if (exif.fFocalLengthIn35mmFilm != 0)
		{
		fExifIFD.Add (&fFocalLength35mm);
		}
		
	if (exif.fFileSource <= 0x0FF)
		{
		fExifIFD.Add (&fFileSource);
		}
		
	if (exif.fSceneType <= 0x0FF)
		{
		fExifIFD.Add (&fSceneType);
		}
		
	if (exif.fCFARepeatPatternRows &&
	    exif.fCFARepeatPatternCols)
		{
		fExifIFD.Add (&fCFAPattern);
		}
		
	if (exif.fCustomRendered <= 0x0FFFF)
		{
		fExifIFD.Add (&fCustomRendered);
		}
		
	if (exif.fExposureMode <= 0x0FFFF)
		{
		fExifIFD.Add (&fExposureMode);
		}
		
	if (exif.fWhiteBalance <= 0x0FFFF)
		{
		fExifIFD.Add (&fWhiteBalance);
		}
		
	if (exif.fSceneCaptureType <= 0x0FFFF)
		{
		fExifIFD.Add (&fSceneCaptureType);
		}
		
	if (exif.fGainControl <= 0x0FFFF)
		{
		fExifIFD.Add (&fGainControl);
		}
		
	if (exif.fContrast <= 0x0FFFF)
		{
		fExifIFD.Add (&fContrast);
		}
		
	if (exif.fSaturation <= 0x0FFFF)
		{
		fExifIFD.Add (&fSaturation);
		}
		
	if (exif.fSharpness <= 0x0FFFF)
		{
		fExifIFD.Add (&fSharpness);
		}
		
	if (exif.fSubjectDistanceRange <= 0x0FFFF)
		{
		fExifIFD.Add (&fSubjectDistanceRange);
		}
		
	if (exif.fDigitalZoomRatio.IsValid ())
		{
		fExifIFD.Add (&fDigitalZoomRatio);
		}
		
	if (exif.fExposureIndex.IsValid ())
		{
		fExifIFD.Add (&fExposureIndex);
		}
		
	if (insideDNG)	// TIFF-EP only tags
		{
		
		if (exif.fImageNumber != 0xFFFFFFFF)
			{
			directory.Add (&fImageNumber);
			}
			
		if (exif.fSelfTimerMode <= 0x0FFFF)
			{
			directory.Add (&fSelfTimerMode);
			}
			
		if (exif.fBatteryLevelA.NotEmpty ())
			{
			directory.Add (&fBatteryLevelA);
			}
			
		else if (exif.fBatteryLevelR.IsValid ())
			{
			directory.Add (&fBatteryLevelR);
			}
		
		}
		
	if (exif.fFocalPlaneXResolution.IsValid ())
		{
		fExifIFD.Add (&fFocalPlaneXResolution);
		}
	
	if (exif.fFocalPlaneYResolution.IsValid ())
		{
		fExifIFD.Add (&fFocalPlaneYResolution);
		}
	
	if (exif.fFocalPlaneResolutionUnit <= 0x0FFFF)
		{
		fExifIFD.Add (&fFocalPlaneResolutionUnit);
		}
		
	if (exif.fSubjectAreaCount)
		{
		
		fSubjectAreaData [0] = (uint16) exif.fSubjectArea [0];
		fSubjectAreaData [1] = (uint16) exif.fSubjectArea [1];
		fSubjectAreaData [2] = (uint16) exif.fSubjectArea [2];
		fSubjectAreaData [3] = (uint16) exif.fSubjectArea [3];
		
		fExifIFD.Add (&fSubjectArea);
		
		}
	
	if (exif.fLensInfo [0].IsValid () &&
		exif.fLensInfo [1].IsValid ())
		{
		
		fLensInfoData [0] = exif.fLensInfo [0];
		fLensInfoData [1] = exif.fLensInfo [1];
		fLensInfoData [2] = exif.fLensInfo [2];
		fLensInfoData [3] = exif.fLensInfo [3];

		if (insideDNG)
			{
			directory.Add (&fLensInfo);
			}
		
		}
		
	if (exif.fDateTime.IsValid ())
		{
		
		directory.Add (&fDateTime);
		
		if (exif.fDateTime.Subseconds ().NotEmpty ())
			{
			fExifIFD.Add (&fSubsecTime);
			}
		
		}
		
	if (exif.fDateTimeOriginal.IsValid ())
		{
		
		fExifIFD.Add (&fDateTimeOriginal);
		
		if (exif.fDateTimeOriginal.Subseconds ().NotEmpty ())
			{
			fExifIFD.Add (&fSubsecTimeOriginal);
			}
		
		}
		
	if (exif.fDateTimeDigitized.IsValid ())
		{
		
		fExifIFD.Add (&fDateTimeDigitized);
		
		if (exif.fDateTimeDigitized.Subseconds ().NotEmpty ())
			{
			fExifIFD.Add (&fSubsecTimeDigitized);
			}
		
		}
		
	if (exif.fMake.NotEmpty ())
		{
		directory.Add (&fMake);
		}
		
	if (exif.fModel.NotEmpty ())
		{
		directory.Add (&fModel);
		}
		
	if (exif.fArtist.NotEmpty ())
		{
		directory.Add (&fArtist);
		}
	
	if (exif.fSoftware.NotEmpty ())
		{
		directory.Add (&fSoftware);
		}
	
	if (exif.fCopyright.NotEmpty ())
		{
		directory.Add (&fCopyright);
		}
	
	if (exif.fImageDescription.NotEmpty ())
		{
		directory.Add (&fImageDescription);
		}
	
	if (exif.fCameraSerialNumber.NotEmpty () && insideDNG)
		{
		directory.Add (&fSerialNumber);
		}
		
	if (makerNoteSafe && makerNoteData)
		{
		
		directory.Add (&fMakerNoteSafety);
		
		fExifIFD.Add (&fMakerNote);
		
		}
		
	if (exif.fUserComment.NotEmpty ())
		{
		fExifIFD.Add (&fUserComment);
		}
		
	if (exif.fImageUniqueID.IsValid ())
		{
		
		for (uint32 j = 0; j < 16; j++)
			{
			
			sprintf (fImageUniqueIDData + j * 2,
					 "%02X",
					 (unsigned) exif.fImageUniqueID.data [j]);
					 
			}
		
		fExifIFD.Add (&fImageUniqueID);
		
		}

	if (exif.AtLeastVersion0230 ())
		{

		if (exif.fSensitivityType != 0)
			{
			
			fExifIFD.Add (&fSensitivityType);
			
			}

		// Sensitivity tags. Do not write these extra tags unless the SensitivityType
		// and PhotographicSensitivity (i.e., ISOSpeedRatings) values are valid.

		if (exif.fSensitivityType	  != 0 &&
			exif.fISOSpeedRatings [0] != 0)
			{

			// Standard Output Sensitivity (SOS).

			if (exif.fStandardOutputSensitivity != 0)
				{
				fExifIFD.Add (&fStandardOutputSensitivity);	
				}

			// Recommended Exposure Index (REI).

			if (exif.fRecommendedExposureIndex != 0)
				{
				fExifIFD.Add (&fRecommendedExposureIndex);
				}

			// ISO Speed.

			if (exif.fISOSpeed != 0)
				{

				fExifIFD.Add (&fISOSpeed);

				if (exif.fISOSpeedLatitudeyyy != 0 &&
					exif.fISOSpeedLatitudezzz != 0)
					{
						
					fExifIFD.Add (&fISOSpeedLatitudeyyy);
					fExifIFD.Add (&fISOSpeedLatitudezzz);
						
					}

				}

			}
		
		if (exif.fOwnerName.NotEmpty ())
			{
			fExifIFD.Add (&fCameraOwnerName);
			}
		
		if (exif.fCameraSerialNumber.NotEmpty ())
			{
			fExifIFD.Add (&fBodySerialNumber);
			}

		if (exif.fLensInfo [0].IsValid () &&
			exif.fLensInfo [1].IsValid ())
			{
			fExifIFD.Add (&fLensSpecification);
			}
		
		if (exif.fLensMake.NotEmpty ())
			{
			fExifIFD.Add (&fLensMake);
			}
		
		if (exif.fLensName.NotEmpty ())
			{
			fExifIFD.Add (&fLensModel);
			}
		
		if (exif.fLensSerialNumber.NotEmpty ())
			{
			fExifIFD.Add (&fLensSerialNumber);
			}
		
		}
		
	if (exif.fGPSVersionID)
		{
		
		fGPSVersionData [0] = (uint8) (exif.fGPSVersionID >> 24);
		fGPSVersionData [1] = (uint8) (exif.fGPSVersionID >> 16);
		fGPSVersionData [2] = (uint8) (exif.fGPSVersionID >>  8);
		fGPSVersionData [3] = (uint8) (exif.fGPSVersionID      );
		
		fGPSIFD.Add (&fGPSVersionID);
		
		}
		
	if (exif.fGPSLatitudeRef.NotEmpty () &&
		exif.fGPSLatitude [0].IsValid ())
		{
		fGPSIFD.Add (&fGPSLatitudeRef);
		fGPSIFD.Add (&fGPSLatitude   );
		}
		
	if (exif.fGPSLongitudeRef.NotEmpty () &&
		exif.fGPSLongitude [0].IsValid ())
		{
		fGPSIFD.Add (&fGPSLongitudeRef);
		fGPSIFD.Add (&fGPSLongitude   );
		}
		
	if (exif.fGPSAltitudeRef <= 0x0FF)
		{
		fGPSIFD.Add (&fGPSAltitudeRef);
		}
		
	if (exif.fGPSAltitude.IsValid ())
		{
		fGPSIFD.Add (&fGPSAltitude);
		}
		
	if (exif.fGPSTimeStamp [0].IsValid ())
		{
		fGPSIFD.Add (&fGPSTimeStamp);
		}
		
	if (exif.fGPSSatellites.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSSatellites);
		}
		
	if (exif.fGPSStatus.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSStatus);
		}
		
	if (exif.fGPSMeasureMode.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSMeasureMode);
		}
		
	if (exif.fGPSDOP.IsValid ())
		{
		fGPSIFD.Add (&fGPSDOP);
		}
		
	if (exif.fGPSSpeedRef.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSSpeedRef);
		}
		
	if (exif.fGPSSpeed.IsValid ())
		{
		fGPSIFD.Add (&fGPSSpeed);
		}
		
	if (exif.fGPSTrackRef.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSTrackRef);
		}
		
	if (exif.fGPSTrack.IsValid ())
		{
		fGPSIFD.Add (&fGPSTrack);
		}
		
	if (exif.fGPSImgDirectionRef.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSImgDirectionRef);
		}
		
	if (exif.fGPSImgDirection.IsValid ())
		{
		fGPSIFD.Add (&fGPSImgDirection);
		}

	if (exif.fGPSMapDatum.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSMapDatum);
		}
		
	if (exif.fGPSDestLatitudeRef.NotEmpty () &&
		exif.fGPSDestLatitude [0].IsValid ())
		{
		fGPSIFD.Add (&fGPSDestLatitudeRef);
		fGPSIFD.Add (&fGPSDestLatitude   );
		}
		
	if (exif.fGPSDestLongitudeRef.NotEmpty () &&
		exif.fGPSDestLongitude [0].IsValid ())
		{
		fGPSIFD.Add (&fGPSDestLongitudeRef);
		fGPSIFD.Add (&fGPSDestLongitude   );
		}
		
	if (exif.fGPSDestBearingRef.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSDestBearingRef);
		}
		
	if (exif.fGPSDestBearing.IsValid ())
		{
		fGPSIFD.Add (&fGPSDestBearing);
		}

	if (exif.fGPSDestDistanceRef.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSDestDistanceRef);
		}
		
	if (exif.fGPSDestDistance.IsValid ())
		{
		fGPSIFD.Add (&fGPSDestDistance);
		}
		
	if (exif.fGPSProcessingMethod.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSProcessingMethod);
		}

	if (exif.fGPSAreaInformation.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSAreaInformation);
		}
	
	if (exif.fGPSDateStamp.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSDateStamp);
		}
	
	if (exif.fGPSDifferential <= 0x0FFFF)
		{
		fGPSIFD.Add (&fGPSDifferential);
		}

	if (exif.AtLeastVersion0230 ())
		{
		
		if (exif.fGPSHPositioningError.IsValid ())
			{
			fGPSIFD.Add (&fGPSHPositioningError);
			}

		}
		
	AddLinks (directory);
	
	}

/******************************************************************************/

void exif_tag_set::AddLinks (dng_tiff_directory &directory)
	{
	
	if (fExifIFD.Size () != 0 && !fAddedExifLink)
		{
		
		directory.Add (&fExifLink);
		
		fAddedExifLink = true;
		
		}
	
	if (fGPSIFD.Size () != 0 && !fAddedGPSLink)
		{
		
		directory.Add (&fGPSLink);
		
		fAddedGPSLink = true;
		
		}
	
	}
	
/******************************************************************************/

class range_tag_set
	{
	
	private:
	
		uint32 fActiveAreaData [4];
	
		tag_uint32_ptr fActiveArea;
	
		uint32 fMaskedAreaData [kMaxMaskedAreas * 4];
	
		tag_uint32_ptr fMaskedAreas;
						    
		tag_uint16_ptr fLinearizationTable;
	
		uint16 fBlackLevelRepeatDimData [2];
		
		tag_uint16_ptr fBlackLevelRepeatDim;
		
		dng_urational fBlackLevelData [kMaxBlackPattern *
								       kMaxBlackPattern *
								       kMaxSamplesPerPixel];
		
		tag_urational_ptr fBlackLevel;
		
		dng_memory_data fBlackLevelDeltaHData;
		dng_memory_data fBlackLevelDeltaVData;
		
		tag_srational_ptr fBlackLevelDeltaH;
		tag_srational_ptr fBlackLevelDeltaV;
		
		uint16 fWhiteLevelData16 [kMaxSamplesPerPixel];
		uint32 fWhiteLevelData32 [kMaxSamplesPerPixel];
		
		tag_uint16_ptr fWhiteLevel16;
		tag_uint32_ptr fWhiteLevel32;
		
	public:
	
		range_tag_set (dng_tiff_directory &directory,
				       const dng_negative &negative);
		
	};
	
/******************************************************************************/

range_tag_set::range_tag_set (dng_tiff_directory &directory,
				     	      const dng_negative &negative)
				     	  
	:	fActiveArea (tcActiveArea,
					 fActiveAreaData,
					 4)
	
	,	fMaskedAreas (tcMaskedAreas,
					  fMaskedAreaData,
					  0)
	
	,	fLinearizationTable (tcLinearizationTable,
							 NULL,
							 0)
				     	  
	,	fBlackLevelRepeatDim (tcBlackLevelRepeatDim,
							  fBlackLevelRepeatDimData,
							  2)
							 
	,	fBlackLevel (tcBlackLevel,
					 fBlackLevelData)
					 
	,	fBlackLevelDeltaHData ()
	,	fBlackLevelDeltaVData ()
					 
	,	fBlackLevelDeltaH (tcBlackLevelDeltaH)
	,	fBlackLevelDeltaV (tcBlackLevelDeltaV)
 
	,	fWhiteLevel16 (tcWhiteLevel,
					   fWhiteLevelData16)
					 
	,	fWhiteLevel32 (tcWhiteLevel,
					   fWhiteLevelData32)
					 
	{
	
	const dng_image &rawImage (negative.RawImage ());
	
	const dng_linearization_info *rangeInfo = negative.GetLinearizationInfo ();
	
	if (rangeInfo)
		{
		
		// ActiveArea:
		
			{
		
			const dng_rect &r = rangeInfo->fActiveArea;
			
			if (r.NotEmpty ())
				{
			
				fActiveAreaData [0] = r.t;
				fActiveAreaData [1] = r.l;
				fActiveAreaData [2] = r.b;
				fActiveAreaData [3] = r.r;
				
				directory.Add (&fActiveArea);
				
				}
				
			}
			
		// MaskedAreas:
			
		if (rangeInfo->fMaskedAreaCount)
			{
			
			fMaskedAreas.SetCount (rangeInfo->fMaskedAreaCount * 4);
			
			for (uint32 index = 0; index < rangeInfo->fMaskedAreaCount; index++)
				{
				
				const dng_rect &r = rangeInfo->fMaskedArea [index];
				
				fMaskedAreaData [index * 4 + 0] = r.t;
				fMaskedAreaData [index * 4 + 1] = r.l;
				fMaskedAreaData [index * 4 + 2] = r.b;
				fMaskedAreaData [index * 4 + 3] = r.r;
				
				}
				
			directory.Add (&fMaskedAreas);
			
			}
			
		// LinearizationTable:

		if (rangeInfo->fLinearizationTable.Get ())
			{
			
			fLinearizationTable.SetData  (rangeInfo->fLinearizationTable->Buffer_uint16 ()     );
			fLinearizationTable.SetCount (rangeInfo->fLinearizationTable->LogicalSize   () >> 1);
			
			directory.Add (&fLinearizationTable);
			
			}
			
		// BlackLevelRepeatDim:
		
			{
		
			fBlackLevelRepeatDimData [0] = (uint16) rangeInfo->fBlackLevelRepeatRows;
			fBlackLevelRepeatDimData [1] = (uint16) rangeInfo->fBlackLevelRepeatCols;
			
			directory.Add (&fBlackLevelRepeatDim);
			
			}
		
		// BlackLevel:
		
			{
		
			uint32 index = 0;
			
			for (uint16 v = 0; v < rangeInfo->fBlackLevelRepeatRows; v++)
				{
				
				for (uint32 h = 0; h < rangeInfo->fBlackLevelRepeatCols; h++)
					{
					
					for (uint32 c = 0; c < rawImage.Planes (); c++)
						{
					
						fBlackLevelData [index++] = rangeInfo->BlackLevel (v, h, c);
					
						}
						
					}
					
				}
				
			fBlackLevel.SetCount (rangeInfo->fBlackLevelRepeatRows *
								  rangeInfo->fBlackLevelRepeatCols * rawImage.Planes ());
			
			directory.Add (&fBlackLevel);
			
			}
		
		// BlackLevelDeltaH:
				
		if (rangeInfo->ColumnBlackCount ())
			{
			
			uint32 count = rangeInfo->ColumnBlackCount ();
		
			fBlackLevelDeltaHData.Allocate (count * (uint32) sizeof (dng_srational));
												 
			dng_srational *blacks = (dng_srational *) fBlackLevelDeltaHData.Buffer ();
			
			for (uint32 col = 0; col < count; col++)
				{
				
				blacks [col] = rangeInfo->ColumnBlack (col);
				
				}
												 
			fBlackLevelDeltaH.SetData  (blacks);
			fBlackLevelDeltaH.SetCount (count );
			
			directory.Add (&fBlackLevelDeltaH);
			
			}
		
		// BlackLevelDeltaV:
				
		if (rangeInfo->RowBlackCount ())
			{
			
			uint32 count = rangeInfo->RowBlackCount ();
		
			fBlackLevelDeltaVData.Allocate (count * (uint32) sizeof (dng_srational));
												 
			dng_srational *blacks = (dng_srational *) fBlackLevelDeltaVData.Buffer ();
			
			for (uint32 row = 0; row < count; row++)
				{
				
				blacks [row] = rangeInfo->RowBlack (row);
				
				}
												 
			fBlackLevelDeltaV.SetData  (blacks);
			fBlackLevelDeltaV.SetCount (count );
			
			directory.Add (&fBlackLevelDeltaV);
			
			}
			
		}
		
	// WhiteLevel:
	
	// Only use the 32-bit data type if we must use it since there
	// are some lazy (non-Adobe) DNG readers out there.
	
	bool needs32 = false;
		
	fWhiteLevel16.SetCount (rawImage.Planes ());
	fWhiteLevel32.SetCount (rawImage.Planes ());
	
	for (uint32 c = 0; c < fWhiteLevel16.Count (); c++)
		{
		
		fWhiteLevelData32 [c] = negative.WhiteLevel (c);
		
		if (fWhiteLevelData32 [c] > 0x0FFFF)
			{
			needs32 = true;
			}
			
		fWhiteLevelData16 [c] = (uint16) fWhiteLevelData32 [c];
		
		}
		
	if (needs32)
		{
		directory.Add (&fWhiteLevel32);
		}
		
	else
		{
		directory.Add (&fWhiteLevel16);
		}
	
	}

/******************************************************************************/

class mosaic_tag_set
	{
	
	private:
	
		uint16 fCFARepeatPatternDimData [2];
		
		tag_uint16_ptr fCFARepeatPatternDim;
										   
		uint8 fCFAPatternData [kMaxCFAPattern *
							   kMaxCFAPattern];
		
		tag_uint8_ptr fCFAPattern;
								 
		uint8 fCFAPlaneColorData [kMaxColorPlanes];
		
		tag_uint8_ptr fCFAPlaneColor;
									
		tag_uint16 fCFALayout;
		
		tag_uint32 fGreenSplit;
		
	public:
	
		mosaic_tag_set (dng_tiff_directory &directory,
				        const dng_mosaic_info &info);
		
	};
	
/******************************************************************************/

mosaic_tag_set::mosaic_tag_set (dng_tiff_directory &directory,
					            const dng_mosaic_info &info)

	:	fCFARepeatPatternDim (tcCFARepeatPatternDim,
						  	  fCFARepeatPatternDimData,
						  	  2)
						  	  
	,	fCFAPattern (tcCFAPattern,
					 fCFAPatternData)
					
	,	fCFAPlaneColor (tcCFAPlaneColor,
						fCFAPlaneColorData)
						
	,	fCFALayout (tcCFALayout,
					(uint16) info.fCFALayout)
	
	,	fGreenSplit (tcBayerGreenSplit,
					 info.fBayerGreenSplit)
	
	{
	
	if (info.IsColorFilterArray ())
		{
	
		// CFARepeatPatternDim:
		
		fCFARepeatPatternDimData [0] = (uint16) info.fCFAPatternSize.v;
		fCFARepeatPatternDimData [1] = (uint16) info.fCFAPatternSize.h;
				
		directory.Add (&fCFARepeatPatternDim);
		
		// CFAPattern:
		
		fCFAPattern.SetCount (info.fCFAPatternSize.v *
							  info.fCFAPatternSize.h);
							  
		for (int32 r = 0; r < info.fCFAPatternSize.v; r++)
			{
			
			for (int32 c = 0; c < info.fCFAPatternSize.h; c++)
				{
				
				fCFAPatternData [r * info.fCFAPatternSize.h + c] = info.fCFAPattern [r] [c];
				
				}
				
			}
				
		directory.Add (&fCFAPattern);
		
		// CFAPlaneColor:
		
		fCFAPlaneColor.SetCount (info.fColorPlanes);
		
		for (uint32 j = 0; j < info.fColorPlanes; j++)
			{
		
			fCFAPlaneColorData [j] = info.fCFAPlaneColor [j];
			
			}
		
		directory.Add (&fCFAPlaneColor);
		
		// CFALayout:
		
		fCFALayout.Set ((uint16) info.fCFALayout);
		
		directory.Add (&fCFALayout);
		
		// BayerGreenSplit:  (only include if the pattern is a Bayer pattern)
			
		if (info.fCFAPatternSize == dng_point (2, 2) &&
			info.fColorPlanes    == 3)
			{
			
			directory.Add (&fGreenSplit);
			
			}
			
		}

	}
	
/******************************************************************************/

class color_tag_set
	{
	
	private:
	
		uint32 fColorChannels;
		
		tag_matrix fCameraCalibration1;
		tag_matrix fCameraCalibration2;
										 
		tag_string fCameraCalibrationSignature;
		
		tag_string fAsShotProfileName;

		dng_urational fAnalogBalanceData [4];
	
		tag_urational_ptr fAnalogBalance;
								    
		dng_urational fAsShotNeutralData [4];
		
		tag_urational_ptr fAsShotNeutral;
									    
		dng_urational fAsShotWhiteXYData [2];
		
		tag_urational_ptr fAsShotWhiteXY;
									    
		tag_urational fLinearResponseLimit;
										  
	public:
	
		color_tag_set (dng_tiff_directory &directory,
				       const dng_negative &negative);
		
	};
	
/******************************************************************************/

color_tag_set::color_tag_set (dng_tiff_directory &directory,
				     	  	  const dng_negative &negative)
				     	  
	:	fColorChannels (negative.ColorChannels ())
	
	,	fCameraCalibration1 (tcCameraCalibration1,
						     negative.CameraCalibration1 ())
						
	,	fCameraCalibration2 (tcCameraCalibration2,
						     negative.CameraCalibration2 ())
							 
	,	fCameraCalibrationSignature (tcCameraCalibrationSignature,
									 negative.CameraCalibrationSignature ())
									 
	,	fAsShotProfileName (tcAsShotProfileName,
							negative.AsShotProfileName ())

	,	fAnalogBalance (tcAnalogBalance,
						fAnalogBalanceData,
						fColorChannels)
						
	,	fAsShotNeutral (tcAsShotNeutral,
						fAsShotNeutralData,
						fColorChannels)
						
	,	fAsShotWhiteXY (tcAsShotWhiteXY,
						fAsShotWhiteXYData,
						2)
								    
	,	fLinearResponseLimit (tcLinearResponseLimit,
						      negative.LinearResponseLimitR ())
						      
	{
	
	if (fColorChannels > 1)
		{
		
		uint32 channels2 = fColorChannels * fColorChannels;
		
		if (fCameraCalibration1.Count () == channels2)
			{
			
			directory.Add (&fCameraCalibration1);
		
			}
			
		if (fCameraCalibration2.Count () == channels2)
			{
			
			directory.Add (&fCameraCalibration2);
		
			}
			
		if (fCameraCalibration1.Count () == channels2 ||
			fCameraCalibration2.Count () == channels2)
			{
			
			if (negative.CameraCalibrationSignature ().NotEmpty ())
				{
				
				directory.Add (&fCameraCalibrationSignature);
				
				}
			
			}
			
		if (negative.AsShotProfileName ().NotEmpty ())
			{
			
			directory.Add (&fAsShotProfileName);
				
			}
			
		for (uint32 j = 0; j < fColorChannels; j++)
			{
			
			fAnalogBalanceData [j] = negative.AnalogBalanceR (j);
			
			}
			
		directory.Add (&fAnalogBalance);
		
		if (negative.HasCameraNeutral ())
			{
			
			for (uint32 k = 0; k < fColorChannels; k++)
				{
				
				fAsShotNeutralData [k] = negative.CameraNeutralR (k);
				
				}
			
			directory.Add (&fAsShotNeutral);
			
			}
			
		else if (negative.HasCameraWhiteXY ())
			{
			
			negative.GetCameraWhiteXY (fAsShotWhiteXYData [0],
									   fAsShotWhiteXYData [1]);
									   
			directory.Add (&fAsShotWhiteXY);
			
			}
		
		directory.Add (&fLinearResponseLimit);
		
		}
	
	}

/******************************************************************************/

class profile_tag_set
	{
	
	private:
	
		tag_uint16 fCalibrationIlluminant1;
		tag_uint16 fCalibrationIlluminant2;
		
		tag_matrix fColorMatrix1;
		tag_matrix fColorMatrix2;
		
		tag_matrix fForwardMatrix1;
		tag_matrix fForwardMatrix2;

		tag_matrix fReductionMatrix1;
		tag_matrix fReductionMatrix2;
		
		tag_string fProfileName;
		
		tag_string fProfileCalibrationSignature;
		
		tag_uint32 fEmbedPolicyTag;
		
		tag_string fCopyrightTag;
		
		uint32 fHueSatMapDimData [3];
		
		tag_uint32_ptr fHueSatMapDims;

		tag_data_ptr fHueSatData1;
		tag_data_ptr fHueSatData2;
		
		tag_uint32 fHueSatMapEncodingTag;
		
		uint32 fLookTableDimData [3];
		
		tag_uint32_ptr fLookTableDims;

		tag_data_ptr fLookTableData;
		
		tag_uint32 fLookTableEncodingTag;

		tag_srational fBaselineExposureOffsetTag;
		
		tag_uint32 fDefaultBlackRenderTag;

		dng_memory_data fToneCurveBuffer;
		
		tag_data_ptr fToneCurveTag;

	public:
	
		profile_tag_set (dng_tiff_directory &directory,
						 const dng_camera_profile &profile);
		
	};
	
/******************************************************************************/

profile_tag_set::profile_tag_set (dng_tiff_directory &directory,
				     	  	      const dng_camera_profile &profile)
				     	  
	:	fCalibrationIlluminant1 (tcCalibrationIlluminant1,
								 (uint16) profile.CalibrationIlluminant1 ())
								 
	,	fCalibrationIlluminant2 (tcCalibrationIlluminant2,
								 (uint16) profile.CalibrationIlluminant2 ())
	
	,	fColorMatrix1 (tcColorMatrix1,
					   profile.ColorMatrix1 ())
						
	,	fColorMatrix2 (tcColorMatrix2,
					   profile.ColorMatrix2 ())

	,	fForwardMatrix1 (tcForwardMatrix1,
						 profile.ForwardMatrix1 ())
						
	,	fForwardMatrix2 (tcForwardMatrix2,
						 profile.ForwardMatrix2 ())
						
	,	fReductionMatrix1 (tcReductionMatrix1,
						   profile.ReductionMatrix1 ())
						
	,	fReductionMatrix2 (tcReductionMatrix2,
						   profile.ReductionMatrix2 ())
						   
	,	fProfileName (tcProfileName,
					  profile.Name (),
					  false)
						   
	,	fProfileCalibrationSignature (tcProfileCalibrationSignature,
									  profile.ProfileCalibrationSignature (),
									  false)
						
	,	fEmbedPolicyTag (tcProfileEmbedPolicy,
						 profile.EmbedPolicy ())
						 
	,	fCopyrightTag (tcProfileCopyright,
					   profile.Copyright (),
					   false)
					   
	,	fHueSatMapDims (tcProfileHueSatMapDims, 
						fHueSatMapDimData,
						3)
		
	,	fHueSatData1 (tcProfileHueSatMapData1,
					  ttFloat,
					  profile.HueSatDeltas1 ().DeltasCount () * 3,
					  profile.HueSatDeltas1 ().GetConstDeltas ())
					  
	,	fHueSatData2 (tcProfileHueSatMapData2,
					  ttFloat,
					  profile.HueSatDeltas2 ().DeltasCount () * 3,
					  profile.HueSatDeltas2 ().GetConstDeltas ())
					  
	,	fHueSatMapEncodingTag (tcProfileHueSatMapEncoding,
							   profile.HueSatMapEncoding ())
						 
	,	fLookTableDims (tcProfileLookTableDims,
						fLookTableDimData,
						3)
						
	,	fLookTableData (tcProfileLookTableData,
						ttFloat,
						profile.LookTable ().DeltasCount () * 3,
					    profile.LookTable ().GetConstDeltas ())
					  
	,	fLookTableEncodingTag (tcProfileLookTableEncoding,
							   profile.LookTableEncoding ())
						 
	,	fBaselineExposureOffsetTag (tcBaselineExposureOffset,
									profile.BaselineExposureOffset ())
						 
	,	fDefaultBlackRenderTag (tcDefaultBlackRender,
								profile.DefaultBlackRender ())
						 
	,	fToneCurveBuffer ()
					  
	,	fToneCurveTag (tcProfileToneCurve,
					   ttFloat,
					   0,
					   NULL)

	{
	
	if (profile.HasColorMatrix1 ())
		{
	
		uint32 colorChannels = profile.ColorMatrix1 ().Rows ();
		
		directory.Add (&fCalibrationIlluminant1);
		
		directory.Add (&fColorMatrix1);
		
		if (fForwardMatrix1.Count () == colorChannels * 3)
			{
			
			directory.Add (&fForwardMatrix1);

			}
		
		if (colorChannels > 3 && fReductionMatrix1.Count () == colorChannels * 3)
			{
			
			directory.Add (&fReductionMatrix1);
			
			}
			
		if (profile.HasColorMatrix2 ())
			{
		
			directory.Add (&fCalibrationIlluminant2);
		
			directory.Add (&fColorMatrix2);
				
			if (fForwardMatrix2.Count () == colorChannels * 3)
				{
				
				directory.Add (&fForwardMatrix2);

				}
		
			if (colorChannels > 3 && fReductionMatrix2.Count () == colorChannels * 3)
				{
				
				directory.Add (&fReductionMatrix2);
				
				}
	
			}
			
		if (profile.Name ().NotEmpty ())
			{
			
			directory.Add (&fProfileName);

			}
			
		if (profile.ProfileCalibrationSignature ().NotEmpty ())
			{
			
			directory.Add (&fProfileCalibrationSignature);
			
			}
			
		directory.Add (&fEmbedPolicyTag);
		
		if (profile.Copyright ().NotEmpty ())
			{
			
			directory.Add (&fCopyrightTag);
			
			}
		
		bool haveHueSat1 = profile.HueSatDeltas1 ().IsValid ();
		
		bool haveHueSat2 = profile.HueSatDeltas2 ().IsValid () &&
						   profile.HasColorMatrix2 ();

		if (haveHueSat1 || haveHueSat2)
			{
			
			uint32 hueDivs = 0;
			uint32 satDivs = 0;
			uint32 valDivs = 0;

			if (haveHueSat1)
				{

				profile.HueSatDeltas1 ().GetDivisions (hueDivs,
													   satDivs,
													   valDivs);

				}

			else
				{

				profile.HueSatDeltas2 ().GetDivisions (hueDivs,
													   satDivs,
													   valDivs);

				}
				
			fHueSatMapDimData [0] = hueDivs;
			fHueSatMapDimData [1] = satDivs;
			fHueSatMapDimData [2] = valDivs;
			
			directory.Add (&fHueSatMapDims);

			// Don't bother including the ProfileHueSatMapEncoding tag unless it's
			// non-linear.

			if (profile.HueSatMapEncoding () != encoding_Linear)
				{

				directory.Add (&fHueSatMapEncodingTag);

				}
		
			}
			
		if (haveHueSat1)
			{
			
			directory.Add (&fHueSatData1);
			
			}
			
		if (haveHueSat2)
			{
			
			directory.Add (&fHueSatData2);
			
			}
			
		if (profile.HasLookTable ())
			{
			
			uint32 hueDivs = 0;
			uint32 satDivs = 0;
			uint32 valDivs = 0;

			profile.LookTable ().GetDivisions (hueDivs,
											   satDivs,
											   valDivs);

			fLookTableDimData [0] = hueDivs;
			fLookTableDimData [1] = satDivs;
			fLookTableDimData [2] = valDivs;
			
			directory.Add (&fLookTableDims);
			
			directory.Add (&fLookTableData);
			
			// Don't bother including the ProfileLookTableEncoding tag unless it's
			// non-linear.

			if (profile.LookTableEncoding () != encoding_Linear)
				{

				directory.Add (&fLookTableEncodingTag);

				}
		
			}

		// Don't bother including the BaselineExposureOffset tag unless it's both
		// valid and non-zero.

		if (profile.BaselineExposureOffset ().IsValid ())
			{

			if (profile.BaselineExposureOffset ().As_real64 () != 0.0)
				{
			
				directory.Add (&fBaselineExposureOffsetTag);

				}
				
			}
			
		if (profile.DefaultBlackRender () != defaultBlackRender_Auto)
			{

			directory.Add (&fDefaultBlackRenderTag);

			}
		
		if (profile.ToneCurve ().IsValid ())
			{
			
			// Tone curve stored as pairs of 32-bit coordinates.  Probably could do with
			// 16-bits here, but should be small number of points so...
			
			uint32 toneCurvePoints = (uint32) (profile.ToneCurve ().fCoord.size ());

			fToneCurveBuffer.Allocate (toneCurvePoints * 2 * (uint32) sizeof (real32));

			real32 *points = fToneCurveBuffer.Buffer_real32 ();
			
			fToneCurveTag.SetCount (toneCurvePoints * 2);
			fToneCurveTag.SetData  (points);
			
			for (uint32 i = 0; i < toneCurvePoints; i++)
				{

				// Transpose coordinates so they are in a more expected
				// order (domain -> range).

				points [i * 2    ] = (real32) profile.ToneCurve ().fCoord [i].h;
				points [i * 2 + 1] = (real32) profile.ToneCurve ().fCoord [i].v;

				}

			directory.Add (&fToneCurveTag);

			}

		}
	
	}

/******************************************************************************/

tiff_dng_extended_color_profile::tiff_dng_extended_color_profile 
								 (const dng_camera_profile &profile)

	:	fProfile (profile)

	{
	
	}

/******************************************************************************/

void tiff_dng_extended_color_profile::Put (dng_stream &stream,
										   bool includeModelRestriction)
	{
	
	// Profile header.

	stream.Put_uint16 (stream.BigEndian () ? byteOrderMM : byteOrderII);

	stream.Put_uint16 (magicExtendedProfile);

	stream.Put_uint32 (8);
	
	// Profile tags.
	
	profile_tag_set tagSet (*this, fProfile);

	// Camera this profile is for.

	tag_string cameraModelTag (tcUniqueCameraModel, 
							   fProfile.UniqueCameraModelRestriction ());
							   
	if (includeModelRestriction)
		{
		
		if (fProfile.UniqueCameraModelRestriction ().NotEmpty ())
			{
			
			Add (&cameraModelTag);
			
			}
			
		}

	// Write it all out.

	dng_tiff_directory::Put (stream, offsetsRelativeToExplicitBase, 8);

	}

/*****************************************************************************/

tag_dng_noise_profile::tag_dng_noise_profile (const dng_noise_profile &profile)

	:	tag_data_ptr (tcNoiseProfile,
					  ttDouble,
					  2 * profile.NumFunctions (),
					  fValues)

	{

	DNG_REQUIRE (profile.NumFunctions () <= kMaxColorPlanes,
				 "Too many noise functions in tag_dng_noise_profile.");

	for (uint32 i = 0; i < profile.NumFunctions (); i++)
		{

		fValues [(2 * i)	] = profile.NoiseFunction (i).Scale	 ();
		fValues [(2 * i) + 1] = profile.NoiseFunction (i).Offset ();

		}
	
	}
		
/*****************************************************************************/

dng_image_writer::dng_image_writer ()
	{
	
	}

/*****************************************************************************/

dng_image_writer::~dng_image_writer ()
	{
	
	}
						    
/*****************************************************************************/

uint32 dng_image_writer::CompressedBufferSize (const dng_ifd &ifd,
											   uint32 uncompressedSize)
	{
	
	switch (ifd.fCompression)
		{
		
		case ccLZW:
			{
			
			// Add lots of slop for LZW to expand data.
				
			return uncompressedSize * 2 + 1024;
			
			}
			
		case ccDeflate:
			{
		
			// ZLib says maximum is source size + 0.1% + 12 bytes.
			
			return uncompressedSize + (uncompressedSize >> 8) + 64;
			
			}
			
		case ccJPEG:
			{
			
			// If we are saving lossless JPEG from an 8-bit image, reserve
			// space to pad the data out to 16-bits.
			
			if (ifd.fBitsPerSample [0] <= 8)
				{
				
				return uncompressedSize * 2;
				
				}
				
			break;
	
			}
			
		default:
			break;
		
		}
	
	return 0;
	
	}
						    
/******************************************************************************/

static void EncodeDelta8 (uint8 *dPtr,
						  uint32 rows,
						  uint32 cols,
						  uint32 channels)
	{
	
	const uint32 dRowStep = cols * channels;
	
	for (uint32 row = 0; row < rows; row++)
		{
		
		for (uint32 col = cols - 1; col > 0; col--)
			{
			
			for (uint32 channel = 0; channel < channels; channel++)
				{
				
				dPtr [col * channels + channel] -= dPtr [(col - 1) * channels + channel];
				
				}
			
			}
		
		dPtr += dRowStep;
		
		}

	}

/******************************************************************************/

static void EncodeDelta16 (uint16 *dPtr,
						   uint32 rows,
						   uint32 cols,
						   uint32 channels)
	{
	
	const uint32 dRowStep = cols * channels;
	
	for (uint32 row = 0; row < rows; row++)
		{
		
		for (uint32 col = cols - 1; col > 0; col--)
			{
			
			for (uint32 channel = 0; channel < channels; channel++)
				{
				
				dPtr [col * channels + channel] -= dPtr [(col - 1) * channels + channel];
				
				}
			
			}
		
		dPtr += dRowStep;
		
		}

	}
	
/******************************************************************************/

static void EncodeDelta32 (uint32 *dPtr,
						   uint32 rows,
						   uint32 cols,
						   uint32 channels)
	{
	
	const uint32 dRowStep = cols * channels;
	
	for (uint32 row = 0; row < rows; row++)
		{
		
		for (uint32 col = cols - 1; col > 0; col--)
			{
			
			for (uint32 channel = 0; channel < channels; channel++)
				{
				
				dPtr [col * channels + channel] -= dPtr [(col - 1) * channels + channel];
				
				}
			
			}
		
		dPtr += dRowStep;
		
		}

	}
	
/*****************************************************************************/

inline void EncodeDeltaBytes (uint8 *bytePtr, int32 cols, int32 channels)
	{
	
	if (channels == 1)
		{
		
		bytePtr += (cols - 1);
		
		uint8 this0 = bytePtr [0];
		
		for (int32 col = 1; col < cols; col++)
			{
			
			uint8 prev0 = bytePtr [-1];
			
			this0 -= prev0;
			
			bytePtr [0] = this0;
			
			this0 = prev0;
			
			bytePtr -= 1;

			}
	
		}
		
	else if (channels == 3)
		{
		
		bytePtr += (cols - 1) * 3;
		
		uint8 this0 = bytePtr [0];
		uint8 this1 = bytePtr [1];
		uint8 this2 = bytePtr [2];
		
		for (int32 col = 1; col < cols; col++)
			{
			
			uint8 prev0 = bytePtr [-3];
			uint8 prev1 = bytePtr [-2];
			uint8 prev2 = bytePtr [-1];
			
			this0 -= prev0;
			this1 -= prev1;
			this2 -= prev2;
			
			bytePtr [0] = this0;
			bytePtr [1] = this1;
			bytePtr [2] = this2;
			
			this0 = prev0;
			this1 = prev1;
			this2 = prev2;
			
			bytePtr -= 3;

			}
	
		}
		
	else
		{
	
		uint32 rowBytes = cols * channels;
		
		bytePtr += rowBytes - 1;
		
		for (uint32 col = channels; col < rowBytes; col++)
			{
			
			bytePtr [0] -= bytePtr [-channels];
				
			bytePtr--;

			}
			
		}

	}

/*****************************************************************************/

static void EncodeFPDelta (uint8 *buffer,
						   uint8 *temp,
						   int32 cols,
						   int32 channels,
						   int32 bytesPerSample)
	{
	
	int32 rowIncrement = cols * channels;
	
	if (bytesPerSample == 2)
		{
		
		const uint8 *src = buffer;
		
		#if qDNGBigEndian
		uint8 *dst0 = temp;
		uint8 *dst1 = temp + rowIncrement;
		#else
		uint8 *dst1 = temp;
		uint8 *dst0 = temp + rowIncrement;
		#endif
				
		for (int32 col = 0; col < rowIncrement; ++col)
			{
			
			dst0 [col] = src [0];
			dst1 [col] = src [1];
			
			src += 2;
			
			}
			
		}
		
	else if (bytesPerSample == 3)
		{
		
		const uint8 *src = buffer;
		
		uint8 *dst0 = temp;
		uint8 *dst1 = temp + rowIncrement;
		uint8 *dst2 = temp + rowIncrement * 2;
				
		for (int32 col = 0; col < rowIncrement; ++col)
			{
			
			dst0 [col] = src [0];
			dst1 [col] = src [1];
			dst2 [col] = src [2];
			
			src += 3;
			
			}
			
		}
		
	else
		{
		
		const uint8 *src = buffer;
		
		#if qDNGBigEndian
		uint8 *dst0 = temp;
		uint8 *dst1 = temp + rowIncrement;
		uint8 *dst2 = temp + rowIncrement * 2;
		uint8 *dst3 = temp + rowIncrement * 3;
		#else
		uint8 *dst3 = temp;
		uint8 *dst2 = temp + rowIncrement;
		uint8 *dst1 = temp + rowIncrement * 2;
		uint8 *dst0 = temp + rowIncrement * 3;
		#endif
				
		for (int32 col = 0; col < rowIncrement; ++col)
			{
			
			dst0 [col] = src [0];
			dst1 [col] = src [1];
			dst2 [col] = src [2];
			dst3 [col] = src [3];
			
			src += 4;
			
			}
			
		}
		
	EncodeDeltaBytes (temp, cols*bytesPerSample, channels);
	
	memcpy (buffer, temp, cols*bytesPerSample*channels);
	
	}

/*****************************************************************************/

void dng_image_writer::EncodePredictor (dng_host &host,
									    const dng_ifd &ifd,
						        	    dng_pixel_buffer &buffer,
										AutoPtr<dng_memory_block> &tempBuffer)
	{
	
	switch (ifd.fPredictor)
		{
		
		case cpHorizontalDifference:
		case cpHorizontalDifferenceX2:
		case cpHorizontalDifferenceX4:
			{
			
			int32 xFactor = 1;
			
			if (ifd.fPredictor == cpHorizontalDifferenceX2)
				{
				xFactor = 2;
				}
				
			else if (ifd.fPredictor == cpHorizontalDifferenceX4)
				{
				xFactor = 4;
				}
			
			switch (buffer.fPixelType)
				{
				
				case ttByte:
					{
					
					EncodeDelta8 ((uint8 *) buffer.fData,
								  buffer.fArea.H (),
								  buffer.fArea.W () / xFactor,
								  buffer.fPlanes    * xFactor);
					
					return;
					
					}
					
				case ttShort:
					{
					
					EncodeDelta16 ((uint16 *) buffer.fData,
								   buffer.fArea.H (),
								   buffer.fArea.W () / xFactor,
								   buffer.fPlanes    * xFactor);
					
					return;
					
					}
					
				case ttLong:
					{
					
					EncodeDelta32 ((uint32 *) buffer.fData,
								   buffer.fArea.H (),
								   buffer.fArea.W () / xFactor,
								   buffer.fPlanes    * xFactor);
					
					return;
					
					}
					
				default:
					break;
					
				}
			
			break;
			
			}
			
		case cpFloatingPoint:
		case cpFloatingPointX2:
		case cpFloatingPointX4:
			{
			
			int32 xFactor = 1;
			
			if (ifd.fPredictor == cpFloatingPointX2)
				{
				xFactor = 2;
				}
				
			else if (ifd.fPredictor == cpFloatingPointX4)
				{
				xFactor = 4;
				}
			
			uint32 tempBufferSize = buffer.fRowStep * buffer.fPixelSize;
			
			if (!tempBuffer.Get () || tempBuffer->LogicalSize () < tempBufferSize)
				{
				
				tempBuffer.Reset (host.Allocate (tempBufferSize));
				
				}
				
			for (int32 row = buffer.fArea.t; row < buffer.fArea.b; row++)
				{
				
				EncodeFPDelta ((uint8 *) buffer.DirtyPixel (row, buffer.fArea.l, buffer.fPlane),
							   tempBuffer->Buffer_uint8 (),
							   buffer.fArea.W () / xFactor,
							   buffer.fPlanes    * xFactor,
							   buffer.fPixelSize);
				
				}
			
			return;
			
			}
			
		default:
			break;
		
		}
	
	if (ifd.fPredictor != cpNullPredictor)
		{
		
		ThrowProgramError ();
		
		}
	
	}
						    
/*****************************************************************************/

void dng_image_writer::ByteSwapBuffer (dng_host & /* host */,
									   dng_pixel_buffer &buffer)
	{
	
	uint32 pixels = buffer.fRowStep * buffer.fArea.H ();
	
	switch (buffer.fPixelSize)
		{
		
		case 2:
			{
			
			DoSwapBytes16 ((uint16 *) buffer.fData,
						   pixels);
						   
			break;
			
			}
			
		case 4:
			{
			
			DoSwapBytes32 ((uint32 *) buffer.fData,
						   pixels);
						   
			break;
			
			}
			
		default:
			break;
			
		}

	}
						    
/*****************************************************************************/

void dng_image_writer::ReorderSubTileBlocks (const dng_ifd &ifd,
											 dng_pixel_buffer &buffer,
											 AutoPtr<dng_memory_block> &uncompressedBuffer,
											 AutoPtr<dng_memory_block> &subTileBlockBuffer)
	{
	
	uint32 blockRows = ifd.fSubTileBlockRows;
	uint32 blockCols = ifd.fSubTileBlockCols;
	
	uint32 rowBlocks = buffer.fArea.H () / blockRows;
	uint32 colBlocks = buffer.fArea.W () / blockCols;
	
	int32 rowStep = buffer.fRowStep * buffer.fPixelSize;
	int32 colStep = buffer.fColStep * buffer.fPixelSize;
	
	int32 rowBlockStep = rowStep * blockRows;
	int32 colBlockStep = colStep * blockCols;
	
	uint32 blockColBytes = blockCols * buffer.fPlanes * buffer.fPixelSize;
	
	const uint8 *s0 = uncompressedBuffer->Buffer_uint8 ();
	      uint8 *d0 = subTileBlockBuffer->Buffer_uint8 ();
	
	for (uint32 rowBlock = 0; rowBlock < rowBlocks; rowBlock++)
		{
		
		const uint8 *s1 = s0;
		
		for (uint32 colBlock = 0; colBlock < colBlocks; colBlock++)
			{
			
			const uint8 *s2 = s1;
			
			for (uint32 blockRow = 0; blockRow < blockRows; blockRow++)
				{
				
				for (uint32 j = 0; j < blockColBytes; j++)
					{
					
					d0 [j] = s2 [j];
					
					}
					
				d0 += blockColBytes;
				
				s2 += rowStep;
				
				}
			
			s1 += colBlockStep;
			
			}
			
		s0 += rowBlockStep;
		
		}
		
	// Copy back reordered pixels.
		
	DoCopyBytes (subTileBlockBuffer->Buffer      (),
				 uncompressedBuffer->Buffer      (),
				 uncompressedBuffer->LogicalSize ());
	
	}
						    
/******************************************************************************/

class dng_lzw_compressor
	{
	
	private:
	
		enum
			{
			kResetCode = 256,
			kEndCode   = 257,
			kTableSize = 4096
			};

		// Compressor nodes have two son pointers.  The low order bit of
		// the next code determines which pointer is used.  This cuts the
		// number of nodes searched for the next code by two on average.

		struct LZWCompressorNode
			{
			int16 final;
			int16 son0;
			int16 son1;
			int16 brother;
			};
			
		dng_memory_data fBuffer;

		LZWCompressorNode *fTable;
		
		uint8 *fDstPtr;
		
		int32 fDstCount;
		
		int32 fBitOffset;

		int32 fNextCode;
		
		int32 fCodeSize;
		
	public:
	
		dng_lzw_compressor ();
		
		void Compress (const uint8 *sPtr,
					   uint8 *dPtr,
					   uint32 sCount,
					   uint32 &dCount);
 
	private:
		
		void InitTable ();
	
		int32 SearchTable (int32 w, int32 k) const
			{
			
			DNG_ASSERT ((w >= 0) && (w <= kTableSize),
						"Bad w value in dng_lzw_compressor::SearchTable");
			
			int32 son0 = fTable [w] . son0;
			int32 son1 = fTable [w] . son1;
			
			// Branchless version of:
			// int32 code = (k & 1) ? son1 : son0;
			
			int32 code = son0 + ((-((int32) (k & 1))) & (son1 - son0));

			while (code > 0 && fTable [code].final != k)
				{
				code = fTable [code].brother;
				}

			return code;

			}

		void AddTable (int32 w, int32 k);
		
		void PutCodeWord (int32 code);

		// Hidden copy constructor and assignment operator.
	
		dng_lzw_compressor (const dng_lzw_compressor &compressor);
		
		dng_lzw_compressor & operator= (const dng_lzw_compressor &compressor);

	};

/******************************************************************************/

dng_lzw_compressor::dng_lzw_compressor ()

	:	fBuffer    ()
	,	fTable     (NULL)
	,	fDstPtr    (NULL)
	,	fDstCount  (0)
	,	fBitOffset (0)
	,	fNextCode  (0)
	,	fCodeSize  (0)
	
	{
	
	fBuffer.Allocate (kTableSize * sizeof (LZWCompressorNode));
	
	fTable = (LZWCompressorNode *) fBuffer.Buffer ();
	
	}

/******************************************************************************/

void dng_lzw_compressor::InitTable ()
	{

	fCodeSize = 9;

	fNextCode = 258;
		
	LZWCompressorNode *node = &fTable [0];
	
	for (int32 code = 0; code < 256; ++code)
		{
		
		node->final   = (int16) code;
		node->son0    = -1;
		node->son1    = -1;
		node->brother = -1;
		
		node++;
		
		}
		
	}

/******************************************************************************/

void dng_lzw_compressor::AddTable (int32 w, int32 k)
	{
	
	DNG_ASSERT ((w >= 0) && (w <= kTableSize),
				"Bad w value in dng_lzw_compressor::AddTable");

	LZWCompressorNode *node = &fTable [w];

	int32 nextCode = fNextCode;

	DNG_ASSERT ((nextCode >= 0) && (nextCode <= kTableSize),
				"Bad fNextCode value in dng_lzw_compressor::AddTable");
	
	LZWCompressorNode *node2 = &fTable [nextCode];
	
	fNextCode++;
	
	int32 oldSon;
	
	if( k&1 )
		{
		oldSon = node->son1;
		node->son1 = (int16) nextCode;
		}
	else
		{
		oldSon = node->son0;
		node->son0 = (int16) nextCode;
		}
	
	node2->final   = (int16) k;
	node2->son0    = -1;
	node2->son1    = -1;
	node2->brother = (int16) oldSon;
	
	if (nextCode == (1 << fCodeSize) - 1)
		{
		if (fCodeSize != 12)
			fCodeSize++;
		}
		
	}

/******************************************************************************/

void dng_lzw_compressor::PutCodeWord (int32 code)
	{
	
	int32 bit = (int32) (fBitOffset & 7);
	
	int32 offset1 = fBitOffset >> 3;
	int32 offset2 = (fBitOffset + fCodeSize - 1) >> 3;
		
	int32 shift1 = (fCodeSize + bit) -  8;
	int32 shift2 = (fCodeSize + bit) - 16;
	
	uint8 byte1 = (uint8) (code >> shift1);
	
	uint8 *dstPtr1 = fDstPtr + offset1;
	uint8 *dstPtr3 = fDstPtr + offset2;
	
	if (offset1 + 1 == offset2)
		{
		
		uint8 byte2 = (uint8) (code << (-shift2));
		
		if (bit)
			*dstPtr1 |= byte1;
		else
			*dstPtr1 = byte1;
		
		*dstPtr3 = byte2;
		
		}

	else
		{
		
		int32 shift3 = (fCodeSize + bit) - 24;
		
		uint8 byte2 = (uint8) (code >> shift2);
		uint8 byte3 = (uint8) (code << (-shift3));
		
		uint8 *dstPtr2 = fDstPtr + (offset1 + 1);
		
		if (bit)
			*dstPtr1 |= byte1;
		else
			*dstPtr1 = byte1;
		
		*dstPtr2 = byte2;
		
		*dstPtr3 = byte3;
		
		}
		
	fBitOffset += fCodeSize;
	
	}

/******************************************************************************/

void dng_lzw_compressor::Compress (const uint8 *sPtr,
						           uint8 *dPtr,
						           uint32 sCount,
						           uint32 &dCount)
	{
	
	fDstPtr = dPtr;
	
	fBitOffset = 0;
	
	InitTable ();
	
	PutCodeWord (kResetCode);
	
	int32 code = -1;
	
	int32 pixel;
	
	if (sCount > 0)
		{
		
		pixel = *sPtr;
		sPtr = sPtr + 1;
		code = pixel;

		sCount--;

		while (sCount--)
			{

			pixel = *sPtr;
			sPtr = sPtr + 1;
			
			int32 newCode = SearchTable (code, pixel);
			
			if (newCode == -1)
				{
				
				PutCodeWord (code);
				
				if (fNextCode < 4093)
					{
					AddTable (code, pixel);
					}
				else
					{
					PutCodeWord (kResetCode);
					InitTable ();
					}
					
				code = pixel;
				
				}
				
			else
				code = newCode;
				
			}
		
		}
		
	if (code != -1)
		{
		PutCodeWord (code);
		AddTable (code, 0);
		}
		
	PutCodeWord (kEndCode);

	dCount = (fBitOffset + 7) >> 3;

	}

/*****************************************************************************/

#if qDNGUseLibJPEG

/*****************************************************************************/

static void dng_error_exit (j_common_ptr cinfo)
	{
	
	// Output message.
	
	(*cinfo->err->output_message) (cinfo);
	
	// Convert to a dng_exception.

	switch (cinfo->err->msg_code)
		{
		
		case JERR_OUT_OF_MEMORY:
			{
			ThrowMemoryFull ();
			break;
			}
			
		default:
			{
			ThrowBadFormat ();
			}
			
		}
			
	}

/*****************************************************************************/

static void dng_output_message (j_common_ptr cinfo)
	{
	
	// Format message to string.
	
	char buffer [JMSG_LENGTH_MAX];

	(*cinfo->err->format_message) (cinfo, buffer);
	
	// Report the libjpeg message as a warning.
	
	ReportWarning ("libjpeg", buffer);

	}

/*****************************************************************************/

struct dng_jpeg_stream_dest
	{
	
	struct jpeg_destination_mgr pub;
	
	dng_stream *fStream;
	
	uint8 fBuffer [4096];
	
	};

/*****************************************************************************/

static void dng_init_destination (j_compress_ptr cinfo)
	{
	
	dng_jpeg_stream_dest *dest = (dng_jpeg_stream_dest *) cinfo->dest;

	dest->pub.next_output_byte = dest->fBuffer;
	dest->pub.free_in_buffer   = sizeof (dest->fBuffer);
	
	}

/*****************************************************************************/

static boolean dng_empty_output_buffer (j_compress_ptr cinfo)
	{
	
	dng_jpeg_stream_dest *dest = (dng_jpeg_stream_dest *) cinfo->dest;
	
	dest->fStream->Put (dest->fBuffer, sizeof (dest->fBuffer));

	dest->pub.next_output_byte = dest->fBuffer;
	dest->pub.free_in_buffer   = sizeof (dest->fBuffer);

	return TRUE;
	
	}

/*****************************************************************************/

static void dng_term_destination (j_compress_ptr cinfo)
	{
	
	dng_jpeg_stream_dest *dest = (dng_jpeg_stream_dest *) cinfo->dest;
	
	uint32 datacount = sizeof (dest->fBuffer) -
					   (uint32) dest->pub.free_in_buffer;
	
	if (datacount)
		{
		dest->fStream->Put (dest->fBuffer, datacount);
		}

	}

/*****************************************************************************/

static void jpeg_set_adobe_quality (struct jpeg_compress_struct *cinfo,
									int32 quality)
	{
	
	// If out of range, map to default.
		
	if (quality < 0 || quality > 12)
		{
		quality = 10;
		}
		
	// Adobe turns off chroma downsampling at high quality levels.
	
	bool useChromaDownsampling = (quality <= 6);
		
	// Approximate mapping from Adobe quality levels to LibJPEG levels.
	
	const int kLibJPEGQuality [13] =
		{
		5, 11, 23, 34, 46, 63, 76, 77, 86, 90, 94, 97, 99
		};
		
	quality = kLibJPEGQuality [quality];
	
	jpeg_set_quality (cinfo, quality, TRUE);
	
	// LibJPEG defaults to always using chroma downsampling.  Turn if off
	// if we need it off to match Adobe.
	
	if (!useChromaDownsampling)
		{
		
		cinfo->comp_info [0].h_samp_factor = 1;
		cinfo->comp_info [0].h_samp_factor = 1;
		
		}
				
	}

/*****************************************************************************/

#endif

/*****************************************************************************/

void dng_image_writer::WriteData (dng_host &host,
								  const dng_ifd &ifd,
						          dng_stream &stream,
						          dng_pixel_buffer &buffer,
								  AutoPtr<dng_memory_block> &compressedBuffer)
	{
	
	switch (ifd.fCompression)
		{
		
		case ccUncompressed:
			{
			
			// Special case support for when we save to 8-bits from
			// 16-bit data.
			
			if (ifd.fBitsPerSample [0] == 8 && buffer.fPixelType == ttShort)
				{
				
				uint32 count = buffer.fRowStep *
							   buffer.fArea.H ();
							   
				const uint16 *sPtr = (const uint16 *) buffer.fData;
				
				for (uint32 j = 0; j < count; j++)
					{
					
					stream.Put_uint8 ((uint8) sPtr [j]);
					
					}
				
				}
				
			else
				{
	
				// Swap bytes if required.
				
				if (stream.SwapBytes ())
					{
					
					ByteSwapBuffer (host, buffer);
										
					}
			
				// Write the bytes.
				
				stream.Put (buffer.fData, buffer.fRowStep *
										  buffer.fArea.H () *
										  buffer.fPixelSize);
										  
				}
			
			break;
			
			}
			
		case ccLZW:
		case ccDeflate:
			{
			
			// Both these compression algorithms are byte based.  The floating
			// point predictor already does byte ordering, so don't ever swap
			// when using it.
			
			if (stream.SwapBytes () && ifd.fPredictor != cpFloatingPoint)
				{
				
				ByteSwapBuffer (host,
								buffer);
								
				}
			
			// Run the compression algorithm.
				
			uint32 sBytes = buffer.fRowStep *
							buffer.fArea.H () *
							buffer.fPixelSize;
				
			uint8 *sBuffer = (uint8 *) buffer.fData;
				
			uint32 dBytes = 0;
				
			uint8 *dBuffer = compressedBuffer->Buffer_uint8 ();
			
			if (ifd.fCompression == ccLZW)
				{
				
				dng_lzw_compressor lzwCompressor;
				
				lzwCompressor.Compress (sBuffer,
										dBuffer,
										sBytes,
										dBytes);
										
				}
				
			else
				{
				
				uLongf dCount = compressedBuffer->LogicalSize ();
				
				int32 level = Z_DEFAULT_COMPRESSION;
				
				if (ifd.fCompressionQuality >= Z_BEST_SPEED &&
					ifd.fCompressionQuality <= Z_BEST_COMPRESSION)
					{
					
					level = ifd.fCompressionQuality;
					
					}
				
				int zResult = ::compress2 (dBuffer,
										   &dCount,
										   sBuffer,
										   sBytes,
										   level);
										  
				if (zResult != Z_OK)
					{
					
					ThrowMemoryFull ();
					
					}

				dBytes = (uint32) dCount;
				
				}
										
			if (dBytes > compressedBuffer->LogicalSize ())
				{
				
				DNG_REPORT ("Compression output buffer overflow");
				
				ThrowProgramError ();
				
				}
										
			stream.Put (dBuffer, dBytes);
				
			return;

			}
			
		case ccJPEG:
			{
			
			dng_pixel_buffer temp (buffer);
				
			if (buffer.fPixelType == ttByte)
				{
				
				// The lossless JPEG encoder needs 16-bit data, so if we are
				// are saving 8 bit data, we need to pad it out to 16-bits.
				
				temp.fData = compressedBuffer->Buffer ();
				
				temp.fPixelType = ttShort;
				temp.fPixelSize = 2;
				
				temp.CopyArea (buffer,
							   buffer.fArea,
							   buffer.fPlane,
							   buffer.fPlanes);
				
				}
				
			EncodeLosslessJPEG ((const uint16 *) temp.fData,
								temp.fArea.H (),
								temp.fArea.W (),
								temp.fPlanes,
								ifd.fBitsPerSample [0],
								temp.fRowStep,
								temp.fColStep,
								stream);
										
			break;
			
			}
			
		#if qDNGUseLibJPEG
		
		case ccLossyJPEG:
			{
			
			struct jpeg_compress_struct cinfo;
	
			// Setup the error manager.
			
			struct jpeg_error_mgr jerr;

			cinfo.err = jpeg_std_error (&jerr);
			
			jerr.error_exit     = dng_error_exit;
			jerr.output_message = dng_output_message;
	
			try
				{
				
				// Create the compression context.

				jpeg_create_compress (&cinfo);
				
				// Setup the destination manager to write to stream.
				
				dng_jpeg_stream_dest dest;
				
				dest.fStream = &stream;
				
				dest.pub.init_destination    = dng_init_destination;
				dest.pub.empty_output_buffer = dng_empty_output_buffer;
				dest.pub.term_destination    = dng_term_destination;
				
				cinfo.dest = &dest.pub;
				
				// Setup basic image info.
				
				cinfo.image_width      = buffer.fArea.W ();
				cinfo.image_height     = buffer.fArea.H ();
				cinfo.input_components = buffer.fPlanes;
				
				switch (buffer.fPlanes)
					{
					
					case 1:
						cinfo.in_color_space = JCS_GRAYSCALE;
						break;
						
					case 3:
						cinfo.in_color_space = JCS_RGB;
						break;
						
					case 4:
						cinfo.in_color_space = JCS_CMYK;
						break;
						
					default:
						ThrowProgramError ();
						
					}
					
				// Setup the compression parameters.

				jpeg_set_defaults (&cinfo);
				
				jpeg_set_adobe_quality (&cinfo, ifd.fCompressionQuality);
				
				// Write the JPEG header.
				
				jpeg_start_compress (&cinfo, TRUE);
				
				// Write the scanlines.
				
				for (int32 row = buffer.fArea.t; row < buffer.fArea.b; row++)
					{
					
					uint8 *sampArray [1];
		
					sampArray [0] = buffer.DirtyPixel_uint8 (row,
															 buffer.fArea.l,
															 0);

					jpeg_write_scanlines (&cinfo, sampArray, 1);
					
					}

				// Cleanup.
					
				jpeg_finish_compress (&cinfo);

				jpeg_destroy_compress (&cinfo);
					
				}
				
			catch (...)
				{
				
				jpeg_destroy_compress (&cinfo);
				
				throw;
				
				}
				
			return;
			
			}
			
		#endif
			
		default:
			{
			
			ThrowProgramError ();
			
			}
			
		}
	
	}
						    
/******************************************************************************/

void dng_image_writer::EncodeJPEGPreview (dng_host &host,
										  const dng_image &image,
										  dng_jpeg_preview &preview,
										  int32 quality)
	{
	
	#if qDNGUseLibJPEG
		
	dng_memory_stream stream (host.Allocator ());
	
	struct jpeg_compress_struct cinfo;

	// Setup the error manager.
	
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error (&jerr);
	
	jerr.error_exit     = dng_error_exit;
	jerr.output_message = dng_output_message;

	try
		{
		
		// Create the compression context.

		jpeg_create_compress (&cinfo);
		
		// Setup the destination manager to write to stream.
		
		dng_jpeg_stream_dest dest;
		
		dest.fStream = &stream;
		
		dest.pub.init_destination    = dng_init_destination;
		dest.pub.empty_output_buffer = dng_empty_output_buffer;
		dest.pub.term_destination    = dng_term_destination;
		
		cinfo.dest = &dest.pub;
		
		// Setup basic image info.
		
		cinfo.image_width      = image.Bounds ().W ();
		cinfo.image_height     = image.Bounds ().H ();
		cinfo.input_components = image.Planes ();
		
		switch (image.Planes ())
			{
			
			case 1:
				cinfo.in_color_space = JCS_GRAYSCALE;
				break;
				
			case 3:
				cinfo.in_color_space = JCS_RGB;
				break;
				
			default:
				ThrowProgramError ();
				
			}
			
		// Setup the compression parameters.

		jpeg_set_defaults (&cinfo);
		
		jpeg_set_adobe_quality (&cinfo, quality);
		
		// Find some preview information based on the compression settings.
		
		preview.fPreviewSize = image.Size ();
	
		if (image.Planes () == 1)
			{
			
			preview.fPhotometricInterpretation = piBlackIsZero;
			
			}
			
		else
			{
			
			preview.fPhotometricInterpretation = piYCbCr;
			
			preview.fYCbCrSubSampling.h  = cinfo.comp_info [0].h_samp_factor;
			preview.fYCbCrSubSampling.v  = cinfo.comp_info [0].v_samp_factor;
			
			}
		
		// Write the JPEG header.
		
		jpeg_start_compress (&cinfo, TRUE);
		
		// Write the scanlines.
		
		dng_pixel_buffer buffer;
		
		buffer.fArea   = image.Bounds ();
		buffer.fPlanes = image.Planes ();
		
		buffer.fColStep = buffer.fPlanes;
		buffer.fRowStep = buffer.fColStep * buffer.fArea.W ();
		
		buffer.fPixelType = ttByte;
		buffer.fPixelSize = 1;
		
		AutoPtr<dng_memory_block> bufferData (host.Allocate (buffer.fRowStep));
		
		buffer.fData = bufferData->Buffer ();
		
		for (uint32 row = 0; row < cinfo.image_height; row++)
			{
			
			buffer.fArea.t = row;
			buffer.fArea.b = row + 1;
			
			image.Get (buffer);
			
			uint8 *sampArray [1];

			sampArray [0] = buffer.DirtyPixel_uint8 (row,
													 buffer.fArea.l,
													 0);

			jpeg_write_scanlines (&cinfo, sampArray, 1);
			
			}

		// Cleanup.
			
		jpeg_finish_compress (&cinfo);

		jpeg_destroy_compress (&cinfo);
			
		}
		
	catch (...)
		{
		
		jpeg_destroy_compress (&cinfo);
		
		throw;
		
		}
				   
	preview.fCompressedData.Reset (stream.AsMemoryBlock (host.Allocator ()));

	#else
	
	(void) host;
	(void) image;
	(void) preview;
	(void) quality;
	
	ThrowProgramError ("No JPEG encoder");
	
	#endif
		
	}
								
/*****************************************************************************/

void dng_image_writer::WriteTile (dng_host &host,
						          const dng_ifd &ifd,
						          dng_stream &stream,
						          const dng_image &image,
						          const dng_rect &tileArea,
						          uint32 fakeChannels,
								  AutoPtr<dng_memory_block> &compressedBuffer,
								  AutoPtr<dng_memory_block> &uncompressedBuffer,
								  AutoPtr<dng_memory_block> &subTileBlockBuffer,
								  AutoPtr<dng_memory_block> &tempBuffer)
	{
	
	// Create pixel buffer to hold uncompressed tile.
	
	dng_pixel_buffer buffer;
	
	buffer.fArea = tileArea;
	
	buffer.fPlane  = 0;
	buffer.fPlanes = ifd.fSamplesPerPixel;
	
	buffer.fRowStep   = buffer.fPlanes * tileArea.W ();
	buffer.fColStep   = buffer.fPlanes;
	buffer.fPlaneStep = 1;
	
	buffer.fPixelType = image.PixelType ();
	buffer.fPixelSize = image.PixelSize ();
	
	buffer.fData = uncompressedBuffer->Buffer ();
	
	// Get the uncompressed data.
	
	image.Get (buffer, dng_image::edge_zero);
	
	// Deal with sub-tile blocks.
	
	if (ifd.fSubTileBlockRows > 1)
		{
		
		ReorderSubTileBlocks (ifd,
							  buffer,
							  uncompressedBuffer,
							  subTileBlockBuffer);
		
		}
		
	// Floating point depth conversion.
	
	if (ifd.fSampleFormat [0] == sfFloatingPoint)
		{
		
		if (ifd.fBitsPerSample [0] == 16)
			{
			
			uint32 *srcPtr = (uint32 *) buffer.fData;
			uint16 *dstPtr = (uint16 *) buffer.fData;
			
			uint32 pixels = tileArea.W () * tileArea.H () * buffer.fPlanes;
			
			for (uint32 j = 0; j < pixels; j++)
				{
				
				dstPtr [j] = DNG_FloatToHalf (srcPtr [j]);
				
				}
				
			buffer.fPixelSize = 2;
			
			}
			
		if (ifd.fBitsPerSample [0] == 24)
			{
			
			uint32 *srcPtr = (uint32 *) buffer.fData;
			uint8  *dstPtr = (uint8  *) buffer.fData;
			
			uint32 pixels = tileArea.W () * tileArea.H () * buffer.fPlanes;
			
			if (stream.BigEndian () || ifd.fPredictor == cpFloatingPoint   ||
									   ifd.fPredictor == cpFloatingPointX2 ||
									   ifd.fPredictor == cpFloatingPointX4)
				{
			
				for (uint32 j = 0; j < pixels; j++)
					{
					
					DNG_FloatToFP24 (srcPtr [j], dstPtr);
					
					dstPtr += 3;
					
					}
					
				}
				
			else
				{
			
				for (uint32 j = 0; j < pixels; j++)
					{
					
					uint8 output [3];
					
					DNG_FloatToFP24 (srcPtr [j], output);
					
					dstPtr [0] = output [2];
					dstPtr [1] = output [1];
					dstPtr [2] = output [0];
					
					dstPtr += 3;
					
					}
					
				}
				
			buffer.fPixelSize = 3;
			
			}
		
		}
	
	// Run predictor.
	
	EncodePredictor (host,
					 ifd,
					 buffer,
					 tempBuffer);
		
	// Adjust pixel buffer for fake channels.
	
	if (fakeChannels > 1)
		{
		
		buffer.fPlanes  *= fakeChannels;
		buffer.fColStep *= fakeChannels;
		
		buffer.fArea.r = buffer.fArea.l + (buffer.fArea.W () / fakeChannels);
		
		}
		
	// Compress (if required) and write out the data.
	
	WriteData (host,
			   ifd,
			   stream,
			   buffer,
			   compressedBuffer);
			   
	}

/*****************************************************************************/

class dng_write_tiles_task : public dng_area_task
	{
	
	private:
	
		dng_image_writer &fImageWriter;
		
		dng_host &fHost;
		
		const dng_ifd &fIFD;
		
		dng_basic_tag_set &fBasic;
		
		dng_stream &fStream;
		
		const dng_image &fImage;
		
		uint32 fFakeChannels;
		
		uint32 fTilesDown;
		
		uint32 fTilesAcross;
		
		uint32 fCompressedSize;
		
		uint32 fUncompressedSize;
		
		dng_mutex fMutex1;
		
		uint32 fNextTileIndex;
		
		dng_mutex fMutex2;
		
		dng_condition fCondition;
		
		bool fTaskFailed;

		uint32 fWriteTileIndex;
		
	public:
	
		dng_write_tiles_task (dng_image_writer &imageWriter,
							  dng_host &host,
							  const dng_ifd &ifd,
							  dng_basic_tag_set &basic,
							  dng_stream &stream,
							  const dng_image &image,
							  uint32 fakeChannels,
							  uint32 tilesDown,
							  uint32 tilesAcross,
							  uint32 compressedSize,
							  uint32 uncompressedSize)
		
			:	fImageWriter      (imageWriter)
			,	fHost		      (host)
			,	fIFD		      (ifd)
			,	fBasic			  (basic)
			,	fStream		      (stream)
			,	fImage		      (image)
			,	fFakeChannels	  (fakeChannels)
			,	fTilesDown        (tilesDown)
			,	fTilesAcross	  (tilesAcross)
			,	fCompressedSize   (compressedSize)
			,	fUncompressedSize (uncompressedSize)
			,	fMutex1			  ("dng_write_tiles_task_1")
			,	fNextTileIndex	  (0)
			,	fMutex2			  ("dng_write_tiles_task_2")
			,	fCondition		  ()
			,	fTaskFailed		  (false)
			,	fWriteTileIndex	  (0)
			
			{
			
			fMinTaskArea = 16 * 16;
			fUnitCell    = dng_point (16, 16);
			fMaxTileSize = dng_point (16, 16);
			
			}
	
		void Process (uint32 /* threadIndex */,
					  const dng_rect & /* tile */,
					  dng_abort_sniffer *sniffer)
			{
			
			try
				{
			
				AutoPtr<dng_memory_block> compressedBuffer;
				AutoPtr<dng_memory_block> uncompressedBuffer;
				AutoPtr<dng_memory_block> subTileBlockBuffer;
				AutoPtr<dng_memory_block> tempBuffer;
				
				if (fCompressedSize)
					{
					compressedBuffer.Reset (fHost.Allocate (fCompressedSize));
					}
				
				if (fUncompressedSize)
					{
					uncompressedBuffer.Reset (fHost.Allocate (fUncompressedSize));
					}
				
				if (fIFD.fSubTileBlockRows > 1 && fUncompressedSize)
					{
					subTileBlockBuffer.Reset (fHost.Allocate (fUncompressedSize));
					}
				
				while (true)
					{
					
					// Find tile index to compress.
					
					uint32 tileIndex;
					
						{
						
						dng_lock_mutex lock (&fMutex1);
						
						if (fNextTileIndex == fTilesDown * fTilesAcross)
							{
							return;
							}
							
						tileIndex = fNextTileIndex++;
						
						}
						
					dng_abort_sniffer::SniffForAbort (sniffer);
					
					// Compress tile.
					
					uint32 rowIndex = tileIndex / fTilesAcross;
					
					uint32 colIndex = tileIndex - rowIndex * fTilesAcross;
					
					dng_rect tileArea = fIFD.TileArea (rowIndex, colIndex);
					
					dng_memory_stream tileStream (fHost.Allocator ());
										   
					tileStream.SetLittleEndian (fStream.LittleEndian ());
					
					dng_host host (&fHost.Allocator (),
								   sniffer);
								
					fImageWriter.WriteTile (host,
											fIFD,
											tileStream,
											fImage,
											tileArea,
											fFakeChannels,
											compressedBuffer,
											uncompressedBuffer,
											subTileBlockBuffer,
											tempBuffer);
											
					tileStream.Flush ();
											
					uint32 tileByteCount = (uint32) tileStream.Length ();
					
					tileStream.SetReadPosition (0);
					
					// Wait until it is our turn to write tile.

						{
					
						dng_lock_mutex lock (&fMutex2);
					
						while (!fTaskFailed &&
							   fWriteTileIndex != tileIndex)
							{

							fCondition.Wait (fMutex2);
							
							}

						// If the task failed in another thread, that thread already threw an exception.

						if (fTaskFailed)
							return;

						}						
					
					dng_abort_sniffer::SniffForAbort (sniffer);
					
					// Remember this offset.
				
					uint32 tileOffset = (uint32) fStream.Position ();
				
					fBasic.SetTileOffset (tileIndex, tileOffset);
						
					// Copy tile stream for tile into main stream.
							
					tileStream.CopyToStream (fStream, tileByteCount);
							
					// Update tile count.
						
					fBasic.SetTileByteCount (tileIndex, tileByteCount);
					
					// Keep the tiles on even byte offsets.
														 
					if (tileByteCount & 1)
						{
						fStream.Put_uint8 (0);
						}
							
					// Let other threads know it is safe to write to stream.
					
						{
						
						dng_lock_mutex lock (&fMutex2);
						
						// If the task failed in another thread, that thread already threw an exception.

						if (fTaskFailed)
							return;

						fWriteTileIndex++;
						
						fCondition.Broadcast ();
						
						}
						
					}
					
				}
				
			catch (...)
				{
				
				// If first to fail, wake up any threads waiting on condition.
				
				bool needBroadcast = false;

					{
					
					dng_lock_mutex lock (&fMutex2);

					needBroadcast = !fTaskFailed;
					fTaskFailed = true;
					
					}

				if (needBroadcast)
					fCondition.Broadcast ();
				
				throw;
				
				}
			
			}
		
	private:

		// Hidden copy constructor and assignment operator.

		dng_write_tiles_task (const dng_write_tiles_task &);

		dng_write_tiles_task & operator= (const dng_write_tiles_task &);
		
	};

/*****************************************************************************/

void dng_image_writer::WriteImage (dng_host &host,
						           const dng_ifd &ifd,
						           dng_basic_tag_set &basic,
						           dng_stream &stream,
						           const dng_image &image,
						           uint32 fakeChannels)
	{
	
	// Deal with row interleaved images.
	
	if (ifd.fRowInterleaveFactor > 1 &&
		ifd.fRowInterleaveFactor < ifd.fImageLength)
		{
		
		dng_ifd tempIFD (ifd);
		
		tempIFD.fRowInterleaveFactor = 1;
		
		dng_row_interleaved_image tempImage (*((dng_image *) &image),
											 ifd.fRowInterleaveFactor);
		
		WriteImage (host,
					tempIFD,
					basic,
					stream,
					tempImage,
					fakeChannels);
			  
		return;
		
		}
	
	// Compute basic information.
	
	uint32 bytesPerSample = TagTypeSize (image.PixelType ());
	
	uint32 bytesPerPixel = ifd.fSamplesPerPixel * bytesPerSample;
	
	uint32 tileRowBytes = ifd.fTileWidth * bytesPerPixel;
	
	// If we can compute the number of bytes needed to store the
	// data, we can split the write for each tile into sub-tiles.
	
	uint32 subTileLength = ifd.fTileLength;
	
	if (ifd.TileByteCount (ifd.TileArea (0, 0)) != 0)
		{
		
		subTileLength = Pin_uint32 (ifd.fSubTileBlockRows,
									kImageBufferSize / tileRowBytes, 
									ifd.fTileLength);
					
		// Don't split sub-tiles across subTileBlocks.
		
		subTileLength = subTileLength / ifd.fSubTileBlockRows
									  * ifd.fSubTileBlockRows;
									
		}
		
	// Find size of uncompressed buffer.
	
	uint32 uncompressedSize = subTileLength * tileRowBytes;
	
	// Find size of compressed buffer, if required.
	
	uint32 compressedSize = CompressedBufferSize (ifd, uncompressedSize);
			
	// See if we can do this write using multiple threads.
	
	uint32 tilesAcross = ifd.TilesAcross ();
	uint32 tilesDown   = ifd.TilesDown   ();
							   
	bool useMultipleThreads = (tilesDown * tilesAcross >= 2) &&
							  (host.PerformAreaTaskThreads () > 1) &&
							  (subTileLength == ifd.fTileLength) &&
							  (ifd.fCompression != ccUncompressed);
	
		
#if qImagecore
	useMultipleThreads = false;	
#endif
		
	if (useMultipleThreads)
		{
		
		uint32 threadCount = Min_uint32 (tilesDown * tilesAcross,
										 host.PerformAreaTaskThreads ());
										 
		dng_write_tiles_task task (*this,
								   host,
								   ifd,
								   basic,
								   stream,
								   image,
								   fakeChannels,
								   tilesDown,
								   tilesAcross,
								   compressedSize,
								   uncompressedSize);
								  
		host.PerformAreaTask (task,
							  dng_rect (0, 0, 16, 16 * threadCount));
		
		}
		
	else
		{
									
		AutoPtr<dng_memory_block> compressedBuffer;
		AutoPtr<dng_memory_block> uncompressedBuffer;
		AutoPtr<dng_memory_block> subTileBlockBuffer;
		AutoPtr<dng_memory_block> tempBuffer;
		
		if (compressedSize)
			{
			compressedBuffer.Reset (host.Allocate (compressedSize));
			}
		
		if (uncompressedSize)
			{
			uncompressedBuffer.Reset (host.Allocate (uncompressedSize));
			}
		
		if (ifd.fSubTileBlockRows > 1 && uncompressedSize)
			{
			subTileBlockBuffer.Reset (host.Allocate (uncompressedSize));
			}
				
		// Write out each tile.
		
		uint32 tileIndex = 0;
		
		for (uint32 rowIndex = 0; rowIndex < tilesDown; rowIndex++)
			{
			
			for (uint32 colIndex = 0; colIndex < tilesAcross; colIndex++)
				{
				
				// Remember this offset.
				
				uint32 tileOffset = (uint32) stream.Position ();
			
				basic.SetTileOffset (tileIndex, tileOffset);
				
				// Split tile into sub-tiles if possible.
				
				dng_rect tileArea = ifd.TileArea (rowIndex, colIndex);
				
				uint32 subTileCount = (tileArea.H () + subTileLength - 1) /
									  subTileLength;
									  
				for (uint32 subIndex = 0; subIndex < subTileCount; subIndex++)
					{
					
					host.SniffForAbort ();
				
					dng_rect subArea (tileArea);
					
					subArea.t = tileArea.t + subIndex * subTileLength;
					
					subArea.b = Min_int32 (subArea.t + subTileLength,
										   tileArea.b);
										   
					// Write the sub-tile.
					
					WriteTile (host,
							   ifd,
							   stream,
							   image,
							   subArea,
							   fakeChannels,
							   compressedBuffer,
							   uncompressedBuffer,
							   subTileBlockBuffer,
							   tempBuffer);
							   
					}
					
				// Update tile count.
					
				uint32 tileByteCount = (uint32) stream.Position () - tileOffset;
					
				basic.SetTileByteCount (tileIndex, tileByteCount);
				
				tileIndex++;
				
				// Keep the tiles on even byte offsets.
													 
				if (tileByteCount & 1)
					{
					stream.Put_uint8 (0);
					}
					
				}

			}
			
		}
		
	}

/*****************************************************************************/

static void CopyString (const dng_xmp &oldXMP,
						dng_xmp &newXMP,
						const char *ns,
						const char *path,
						dng_string *exif = NULL)
	{
	
	dng_string s;
	
	if (oldXMP.GetString (ns, path, s))
		{
		
		if (s.NotEmpty ())
			{
			
			newXMP.SetString (ns, path, s);
			
			if (exif)
				{
				
				*exif = s;
				
				}
			
			}
			
		}
	
	}
								 
/*****************************************************************************/

static void CopyStringList (const dng_xmp &oldXMP,
							dng_xmp &newXMP,
							const char *ns,
							const char *path,
							bool isBag)
	{
	
	dng_string_list list;
	
	if (oldXMP.GetStringList (ns, path, list))
		{
		
		if (list.Count ())
			{
			
			newXMP.SetStringList (ns, path, list, isBag);
						
			}
			
		}
	
	}
								 
/*****************************************************************************/

static void CopyAltLangDefault (const dng_xmp &oldXMP,
								dng_xmp &newXMP,
								const char *ns,
								const char *path,
								dng_string *exif = NULL)
	{
	
	dng_string s;
	
	if (oldXMP.GetAltLangDefault (ns, path, s))
		{
		
		if (s.NotEmpty ())
			{
			
			newXMP.SetAltLangDefault (ns, path, s);
			
			if (exif)
				{
				
				*exif = s;
				
				}
			
			}
			
		}
	
	}
								 
/*****************************************************************************/

static void CopyStructField (const dng_xmp &oldXMP,
							 dng_xmp &newXMP,
							 const char *ns,
							 const char *path,
							 const char *field)
	{
	
	dng_string s;
	
	if (oldXMP.GetStructField (ns, path, ns, field, s))
		{
		
		if (s.NotEmpty ())
			{
			
			newXMP.SetStructField (ns, path, ns, field, s);
			
			}
			
		}
	
	}

/*****************************************************************************/

static void CopyBoolean (const dng_xmp &oldXMP,
						 dng_xmp &newXMP,
						 const char *ns,
						 const char *path)
	{
	
	bool b;
	
	if (oldXMP.GetBoolean (ns, path, b))
		{
		
		newXMP.SetBoolean (ns, path, b);
						
		}
	
	}
								 
/*****************************************************************************/

void dng_image_writer::CleanUpMetadata (dng_host &host,
										dng_metadata &metadata,
										dng_metadata_subset metadataSubset,
										const char *dstMIMI,
										const char *software)
	{
	
	if (metadata.GetXMP () && metadata.GetExif ())
		{
		
		dng_xmp  &newXMP  (*metadata.GetXMP  ());
		dng_exif &newEXIF (*metadata.GetExif ());
		
		// Update software tag.
		
		if (software)
			{
	
			newEXIF.fSoftware.Set (software);
			
			newXMP.Set (XMP_NS_XAP,
						"CreatorTool",
						software);
			
			}
		
		#if qDNGXMPDocOps
		
		newXMP.DocOpsPrepareForSave (metadata.SourceMIMI ().Get (),
									 dstMIMI);
												  
		#else
		
		metadata.UpdateDateTimeToNow ();
		
		#endif
		
		// Update EXIF version to at least 2.3 so all the exif tags
		// can be written.
		
		if (newEXIF.fExifVersion < DNG_CHAR4 ('0','2','3','0'))
			{
		
			newEXIF.fExifVersion = DNG_CHAR4 ('0','2','3','0');
			
			newXMP.Set (XMP_NS_EXIF, "ExifVersion", "0230");
			
			}
			
		// Resync EXIF, remove EXIF tags from XMP.
	
		newXMP.SyncExif (newEXIF,
						 metadata.GetOriginalExif (),
						 false,
						 true);
									  
		// Deal with ImageIngesterPro bug.  This program is adding lots of
		// empty metadata strings into the XMP, which is screwing up Adobe CS4.
		// We are saving a new file, so this is a chance to clean up this mess.
		
		newXMP.RemoveEmptyStringsAndArrays (XMP_NS_DC);
		newXMP.RemoveEmptyStringsAndArrays (XMP_NS_XAP);
		newXMP.RemoveEmptyStringsAndArrays (XMP_NS_PHOTOSHOP);
		newXMP.RemoveEmptyStringsAndArrays (XMP_NS_IPTC);
		newXMP.RemoveEmptyStringsAndArrays (XMP_NS_XAP_RIGHTS);
		newXMP.RemoveEmptyStringsAndArrays ("http://ns.iview-multimedia.com/mediapro/1.0/");

		// Process metadata subset.
		
		if (metadataSubset == kMetadataSubset_CopyrightOnly ||
			metadataSubset == kMetadataSubset_CopyrightAndContact)
			{
			
			dng_xmp  oldXMP  (newXMP );
			dng_exif oldEXIF (newEXIF);
			
			// For these options, we start from nothing, and only fill in the
			// fields that we absolutely need.
			
			newXMP.RemoveProperties (NULL);
			
			newEXIF.SetEmpty ();
			
			metadata.ClearMakerNote ();
			
			// Move copyright related fields over.
			
			CopyAltLangDefault (oldXMP,
								newXMP,
								XMP_NS_DC,
								"rights",
								&newEXIF.fCopyright);
												
			CopyAltLangDefault (oldXMP,
								newXMP,
								XMP_NS_XAP_RIGHTS,
								"UsageTerms");
								
			CopyString (oldXMP,
						newXMP,
						XMP_NS_XAP_RIGHTS,
						"WebStatement");
						
			CopyBoolean (oldXMP,
						 newXMP,
						 XMP_NS_XAP_RIGHTS,
						 "Marked");
						 
			#if qDNGXMPDocOps
			
			// Include basic DocOps fields, but not the full history.
			
			CopyString (oldXMP,
						newXMP,
						XMP_NS_MM,
						"OriginalDocumentID");
						
			CopyString (oldXMP,
						newXMP,
						XMP_NS_MM,
						"DocumentID");
			
			CopyString (oldXMP,
						newXMP,
						XMP_NS_MM,
						"InstanceID");
			
			CopyString (oldXMP,
						newXMP,
						XMP_NS_XAP,
						"MetadataDate");
			
			#endif
			
			// Copyright and Contact adds the contact info fields.
			
			if (metadataSubset == kMetadataSubset_CopyrightAndContact)
				{
				
				// Note: Save for Web is not including the dc:creator list, but it
				// is part of the IPTC contract info metadata panel, so I 
				// think it should be copied as part of the contact info.
				
				CopyStringList (oldXMP,
								newXMP,
								XMP_NS_DC,
								"creator",
								false);
								
				// The first string dc:creator list is mirrored to the
				// the exif artist tag, so copy that also.
								
				newEXIF.fArtist = oldEXIF.fArtist;
				
				// Copy other contact fields.
				
				CopyString (oldXMP,
							newXMP,
							XMP_NS_PHOTOSHOP,
							"AuthorsPosition");
							
				CopyStructField (oldXMP,
								 newXMP,
								 XMP_NS_IPTC,
								 "CreatorContactInfo",
								 "CiEmailWork");
							
				CopyStructField (oldXMP,
								 newXMP,
								 XMP_NS_IPTC,
								 "CreatorContactInfo",
								 "CiAdrExtadr");
							
				CopyStructField (oldXMP,
								 newXMP,
								 XMP_NS_IPTC,
								 "CreatorContactInfo",
								 "CiAdrCity");
							
				CopyStructField (oldXMP,
								 newXMP,
								 XMP_NS_IPTC,
								 "CreatorContactInfo",
								 "CiAdrRegion");
							
				CopyStructField (oldXMP,
								 newXMP,
								 XMP_NS_IPTC,
								 "CreatorContactInfo",
								 "CiAdrPcode");
							
				CopyStructField (oldXMP,
								 newXMP,
								 XMP_NS_IPTC,
								 "CreatorContactInfo",
								 "CiAdrCtry");
							
				CopyStructField (oldXMP,
								 newXMP,
								 XMP_NS_IPTC,
								 "CreatorContactInfo",
								 "CiTelWork");
							
				CopyStructField (oldXMP,
								 newXMP,
								 XMP_NS_IPTC,
								 "CreatorContactInfo",
								 "CiUrlWork");
								 
				CopyAltLangDefault (oldXMP,
									newXMP,
									XMP_NS_DC,
									"title");
												
				}

 			}
			
		else if (metadataSubset == kMetadataSubset_AllExceptCameraInfo        ||
				 metadataSubset == kMetadataSubset_AllExceptCameraAndLocation ||
				 metadataSubset == kMetadataSubset_AllExceptLocationInfo)
			{
			
			dng_xmp  oldXMP  (newXMP );
			dng_exif oldEXIF (newEXIF);
			
			if (metadataSubset == kMetadataSubset_AllExceptCameraInfo ||
				metadataSubset == kMetadataSubset_AllExceptCameraAndLocation)
				{
			
				// This removes most of the EXIF info, so just copy the fields
				// we are not deleting.
				
				newEXIF.SetEmpty ();
				
				newEXIF.fImageDescription  = oldEXIF.fImageDescription;		// Note: Differs from SFW
				newEXIF.fSoftware          = oldEXIF.fSoftware;
				newEXIF.fArtist            = oldEXIF.fArtist;
				newEXIF.fCopyright         = oldEXIF.fCopyright;
				newEXIF.fCopyright2        = oldEXIF.fCopyright2;
				newEXIF.fDateTime          = oldEXIF.fDateTime;
				newEXIF.fDateTimeOriginal  = oldEXIF.fDateTimeOriginal;
				newEXIF.fDateTimeDigitized = oldEXIF.fDateTimeDigitized;
				newEXIF.fExifVersion       = oldEXIF.fExifVersion;
				newEXIF.fImageUniqueID	   = oldEXIF.fImageUniqueID;
				
				newEXIF.CopyGPSFrom (oldEXIF);
				
				// Remove exif info from XMP.
				
				newXMP.RemoveProperties (XMP_NS_EXIF);
				newXMP.RemoveProperties (XMP_NS_AUX);
				
				// Remove Camera Raw info
				
				newXMP.RemoveProperties (XMP_NS_CRS);
				newXMP.RemoveProperties (XMP_NS_CRSS);
				newXMP.RemoveProperties (XMP_NS_CRX);
				
				// Remove DocOps history, since it contains the original
				// camera format.
				
				newXMP.Remove (XMP_NS_MM, "History");
				
				// MakerNote contains camera info.
				
				metadata.ClearMakerNote ();
			
				}
				
			if (metadataSubset == kMetadataSubset_AllExceptLocationInfo ||
				metadataSubset == kMetadataSubset_AllExceptCameraAndLocation)
				{
				
				// Remove GPS fields.
				
				dng_exif blankExif;
				
				newEXIF.CopyGPSFrom (blankExif);
				
				// Remove MakerNote just in case, because we don't know
				// all of what is in it.
				
				metadata.ClearMakerNote ();
				
				// Remove XMP & IPTC location fields.
				
				newXMP.Remove (XMP_NS_PHOTOSHOP, "City");
				newXMP.Remove (XMP_NS_PHOTOSHOP, "State");
				newXMP.Remove (XMP_NS_PHOTOSHOP, "Country");
				newXMP.Remove (XMP_NS_IPTC, "Location");
				newXMP.Remove (XMP_NS_IPTC, "CountryCode");
				newXMP.Remove (XMP_NS_IPTC_EXT, "LocationCreated");
				newXMP.Remove (XMP_NS_IPTC_EXT, "LocationShown");
				
				}
			
			}
									  
		// Rebuild the legacy IPTC block, if needed.
		
		bool isTIFF = (strcmp (dstMIMI, "image/tiff") == 0);
		bool isDNG  = (strcmp (dstMIMI, "image/dng" ) == 0);

		if (!isDNG)
			{
		
			metadata.RebuildIPTC (host.Allocator (),
								  isTIFF);
								  
			}
			
		else
			{
			
			metadata.ClearIPTC ();
			
			}
	
		// Clear format related XMP.
									  
		newXMP.ClearOrientation ();
		
		newXMP.ClearImageInfo ();
		
		newXMP.RemoveProperties (XMP_NS_DNG);
		
		// All the formats we care about already keep the IPTC digest
		// elsewhere, do we don't need to write it to the XMP.
		
		newXMP.ClearIPTCDigest ();
		
		// Make sure that sidecar specific tags never get written to files.
		
		newXMP.Remove (XMP_NS_PHOTOSHOP, "SidecarForExtension");
		newXMP.Remove (XMP_NS_PHOTOSHOP, "EmbeddedXMPDigest");
		
		}
	
	}

/*****************************************************************************/

void dng_image_writer::WriteTIFF (dng_host &host,
								  dng_stream &stream,
								  const dng_image &image,
								  uint32 photometricInterpretation,
								  uint32 compression,
								  dng_negative *negative,
								  const dng_color_space *space,
								  const dng_resolution *resolution,
								  const dng_jpeg_preview *thumbnail,
								  const dng_memory_block *imageResources,
								  dng_metadata_subset metadataSubset)
	{
	
	WriteTIFF (host,
			   stream,
			   image,
			   photometricInterpretation,
			   compression,
			   negative ? &(negative->Metadata ()) : NULL,
			   space,
			   resolution,
			   thumbnail,
			   imageResources,
			   metadataSubset);
	
	}
	
/*****************************************************************************/

void dng_image_writer::WriteTIFF (dng_host &host,
								  dng_stream &stream,
								  const dng_image &image,
								  uint32 photometricInterpretation,
								  uint32 compression,
								  const dng_metadata *metadata,
								  const dng_color_space *space,
								  const dng_resolution *resolution,
								  const dng_jpeg_preview *thumbnail,
								  const dng_memory_block *imageResources,
								  dng_metadata_subset metadataSubset)
	{
	
	const void *profileData = NULL;
	uint32 profileSize = 0;
	
	const uint8 *data = NULL;
	uint32 size = 0;
	
	if (space && space->ICCProfile (size, data))
		{
		
		profileData = data;
		profileSize = size;
		
		}
		
	WriteTIFFWithProfile (host,
						  stream,
						  image,
						  photometricInterpretation,
						  compression,
						  metadata,
						  profileData,
						  profileSize,
						  resolution,
						  thumbnail,
						  imageResources,
						  metadataSubset);
	
	}

/*****************************************************************************/

void dng_image_writer::WriteTIFFWithProfile (dng_host &host,
											 dng_stream &stream,
											 const dng_image &image,
											 uint32 photometricInterpretation,
											 uint32 compression,
											 dng_negative *negative,
											 const void *profileData,
											 uint32 profileSize,
											 const dng_resolution *resolution,
											 const dng_jpeg_preview *thumbnail,
											 const dng_memory_block *imageResources,
											 dng_metadata_subset metadataSubset)
	{
	
	WriteTIFFWithProfile (host,
						  stream,
						  image,
						  photometricInterpretation,
						  compression,
						  negative ? &(negative->Metadata ()) : NULL,
						  profileData,
						  profileSize,
						  resolution,
						  thumbnail,
						  imageResources,
						  metadataSubset);
	
	}
	
/*****************************************************************************/

void dng_image_writer::WriteTIFFWithProfile (dng_host &host,
											 dng_stream &stream,
											 const dng_image &image,
											 uint32 photometricInterpretation,
											 uint32 compression,
											 const dng_metadata *constMetadata,
											 const void *profileData,
											 uint32 profileSize,
											 const dng_resolution *resolution,
											 const dng_jpeg_preview *thumbnail,
											 const dng_memory_block *imageResources,
											 dng_metadata_subset metadataSubset)
	{
	
	uint32 j;
	
	AutoPtr<dng_metadata> metadata;
	
	if (constMetadata)
		{
		
		metadata.Reset (constMetadata->Clone (host.Allocator ()));
		
		CleanUpMetadata (host, 
						 *metadata,
						 metadataSubset,
						 "image/tiff");
		
		}
	
	dng_ifd ifd;
	
	ifd.fNewSubFileType = sfMainImage;
	
	ifd.fImageWidth  = image.Bounds ().W ();
	ifd.fImageLength = image.Bounds ().H ();
	
	ifd.fSamplesPerPixel = image.Planes ();
	
	ifd.fBitsPerSample [0] = TagTypeSize (image.PixelType ()) * 8;
	
	for (j = 1; j < ifd.fSamplesPerPixel; j++)
		{
		ifd.fBitsPerSample [j] = ifd.fBitsPerSample [0];
		}
		
	ifd.fPhotometricInterpretation = photometricInterpretation;
	
	ifd.fCompression = compression;
	
	if (ifd.fCompression == ccUncompressed)
		{
		
		ifd.SetSingleStrip ();
		
		}
		
	else
		{
		
		ifd.FindStripSize (128 * 1024);
		
		ifd.fPredictor = cpHorizontalDifference;
		
		}

	uint32 extraSamples = 0;
	
	switch (photometricInterpretation)
		{
		
		case piBlackIsZero:
			{
			extraSamples = image.Planes () - 1;
			break;
			}
			
		case piRGB:
			{
			extraSamples = image.Planes () - 3;
			break;
			}
			
		default:
			break;
			
		}
		
	ifd.fExtraSamplesCount = extraSamples;
	
	if (image.PixelType () == ttFloat)
		{
		
		for (j = 0; j < ifd.fSamplesPerPixel; j++)
			{
			ifd.fSampleFormat [j] = sfFloatingPoint;
			}
			
		}
	
	dng_tiff_directory mainIFD;
	
	dng_basic_tag_set basic (mainIFD, ifd);
	
	// Resolution.
	
	dng_resolution res;
	
	if (resolution)
		{
		res = *resolution;
		}
	
	tag_urational tagXResolution (tcXResolution, res.fXResolution);
	tag_urational tagYResolution (tcYResolution, res.fYResolution);
	
	tag_uint16 tagResolutionUnit (tcResolutionUnit, res.fResolutionUnit);
	
	if (resolution)
		{
		mainIFD.Add (&tagXResolution   );
		mainIFD.Add (&tagYResolution   );
		mainIFD.Add (&tagResolutionUnit);
		}

	// ICC Profile.
	
	tag_icc_profile iccProfileTag (profileData, profileSize);
	
	if (iccProfileTag.Count ())
		{
		mainIFD.Add (&iccProfileTag);
		}
		
	// XMP metadata.
	
	tag_xmp tagXMP (metadata.Get () ? metadata->GetXMP () : NULL);
	
	if (tagXMP.Count ())
		{
		mainIFD.Add (&tagXMP);
		}
		
	// IPTC metadata.
	
	tag_iptc tagIPTC (metadata.Get () ? metadata->IPTCData   () : NULL,
					  metadata.Get () ? metadata->IPTCLength () : 0);
		
	if (tagIPTC.Count ())
		{
		mainIFD.Add (&tagIPTC);
		}
		
	// Adobe data (thumbnail and IPTC digest)
	
	AutoPtr<dng_memory_block> adobeData (BuildAdobeData (host,
														 metadata.Get (),
														 thumbnail,
														 imageResources));
														 
	tag_uint8_ptr tagAdobe (tcAdobeData,
							adobeData->Buffer_uint8 (),
							adobeData->LogicalSize ());
							     
	if (tagAdobe.Count ())
		{
		mainIFD.Add (&tagAdobe);
		}
		
	// Exif metadata.
	
	exif_tag_set exifSet (mainIFD,
						  metadata.Get () && metadata->GetExif () ? *metadata->GetExif () 
																  : dng_exif (),
						  metadata.Get () ? metadata->IsMakerNoteSafe () : false,
						  metadata.Get () ? metadata->MakerNoteData   () : NULL,
						  metadata.Get () ? metadata->MakerNoteLength () : 0,
						  false);

	// Find offset to main image data.
	
	uint32 offsetMainIFD = 8;
	
	uint32 offsetExifData = offsetMainIFD + mainIFD.Size ();
	
	exifSet.Locate (offsetExifData);
	
	uint32 offsetMainData = offsetExifData + exifSet.Size ();
		
	stream.SetWritePosition (offsetMainData);
	
	// Write the main image data.
	
	WriteImage (host,
				ifd,
				basic,
				stream,
				image);
				
	// Trim the file to this length.
	
	stream.SetLength (stream.Position ());
	
	// TIFF has a 4G size limit.
	
	if (stream.Length () > 0x0FFFFFFFFL)
		{
		ThrowImageTooBigTIFF ();
		}
	
	// Write TIFF Header.
	
	stream.SetWritePosition (0);
	
	stream.Put_uint16 (stream.BigEndian () ? byteOrderMM : byteOrderII);
	
	stream.Put_uint16 (42);
	
	stream.Put_uint32 (offsetMainIFD);
	
	// Write the IFDs.
	
	mainIFD.Put (stream);
	
	exifSet.Put (stream);
	
	stream.Flush ();
	
	}
							   
/*****************************************************************************/

void dng_image_writer::WriteDNG (dng_host &host,
							     dng_stream &stream,
							     dng_negative &negative,
							     const dng_preview_list *previewList,
								 uint32 maxBackwardVersion,
							     bool uncompressed)
	{
	
	WriteDNG (host,
			  stream,
			  negative,
			  negative.Metadata (),
			  previewList,
			  maxBackwardVersion,
			  uncompressed);
	
	}
	
/*****************************************************************************/

void dng_image_writer::WriteDNG (dng_host &host,
							     dng_stream &stream,
							     const dng_negative &negative,
								 const dng_metadata &constMetadata,
								 const dng_preview_list *previewList,
								 uint32 maxBackwardVersion,
							     bool uncompressed)
	{

	uint32 j;
	
	// Clean up metadata per MWG recommendations.
	
	AutoPtr<dng_metadata> metadata (constMetadata.Clone (host.Allocator ()));

	CleanUpMetadata (host, 
					 *metadata,
					 kMetadataSubset_All,
					 "image/dng");
					 
	// Figure out the compression to use.  Most of the time this is lossless
	// JPEG.
	
	uint32 compression = uncompressed ? ccUncompressed : ccJPEG;
		
	// Was the the original file lossy JPEG compressed?
	
	const dng_jpeg_image *rawJPEGImage = negative.RawJPEGImage ();
	
	// If so, can we save it using the requested compression and DNG version?
	
	if (uncompressed || maxBackwardVersion < dngVersion_1_4_0_0)
		{
		
		if (rawJPEGImage || negative.RawJPEGImageDigest ().IsValid ())
			{
			
			rawJPEGImage = NULL;
			
			negative.ClearRawJPEGImageDigest ();
			
			negative.ClearRawImageDigest ();
			
			}
		
		}
		
	else if (rawJPEGImage)
		{
		
		compression = ccLossyJPEG;
		
		}
		
	// Are we saving the original size tags?
	
	bool saveOriginalDefaultFinalSize     = false;
	bool saveOriginalBestQualityFinalSize = false;
	bool saveOriginalDefaultCropSize      = false;
	
		{
		
		// See if we are saving a proxy image.
		
		dng_point defaultFinalSize (negative.DefaultFinalHeight (),
									negative.DefaultFinalWidth  ());
									
		saveOriginalDefaultFinalSize = (negative.OriginalDefaultFinalSize () !=
										defaultFinalSize);
		
		if (saveOriginalDefaultFinalSize)
			{
			
			// If the save OriginalDefaultFinalSize tag, this changes the defaults
			// for the OriginalBestQualityFinalSize and OriginalDefaultCropSize tags.
			
			saveOriginalBestQualityFinalSize = (negative.OriginalBestQualityFinalSize () != 
												defaultFinalSize);
												
			saveOriginalDefaultCropSize = (negative.OriginalDefaultCropSizeV () !=
										   dng_urational (defaultFinalSize.v, 1)) ||
										  (negative.OriginalDefaultCropSizeH () !=
										   dng_urational (defaultFinalSize.h, 1));

			}
			
		else
			{
			
			// Else these two tags default to the normal non-proxy size image values.
			
			dng_point bestQualityFinalSize (negative.BestQualityFinalHeight (),
											negative.BestQualityFinalWidth  ());
											
			saveOriginalBestQualityFinalSize = (negative.OriginalBestQualityFinalSize () != 
												bestQualityFinalSize);
												
			saveOriginalDefaultCropSize = (negative.OriginalDefaultCropSizeV () !=
										   negative.DefaultCropSizeV ()) ||
										  (negative.OriginalDefaultCropSizeH () !=
										   negative.DefaultCropSizeH ());
			
			}
		
		}
		
	// Is this a floating point image that we are saving?
	
	bool isFloatingPoint = (negative.RawImage ().PixelType () == ttFloat);
	
	// Does this image have a transparency mask?
	
	bool hasTransparencyMask = (negative.RawTransparencyMask () != NULL);
	
	// Should we save a compressed 32-bit integer file?
	
	bool isCompressed32BitInteger = (negative.RawImage ().PixelType () == ttLong) &&
								    (maxBackwardVersion >= dngVersion_1_4_0_0) &&
									(!uncompressed);
	
	// Figure out what main version to use.
	
	uint32 dngVersion = dngVersion_Current;
	
	// Don't write version 1.4 files unless we actually use some feature of the 1.4 spec.
	
	if (dngVersion == dngVersion_1_4_0_0)
		{
		
		if (!rawJPEGImage                     &&
			!isFloatingPoint				  &&
			!hasTransparencyMask			  &&
			!isCompressed32BitInteger		  &&
			!saveOriginalDefaultFinalSize     &&
			!saveOriginalBestQualityFinalSize &&
			!saveOriginalDefaultCropSize      )
			{
				
			dngVersion = dngVersion_1_3_0_0;
				
			}
			
		}
	
	// Figure out what backward version to use.
	
	uint32 dngBackwardVersion = dngVersion_1_1_0_0;
	
	#if defined(qTestRowInterleave) || defined(qTestSubTileBlockRows) || defined(qTestSubTileBlockCols)
	dngBackwardVersion = Max_uint32 (dngBackwardVersion, dngVersion_1_2_0_0);
	#endif
	
	dngBackwardVersion = Max_uint32 (dngBackwardVersion,
									 negative.OpcodeList1 ().MinVersion (false));

	dngBackwardVersion = Max_uint32 (dngBackwardVersion,
									 negative.OpcodeList2 ().MinVersion (false));

	dngBackwardVersion = Max_uint32 (dngBackwardVersion,
									 negative.OpcodeList3 ().MinVersion (false));
									 
	if (negative.GetMosaicInfo () &&
		negative.GetMosaicInfo ()->fCFALayout >= 6)
		{
		dngBackwardVersion = Max_uint32 (dngBackwardVersion, dngVersion_1_3_0_0);
		}
		
	if (rawJPEGImage || isFloatingPoint || hasTransparencyMask || isCompressed32BitInteger)
		{
		dngBackwardVersion = Max_uint32 (dngBackwardVersion, dngVersion_1_4_0_0);
		}
									 
	if (dngBackwardVersion > dngVersion)
		{
		ThrowProgramError ();
		}
		
	// Find best thumbnail from preview list, if any.

	const dng_preview *thumbnail = NULL;
	
	if (previewList)
		{
		
		uint32 thumbArea = 0;
		
		for (j = 0; j < previewList->Count (); j++)
			{
			
			const dng_image_preview *imagePreview = dynamic_cast<const dng_image_preview *>(&previewList->Preview (j));
			
			if (imagePreview)
				{
				
				uint32 thisArea = imagePreview->fImage->Bounds ().W () *
								  imagePreview->fImage->Bounds ().H ();
								  
				if (!thumbnail || thisArea < thumbArea)
					{
					
					thumbnail = &previewList->Preview (j);
					
					thumbArea = thisArea;
					
					}
			
				}
				
			const dng_jpeg_preview *jpegPreview = dynamic_cast<const dng_jpeg_preview *>(&previewList->Preview (j));
			
			if (jpegPreview)
				{
				
				uint32 thisArea = jpegPreview->fPreviewSize.h *
								  jpegPreview->fPreviewSize.v;
								  
				if (!thumbnail || thisArea < thumbArea)
					{
					
					thumbnail = &previewList->Preview (j);
					
					thumbArea = thisArea;
					
					}
				
				}
				
			}
		
		}
		
	// Create the main IFD
										 
	dng_tiff_directory mainIFD;
	
	// Create the IFD for the raw data. If there is no thumnail, this is
	// just a reference the main IFD.  Otherwise allocate a new one.
	
	AutoPtr<dng_tiff_directory> rawIFD_IfNotMain;
	
	if (thumbnail)
		{
		rawIFD_IfNotMain.Reset (new dng_tiff_directory);
		}

	dng_tiff_directory &rawIFD (thumbnail ? *rawIFD_IfNotMain : mainIFD);
	
	// Include DNG version tags.
	
	uint8 dngVersionData [4];
	
	dngVersionData [0] = (uint8) (dngVersion >> 24);
	dngVersionData [1] = (uint8) (dngVersion >> 16);
	dngVersionData [2] = (uint8) (dngVersion >>  8);
	dngVersionData [3] = (uint8) (dngVersion      );
	
	tag_uint8_ptr tagDNGVersion (tcDNGVersion, dngVersionData, 4);
	
	mainIFD.Add (&tagDNGVersion);
	
	uint8 dngBackwardVersionData [4];

	dngBackwardVersionData [0] = (uint8) (dngBackwardVersion >> 24);
	dngBackwardVersionData [1] = (uint8) (dngBackwardVersion >> 16);
	dngBackwardVersionData [2] = (uint8) (dngBackwardVersion >>  8);
	dngBackwardVersionData [3] = (uint8) (dngBackwardVersion      );
	
	tag_uint8_ptr tagDNGBackwardVersion (tcDNGBackwardVersion, dngBackwardVersionData, 4);
	
	mainIFD.Add (&tagDNGBackwardVersion);
	
	// The main IFD contains the thumbnail, if there is a thumbnail.
								
	AutoPtr<dng_basic_tag_set> thmBasic;
	
	if (thumbnail)
		{
		thmBasic.Reset (thumbnail->AddTagSet (mainIFD));
		}
						  
	// Get the raw image we are writing.
	
	const dng_image &rawImage (negative.RawImage ());
	
	// For floating point, we only support ZIP compression.
	
	if (isFloatingPoint && !uncompressed)
		{
		
		compression = ccDeflate;
		
		}
	
	// For 32-bit integer images, we only support ZIP and uncompressed.
	
	if (rawImage.PixelType () == ttLong)
		{
		
		if (isCompressed32BitInteger)
			{
			compression = ccDeflate;
			}
			
		else
			{
			compression = ccUncompressed;
			}
	
		}
	
	// Get a copy of the mosaic info.
	
	dng_mosaic_info mosaicInfo;
	
	if (negative.GetMosaicInfo ())
		{
		mosaicInfo = *(negative.GetMosaicInfo ());
		}
		
	// Create a dng_ifd record for the raw image.
	
	dng_ifd info;
	
	info.fImageWidth  = rawImage.Width  ();
	info.fImageLength = rawImage.Height ();
	
	info.fSamplesPerPixel = rawImage.Planes ();
	
	info.fPhotometricInterpretation = mosaicInfo.IsColorFilterArray () ? piCFA
																	   : piLinearRaw;
			
	info.fCompression = compression;
	
	if (isFloatingPoint && compression == ccDeflate)
		{
		
		info.fPredictor = cpFloatingPoint;
		
		if (mosaicInfo.IsColorFilterArray ())
			{
			
			if (mosaicInfo.fCFAPatternSize.h == 2)
				{
				info.fPredictor = cpFloatingPointX2;
				}
				
			else if (mosaicInfo.fCFAPatternSize.h == 4)
				{
				info.fPredictor = cpFloatingPointX4;
				}
				
			}
			
		} 
		
	if (isCompressed32BitInteger)
		{
		
		info.fPredictor = cpHorizontalDifference;
		
		if (mosaicInfo.IsColorFilterArray ())
			{
			
			if (mosaicInfo.fCFAPatternSize.h == 2)
				{
				info.fPredictor = cpHorizontalDifferenceX2;
				}
				
			else if (mosaicInfo.fCFAPatternSize.h == 4)
				{
				info.fPredictor = cpHorizontalDifferenceX4;
				}
				
			}
			
		}
	
	uint32 rawPixelType = rawImage.PixelType ();
	
	if (rawPixelType == ttShort)
		{
		
		// See if we are using a linearization table with <= 256 entries, in which
		// case the useful data will all fit within 8-bits.
		
		const dng_linearization_info *rangeInfo = negative.GetLinearizationInfo ();
	
		if (rangeInfo)
			{

			if (rangeInfo->fLinearizationTable.Get ())
				{
				
				uint32 entries = rangeInfo->fLinearizationTable->LogicalSize () >> 1;
				
				if (entries <= 256)
					{
					
					rawPixelType = ttByte;
					
					}
												
				}
				
			}

		}
	
	switch (rawPixelType)
		{
		
		case ttByte:
			{
			info.fBitsPerSample [0] = 8;
			break;
			}
		
		case ttShort:
			{
			info.fBitsPerSample [0] = 16;
			break;
			}
			
		case ttLong:
			{
			info.fBitsPerSample [0] = 32;
			break;
			}
			
		case ttFloat:
			{
			
			if (negative.RawFloatBitDepth () == 16)
				{
				info.fBitsPerSample [0] = 16;
				}
				
			else if (negative.RawFloatBitDepth () == 24)
				{
				info.fBitsPerSample [0] = 24;
				}
				
			else
				{
				info.fBitsPerSample [0] = 32;
				}

			for (j = 0; j < info.fSamplesPerPixel; j++)
				{
				info.fSampleFormat [j] = sfFloatingPoint;
				}

			break;
			
			}
			
		default:
			{
			ThrowProgramError ();
			}
			
		}
	
	// For lossless JPEG compression, we often lie about the
	// actual channel count to get the predictors to work across
	// same color mosaic pixels.
	
	uint32 fakeChannels = 1;
	
	if (info.fCompression == ccJPEG)
		{
		
		if (mosaicInfo.IsColorFilterArray ())
			{
			
			if (mosaicInfo.fCFAPatternSize.h == 4)
				{
				fakeChannels = 4;
				}
				
			else if (mosaicInfo.fCFAPatternSize.h == 2)
				{
				fakeChannels = 2;
				}
			
			// However, lossless JEPG is limited to four channels,
			// so compromise might be required.
			
			while (fakeChannels * info.fSamplesPerPixel > 4 &&
				   fakeChannels > 1)
				{
				
				fakeChannels >>= 1;
				
				}
			
			}
	
		}
		
	// Figure out tile sizes.
	
	if (rawJPEGImage)
		{
		
		DNG_ASSERT (rawPixelType == ttByte,
					"Unexpected jpeg pixel type");
		
		DNG_ASSERT (info.fImageWidth  == (uint32) rawJPEGImage->fImageSize.h &&
					info.fImageLength == (uint32) rawJPEGImage->fImageSize.v,
					"Unexpected jpeg image size");
					
		info.fTileWidth  = rawJPEGImage->fTileSize.h;
		info.fTileLength = rawJPEGImage->fTileSize.v;

		info.fUsesStrips = rawJPEGImage->fUsesStrips;
		
		info.fUsesTiles = !info.fUsesStrips;
		
		}
	
	else if (info.fCompression == ccJPEG)
		{
		
		info.FindTileSize (128 * 1024);
		
		}
		
	else if (info.fCompression == ccDeflate)
		{
		
		info.FindTileSize (512 * 1024);
		
		}
		
	else if (info.fCompression == ccLossyJPEG)
		{
		
		ThrowProgramError ("No JPEG compressed image");
				
		}
		
	// Don't use tiles for uncompressed images.
		
	else
		{
		
		info.SetSingleStrip ();
		
		}
		
	#ifdef qTestRowInterleave
	
	info.fRowInterleaveFactor = qTestRowInterleave;
	
	#endif
			
	#if defined(qTestSubTileBlockRows) && defined(qTestSubTileBlockCols)
	
	info.fSubTileBlockRows = qTestSubTileBlockRows;
	info.fSubTileBlockCols = qTestSubTileBlockCols;
	
	if (fakeChannels == 2)
		fakeChannels = 4;
	
	#endif
	
	// Basic information.
	
	dng_basic_tag_set rawBasic (rawIFD, info);
	
	// JPEG tables, if any.
	
	tag_data_ptr tagJPEGTables (tcJPEGTables,
								ttUndefined,
								0,
								NULL);
								
	if (rawJPEGImage && rawJPEGImage->fJPEGTables.Get ())
		{
		
		tagJPEGTables.SetData (rawJPEGImage->fJPEGTables->Buffer ());
		
		tagJPEGTables.SetCount (rawJPEGImage->fJPEGTables->LogicalSize ());
		
		rawIFD.Add (&tagJPEGTables);
		
		}
						  
	// DefaultScale tag.

	dng_urational defaultScaleData [2];
	
	defaultScaleData [0] = negative.DefaultScaleH ();
	defaultScaleData [1] = negative.DefaultScaleV ();
												
	tag_urational_ptr tagDefaultScale (tcDefaultScale,
								       defaultScaleData,
								       2);

	rawIFD.Add (&tagDefaultScale);
	
	// Best quality scale tag.
	
	tag_urational tagBestQualityScale (tcBestQualityScale,
									   negative.BestQualityScale ());
									  
	rawIFD.Add (&tagBestQualityScale);
	
	// DefaultCropOrigin tag.

	dng_urational defaultCropOriginData [2];
	
	defaultCropOriginData [0] = negative.DefaultCropOriginH ();
	defaultCropOriginData [1] = negative.DefaultCropOriginV ();
												
	tag_urational_ptr tagDefaultCropOrigin (tcDefaultCropOrigin,
								            defaultCropOriginData,
								            2);

	rawIFD.Add (&tagDefaultCropOrigin);
	
	// DefaultCropSize tag.

	dng_urational defaultCropSizeData [2];
	
	defaultCropSizeData [0] = negative.DefaultCropSizeH ();
	defaultCropSizeData [1] = negative.DefaultCropSizeV ();
												
	tag_urational_ptr tagDefaultCropSize (tcDefaultCropSize,
								          defaultCropSizeData,
								          2);

	rawIFD.Add (&tagDefaultCropSize);
	
	// DefaultUserCrop tag.

	dng_urational defaultUserCropData [4];
	
	defaultUserCropData [0] = negative.DefaultUserCropT ();
	defaultUserCropData [1] = negative.DefaultUserCropL ();
	defaultUserCropData [2] = negative.DefaultUserCropB ();
	defaultUserCropData [3] = negative.DefaultUserCropR ();
												
	tag_urational_ptr tagDefaultUserCrop (tcDefaultUserCrop,
										  defaultUserCropData,
										  4);

	rawIFD.Add (&tagDefaultUserCrop);
	
	// Range mapping tag set.
	
	range_tag_set rangeSet (rawIFD, negative);
						  
	// Mosaic pattern information.
	
	mosaic_tag_set mosaicSet (rawIFD, mosaicInfo);
			
	// Chroma blur radius.
	
	tag_urational tagChromaBlurRadius (tcChromaBlurRadius,
									   negative.ChromaBlurRadius ());
									  
	if (negative.ChromaBlurRadius ().IsValid ())
		{
		
		rawIFD.Add (&tagChromaBlurRadius);
		
		}
	
	// Anti-alias filter strength.
	
	tag_urational tagAntiAliasStrength (tcAntiAliasStrength,
									    negative.AntiAliasStrength ());
									  
	if (negative.AntiAliasStrength ().IsValid ())
		{
		
		rawIFD.Add (&tagAntiAliasStrength);
		
		}
		
	// Profile and other color related tags.
	
	AutoPtr<profile_tag_set> profileSet;
	
	AutoPtr<color_tag_set> colorSet;
	
	std::vector<uint32> extraProfileIndex;
	
	if (!negative.IsMonochrome ())
		{
		
		const dng_camera_profile &mainProfile (*negative.ComputeCameraProfileToEmbed (constMetadata));
		
		profileSet.Reset (new profile_tag_set (mainIFD,
											   mainProfile));
		
		colorSet.Reset (new color_tag_set (mainIFD,
										   negative));
										   
		// Build list of profile indices to include in extra profiles tag.
										   
		uint32 profileCount = negative.ProfileCount ();
		
		for (uint32 index = 0; index < profileCount; index++)
			{
			
			const dng_camera_profile &profile (negative.ProfileByIndex (index));
			
			if (&profile != &mainProfile)
				{
				
				if (profile.WasReadFromDNG ())
					{
					
					extraProfileIndex.push_back (index);
					
					}
				
				}
				
			}
										   
		}
		
	// Extra camera profiles tag.
	
	uint32 extraProfileCount = (uint32) extraProfileIndex.size ();
	
	dng_memory_data extraProfileOffsets (extraProfileCount * (uint32) sizeof (uint32));
	
	tag_uint32_ptr extraProfileTag (tcExtraCameraProfiles,
									extraProfileOffsets.Buffer_uint32 (),
									extraProfileCount);
									
	if (extraProfileCount)
		{
		
		mainIFD.Add (&extraProfileTag);

		}
			
	// Other tags.
	
	tag_uint16 tagOrientation (tcOrientation,
						       (uint16) negative.ComputeOrientation (constMetadata).GetTIFF ());
							   
	mainIFD.Add (&tagOrientation);

	tag_srational tagBaselineExposure (tcBaselineExposure,
								       negative.BaselineExposureR ());
										  
	mainIFD.Add (&tagBaselineExposure);

	tag_urational tagBaselineNoise (tcBaselineNoise,
							        negative.BaselineNoiseR ());
										  
	mainIFD.Add (&tagBaselineNoise);
	
	tag_urational tagNoiseReductionApplied (tcNoiseReductionApplied,
											negative.NoiseReductionApplied ());
											
	if (negative.NoiseReductionApplied ().IsValid ())
		{
		
		mainIFD.Add (&tagNoiseReductionApplied);
	
		}

	tag_dng_noise_profile tagNoiseProfile (negative.NoiseProfile ());
		
	if (negative.NoiseProfile ().IsValidForNegative (negative))
		{

		mainIFD.Add (&tagNoiseProfile);
		
		}

	tag_urational tagBaselineSharpness (tcBaselineSharpness,
								        negative.BaselineSharpnessR ());
										  
	mainIFD.Add (&tagBaselineSharpness);

	tag_string tagUniqueName (tcUniqueCameraModel,
						      negative.ModelName (),
						      true);
								
	mainIFD.Add (&tagUniqueName);
	
	tag_string tagLocalName (tcLocalizedCameraModel,
						     negative.LocalName (),
						     false);
						   
	if (negative.LocalName ().NotEmpty ())
		{
		
		mainIFD.Add (&tagLocalName);
	
		}
	
	tag_urational tagShadowScale (tcShadowScale,
							      negative.ShadowScaleR ());
										  
	mainIFD.Add (&tagShadowScale);
	
	tag_uint16 tagColorimetricReference (tcColorimetricReference,
										 (uint16) negative.ColorimetricReference ());
										 
	if (negative.ColorimetricReference () != crSceneReferred)
		{
		
		mainIFD.Add (&tagColorimetricReference);
		
		}
		
	bool useNewDigest = (maxBackwardVersion >= dngVersion_1_4_0_0);
		
	if (compression == ccLossyJPEG)
		{
		
		negative.FindRawJPEGImageDigest (host);
		
		}
		
	else
		{
		
		if (useNewDigest)
			{
			negative.FindNewRawImageDigest (host);
			}
		else
			{
			negative.FindRawImageDigest (host);
			}
		
		}
	
	tag_uint8_ptr tagRawImageDigest (useNewDigest ? tcNewRawImageDigest : tcRawImageDigest,
									 compression == ccLossyJPEG ?
									 negative.RawJPEGImageDigest ().data :
									 (useNewDigest ? negative.NewRawImageDigest ().data
												   : negative.RawImageDigest    ().data),
							   		 16);
							   		  
	mainIFD.Add (&tagRawImageDigest);
	
	negative.FindRawDataUniqueID (host);
	
	tag_uint8_ptr tagRawDataUniqueID (tcRawDataUniqueID,
							   		  negative.RawDataUniqueID ().data,
							   		  16);
							   		  
	if (negative.RawDataUniqueID ().IsValid ())
		{
							   
		mainIFD.Add (&tagRawDataUniqueID);
		
		}
	
	tag_string tagOriginalRawFileName (tcOriginalRawFileName,
						   			   negative.OriginalRawFileName (),
						   			   false);
						   
	if (negative.HasOriginalRawFileName ())
		{
		
		mainIFD.Add (&tagOriginalRawFileName);
	
		}
		
	negative.FindOriginalRawFileDigest ();
		
	tag_data_ptr tagOriginalRawFileData (tcOriginalRawFileData,
										 ttUndefined,
										 negative.OriginalRawFileDataLength (),
										 negative.OriginalRawFileData       ());
										 
	tag_uint8_ptr tagOriginalRawFileDigest (tcOriginalRawFileDigest,
											negative.OriginalRawFileDigest ().data,
											16);
										 
	if (negative.OriginalRawFileData ())
		{
		
		mainIFD.Add (&tagOriginalRawFileData);
		
		mainIFD.Add (&tagOriginalRawFileDigest);
	
		}

	// XMP metadata.
	
	tag_xmp tagXMP (metadata->GetXMP ());
	
	if (tagXMP.Count ())
		{
		
		mainIFD.Add (&tagXMP);
		
		}
		
	// Exif tags.
	
	exif_tag_set exifSet (mainIFD,
						  *metadata->GetExif (),
						  metadata->IsMakerNoteSafe (),
						  metadata->MakerNoteData   (),
						  metadata->MakerNoteLength (),
						  true);
						
	// Private data.
	
	tag_uint8_ptr tagPrivateData (tcDNGPrivateData,
						   		  negative.PrivateData (),
						   		  negative.PrivateLength ());
						   
	if (negative.PrivateLength ())
		{
		
		mainIFD.Add (&tagPrivateData);
		
		}
		
	// Proxy size tags.
	
	uint32 originalDefaultFinalSizeData [2];
	
	originalDefaultFinalSizeData [0] = negative.OriginalDefaultFinalSize ().h;
	originalDefaultFinalSizeData [1] = negative.OriginalDefaultFinalSize ().v;
	
	tag_uint32_ptr tagOriginalDefaultFinalSize (tcOriginalDefaultFinalSize,
												originalDefaultFinalSizeData,
												2);
	
	if (saveOriginalDefaultFinalSize)
		{
		
		mainIFD.Add (&tagOriginalDefaultFinalSize);
		
		}
		
	uint32 originalBestQualityFinalSizeData [2];
	
	originalBestQualityFinalSizeData [0] = negative.OriginalBestQualityFinalSize ().h;
	originalBestQualityFinalSizeData [1] = negative.OriginalBestQualityFinalSize ().v;
	
	tag_uint32_ptr tagOriginalBestQualityFinalSize (tcOriginalBestQualityFinalSize,
													originalBestQualityFinalSizeData,
													2);
	
	if (saveOriginalBestQualityFinalSize)
		{
		
		mainIFD.Add (&tagOriginalBestQualityFinalSize);
		
		}
		
	dng_urational originalDefaultCropSizeData [2];
	
	originalDefaultCropSizeData [0] = negative.OriginalDefaultCropSizeH ();
	originalDefaultCropSizeData [1] = negative.OriginalDefaultCropSizeV ();
	
	tag_urational_ptr tagOriginalDefaultCropSize (tcOriginalDefaultCropSize,
												  originalDefaultCropSizeData,
												  2);
	
	if (saveOriginalDefaultCropSize)
		{
		
		mainIFD.Add (&tagOriginalDefaultCropSize);
		
		}
		
	// Opcode list 1.
	
	AutoPtr<dng_memory_block> opcodeList1Data (negative.OpcodeList1 ().Spool (host));
	
	tag_data_ptr tagOpcodeList1 (tcOpcodeList1,
								 ttUndefined,
								 opcodeList1Data.Get () ? opcodeList1Data->LogicalSize () : 0,
								 opcodeList1Data.Get () ? opcodeList1Data->Buffer      () : NULL);
								 
	if (opcodeList1Data.Get ())
		{
		
		rawIFD.Add (&tagOpcodeList1);
		
		}
		
	// Opcode list 2.
	
	AutoPtr<dng_memory_block> opcodeList2Data (negative.OpcodeList2 ().Spool (host));
	
	tag_data_ptr tagOpcodeList2 (tcOpcodeList2,
								 ttUndefined,
								 opcodeList2Data.Get () ? opcodeList2Data->LogicalSize () : 0,
								 opcodeList2Data.Get () ? opcodeList2Data->Buffer      () : NULL);
								 
	if (opcodeList2Data.Get ())
		{
		
		rawIFD.Add (&tagOpcodeList2);
		
		}
		
	// Opcode list 3.
	
	AutoPtr<dng_memory_block> opcodeList3Data (negative.OpcodeList3 ().Spool (host));
	
	tag_data_ptr tagOpcodeList3 (tcOpcodeList3,
								 ttUndefined,
								 opcodeList3Data.Get () ? opcodeList3Data->LogicalSize () : 0,
								 opcodeList3Data.Get () ? opcodeList3Data->Buffer      () : NULL);
								 
	if (opcodeList3Data.Get ())
		{
		
		rawIFD.Add (&tagOpcodeList3);
		
		}
		
	// Transparency mask, if any.
	
	AutoPtr<dng_ifd> maskInfo;
	
	AutoPtr<dng_tiff_directory> maskIFD;
	
	AutoPtr<dng_basic_tag_set> maskBasic;
	
	if (hasTransparencyMask)
		{
		
		// Create mask IFD.
		
		maskInfo.Reset (new dng_ifd);
		
		maskInfo->fNewSubFileType = sfTransparencyMask;
		
		maskInfo->fImageWidth  = negative.RawTransparencyMask ()->Bounds ().W ();
		maskInfo->fImageLength = negative.RawTransparencyMask ()->Bounds ().H ();
		
		maskInfo->fSamplesPerPixel = 1;
		
		maskInfo->fBitsPerSample [0] = negative.RawTransparencyMaskBitDepth ();
		
		maskInfo->fPhotometricInterpretation = piTransparencyMask;
		
		maskInfo->fCompression = uncompressed ? ccUncompressed  : ccDeflate;
		maskInfo->fPredictor   = uncompressed ? cpNullPredictor : cpHorizontalDifference;
		
		if (negative.RawTransparencyMask ()->PixelType () == ttFloat)
			{
			
			maskInfo->fSampleFormat [0] = sfFloatingPoint;
			
			if (maskInfo->fCompression == ccDeflate)
				{
				maskInfo->fPredictor = cpFloatingPoint;
				}
			
			}
				
		if (maskInfo->fCompression == ccDeflate)
			{
			maskInfo->FindTileSize (512 * 1024);
			}
		else
			{
			maskInfo->SetSingleStrip ();
			}
			
		// Create mask tiff directory.
			
		maskIFD.Reset (new dng_tiff_directory);
		
		// Add mask basic tag set.
		
		maskBasic.Reset (new dng_basic_tag_set (*maskIFD, *maskInfo));
				
		}
		
	// Add other subfiles.
		
	uint32 subFileCount = thumbnail ? 1 : 0;
	
	if (hasTransparencyMask)
		{
		subFileCount++;
		}
	
	// Add previews.
	
	uint32 previewCount = previewList ? previewList->Count () : 0;
	
	AutoPtr<dng_tiff_directory> previewIFD [kMaxDNGPreviews];
	
	AutoPtr<dng_basic_tag_set> previewBasic [kMaxDNGPreviews];
	
	for (j = 0; j < previewCount; j++)
		{
		
		if (thumbnail != &previewList->Preview (j))
			{
		
			previewIFD [j] . Reset (new dng_tiff_directory);
			
			previewBasic [j] . Reset (previewList->Preview (j).AddTagSet (*previewIFD [j]));
				
			subFileCount++;
			
			}
		
		}
		
	// And a link to the raw and JPEG image IFDs.
	
	uint32 subFileData [kMaxDNGPreviews + 2];
	
	tag_uint32_ptr tagSubFile (tcSubIFDs,
							   subFileData,
							   subFileCount);
							   
	if (subFileCount)
		{
	
		mainIFD.Add (&tagSubFile);
		
		}
	
	// Skip past the header and IFDs for now.
	
	uint32 currentOffset = 8;
	
	currentOffset += mainIFD.Size ();
	
	uint32 subFileIndex = 0;
	
	if (thumbnail)
		{
	
		subFileData [subFileIndex++] = currentOffset;
	
		currentOffset += rawIFD.Size ();
		
		}
		
	if (hasTransparencyMask)
		{
		
		subFileData [subFileIndex++] = currentOffset;
		
		currentOffset += maskIFD->Size ();
		
		}
	
	for (j = 0; j < previewCount; j++)
		{
		
		if (thumbnail != &previewList->Preview (j))
			{
	
			subFileData [subFileIndex++] = currentOffset;

			currentOffset += previewIFD [j]->Size ();
			
			}
		
		}
		
	exifSet.Locate (currentOffset);
	
	currentOffset += exifSet.Size ();
	
	stream.SetWritePosition (currentOffset);
	
	// Write the extra profiles.
	
	if (extraProfileCount)
		{
		
		for (j = 0; j < extraProfileCount; j++)
			{
			
			extraProfileOffsets.Buffer_uint32 () [j] = (uint32) stream.Position ();
			
			uint32 index = extraProfileIndex [j];
			
			const dng_camera_profile &profile (negative.ProfileByIndex (index));
			
			tiff_dng_extended_color_profile extraWriter (profile);
			
			extraWriter.Put (stream, false);
			
			}
		
		}
	
	// Write the thumbnail data.
	
	if (thumbnail)
		{
	
		thumbnail->WriteData (host,
							  *this,
							  *thmBasic,
							  stream);
							 
		}
	
	// Write the preview data.
	
	for (j = 0; j < previewCount; j++)
		{
		
		if (thumbnail != &previewList->Preview (j))
			{
	
			previewList->Preview (j).WriteData (host,
												*this,
												*previewBasic [j],
												stream);
												
			}
			
		}
		
	// Write the raw data.
	
	if (rawJPEGImage)
		{
		
		uint32 tileCount = info.TilesAcross () *
						   info.TilesDown   ();
							
		for (uint32 tileIndex = 0; tileIndex < tileCount; tileIndex++)
			{
			
			// Remember this offset.
			
			uint32 tileOffset = (uint32) stream.Position ();
		
			rawBasic.SetTileOffset (tileIndex, tileOffset);
			
			// Write JPEG data.
			
			stream.Put (rawJPEGImage->fJPEGData [tileIndex]->Buffer      (),
						rawJPEGImage->fJPEGData [tileIndex]->LogicalSize ());
							
			// Update tile count.
				
			uint32 tileByteCount = (uint32) stream.Position () - tileOffset;
				
			rawBasic.SetTileByteCount (tileIndex, tileByteCount);
			
			// Keep the tiles on even byte offsets.
												 
			if (tileByteCount & 1)
				{
				stream.Put_uint8 (0);
				}

			}
		
		}
		
	else
		{
		
		#if qDNGValidate
		dng_timer timer ("Write raw image time");
		#endif
	
		WriteImage (host,
					info,
					rawBasic,
					stream,
					rawImage,
					fakeChannels);
					
		}
		
	// Write transparency mask image.
	
	if (hasTransparencyMask)
		{
		
		#if qDNGValidate
		dng_timer timer ("Write transparency mask time");
		#endif
	
		WriteImage (host,
					*maskInfo,
					*maskBasic,
					stream,
					*negative.RawTransparencyMask ());
					
		}
					
	// Trim the file to this length.
	
	stream.SetLength (stream.Position ());
	
	// DNG has a 4G size limit.
	
	if (stream.Length () > 0x0FFFFFFFFL)
		{
		ThrowImageTooBigDNG ();
		}
	
	// Write TIFF Header.
	
	stream.SetWritePosition (0);
	
	stream.Put_uint16 (stream.BigEndian () ? byteOrderMM : byteOrderII);
	
	stream.Put_uint16 (42);
	
	stream.Put_uint32 (8);
	
	// Write the IFDs.
	
	mainIFD.Put (stream);
	
	if (thumbnail)
		{
	
		rawIFD.Put (stream);
		
		}
	
	if (hasTransparencyMask)
		{
		
		maskIFD->Put (stream);
		
		}
		
	for (j = 0; j < previewCount; j++)
		{
		
		if (thumbnail != &previewList->Preview (j))
			{
	
			previewIFD [j]->Put (stream);
			
			}
		
		}
		
	exifSet.Put (stream);
	
	stream.Flush ();
	
	}
							   
/*****************************************************************************/
