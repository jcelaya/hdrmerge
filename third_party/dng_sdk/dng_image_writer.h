/*****************************************************************************/
// Copyright 2006-2012 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in
// accordance with the terms of the Adobe license agreement accompanying it.
/*****************************************************************************/

/* $Id: //mondo/dng_sdk_1_4/dng_sdk/source/dng_image_writer.h#3 $ */ 
/* $DateTime: 2012/07/31 22:04:34 $ */
/* $Change: 840853 $ */
/* $Author: tknoll $ */

/** \file
 * Support for writing DNG images to files.
 */

/*****************************************************************************/

#ifndef __dng_image_writer__
#define __dng_image_writer__

/*****************************************************************************/

#include "dng_auto_ptr.h"
#include "dng_classes.h"
#include "dng_fingerprint.h"
#include "dng_memory.h"
#include "dng_point.h"
#include "dng_rational.h"
#include "dng_sdk_limits.h"
#include "dng_string.h"
#include "dng_tag_types.h"
#include "dng_tag_values.h"
#include "dng_types.h"

/*****************************************************************************/

/// \brief Image resolution.

class dng_resolution
	{
	
	public:
	
		dng_urational fXResolution;
		dng_urational fYResolution;
		
		uint16 fResolutionUnit;
		
	public:
	
		dng_resolution ();
		
	};

/*****************************************************************************/

class tiff_tag
	{
	
	protected:
	
		uint16 fCode;
	
		uint16 fType;
		
		uint32 fCount;
		
	protected:
	
		tiff_tag (uint16 code,
				  uint16 type,
				  uint32 count)
				
			:	fCode  (code)
			,	fType  (type)
			,	fCount (count)
			
			{
			}
			
	public:
	
		virtual ~tiff_tag ()
			{
			}
			
		uint16 Code () const
			{
			return fCode;
			}
			
		uint16 Type () const
			{
			return fType;
			}
			
		uint32 Count () const
			{
			return fCount;
			}
			
		void SetCount (uint32 count)
			{
			fCount = count;
			}
		
		uint32 Size () const
			{
			return TagTypeSize (Type ()) * Count ();
			}
	
		virtual void Put (dng_stream &stream) const = 0;
	
	private:
	
		// Hidden copy constructor and assignment operator.
		
		tiff_tag (const tiff_tag &tag);
		
		tiff_tag & operator= (const tiff_tag &tag);
					 
	};

/******************************************************************************/

class tag_data_ptr: public tiff_tag
	{
	
	protected:
	
		const void *fData;
		
	public:
	
		tag_data_ptr (uint16 code,
				      uint16 type,
				      uint32 count,
				      const void *data)
				
			:	tiff_tag (code, type, count)
			
			,	fData (data)
			
			{
			}
			
		void SetData (const void *data)
			{
			fData = data;
			}
			
		virtual void Put (dng_stream &stream) const;
		
	private:
	
		// Hidden copy constructor and assignment operator.
		
		tag_data_ptr (const tag_data_ptr &tag);
		
		tag_data_ptr & operator= (const tag_data_ptr &tag);
					 
	};
		
/******************************************************************************/

class tag_string: public tiff_tag
	{
	
	protected:
	
		dng_string fString;
		
	public:
	
		tag_string (uint16 code,
				    const dng_string &s,
				    bool forceASCII = true);

		virtual void Put (dng_stream &stream) const;
	
	};
		
/******************************************************************************/

class tag_encoded_text: public tiff_tag
	{
	
	private:
	
		dng_string fText;
		
		dng_memory_data fUTF16;
		
	public:
	
		tag_encoded_text (uint16 code,
						  const dng_string &text);

		virtual void Put (dng_stream &stream) const;

	};

/******************************************************************************/

class tag_uint8: public tag_data_ptr
	{
	
	private:
	
		uint8 fValue;
		
	public:
	
		tag_uint8 (uint16 code,
				   uint8 value = 0)
			
			:	tag_data_ptr (code, ttByte, 1, &fValue)
			
			,	fValue (value)
			
			{
			}
			
		void Set (uint8 value)
			{
			fValue = value;
			}
		
	};

/******************************************************************************/

class tag_uint8_ptr: public tag_data_ptr
	{
	
	public:
	
		tag_uint8_ptr (uint16 code,
			    	   const uint8 *data,
			    	   uint32 count = 1)
			
			:	tag_data_ptr (code, ttByte, count, data)
			
			{
			}
			
	};

/******************************************************************************/

class tag_uint16: public tag_data_ptr
	{
	
	private:
	
		uint16 fValue;
		
	public:
	
		tag_uint16 (uint16 code,
					uint16 value = 0)
			
			:	tag_data_ptr (code, ttShort, 1, &fValue)
			
			,	fValue (value)
			
			{
			}
			
		void Set (uint16 value)
			{
			fValue = value;
			}
		
	};

/******************************************************************************/

class tag_int16_ptr: public tag_data_ptr
	{
	
	public:
	
		tag_int16_ptr (uint16 code,
				       const int16 *data,
				       uint32 count = 1)
			
			:	tag_data_ptr (code, ttSShort, count, data)
			
			{
			}
			
	};

/******************************************************************************/

class tag_uint16_ptr: public tag_data_ptr
	{
	
	public:
	
		tag_uint16_ptr (uint16 code,
				        const uint16 *data,
				        uint32 count = 1)
			
			:	tag_data_ptr (code, ttShort, count, data)
			
			{
			}
			
	};

/******************************************************************************/

class tag_uint32: public tag_data_ptr
	{
	
	private:
	
		uint32 fValue;
		
	public:
	
		tag_uint32 (uint16 code,
				    uint32 value = 0)
			
			:	tag_data_ptr (code, ttLong, 1, &fValue)
			
			,	fValue (value)
			
			{
			}
			
		void Set (uint32 value)
			{
			fValue = value;
			}
		
	};

/******************************************************************************/

class tag_uint32_ptr: public tag_data_ptr
	{
	
	public:
	
		tag_uint32_ptr (uint16 code,
				 		const uint32 *data,
				 		uint32 count = 1)
			
			:	tag_data_ptr (code, ttLong, count, data)
			
			{
			}
			
	};

/******************************************************************************/

class tag_urational: public tag_data_ptr
	{
	
	private:
	
		const dng_urational fValue;
		
	public:
	
		tag_urational (uint16 code,
				       const dng_urational &value)
			
			:	tag_data_ptr (code, ttRational, 1, &fValue)
			
			,	fValue (value)
			
			{
			}
			
	};

/******************************************************************************/

class tag_urational_ptr: public tag_data_ptr
	{
	
	public:
	
		tag_urational_ptr (uint16 code,
				           const dng_urational *data = NULL,
				           uint32 count = 1)
			
			:	tag_data_ptr (code, ttRational, count, data)
			
			{
			}
			
	};

/******************************************************************************/

class tag_srational: public tag_data_ptr
	{
	
	private:
	
		const dng_srational fValue;
		
	public:
	
		tag_srational (uint16 code,
				       const dng_srational &value)
			
			:	tag_data_ptr (code, ttSRational, 1, &fValue)
			
			,	fValue (value)
			
			{
			}
			
	};

/******************************************************************************/

class tag_srational_ptr: public tag_data_ptr
	{
	
	public:
	
		tag_srational_ptr (uint16 code,
				           const dng_srational *data = NULL,
				           uint32 count = 1)
			
			:	tag_data_ptr (code, ttSRational, count, data)
			
			{
			}
			
	};

/******************************************************************************/

class tag_real64: public tag_data_ptr
	{
	
	private:
	
		real64 fValue;
		
	public:
	
		tag_real64 (uint16 code,
				    real64 value = 0.0)
			
			:	tag_data_ptr (code, ttDouble, 1, &fValue)
			
			,	fValue (value)
			
			{
			}
			
		void Set (real64 value)
			{
			fValue = value;
			}
		
	};

/******************************************************************************/

class tag_matrix: public tag_srational_ptr
	{
	
	private:
	
		dng_srational fEntry [kMaxColorPlanes *
							  kMaxColorPlanes];
		
	public:
	
		tag_matrix (uint16 code,
				    const dng_matrix &m);
				    	   
	};

/******************************************************************************/

class tag_icc_profile: public tag_data_ptr
	{
	
	public:
	
		tag_icc_profile (const void *profileData, uint32 profileSize);
			
	};

/******************************************************************************/

class tag_cfa_pattern: public tiff_tag
	{
	
	private:
	
		uint32 fRows;
		uint32 fCols;
		
		const uint8 *fPattern;
	
	public:
	
		tag_cfa_pattern (uint16 code,
					   	 uint32 rows,
					     uint32 cols,
					   	 const uint8 *pattern)
					   
			:	tiff_tag (code, ttUndefined, 4 + rows * cols)
			
			,	fRows    (rows   )
			,	fCols    (cols   )
			,	fPattern (pattern)
			
			{
			}
			
		virtual void Put (dng_stream &stream) const;
			
	private:
	
		// Hidden copy constructor and assignment operator.
		
		tag_cfa_pattern (const tag_cfa_pattern &tag);
		
		tag_cfa_pattern & operator= (const tag_cfa_pattern &tag);
					 
	};
	
/******************************************************************************/

class tag_exif_date_time: public tag_data_ptr
	{
	
	private:
	
		char fData [20];
		
	public:
	
		tag_exif_date_time (uint16 code,
				            const dng_date_time &dt);
			
	};

/******************************************************************************/

class tag_iptc: public tiff_tag
	{
	
	private:
	
		const void *fData;
		
		uint32 fLength;
		
	public:
	
		tag_iptc (const void *data,
				  uint32 length);
			
		virtual void Put (dng_stream &stream) const;

	private:
	
		// Hidden copy constructor and assignment operator.
		
		tag_iptc (const tag_iptc &tag);
		
		tag_iptc & operator= (const tag_iptc &tag);
					 
	};

/******************************************************************************/

class tag_xmp: public tag_uint8_ptr
	{
	
	private:
	
		AutoPtr<dng_memory_block> fBuffer;
		
	public:
		
		tag_xmp (const dng_xmp *xmp);
				 
	private:
	
		// Hidden copy constructor and assignment operator.
		
		tag_xmp (const tag_xmp &tag);
		
		tag_xmp & operator= (const tag_xmp &tag);
					 
	};

/******************************************************************************/

class dng_tiff_directory
	{
	
	private:
	
		enum
			{
			kMaxEntries = 100
			};
			
		uint32 fEntries;
		
		const tiff_tag *fTag [kMaxEntries];
		
		uint32 fChained;
		
	public:
	
		dng_tiff_directory ()
		
			:	fEntries (0)
			,	fChained (0)
			
			{
			}
			
		virtual ~dng_tiff_directory ()
			{
			}
			
		void Add (const tiff_tag *tag);
		
		void SetChained (uint32 offset)
			{
			fChained = offset;
			}
		
		uint32 Size () const;
		
		enum OffsetsBase
			{
			offsetsRelativeToStream			= 0,
			offsetsRelativeToExplicitBase	= 1,
			offsetsRelativeToIFD			= 2
			};

		void Put (dng_stream &stream,
				  OffsetsBase offsetsBase = offsetsRelativeToStream,
				  uint32 explicitBase = 0) const;
	
	private:
	
		// Hidden copy constructor and assignment operator.
		
		dng_tiff_directory (const dng_tiff_directory &dir);
		
		dng_tiff_directory & operator= (const dng_tiff_directory &dir);
					 
	};

/******************************************************************************/

class dng_basic_tag_set
	{
	
	private:
	
		tag_uint32 fNewSubFileType;
		
		tag_uint32 fImageWidth;
		tag_uint32 fImageLength;
	
		tag_uint16 fPhotoInterpretation;
	
		tag_uint16 fFillOrder;
	
		tag_uint16 fSamplesPerPixel;
	
		uint16 fBitsPerSampleData [kMaxSamplesPerPixel];
		
		tag_uint16_ptr fBitsPerSample;
		
		bool fStrips;
								 
		tag_uint32 fTileWidth;
		tag_uint32 fTileLength;

		dng_memory_data fTileInfoBuffer;
		
		uint32 *fTileOffsetData;
		
		tag_uint32_ptr fTileOffsets;
		
		uint32 *fTileByteCountData;
		
		tag_uint32_ptr fTileByteCounts;
								
		tag_uint16 fPlanarConfiguration;
	
		tag_uint16 fCompression;
		
		tag_uint16 fPredictor;
		
		uint16 fExtraSamplesData [kMaxSamplesPerPixel];
		
		tag_uint16_ptr fExtraSamples;
		
		uint16 fSampleFormatData [kMaxSamplesPerPixel];
		
		tag_uint16_ptr fSampleFormat;
		
		tag_uint16 fRowInterleaveFactor;
		
		uint16 fSubTileBlockSizeData [2];
		
		tag_uint16_ptr fSubTileBlockSize;

	public:
	
		dng_basic_tag_set (dng_tiff_directory &directory,
					       const dng_ifd &info);
					   
		virtual ~dng_basic_tag_set ()
			{
			}
					 
		void SetTileOffset (uint32 index,
							uint32 offset)
			{
			fTileOffsetData [index] = offset;
			}
			
		void SetTileByteCount (uint32 index,
							   uint32 count)
			{
			fTileByteCountData [index] = count;
			}
			
		bool WritingStrips () const
			{
			return fStrips;
			}
			
	private:
	
		// Hidden copy constructor and assignment operator.
		
		dng_basic_tag_set (const dng_basic_tag_set &set);
		
		dng_basic_tag_set & operator= (const dng_basic_tag_set &set);
					 
	};
	
/******************************************************************************/

class exif_tag_set
	{
	
	protected:
	
		dng_tiff_directory fExifIFD;
		dng_tiff_directory fGPSIFD;
		
	private:
		
		tag_uint32 fExifLink;
		tag_uint32 fGPSLink;
		
		bool fAddedExifLink;
		bool fAddedGPSLink;
		
		uint8 fExifVersionData [4];
		
		tag_data_ptr fExifVersion;
		
		tag_urational fExposureTime;
		tag_srational fShutterSpeedValue;
		
		tag_urational fFNumber;
		tag_urational fApertureValue;
		
		tag_srational fBrightnessValue;
		
		tag_srational fExposureBiasValue;
		
		tag_urational fMaxApertureValue;
		
		tag_urational fSubjectDistance;
		
		tag_urational fFocalLength;
		
		tag_uint16 fISOSpeedRatings;

		tag_uint16 fSensitivityType;
		tag_uint32 fStandardOutputSensitivity;
		tag_uint32 fRecommendedExposureIndex;
		tag_uint32 fISOSpeed;
		tag_uint32 fISOSpeedLatitudeyyy;
		tag_uint32 fISOSpeedLatitudezzz;
		
		tag_uint16 fFlash;
		
		tag_uint16 fExposureProgram;
		
		tag_uint16 fMeteringMode;
		
		tag_uint16 fLightSource;
		
		tag_uint16 fSensingMethod;
		
		tag_uint16 fFocalLength35mm;
		
		uint8 fFileSourceData;
		tag_data_ptr fFileSource;

		uint8 fSceneTypeData;
		tag_data_ptr fSceneType;
		
		tag_cfa_pattern fCFAPattern;
		
		tag_uint16 fCustomRendered;
		tag_uint16 fExposureMode;
		tag_uint16 fWhiteBalance;
		tag_uint16 fSceneCaptureType;
		tag_uint16 fGainControl;
		tag_uint16 fContrast;
		tag_uint16 fSaturation;
		tag_uint16 fSharpness;
		tag_uint16 fSubjectDistanceRange;
		
		tag_urational fDigitalZoomRatio;
		
		tag_urational fExposureIndex;
		
		tag_uint32 fImageNumber;
		
		tag_uint16 fSelfTimerMode;
		
		tag_string    fBatteryLevelA;
		tag_urational fBatteryLevelR;
		
		tag_urational fFocalPlaneXResolution;
		tag_urational fFocalPlaneYResolution;
		
		tag_uint16 fFocalPlaneResolutionUnit;
		
		uint16 fSubjectAreaData [4];
		
		tag_uint16_ptr fSubjectArea;
		
		dng_urational fLensInfoData [4];
		
		tag_urational_ptr fLensInfo;
		
		tag_exif_date_time fDateTime;
		tag_exif_date_time fDateTimeOriginal;
		tag_exif_date_time fDateTimeDigitized;
		
		tag_string fSubsecTime;
		tag_string fSubsecTimeOriginal;
		tag_string fSubsecTimeDigitized;
		
		tag_string fMake;
		tag_string fModel;
		tag_string fArtist;
		tag_string fSoftware;
		tag_string fCopyright;
		tag_string fImageDescription;
		
		tag_string fSerialNumber;
		
		tag_uint16 fMakerNoteSafety;

		tag_data_ptr fMakerNote;
		
		tag_encoded_text fUserComment;
		
		char fImageUniqueIDData [33];
		
		tag_data_ptr fImageUniqueID;

		// EXIF 2.3 tags.

		tag_string fCameraOwnerName;
		tag_string fBodySerialNumber;
		tag_urational_ptr fLensSpecification;
		tag_string fLensMake;
		tag_string fLensModel;
		tag_string fLensSerialNumber;
		
		uint8 fGPSVersionData [4];
		
		tag_uint8_ptr fGPSVersionID;
		
		tag_string        fGPSLatitudeRef;
		tag_urational_ptr fGPSLatitude;
		
		tag_string        fGPSLongitudeRef;
		tag_urational_ptr fGPSLongitude;
		
		tag_uint8     fGPSAltitudeRef;
		tag_urational fGPSAltitude;
		
		tag_urational_ptr fGPSTimeStamp;
		
		tag_string fGPSSatellites;
		tag_string fGPSStatus;
		tag_string fGPSMeasureMode;
		
		tag_urational fGPSDOP;
		
		tag_string    fGPSSpeedRef;
		tag_urational fGPSSpeed;
		
		tag_string    fGPSTrackRef;
		tag_urational fGPSTrack;
		
		tag_string    fGPSImgDirectionRef;
		tag_urational fGPSImgDirection;
		
		tag_string fGPSMapDatum;
		
		tag_string        fGPSDestLatitudeRef;
		tag_urational_ptr fGPSDestLatitude;
		
		tag_string         fGPSDestLongitudeRef;
		tag_urational_ptr fGPSDestLongitude;
		
		tag_string    fGPSDestBearingRef;
		tag_urational fGPSDestBearing;
		
		tag_string    fGPSDestDistanceRef;
		tag_urational fGPSDestDistance;
		
		tag_encoded_text fGPSProcessingMethod;
		tag_encoded_text fGPSAreaInformation;
		
		tag_string fGPSDateStamp;
		
		tag_uint16 fGPSDifferential;
		
		tag_urational fGPSHPositioningError;
		
	public:
	
		exif_tag_set (dng_tiff_directory &directory,
					  const dng_exif &exif,
					  bool makerNoteSafe = false,
					  const void *makerNoteData = NULL,
					  uint32 makerNoteLength = 0,
					  bool insideDNG = false);
					
		void Locate (uint32 offset)
			{
			fExifLink.Set (offset);
			fGPSLink .Set (offset + fExifIFD.Size ());
			}
					
		uint32 Size () const
			{
			return fExifIFD.Size () +
				   fGPSIFD .Size ();
			}
			
		void Put (dng_stream &stream) const
			{
			fExifIFD.Put (stream);
			fGPSIFD .Put (stream);
			}
			
	protected:
	
		void AddLinks (dng_tiff_directory &directory);
	
	private:
	
		// Hidden copy constructor and assignment operator.
		
		exif_tag_set (const exif_tag_set &set);
		
		exif_tag_set & operator= (const exif_tag_set &set);
					 
	};

/******************************************************************************/

class tiff_dng_extended_color_profile: private dng_tiff_directory
	{

	protected:

		const dng_camera_profile &fProfile;

	public:

		tiff_dng_extended_color_profile (const dng_camera_profile &profile);

		void Put (dng_stream &stream,
				  bool includeModelRestriction = true);

	};

/*****************************************************************************/

class tag_dng_noise_profile: public tag_data_ptr
	{
		
	protected:

		real64 fValues [2 * kMaxColorPlanes];

	public:

		explicit tag_dng_noise_profile (const dng_noise_profile &profile);
		
	};

/*****************************************************************************/

// Enum to control the subset of metadata to save to a file.

enum dng_metadata_subset
	{
	
	kMetadataSubset_CopyrightOnly = 0,
	kMetadataSubset_CopyrightAndContact,
	kMetadataSubset_AllExceptCameraInfo,
	kMetadataSubset_All,
	kMetadataSubset_AllExceptLocationInfo,
	kMetadataSubset_AllExceptCameraAndLocation,
	
	kMetadataSubset_Last = kMetadataSubset_AllExceptCameraAndLocation
	
	};

/*****************************************************************************/

/// \brief Support for writing dng_image or dng_negative instances to a
/// dng_stream in TIFF or DNG format.

class dng_image_writer
	{
	
	friend class dng_jpeg_image;
	friend class dng_jpeg_image_encode_task;
	friend class dng_write_tiles_task;
	
	protected:
	
		enum
			{
			
			// Target size for buffer used to copy data to the image.
			
			kImageBufferSize = 128 * 1024
			
			};
	
	public:
	
		dng_image_writer ();
		
		virtual ~dng_image_writer ();
		
		virtual void EncodeJPEGPreview (dng_host &host,
							            const dng_image &image,
							            dng_jpeg_preview &preview,
							            int32 quality = -1);

		virtual void WriteImage (dng_host &host,
						         const dng_ifd &ifd,
						         dng_basic_tag_set &basic,
						         dng_stream &stream,
						         const dng_image &image,
						         uint32 fakeChannels = 1);
						    
		/// Write a dng_image to a dng_stream in TIFF format.
		/// \param host Host interface used for progress updates, abort testing, buffer allocation, etc.
		/// \param stream The dng_stream on which to write the TIFF.
		/// \param image The actual image data to be written.
		/// \param photometricInterpretation Either piBlackIsZero for monochrome or piRGB for RGB images.
		/// \param compression Must be ccUncompressed.
		/// \param negative or metadata If non-NULL, EXIF, IPTC, and XMP metadata from this negative is written to TIFF. 
		/// \param space If non-null and color space has an ICC profile, TIFF will be tagged with this
		/// profile. No color space conversion of image data occurs.
		/// \param resolution If non-NULL, TIFF will be tagged with this resolution.
		/// \param thumbnail If non-NULL, will be stored in TIFF as preview image.
		/// \param imageResources If non-NULL, will image resources be stored in TIFF as well.
		/// \param metadataSubset The subset of metadata (e.g., copyright only) to include in the TIFF.

		void WriteTIFF (dng_host &host,
						dng_stream &stream,
						const dng_image &image,
						uint32 photometricInterpretation,
						uint32 compression,
						dng_negative *negative,
						const dng_color_space *space = NULL,
						const dng_resolution *resolution = NULL,
						const dng_jpeg_preview *thumbnail = NULL,
						const dng_memory_block *imageResources = NULL,
						dng_metadata_subset metadataSubset = kMetadataSubset_All);
								
		void WriteTIFF (dng_host &host,
						dng_stream &stream,
						const dng_image &image,
						uint32 photometricInterpretation = piBlackIsZero,
						uint32 compression = ccUncompressed,
						const dng_metadata *metadata = NULL,
						const dng_color_space *space = NULL,
						const dng_resolution *resolution = NULL,
						const dng_jpeg_preview *thumbnail = NULL,
						const dng_memory_block *imageResources = NULL,
						dng_metadata_subset metadataSubset = kMetadataSubset_All);
								
		/// Write a dng_image to a dng_stream in TIFF format.
		/// \param host Host interface used for progress updates, abort testing, buffer allocation, etc.
		/// \param stream The dng_stream on which to write the TIFF.
		/// \param image The actual image data to be written.
		/// \param photometricInterpretation Either piBlackIsZero for monochrome or piRGB for RGB images.
		/// \param compression Must be ccUncompressed.
		/// \param negative or metadata If non-NULL, EXIF, IPTC, and XMP metadata from this negative is written to TIFF. 
		/// \param profileData If non-null, TIFF will be tagged with this profile. No color space conversion
		/// of image data occurs.
		/// \param profileSize The size for the profile data.
		/// \param resolution If non-NULL, TIFF will be tagged with this resolution.
		/// \param thumbnail If non-NULL, will be stored in TIFF as preview image.
		/// \param imageResources If non-NULL, will image resources be stored in TIFF as well.
		/// \param metadataSubset The subset of metadata (e.g., copyright only) to include in the TIFF.

		void WriteTIFFWithProfile (dng_host &host,
								   dng_stream &stream,
								   const dng_image &image,
								   uint32 photometricInterpretation,
								   uint32 compression,
								   dng_negative *negative,
								   const void *profileData = NULL,
								   uint32 profileSize = 0,
								   const dng_resolution *resolution = NULL,
								   const dng_jpeg_preview *thumbnail = NULL,
								   const dng_memory_block *imageResources = NULL,
								   dng_metadata_subset metadataSubset = kMetadataSubset_All);
								
		virtual void WriteTIFFWithProfile (dng_host &host,
										   dng_stream &stream,
										   const dng_image &image,
										   uint32 photometricInterpretation = piBlackIsZero,
										   uint32 compression = ccUncompressed,
										   const dng_metadata *metadata = NULL,
										   const void *profileData = NULL,
										   uint32 profileSize = 0,
										   const dng_resolution *resolution = NULL,
										   const dng_jpeg_preview *thumbnail = NULL,
										   const dng_memory_block *imageResources = NULL,
										   dng_metadata_subset metadataSubset = kMetadataSubset_All);
								
		/// Write a dng_image to a dng_stream in DNG format.
		/// \param host Host interface used for progress updates, abort testing, buffer allocation, etc.
		/// \param stream The dng_stream on which to write the TIFF.
		/// \param negative The image data and metadata (EXIF, IPTC, XMP) to be written.
		/// \param previewList List of previews (not counting thumbnail) to write to the file. Defaults to empty.
		/// \param maxBackwardVersion The DNG file should be readable by readers at least back to this version.
		/// \param uncompressed True to force uncompressed images. Otherwise use normal compression.

		void WriteDNG (dng_host &host,
					   dng_stream &stream,
					   dng_negative &negative,
					   const dng_preview_list *previewList = NULL,
					   uint32 maxBackwardVersion = dngVersion_SaveDefault,
					   bool uncompressed = false);
							   
		/// Write a dng_image to a dng_stream in DNG format.
		/// \param host Host interface used for progress updates, abort testing, buffer allocation, etc.
		/// \param stream The dng_stream on which to write the TIFF.
		/// \param negative The image data to be written.
		/// \param metadata The metadata (EXIF, IPTC, XMP) to be written.
		/// \param previewList List of previews (not counting thumbnail) to write to the file. Defaults to empty.
		/// \param maxBackwardVersion The DNG file should be readable by readers at least back to this version.
		/// \param uncompressed True to force uncompressed images. Otherwise use normal compression.

		virtual void WriteDNG (dng_host &host,
							   dng_stream &stream,
							   const dng_negative &negative,
							   const dng_metadata &metadata,
							   const dng_preview_list *previewList = NULL,
							   uint32 maxBackwardVersion = dngVersion_SaveDefault,
							   bool uncompressed = false);

		/// Resolve metadata conflicts and apply metadata policies in keeping
		/// with Metadata Working Group (MWG) guidelines.
							   
		virtual void CleanUpMetadata (dng_host &host,
									  dng_metadata &metadata,
									  dng_metadata_subset metadataSubset,
									  const char *dstMIMI,
									  const char *software = NULL);
		
	protected:
	
		virtual uint32 CompressedBufferSize (const dng_ifd &ifd,
											 uint32 uncompressedSize);
											 
		virtual void EncodePredictor (dng_host &host,
									  const dng_ifd &ifd,
						        	  dng_pixel_buffer &buffer,
									  AutoPtr<dng_memory_block> &tempBuffer);
						        	  
		virtual void ByteSwapBuffer (dng_host &host,
									 dng_pixel_buffer &buffer);
									 
		void ReorderSubTileBlocks (const dng_ifd &ifd,
								   dng_pixel_buffer &buffer,
								   AutoPtr<dng_memory_block> &uncompressedBuffer,
								   AutoPtr<dng_memory_block> &subTileBlockBuffer);
						    
		virtual void WriteData (dng_host &host,
								const dng_ifd &ifd,
						        dng_stream &stream,
						        dng_pixel_buffer &buffer,
								AutoPtr<dng_memory_block> &compressedBuffer);
						        
		virtual void WriteTile (dng_host &host,
						        const dng_ifd &ifd,
						        dng_stream &stream,
						        const dng_image &image,
						        const dng_rect &tileArea,
						        uint32 fakeChannels,
								AutoPtr<dng_memory_block> &compressedBuffer,
								AutoPtr<dng_memory_block> &uncompressedBuffer,
								AutoPtr<dng_memory_block> &subTileBlockBuffer,
								AutoPtr<dng_memory_block> &tempBuffer);
								
	};
	
/*****************************************************************************/

#endif
	
/*****************************************************************************/
