/*****************************************************************************/
// Copyright 2006-2012 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in
// accordance with the terms of the Adobe license agreement accompanying it.
/*****************************************************************************/

/* $Id: //mondo/dng_sdk_1_4/dng_sdk/source/dng_negative.cpp#3 $ */ 
/* $DateTime: 2012/06/14 20:24:41 $ */
/* $Change: 835078 $ */
/* $Author: tknoll $ */

/*****************************************************************************/

#include "dng_negative.h"

#include "dng_1d_table.h"
#include "dng_abort_sniffer.h"
#include "dng_area_task.h"
#include "dng_assertions.h"
#include "dng_bottlenecks.h"
#include "dng_camera_profile.h"
#include "dng_color_space.h"
#include "dng_color_spec.h"
#include "dng_exceptions.h"
#include "dng_globals.h"
#include "dng_host.h"
#include "dng_image.h"
#include "dng_image_writer.h"
#include "dng_info.h"
#include "dng_jpeg_image.h"
#include "dng_linearization_info.h"
#include "dng_memory_stream.h"
#include "dng_misc_opcodes.h"
#include "dng_mosaic_info.h"
#include "dng_preview.h"
#include "dng_resample.h"
#include "dng_simple_image.h"
#include "dng_tag_codes.h"
#include "dng_tag_values.h"
#include "dng_tile_iterator.h"
#include "dng_utils.h"
#include "dng_xmp.h"

/*****************************************************************************/

dng_noise_profile::dng_noise_profile ()

	:	fNoiseFunctions ()

	{
	
	}

/*****************************************************************************/

dng_noise_profile::dng_noise_profile (const std::vector<dng_noise_function> &functions)

	:	fNoiseFunctions (functions)

	{

	}

/*****************************************************************************/

bool dng_noise_profile::IsValid () const
	{

	if (NumFunctions () == 0 || NumFunctions () > kMaxColorPlanes)
		{
		return false;
		}
	
	for (uint32 plane = 0; plane < NumFunctions (); plane++)
		{
		
		if (!NoiseFunction (plane).IsValid ())
			{
			return false;
			}
		
		}

	return true;
	
	}

/*****************************************************************************/

bool dng_noise_profile::IsValidForNegative (const dng_negative &negative) const
	{
	
	if (!(NumFunctions () == 1 || NumFunctions () == negative.ColorChannels ()))
		{
		return false;
		}

	return IsValid ();

	}

/*****************************************************************************/

const dng_noise_function & dng_noise_profile::NoiseFunction (uint32 plane) const
	{
	
	if (NumFunctions () == 1)
		{
		return fNoiseFunctions.front ();
		}

	DNG_REQUIRE (plane < NumFunctions (), 
				 "Bad plane index argument for NoiseFunction ().");

	return fNoiseFunctions [plane];
	
	}

/*****************************************************************************/

uint32 dng_noise_profile::NumFunctions () const
	{
	return (uint32) fNoiseFunctions.size ();
	}

/*****************************************************************************/

dng_metadata::dng_metadata (dng_host &host)

	:	fHasBaseOrientation 		(false)
	,	fBaseOrientation    		()
	,	fIsMakerNoteSafe			(false)
	,	fMakerNote					()
	,	fExif			    		(host.Make_dng_exif ())
	,	fOriginalExif				()
	,	fIPTCBlock          		()
	,	fIPTCOffset					(kDNGStreamInvalidOffset)
	,	fXMP			    		(host.Make_dng_xmp ())
	,	fEmbeddedXMPDigest       	()
	,	fXMPinSidecar	    		(false)
	,	fXMPisNewer		    		(false)
	,	fSourceMIMI					()

	{
	}

/*****************************************************************************/

dng_metadata::~dng_metadata ()
	{
	}
	
/******************************************************************************/

template< class T >
T * CloneAutoPtr (const AutoPtr< T > &ptr)
	{
	
	return ptr.Get () ? ptr->Clone () : NULL;
	
	}

/******************************************************************************/

template< class T, typename U >
T * CloneAutoPtr (const AutoPtr< T > &ptr, U &u)
	{
	
	return ptr.Get () ? ptr->Clone (u) : NULL;
	
	}

/******************************************************************************/

dng_metadata::dng_metadata (const dng_metadata &rhs,
							dng_memory_allocator &allocator)

	:	fHasBaseOrientation 		(rhs.fHasBaseOrientation)
	,	fBaseOrientation    		(rhs.fBaseOrientation)
	,	fIsMakerNoteSafe			(rhs.fIsMakerNoteSafe)
	,	fMakerNote					(CloneAutoPtr (rhs.fMakerNote, allocator))
	,	fExif			    		(CloneAutoPtr (rhs.fExif))
	,	fOriginalExif				(CloneAutoPtr (rhs.fOriginalExif))
	,	fIPTCBlock          		(CloneAutoPtr (rhs.fIPTCBlock, allocator))
	,	fIPTCOffset					(rhs.fIPTCOffset)
	,	fXMP			    		(CloneAutoPtr (rhs.fXMP))
	,	fEmbeddedXMPDigest       	(rhs.fEmbeddedXMPDigest)
	,	fXMPinSidecar	    		(rhs.fXMPinSidecar)
	,	fXMPisNewer		    		(rhs.fXMPisNewer)
	,	fSourceMIMI					(rhs.fSourceMIMI)

	{

	}

/******************************************************************************/

dng_metadata * dng_metadata::Clone (dng_memory_allocator &allocator) const
	{
	
	return new dng_metadata (*this, allocator);
	
	}

/******************************************************************************/

void dng_metadata::SetBaseOrientation (const dng_orientation &orientation)
	{
	
	fHasBaseOrientation = true;
	
	fBaseOrientation = orientation;
	
	}
		
/******************************************************************************/

void dng_metadata::ApplyOrientation (const dng_orientation &orientation)
	{
	
	fBaseOrientation += orientation;
	
	fXMP->SetOrientation (fBaseOrientation);

	}
				  
/*****************************************************************************/

void dng_metadata::ResetExif (dng_exif * newExif)
	{

	fExif.Reset (newExif);

	}

/******************************************************************************/

dng_memory_block * dng_metadata::BuildExifBlock (dng_memory_allocator &allocator,
												 const dng_resolution *resolution,
												 bool includeIPTC,
												 const dng_jpeg_preview *thumbnail) const
	{
	
	dng_memory_stream stream (allocator);
	
		{
	
		// Create the main IFD
											 
		dng_tiff_directory mainIFD;
		
		// Optionally include the resolution tags.
		
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

		// Optionally include IPTC block.
		
		tag_iptc tagIPTC (IPTCData   (),
						  IPTCLength ());
			
		if (includeIPTC && tagIPTC.Count ())
			{
			mainIFD.Add (&tagIPTC);
			}
							
		// Exif tags.
		
		exif_tag_set exifSet (mainIFD,
							  *GetExif (),
							  IsMakerNoteSafe (),
							  MakerNoteData   (),
							  MakerNoteLength (),
							  false);
							  
		// Figure out the Exif IFD offset.
		
		uint32 exifOffset = 8 + mainIFD.Size ();
		
		exifSet.Locate (exifOffset);
		
		// Thumbnail IFD (if any).
		
		dng_tiff_directory thumbIFD;
		
		tag_uint16 thumbCompression (tcCompression, ccOldJPEG);
		
		tag_urational thumbXResolution (tcXResolution, dng_urational (72, 1));
		tag_urational thumbYResolution (tcYResolution, dng_urational (72, 1));
		
		tag_uint16 thumbResolutionUnit (tcResolutionUnit, ruInch);
		
		tag_uint32 thumbDataOffset (tcJPEGInterchangeFormat      , 0);
		tag_uint32 thumbDataLength (tcJPEGInterchangeFormatLength, 0);
		
		if (thumbnail)
			{
			
			thumbIFD.Add (&thumbCompression);
			
			thumbIFD.Add (&thumbXResolution);
			thumbIFD.Add (&thumbYResolution);
			thumbIFD.Add (&thumbResolutionUnit);
			
			thumbIFD.Add (&thumbDataOffset);
			thumbIFD.Add (&thumbDataLength);
			
			thumbDataLength.Set (thumbnail->fCompressedData->LogicalSize ());
			
			uint32 thumbOffset = exifOffset + exifSet.Size ();
			
			mainIFD.SetChained (thumbOffset);
			
			thumbDataOffset.Set (thumbOffset + thumbIFD.Size ());
			
			}
			
		// Don't write anything unless the main IFD has some tags.
		
		if (mainIFD.Size () != 0)
			{
					
			// Write TIFF Header.
			
			stream.SetWritePosition (0);
			
			stream.Put_uint16 (stream.BigEndian () ? byteOrderMM : byteOrderII);
			
			stream.Put_uint16 (42);
			
			stream.Put_uint32 (8);
			
			// Write the IFDs.
			
			mainIFD.Put (stream);
			
			exifSet.Put (stream);
			
			if (thumbnail)
				{
				
				thumbIFD.Put (stream);
				
				stream.Put (thumbnail->fCompressedData->Buffer      (),
							thumbnail->fCompressedData->LogicalSize ());
				
				}
				
			// Trim the file to this length.
			
			stream.Flush ();
			
			stream.SetLength (stream.Position ());
			
			}
		
		}
		
	return stream.AsMemoryBlock (allocator);
		
	}
			
/******************************************************************************/

void dng_metadata::SetIPTC (AutoPtr<dng_memory_block> &block, uint64 offset)
	{
	
	fIPTCBlock.Reset (block.Release ());
	
	fIPTCOffset = offset;
	
	}
					  
/******************************************************************************/

void dng_metadata::SetIPTC (AutoPtr<dng_memory_block> &block)
	{
	
	SetIPTC (block, kDNGStreamInvalidOffset);
	
	}
					  
/******************************************************************************/

void dng_metadata::ClearIPTC ()
	{
	
	fIPTCBlock.Reset ();
	
	fIPTCOffset = kDNGStreamInvalidOffset;
	
	}
					  
/*****************************************************************************/

const void * dng_metadata::IPTCData () const
	{
	
	if (fIPTCBlock.Get ())
		{
		
		return fIPTCBlock->Buffer ();
		
		}
		
	return NULL;
	
	}

/*****************************************************************************/

uint32 dng_metadata::IPTCLength () const
	{
	
	if (fIPTCBlock.Get ())
		{
		
		return fIPTCBlock->LogicalSize ();
		
		}
		
	return 0;
	
	}
		
/*****************************************************************************/

uint64 dng_metadata::IPTCOffset () const
	{
	
	if (fIPTCBlock.Get ())
		{
		
		return fIPTCOffset;
		
		}
		
	return kDNGStreamInvalidOffset;
	
	}
		
/*****************************************************************************/

dng_fingerprint dng_metadata::IPTCDigest (bool includePadding) const
	{
	
	if (IPTCLength ())
		{
		
		dng_md5_printer printer;
		
		const uint8 *data = (const uint8 *) IPTCData ();
		
		uint32 count = IPTCLength ();
		
		// Because of some stupid ways of storing the IPTC data, the IPTC
		// data might be padded with up to three zeros.  The official Adobe
		// logic is to include these zeros in the digest.  However, older
		// versions of the Camera Raw code did not include the padding zeros
		// in the digest, so we support both methods and allow either to
		// match.
		
		if (!includePadding)
			{
		
			uint32 removed = 0;
			
			while ((removed < 3) && (count > 0) && (data [count - 1] == 0))
				{
				removed++;
				count--;
				}
				
			}
		
		printer.Process (data, count);
						 
		return printer.Result ();
			
		}
	
	return dng_fingerprint ();
	
	}
		
/******************************************************************************/

void dng_metadata::RebuildIPTC (dng_memory_allocator &allocator,
								bool padForTIFF)
	{
	
	ClearIPTC ();
	
	fXMP->RebuildIPTC (*this, allocator, padForTIFF);
	
	dng_fingerprint digest = IPTCDigest ();
	
	fXMP->SetIPTCDigest (digest);
	
	}
			  
/*****************************************************************************/

void dng_metadata::ResetXMP (dng_xmp * newXMP)
	{
	
	fXMP.Reset (newXMP);

	}

/*****************************************************************************/

void dng_metadata::ResetXMPSidecarNewer (dng_xmp * newXMP,
										 bool inSidecar,
										 bool isNewer )
	{

	fXMP.Reset (newXMP);

	fXMPinSidecar = inSidecar;

	fXMPisNewer = isNewer;

	}

/*****************************************************************************/

bool dng_metadata::SetXMP (dng_host &host,
						   const void *buffer,
					 	   uint32 count,
					 	   bool xmpInSidecar,
					 	   bool xmpIsNewer)
	{
	
	bool result = false;
	
	try
		{
		
		AutoPtr<dng_xmp> tempXMP (host.Make_dng_xmp ());
		
		tempXMP->Parse (host, buffer, count);
		
		ResetXMPSidecarNewer (tempXMP.Release (), xmpInSidecar, xmpIsNewer);
		
		result = true;
		
		}
		
	catch (dng_exception &except)
		{
		
		// Don't ignore transient errors.
		
		if (host.IsTransientError (except.ErrorCode ()))
			{
			
			throw;
			
			}
			
		// Eat other parsing errors.
		
		}
		
	catch (...)
		{
		
		// Eat unknown parsing exceptions.
		
		}
	
	return result;
	
	}

/*****************************************************************************/

void dng_metadata::SetEmbeddedXMP (dng_host &host,
								   const void *buffer,
								   uint32 count)
	{
	
	if (SetXMP (host, buffer, count))
		{
		
		dng_md5_printer printer;
		
		printer.Process (buffer, count);
		
		fEmbeddedXMPDigest = printer.Result ();
		
		// Remove any sidecar specific tags from embedded XMP.
		
		if (fXMP.Get ())
			{
		
			fXMP->Remove (XMP_NS_PHOTOSHOP, "SidecarForExtension");
			fXMP->Remove (XMP_NS_PHOTOSHOP, "EmbeddedXMPDigest");
			
			}
		
		}
		
	else
		{
		
		fEmbeddedXMPDigest.Clear ();
		
		}

	}

/*****************************************************************************/

void dng_metadata::SynchronizeMetadata ()
	{
	
	if (!fOriginalExif.Get ())
		{
		
		fOriginalExif.Reset (fExif->Clone ());
		
		}
		
	fXMP->ValidateMetadata ();
	
	fXMP->IngestIPTC (*this, fXMPisNewer);
	
	fXMP->SyncExif (*fExif.Get ());
	
	fXMP->SyncOrientation (*this, fXMPinSidecar);
	
	}
					
/*****************************************************************************/

void dng_metadata::UpdateDateTime (const dng_date_time_info &dt)
	{
	
	fExif->UpdateDateTime (dt);
	
	fXMP->UpdateDateTime (dt);
	
	}
					
/*****************************************************************************/

void dng_metadata::UpdateDateTimeToNow ()
	{
	
	dng_date_time_info dt;
	
	CurrentDateTimeAndZone (dt);
	
	UpdateDateTime (dt);
	
	fXMP->UpdateMetadataDate (dt);
	
	}
					
/*****************************************************************************/

void dng_metadata::UpdateMetadataDateTimeToNow ()
	{
	
	dng_date_time_info dt;
	
	CurrentDateTimeAndZone (dt);
	
	fXMP->UpdateMetadataDate (dt);
	
	}
					
/*****************************************************************************/

dng_negative::dng_negative (dng_host &host)

	:	fAllocator						(host.Allocator ())
	
	,	fModelName						()
	,	fLocalName						()
	,	fDefaultCropSizeH				()
	,	fDefaultCropSizeV				()
	,	fDefaultCropOriginH				(0, 1)
	,	fDefaultCropOriginV				(0, 1)
	,	fDefaultUserCropT				(0, 1)
	,	fDefaultUserCropL				(0, 1)
	,	fDefaultUserCropB				(1, 1)
	,	fDefaultUserCropR				(1, 1)
	,	fDefaultScaleH					(1, 1)
	,	fDefaultScaleV					(1, 1)
	,	fBestQualityScale				(1, 1)
	,	fOriginalDefaultFinalSize		()
	,	fOriginalBestQualityFinalSize	()
	,	fOriginalDefaultCropSizeH	    ()
	,	fOriginalDefaultCropSizeV	    ()
	,	fRawToFullScaleH				(1.0)
	,	fRawToFullScaleV				(1.0)
	,	fBaselineNoise					(100, 100)
	,	fNoiseReductionApplied			(0, 0)
	,	fNoiseProfile					()
	,	fBaselineExposure				(  0, 100)
	,	fBaselineSharpness				(100, 100)
	,	fChromaBlurRadius				()
	,	fAntiAliasStrength				(100, 100)
	,	fLinearResponseLimit			(100, 100)
	,	fShadowScale					(1, 1)
	,	fColorimetricReference			(crSceneReferred)
	,	fColorChannels					(0)
	,	fAnalogBalance					()
	,	fCameraNeutral					()
	,	fCameraWhiteXY					()
	,	fCameraCalibration1				()
	,	fCameraCalibration2				()
	,	fCameraCalibrationSignature		()
	,	fCameraProfile					()
	,	fAsShotProfileName				()
	,	fRawImageDigest					()
	,	fNewRawImageDigest				()
	,	fRawDataUniqueID				()
	,	fOriginalRawFileName			()
	,	fHasOriginalRawFileData			(false)
	,	fOriginalRawFileData			()
	,	fOriginalRawFileDigest			()
	,	fDNGPrivateData					()
	,	fMetadata						(host)
	,	fLinearizationInfo				()
	,	fMosaicInfo						()
	,	fOpcodeList1					(1)
	,	fOpcodeList2					(2)
	,	fOpcodeList3					(3)
	,	fStage1Image					()
	,	fStage2Image					()
	,	fStage3Image					()
	,	fStage3Gain						(1.0)
	,	fIsPreview						(false)
	,	fIsDamaged						(false)
	,	fRawImageStage					(rawImageStageNone)
	,	fRawImage						()
	,	fRawFloatBitDepth				(0)
	,	fRawJPEGImage					()
	,	fRawJPEGImageDigest				()
	,	fTransparencyMask				()
	,	fRawTransparencyMask			()
	,	fRawTransparencyMaskBitDepth	(0)
	,	fUnflattenedStage3Image			()

	{

	}

/*****************************************************************************/

dng_negative::~dng_negative ()
	{
	
	// Delete any camera profiles owned by this negative.
	
	ClearProfiles ();
		
	}

/******************************************************************************/

void dng_negative::Initialize ()
	{
	
	}

/******************************************************************************/

dng_negative * dng_negative::Make (dng_host &host)
	{
	
	AutoPtr<dng_negative> result (new dng_negative (host));
	
	if (!result.Get ())
		{
		ThrowMemoryFull ();
		}
	
	result->Initialize ();
	
	return result.Release ();
	
	}

/******************************************************************************/

dng_metadata * dng_negative::CloneInternalMetadata () const
	{
	
	return InternalMetadata ().Clone (Allocator ());
	
	}

/******************************************************************************/

dng_orientation dng_negative::ComputeOrientation (const dng_metadata &metadata) const
	{
	
	return metadata.BaseOrientation ();
	
	}
		
/******************************************************************************/

void dng_negative::SetAnalogBalance (const dng_vector &b)
	{
	
	real64 minEntry = b.MinEntry ();
	
	if (b.NotEmpty () && minEntry > 0.0)
		{
		
		fAnalogBalance = b;
	
		fAnalogBalance.Scale (1.0 / minEntry);
		
		fAnalogBalance.Round (1000000.0);
		
		}
		
	else
		{
		
		fAnalogBalance.Clear ();
		
		}
		
	}
					  
/*****************************************************************************/

real64 dng_negative::AnalogBalance (uint32 channel) const
	{
	
	DNG_ASSERT (channel < ColorChannels (), "Channel out of bounds");
	
	if (channel < fAnalogBalance.Count ())
		{
		
		return fAnalogBalance [channel];
		
		}
		
	return 1.0;
	
	}
		
/*****************************************************************************/

dng_urational dng_negative::AnalogBalanceR (uint32 channel) const
	{
	
	dng_urational result;
	
	result.Set_real64 (AnalogBalance (channel), 1000000);
	
	return result;
	
	}

/******************************************************************************/

void dng_negative::SetCameraNeutral (const dng_vector &n)
	{
	
	real64 maxEntry = n.MaxEntry ();
		
	if (n.NotEmpty () && maxEntry > 0.0)
		{
		
		fCameraNeutral = n;
	
		fCameraNeutral.Scale (1.0 / maxEntry);
		
		fCameraNeutral.Round (1000000.0);
		
		}
		
	else
		{
		
		fCameraNeutral.Clear ();
		
		}

	}
	  
/*****************************************************************************/

dng_urational dng_negative::CameraNeutralR (uint32 channel) const
	{
	
	dng_urational result;
	
	result.Set_real64 (CameraNeutral () [channel], 1000000);
	
	return result;
	
	}

/******************************************************************************/

void dng_negative::SetCameraWhiteXY (const dng_xy_coord &coord)
	{
	
	if (coord.IsValid ())
		{
		
		fCameraWhiteXY.x = Round_int32 (coord.x * 1000000.0) / 1000000.0;
		fCameraWhiteXY.y = Round_int32 (coord.y * 1000000.0) / 1000000.0;
		
		}
		
	else
		{
		
		fCameraWhiteXY.Clear ();
		
		}
	
	}
		
/*****************************************************************************/

const dng_xy_coord & dng_negative::CameraWhiteXY () const
	{
	
	DNG_ASSERT (HasCameraWhiteXY (), "Using undefined CameraWhiteXY");

	return fCameraWhiteXY;
	
	}
							   
/*****************************************************************************/

void dng_negative::GetCameraWhiteXY (dng_urational &x,
							   		 dng_urational &y) const
	{
	
	dng_xy_coord coord = CameraWhiteXY ();
	
	x.Set_real64 (coord.x, 1000000);
	y.Set_real64 (coord.y, 1000000);
	
	}
		
/*****************************************************************************/

void dng_negative::SetCameraCalibration1 (const dng_matrix &m)
	{
	
	fCameraCalibration1 = m;
	
	fCameraCalibration1.Round (10000);
	
	}

/******************************************************************************/

void dng_negative::SetCameraCalibration2 (const dng_matrix &m)
	{
	
	fCameraCalibration2 = m;
	
	fCameraCalibration2.Round (10000);
		
	}

/******************************************************************************/

void dng_negative::AddProfile (AutoPtr<dng_camera_profile> &profile)
	{
	
	// Make sure we have a profile to add.
	
	if (!profile.Get ())
		{
		
		return;
		
		}
	
	// We must have some profile name.  Use "embedded" if nothing else.
	
	if (profile->Name ().IsEmpty ())
		{
		
		profile->SetName (kProfileName_Embedded);
		
		}
		
	// Special case support for reading older DNG files which did not store
	// the profile name in the main IFD profile.
	
	if (fCameraProfile.size ())
		{
		
		// See the first profile has a default "embedded" name, and has
		// the same data as the profile we are adding.
		
		if (fCameraProfile [0]->NameIsEmbedded () &&
			fCameraProfile [0]->EqualData (*profile.Get ()))
			{
			
			// If the profile we are deleting was read from DNG
			// then the new profile should be marked as such also.
			
			if (fCameraProfile [0]->WasReadFromDNG ())
				{
				
				profile->SetWasReadFromDNG ();
				
				}
				
			// If the profile we are deleting wasn't read from disk then the new
			// profile should be marked as such also.
			
			if (!fCameraProfile [0]->WasReadFromDisk ())
				{
				
				profile->SetWasReadFromDisk (false);
				
				}
				
			// Delete the profile with default name.
			
			delete fCameraProfile [0];
			
			fCameraProfile [0] = NULL;
			
			fCameraProfile.erase (fCameraProfile.begin ());
			
			}
		
		}
		
	// Duplicate detection logic.  We give a preference to last added profile
	// so the profiles end up in a more consistent order no matter what profiles
	// happen to be embedded in the DNG.
	
	for (uint32 index = 0; index < (uint32) fCameraProfile.size (); index++)
		{

		// Instead of checking for matching fingerprints, we check that the two
		// profiles have the same color and have the same name. This allows two
		// profiles that are identical except for copyright string and embed policy
		// to be considered duplicates.

		const bool equalColorAndSameName = (fCameraProfile [index]->EqualData (*profile.Get ()) &&
											fCameraProfile [index]->Name () == profile->Name ());

		if (equalColorAndSameName)
			{
			
			// If the profile we are deleting was read from DNG
			// then the new profile should be marked as such also.
			
			if (fCameraProfile [index]->WasReadFromDNG ())
				{
				
				profile->SetWasReadFromDNG ();
				
				}
				
			// If the profile we are deleting wasn't read from disk then the new
			// profile should be marked as such also.
			
			if (!fCameraProfile [index]->WasReadFromDisk ())
				{
				
				profile->SetWasReadFromDisk (false);
				
				}
				
			// Delete the duplicate profile.
			
			delete fCameraProfile [index];
			
			fCameraProfile [index] = NULL;
			
			fCameraProfile.erase (fCameraProfile.begin () + index);
			
			break;
			
			}
			
		}
		
	// Now add to profile list.
	
	fCameraProfile.push_back (NULL);
	
	fCameraProfile [fCameraProfile.size () - 1] = profile.Release ();
	
	}
			
/******************************************************************************/

void dng_negative::ClearProfiles ()
	{
	
	// Delete any camera profiles owned by this negative.
	
	for (uint32 index = 0; index < (uint32) fCameraProfile.size (); index++)
		{
		
		if (fCameraProfile [index])
			{
			
			delete fCameraProfile [index];
			
			fCameraProfile [index] = NULL;
			
			}
		
		}
		
	// Now empty list.
	
	fCameraProfile.clear ();
	
	}

/*****************************************************************************/

void dng_negative::ClearProfiles (bool clearBuiltinMatrixProfiles,
								  bool clearReadFromDisk)
	{

	// If neither flag is set, then there's nothing to do.

	if (!clearBuiltinMatrixProfiles &&
		!clearReadFromDisk)
		{
		return;
		}
	
	// Delete any camera profiles in this negative that match the specified criteria.

	std::vector<dng_camera_profile *>::iterator iter = fCameraProfile.begin ();
	std::vector<dng_camera_profile *>::iterator next;
	
	for (; iter != fCameraProfile.end (); iter = next)
		{

		dng_camera_profile *profile = *iter;

		// If the profile is invalid (i.e., NULL pointer), or meets one of the
		// specified criteria, then axe it.

		if (!profile ||
			(clearBuiltinMatrixProfiles && profile->WasBuiltinMatrix ()) ||
			(clearReadFromDisk			&& profile->WasReadFromDisk	 ()))
			{
			
			delete profile;

			next = fCameraProfile.erase (iter);

			}

		// Otherwise, just advance to the next element.

		else
			{
			
			next = iter + 1;
			
			}

		}
	
	}

/******************************************************************************/

uint32 dng_negative::ProfileCount () const
	{
	
	return (uint32) fCameraProfile.size ();
	
	}
		
/******************************************************************************/

const dng_camera_profile & dng_negative::ProfileByIndex (uint32 index) const
	{
	
	DNG_ASSERT (index < ProfileCount (),
				"Invalid index for ProfileByIndex");
				
	return *fCameraProfile [index];
		
	}
		
/*****************************************************************************/

const dng_camera_profile * dng_negative::ProfileByID (const dng_camera_profile_id &id,
													  bool useDefaultIfNoMatch) const
	{
	
	uint32 index;
	
	// If this negative does not have any profiles, we are not going to
	// find a match.
	
	uint32 profileCount = ProfileCount ();
	
	if (profileCount == 0)
		{
		return NULL;
		}
		
	// If we have both a profile name and fingerprint, try matching both.
	
	if (id.Name ().NotEmpty () && id.Fingerprint ().IsValid ())
		{
		
		for (index = 0; index < profileCount; index++)
			{
			
			const dng_camera_profile &profile = ProfileByIndex (index);
			
			if (id.Name        () == profile.Name        () &&
				id.Fingerprint () == profile.Fingerprint ())
				{
				
				return &profile;
				
				}
			
			}

		}
		
	// If we have a name, try matching that.
	
	if (id.Name ().NotEmpty ())
		{
		
		for (index = 0; index < profileCount; index++)
			{
			
			const dng_camera_profile &profile = ProfileByIndex (index);
			
			if (id.Name () == profile.Name ())
				{
				
				return &profile;
				
				}
			
			}

		}
		
	// If we have a valid fingerprint, try matching that.
		
	if (id.Fingerprint ().IsValid ())
		{
		
		for (index = 0; index < profileCount; index++)
			{
			
			const dng_camera_profile &profile = ProfileByIndex (index);
			
			if (id.Fingerprint () == profile.Fingerprint ())
				{
				
				return &profile;
				
				}
			
			}

		}
		
	// Try "upgrading" profile name versions.
	
	if (id.Name ().NotEmpty ())
		{
		
		dng_string baseName;
		int32      version;
		
		SplitCameraProfileName (id.Name (),
								baseName,
								version);
		
		int32 bestIndex   = -1;
		int32 bestVersion = 0;
		
		for (index = 0; index < profileCount; index++)
			{
			
			const dng_camera_profile &profile = ProfileByIndex (index);
			
			if (profile.Name ().StartsWith (baseName.Get ()))
				{
				
				dng_string testBaseName;
				int32      testVersion;
				
				SplitCameraProfileName (profile.Name (),
										testBaseName,
										testVersion);
										
				if (bestIndex == -1 || testVersion > bestVersion)
					{
					
					bestIndex   = index;
					bestVersion = testVersion;
					
					}
					
				}
				
			}
			
		if (bestIndex != -1)
			{
			
			return &ProfileByIndex (bestIndex);
			
			}
		
		}
		
	// Did not find a match any way.  See if we should return a default value.
	
	if (useDefaultIfNoMatch)
		{
		
		return &ProfileByIndex (0);
		
		}
		
	// Found nothing.
	
	return NULL;
		
	}

/*****************************************************************************/

const dng_camera_profile * dng_negative::ComputeCameraProfileToEmbed
										(const dng_metadata & /* metadata */) const
	{
	
	uint32 index;
	
	uint32 count = ProfileCount ();
	
	if (count == 0)
		{
		
		return NULL;
		
		}
		
	// First try to look for the first profile that was already in the DNG
	// when we read it.
	
	for (index = 0; index < count; index++)
		{
		
		const dng_camera_profile &profile (ProfileByIndex (index));
		
		if (profile.WasReadFromDNG ())
			{
			
			return &profile;
			
			}
		
		}
		
	// Next we look for the first profile that is legal to embed.
	
	for (index = 0; index < count; index++)
		{
		
		const dng_camera_profile &profile (ProfileByIndex (index));
		
		if (profile.IsLegalToEmbed ())
			{
			
			return &profile;
			
			}
		
		}
		
	// Else just return the first profile.
	
	return fCameraProfile [0];
	
	}
							   
/*****************************************************************************/

dng_color_spec * dng_negative::MakeColorSpec (const dng_camera_profile_id &id) const
	{

	dng_color_spec *spec = new dng_color_spec (*this, ProfileByID (id));
											   
	if (!spec)
		{
		ThrowMemoryFull ();
		}
		
	return spec;
	
	}
							   
/*****************************************************************************/

dng_fingerprint dng_negative::FindImageDigest (dng_host &host,
											   const dng_image &image) const
	{
	
	dng_md5_printer printer;
	
	dng_pixel_buffer buffer;
	
	buffer.fPlane  = 0;
	buffer.fPlanes = image.Planes ();
	
	buffer.fRowStep   = image.Planes () * image.Width ();
	buffer.fColStep   = image.Planes ();
	buffer.fPlaneStep = 1;
	
	buffer.fPixelType = image.PixelType ();
	buffer.fPixelSize = image.PixelSize ();
	
	// Sometimes we expand 8-bit data to 16-bit data while reading or
	// writing, so always compute the digest of 8-bit data as 16-bits.
	
	if (buffer.fPixelType == ttByte)
		{
		buffer.fPixelType = ttShort;
		buffer.fPixelSize = 2;
		}
	
	const uint32 kBufferRows = 16;
	
	uint32 bufferBytes = kBufferRows * buffer.fRowStep * buffer.fPixelSize;
	
	AutoPtr<dng_memory_block> bufferData (host.Allocate (bufferBytes));
	
	buffer.fData = bufferData->Buffer ();
	
	dng_rect area;
	
	dng_tile_iterator iter (dng_point (kBufferRows,
									   image.Width ()),
							image.Bounds ());
							
	while (iter.GetOneTile (area))
		{
		
		host.SniffForAbort ();
		
		buffer.fArea = area;
		
		image.Get (buffer);
		
		uint32 count = buffer.fArea.H () *
					   buffer.fRowStep *
					   buffer.fPixelSize;
					   
		#if qDNGBigEndian
		
		// We need to use the same byte order to compute
		// the digest, no matter the native order.  Little-endian
		// is more common now, so use that.
		
		switch (buffer.fPixelSize)
			{
			
			case 1:
				break;
			
			case 2:
				{
				DoSwapBytes16 ((uint16 *) buffer.fData, count >> 1);
				break;
				}
			
			case 4:
				{
				DoSwapBytes32 ((uint32 *) buffer.fData, count >> 2);
				break;
				}
				
			default:
				{
				DNG_REPORT ("Unexpected pixel size");
				break;
				}
			
			}
		
		#endif

		printer.Process (buffer.fData,
						 count);
		
		}
			
	return printer.Result ();
	
	}
							   
/*****************************************************************************/

void dng_negative::FindRawImageDigest (dng_host &host) const
	{
	
	if (fRawImageDigest.IsNull ())
		{
		
		// Since we are adding the floating point and transparency support 
		// in DNG 1.4, and there are no legacy floating point or transparent
		// DNGs, switch to using the more MP friendly algorithm to compute
		// the digest for these images.
		
		if (RawImage ().PixelType () == ttFloat || RawTransparencyMask ())
			{
			
			FindNewRawImageDigest (host);
			
			fRawImageDigest = fNewRawImageDigest;
			
			}
			
		else
			{
			
			#if qDNGValidate
			
			dng_timer timeScope ("FindRawImageDigest time");

			#endif
		
			fRawImageDigest = FindImageDigest (host, RawImage ());
			
			}
	
		}
	
	}
							   
/*****************************************************************************/

class dng_find_new_raw_image_digest_task : public dng_area_task
	{
	
	private:
	
		enum
			{
			kTileSize = 256
			};
			
		const dng_image &fImage;
		
		uint32 fPixelType;
		uint32 fPixelSize;
		
		uint32 fTilesAcross;
		uint32 fTilesDown;
		
		uint32 fTileCount;
		
		AutoArray<dng_fingerprint> fTileHash;
		
		AutoPtr<dng_memory_block> fBufferData [kMaxMPThreads];
	
	public:
	
		dng_find_new_raw_image_digest_task (const dng_image &image,
											uint32 pixelType)
		
			:	fImage       (image)
			,	fPixelType   (pixelType)
			,	fPixelSize	 (TagTypeSize (pixelType))
			,	fTilesAcross (0)
			,	fTilesDown   (0)
			,	fTileCount   (0)
			,	fTileHash    (NULL)
			
			{
			
			fMinTaskArea = 1;
									
			fUnitCell = dng_point (Min_int32 (kTileSize, fImage.Bounds ().H ()),
								   Min_int32 (kTileSize, fImage.Bounds ().W ()));
								   
			fMaxTileSize = fUnitCell;
						
			}
	
		virtual void Start (uint32 threadCount,
							const dng_point &tileSize,
							dng_memory_allocator *allocator,
							dng_abort_sniffer * /* sniffer */)
			{
			
			if (tileSize != fUnitCell)
				{
				ThrowProgramError ();
				}
				
			fTilesAcross = (fImage.Bounds ().W () + fUnitCell.h - 1) / fUnitCell.h;
			fTilesDown   = (fImage.Bounds ().H () + fUnitCell.v - 1) / fUnitCell.v;
			
			fTileCount = fTilesAcross * fTilesDown;
						 
			fTileHash.Reset (new dng_fingerprint [fTileCount]);
			
			uint32 bufferSize = fImage.Planes () *
								fPixelSize *
								tileSize.h *
								tileSize.v;
								
			for (uint32 index = 0; index < threadCount; index++)
				{
				
				fBufferData [index].Reset (allocator->Allocate (bufferSize));
				
				}
			
			}

		virtual void Process (uint32 threadIndex,
							  const dng_rect &tile,
							  dng_abort_sniffer * /* sniffer */)
			{
			
			int32 colIndex = (tile.l - fImage.Bounds ().l) / fUnitCell.h;
			int32 rowIndex = (tile.t - fImage.Bounds ().t) / fUnitCell.v;
			
			DNG_ASSERT (tile.l == fImage.Bounds ().l + colIndex * fUnitCell.h &&
						tile.t == fImage.Bounds ().t + rowIndex * fUnitCell.v,
						"Bad tile origin");
			
			uint32 tileIndex = rowIndex * fTilesAcross + colIndex;
			
			dng_pixel_buffer buffer;
			
			buffer.fArea = tile;
			
			buffer.fPlane  = 0;
			buffer.fPlanes = fImage.Planes ();
			
			buffer.fRowStep   = tile.W ();
			buffer.fColStep   = 1;
			buffer.fPlaneStep = tile.W () * tile.H ();
			
			buffer.fPixelType = fPixelType;
			buffer.fPixelSize = fPixelSize;
	
			buffer.fData = fBufferData [threadIndex]->Buffer ();
			
			fImage.Get (buffer);
			
			uint32 count = buffer.fPlaneStep *
						   buffer.fPlanes *
						   buffer.fPixelSize;
			
			#if qDNGBigEndian
			
			// We need to use the same byte order to compute
			// the digest, no matter the native order.  Little-endian
			// is more common now, so use that.
			
			switch (buffer.fPixelSize)
				{
				
				case 1:
					break;
				
				case 2:
					{
					DoSwapBytes16 ((uint16 *) buffer.fData, count >> 1);
					break;
					}
				
				case 4:
					{
					DoSwapBytes32 ((uint32 *) buffer.fData, count >> 2);
					break;
					}
					
				default:
					{
					DNG_REPORT ("Unexpected pixel size");
					break;
					}
				
				}

			#endif
			
			dng_md5_printer printer;
			
			printer.Process (buffer.fData, count);
							 
			fTileHash [tileIndex] = printer.Result ();
			
			}
			
		dng_fingerprint Result ()
			{
			
			dng_md5_printer printer;
			
			for (uint32 tileIndex = 0; tileIndex < fTileCount; tileIndex++)
				{
				
				printer.Process (fTileHash [tileIndex] . data, 16);
				
				}
				
			return printer.Result ();
			
			}
		
	};

/*****************************************************************************/

void dng_negative::FindNewRawImageDigest (dng_host &host) const
	{
	
	if (fNewRawImageDigest.IsNull ())
		{
		
		#if qDNGValidate
		
		dng_timer timeScope ("FindNewRawImageDigest time");

		#endif
		
		// Find fast digest of the raw image.
		
			{
		
			const dng_image &rawImage = RawImage ();
			
			// Find pixel type that will be saved in the file.  When saving DNGs, we convert
			// some 16-bit data to 8-bit data, so we need to do the matching logic here.
			
			uint32 rawPixelType = rawImage.PixelType ();
			
			if (rawPixelType == ttShort)
				{
			
				// See if we are using a linearization table with <= 256 entries, in which
				// case the useful data will all fit within 8-bits.
				
				const dng_linearization_info *rangeInfo = GetLinearizationInfo ();
			
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
			
			// Find the fast digest on the raw image.
				
			dng_find_new_raw_image_digest_task task (rawImage, rawPixelType);
			
			host.PerformAreaTask (task, rawImage.Bounds ());
			
			fNewRawImageDigest = task.Result ();
				
			}
			
		// If there is a transparancy mask, we need to include that in the
		// digest also.
		
		if (RawTransparencyMask () != NULL)
			{
			
			// Find the fast digest on the raw mask.
			
			dng_fingerprint maskDigest;
			
				{
				
				dng_find_new_raw_image_digest_task task (*RawTransparencyMask (),
														 RawTransparencyMask ()->PixelType ());
				
				host.PerformAreaTask (task, RawTransparencyMask ()->Bounds ());
				
				maskDigest = task.Result ();
				
				}
				
			// Combine the two digests into a single digest.
			
			dng_md5_printer printer;
			
			printer.Process (fNewRawImageDigest.data, 16);
			
			printer.Process (maskDigest.data, 16);
			
			fNewRawImageDigest = printer.Result ();
			
			}
		
		}
	
	}
							   
/*****************************************************************************/

void dng_negative::ValidateRawImageDigest (dng_host &host)
	{
	
	if (Stage1Image () && !IsPreview () && (fRawImageDigest   .IsValid () ||
										    fNewRawImageDigest.IsValid ()))
		{
		
		bool isNewDigest = fNewRawImageDigest.IsValid ();
		
		dng_fingerprint &rawDigest = isNewDigest ? fNewRawImageDigest
												 : fRawImageDigest;
		
		// For lossy compressed JPEG images, we need to compare the stored
		// digest to the digest computed from the compressed data, since
		// decompressing lossy JPEG data is itself a lossy process.
		
		if (RawJPEGImageDigest ().IsValid () || RawJPEGImage ())
			{
			
			// Compute the raw JPEG image digest if we have not done so
			// already.
			
			FindRawJPEGImageDigest (host);
			
			if (rawDigest != RawJPEGImageDigest ())
				{
				
				#if qDNGValidate
				
				ReportError ("RawImageDigest does not match raw jpeg image");
				
				#else
				
				SetIsDamaged (true);
				
				#endif
				
				}
			
			}
			
		// Else we can compare the stored digest to the image in memory.
			
		else
			{
		
			dng_fingerprint oldDigest = rawDigest;
			
			try
				{
				
				rawDigest.Clear ();
				
				if (isNewDigest)
					{
					
					FindNewRawImageDigest (host);
					
					}
					
				else
					{
					
					FindRawImageDigest (host);
					
					}
				
				}
				
			catch (...)
				{
				
				rawDigest = oldDigest;
				
				throw;
				
				}
			
			if (oldDigest != rawDigest)
				{
				
				#if qDNGValidate
				
				if (isNewDigest)
					{
					ReportError ("NewRawImageDigest does not match raw image");
					}
				else
					{
					ReportError ("RawImageDigest does not match raw image");
					}
				
				SetIsDamaged (true);
				
				#else
				
				if (!isNewDigest)
					{
				
					// Note that Lightroom 1.4 Windows had a bug that corrupts the
					// first four bytes of the RawImageDigest tag.  So if the last
					// twelve bytes match, this is very likely the result of the
					// bug, and not an actual corrupt file.  So don't report this
					// to the user--just fix it.
					
						{
					
						bool matchLast12 = true;
						
						for (uint32 j = 4; j < 16; j++)
							{
							matchLast12 = matchLast12 && (oldDigest.data [j] == fRawImageDigest.data [j]);
							}
							
						if (matchLast12)
							{
							return;
							}
							
						}
						
					// Sometimes Lightroom 1.4 would corrupt more than the first four
					// bytes, but for all those files that I have seen so far the
					// resulting first four bytes are 0x08 0x00 0x00 0x00.
					
					if (oldDigest.data [0] == 0x08 &&
						oldDigest.data [1] == 0x00 &&
						oldDigest.data [2] == 0x00 &&
						oldDigest.data [3] == 0x00)
						{
						return;
						}
						
					}
					
				SetIsDamaged (true);
				
				#endif
				
				}
				
			}
			
		}
	
	}

/*****************************************************************************/

// If the raw data unique ID is missing, compute one based on a MD5 hash of
// the raw image hash and the model name, plus other commonly changed
// data that can affect rendering.

void dng_negative::FindRawDataUniqueID (dng_host &host) const
	{
	
	if (fRawDataUniqueID.IsNull ())
		{
		
		dng_md5_printer_stream printer;
		
		// If we have a raw jpeg image, it is much faster to
		// use its digest as part of the unique ID since
		// the data size is much smaller.  We cannot use it
		// if there a transparency mask, since that is not
		// included in the RawJPEGImageDigest.
		
		if (RawJPEGImage () && !RawTransparencyMask ())
			{
			
			FindRawJPEGImageDigest (host);
			
			printer.Put (fRawJPEGImageDigest.data, 16);
			
			}
		
		// Include the new raw image digest in the unique ID.
		
		else
			{
		
			FindNewRawImageDigest (host);
					
			printer.Put (fNewRawImageDigest.data, 16);
			
			}
		
		// Include model name.
					
		printer.Put (ModelName ().Get    (),
					 ModelName ().Length ());
					 
		// Include default crop area, since DNG Recover Edges can modify
		// these values and they affect rendering.
					 
		printer.Put_uint32 (fDefaultCropSizeH.n);
		printer.Put_uint32 (fDefaultCropSizeH.d);
		
		printer.Put_uint32 (fDefaultCropSizeV.n);
		printer.Put_uint32 (fDefaultCropSizeV.d);
		
		printer.Put_uint32 (fDefaultCropOriginH.n);
		printer.Put_uint32 (fDefaultCropOriginH.d);
		
		printer.Put_uint32 (fDefaultCropOriginV.n);
		printer.Put_uint32 (fDefaultCropOriginV.d);

		// Include default user crop.

		printer.Put_uint32 (fDefaultUserCropT.n);
		printer.Put_uint32 (fDefaultUserCropT.d);
		
		printer.Put_uint32 (fDefaultUserCropL.n);
		printer.Put_uint32 (fDefaultUserCropL.d);
		
		printer.Put_uint32 (fDefaultUserCropB.n);
		printer.Put_uint32 (fDefaultUserCropB.d);
		
		printer.Put_uint32 (fDefaultUserCropR.n);
		printer.Put_uint32 (fDefaultUserCropR.d);
		
		// Include opcode lists, since lens correction utilities can modify
		// these values and they affect rendering.
		
		fOpcodeList1.FingerprintToStream (printer);
		fOpcodeList2.FingerprintToStream (printer);
		fOpcodeList3.FingerprintToStream (printer);
		
		fRawDataUniqueID = printer.Result ();
	
		}
	
	}
		
/******************************************************************************/

// Forces recomputation of RawDataUniqueID, useful to call
// after modifying the opcode lists, etc.

void dng_negative::RecomputeRawDataUniqueID (dng_host &host)
	{
	
	fRawDataUniqueID.Clear ();
	
	FindRawDataUniqueID (host);
	
	}
		
/******************************************************************************/

void dng_negative::FindOriginalRawFileDigest () const
	{

	if (fOriginalRawFileDigest.IsNull () && fOriginalRawFileData.Get ())
		{
		
		dng_md5_printer printer;
		
		printer.Process (fOriginalRawFileData->Buffer      (),
						 fOriginalRawFileData->LogicalSize ());
					
		fOriginalRawFileDigest = printer.Result ();
	
		}

	}
		
/*****************************************************************************/

void dng_negative::ValidateOriginalRawFileDigest ()
	{
	
	if (fOriginalRawFileDigest.IsValid () && fOriginalRawFileData.Get ())
		{
		
		dng_fingerprint oldDigest = fOriginalRawFileDigest;
		
		try
			{
			
			fOriginalRawFileDigest.Clear ();
			
			FindOriginalRawFileDigest ();
			
			}
			
		catch (...)
			{
			
			fOriginalRawFileDigest = oldDigest;
			
			throw;
			
			}
		
		if (oldDigest != fOriginalRawFileDigest)
			{
			
			#if qDNGValidate
			
			ReportError ("OriginalRawFileDigest does not match OriginalRawFileData");
			
			#else
			
			SetIsDamaged (true);
			
			#endif
			
			// Don't "repair" the original image data digest.  Once it is
			// bad, it stays bad.  The user cannot tell by looking at the image
			// whether the damage is acceptable and can be ignored in the
			// future.
			
			fOriginalRawFileDigest = oldDigest;
			
			}
			
		}
		
	}
							   
/******************************************************************************/

dng_rect dng_negative::DefaultCropArea () const
	{
	
	// First compute the area using simple rounding.
		
	dng_rect result;
	
	result.l = Round_int32 (fDefaultCropOriginH.As_real64 () * fRawToFullScaleH);
	result.t = Round_int32 (fDefaultCropOriginV.As_real64 () * fRawToFullScaleV);
	
	result.r = result.l + Round_int32 (fDefaultCropSizeH.As_real64 () * fRawToFullScaleH);
	result.b = result.t + Round_int32 (fDefaultCropSizeV.As_real64 () * fRawToFullScaleV);
	
	// Sometimes the simple rounding causes the resulting default crop
	// area to slide off the scaled image area.  So we force this not
	// to happen.  We only do this if the image is not stubbed.
		
	const dng_image *image = Stage3Image ();
	
	if (image)
		{
	
		dng_point imageSize = image->Size ();
		
		if (result.r > imageSize.h)
			{
			result.l -= result.r - imageSize.h;
			result.r  = imageSize.h;
			}
			
		if (result.b > imageSize.v)
			{
			result.t -= result.b - imageSize.v;
			result.b  = imageSize.v;
			}
			
		}
		
	return result;
	
	}

/*****************************************************************************/

real64 dng_negative::TotalBaselineExposure (const dng_camera_profile_id &profileID) const
	{
	
	real64 total = BaselineExposure ();

	const dng_camera_profile *profile = ProfileByID (profileID);
	
	if (profile)
		{

		real64 offset = profile->BaselineExposureOffset ().As_real64 ();

		total += offset;
		
		}

	return total;
	
	}

/******************************************************************************/

void dng_negative::SetShadowScale (const dng_urational &scale)
	{
	
	if (scale.d > 0)
		{
		
		real64 s = scale.As_real64 ();
		
		if (s > 0.0 && s <= 1.0)
			{
	
			fShadowScale = scale;
			
			}
		
		}
	
	}
			
/******************************************************************************/

void dng_negative::SetActiveArea (const dng_rect &area)
	{
	
	NeedLinearizationInfo ();
	
	dng_linearization_info &info = *fLinearizationInfo.Get ();
								    
	info.fActiveArea = area;
	
	}

/******************************************************************************/

void dng_negative::SetMaskedAreas (uint32 count,
							 	   const dng_rect *area)
	{
	
	DNG_ASSERT (count <= kMaxMaskedAreas, "Too many masked areas");
	
	NeedLinearizationInfo ();
	
	dng_linearization_info &info = *fLinearizationInfo.Get ();
								    
	info.fMaskedAreaCount = Min_uint32 (count, kMaxMaskedAreas);
	
	for (uint32 index = 0; index < info.fMaskedAreaCount; index++)
		{
		
		info.fMaskedArea [index] = area [index];
		
		}
		
	}
		
/*****************************************************************************/

void dng_negative::SetLinearization (AutoPtr<dng_memory_block> &curve)
	{
	
	NeedLinearizationInfo ();
	
	dng_linearization_info &info = *fLinearizationInfo.Get ();
								    
	info.fLinearizationTable.Reset (curve.Release ());
	
	}
		
/*****************************************************************************/

void dng_negative::SetBlackLevel (real64 black,
								  int32 plane)
	{

	NeedLinearizationInfo ();
	
	dng_linearization_info &info = *fLinearizationInfo.Get ();
								    
	info.fBlackLevelRepeatRows = 1;
	info.fBlackLevelRepeatCols = 1;
	
	if (plane < 0)
		{
		
		for (uint32 j = 0; j < kMaxSamplesPerPixel; j++)
			{
			
			info.fBlackLevel [0] [0] [j] = black;
			
			}
		
		}
		
	else
		{
		
		info.fBlackLevel [0] [0] [plane] = black;
			
		}
	
	info.RoundBlacks ();
		
	}
		
/*****************************************************************************/

void dng_negative::SetQuadBlacks (real64 black0,
						    	  real64 black1,
						    	  real64 black2,
						    	  real64 black3,
								  int32 plane)
	{
	
	NeedLinearizationInfo ();
	
	dng_linearization_info &info = *fLinearizationInfo.Get ();
								    
	info.fBlackLevelRepeatRows = 2;
	info.fBlackLevelRepeatCols = 2;

	if (plane < 0)
		{
	
		for (uint32 j = 0; j < kMaxSamplesPerPixel; j++)
			{

			info.fBlackLevel [0] [0] [j] = black0;
			info.fBlackLevel [0] [1] [j] = black1;
			info.fBlackLevel [1] [0] [j] = black2;
			info.fBlackLevel [1] [1] [j] = black3;

			}

		}

	else
		{
		
		info.fBlackLevel [0] [0] [plane] = black0;
		info.fBlackLevel [0] [1] [plane] = black1;
		info.fBlackLevel [1] [0] [plane] = black2;
		info.fBlackLevel [1] [1] [plane] = black3;
		
		}
		
	info.RoundBlacks ();
		
	}

/*****************************************************************************/

void dng_negative::SetRowBlacks (const real64 *blacks,
				   		   		 uint32 count)
	{
	
	if (count)
		{
	
		NeedLinearizationInfo ();
		
		dng_linearization_info &info = *fLinearizationInfo.Get ();
		
		uint32 byteCount = count * (uint32) sizeof (real64);
									    
		info.fBlackDeltaV.Reset (Allocator ().Allocate (byteCount));
		
		DoCopyBytes (blacks,
					 info.fBlackDeltaV->Buffer (),
					 byteCount);
		
		info.RoundBlacks ();
		
		}
		
	else if (fLinearizationInfo.Get ())
		{
		
		dng_linearization_info &info = *fLinearizationInfo.Get ();
		
		info.fBlackDeltaV.Reset ();
	
		}
								    
	}
							
/*****************************************************************************/

void dng_negative::SetColumnBlacks (const real64 *blacks,
					  		  	    uint32 count)
	{
	
	if (count)
		{
	
		NeedLinearizationInfo ();
		
		dng_linearization_info &info = *fLinearizationInfo.Get ();
		
		uint32 byteCount = count * (uint32) sizeof (real64);
									    
		info.fBlackDeltaH.Reset (Allocator ().Allocate (byteCount));
		
		DoCopyBytes (blacks,
					 info.fBlackDeltaH->Buffer (),
					 byteCount);
		
		info.RoundBlacks ();
		
		}
		
	else if (fLinearizationInfo.Get ())
		{
		
		dng_linearization_info &info = *fLinearizationInfo.Get ();
									    
		info.fBlackDeltaH.Reset ();
	
		}
								    
	}
							
/*****************************************************************************/

uint32 dng_negative::WhiteLevel (uint32 plane) const
	{
	
	if (fLinearizationInfo.Get ())
		{
		
		const dng_linearization_info &info = *fLinearizationInfo.Get ();
									    
		return Round_uint32 (info.fWhiteLevel [plane]);
									    
		}
		
	if (RawImage ().PixelType () == ttFloat)
		{
		
		return 1;
		
		}
		
	return 0x0FFFF;
	
	}
							
/*****************************************************************************/

void dng_negative::SetWhiteLevel (uint32 white,
							      int32 plane)
	{

	NeedLinearizationInfo ();
	
	dng_linearization_info &info = *fLinearizationInfo.Get ();
								    
	if (plane < 0)
		{
		
		for (uint32 j = 0; j < kMaxSamplesPerPixel; j++)
			{
			
			info.fWhiteLevel [j] = (real64) white;
			
			}
		
		}
		
	else
		{
		
		info.fWhiteLevel [plane] = (real64) white;
			
		}
	
	}

/******************************************************************************/

void dng_negative::SetColorKeys (ColorKeyCode color0,
						   		 ColorKeyCode color1,
						   		 ColorKeyCode color2,
						   		 ColorKeyCode color3)
	{
	
	NeedMosaicInfo ();
	
	dng_mosaic_info &info = *fMosaicInfo.Get ();
							 
	info.fCFAPlaneColor [0] = color0;
	info.fCFAPlaneColor [1] = color1;
	info.fCFAPlaneColor [2] = color2;
	info.fCFAPlaneColor [3] = color3;
	
	}

/******************************************************************************/

void dng_negative::SetBayerMosaic (uint32 phase)
	{
	
	NeedMosaicInfo ();
	
	dng_mosaic_info &info = *fMosaicInfo.Get ();
	
	ColorKeyCode color0 = (ColorKeyCode) info.fCFAPlaneColor [0];
	ColorKeyCode color1 = (ColorKeyCode) info.fCFAPlaneColor [1];
	ColorKeyCode color2 = (ColorKeyCode) info.fCFAPlaneColor [2];
	
	info.fCFAPatternSize = dng_point (2, 2);
	
	switch (phase)
		{
		
		case 0:
			{
			info.fCFAPattern [0] [0] = color1;
			info.fCFAPattern [0] [1] = color0;
			info.fCFAPattern [1] [0] = color2;
			info.fCFAPattern [1] [1] = color1;
			break;
			}
			
		case 1:
			{
			info.fCFAPattern [0] [0] = color0;
			info.fCFAPattern [0] [1] = color1;
			info.fCFAPattern [1] [0] = color1;
			info.fCFAPattern [1] [1] = color2;
			break;
			}
			
		case 2:
			{
			info.fCFAPattern [0] [0] = color2;
			info.fCFAPattern [0] [1] = color1;
			info.fCFAPattern [1] [0] = color1;
			info.fCFAPattern [1] [1] = color0;
			break;
			}
			
		case 3:
			{
			info.fCFAPattern [0] [0] = color1;
			info.fCFAPattern [0] [1] = color2;
			info.fCFAPattern [1] [0] = color0;
			info.fCFAPattern [1] [1] = color1;
			break;
			}
			
		}
		
	info.fColorPlanes = 3;
	
	info.fCFALayout = 1;
	
	}

/******************************************************************************/

void dng_negative::SetFujiMosaic (uint32 phase)
	{
	
	NeedMosaicInfo ();
	
	dng_mosaic_info &info = *fMosaicInfo.Get ();
	
	ColorKeyCode color0 = (ColorKeyCode) info.fCFAPlaneColor [0];
	ColorKeyCode color1 = (ColorKeyCode) info.fCFAPlaneColor [1];
	ColorKeyCode color2 = (ColorKeyCode) info.fCFAPlaneColor [2];
	
	info.fCFAPatternSize = dng_point (2, 4);
	
	switch (phase)
		{
		
		case 0:
			{
			info.fCFAPattern [0] [0] = color0;
			info.fCFAPattern [0] [1] = color1;
			info.fCFAPattern [0] [2] = color2;
			info.fCFAPattern [0] [3] = color1;
			info.fCFAPattern [1] [0] = color2;
			info.fCFAPattern [1] [1] = color1;
			info.fCFAPattern [1] [2] = color0;
			info.fCFAPattern [1] [3] = color1;
			break;
			}
			
		case 1:
			{
			info.fCFAPattern [0] [0] = color2;
			info.fCFAPattern [0] [1] = color1;
			info.fCFAPattern [0] [2] = color0;
			info.fCFAPattern [0] [3] = color1;
			info.fCFAPattern [1] [0] = color0;
			info.fCFAPattern [1] [1] = color1;
			info.fCFAPattern [1] [2] = color2;
			info.fCFAPattern [1] [3] = color1;
			break;
			}
			
		}
		
	info.fColorPlanes = 3;
	
	info.fCFALayout = 2;
			
	}

/*****************************************************************************/

void dng_negative::SetFujiMosaic6x6 (uint32 phase)
	{
	
	NeedMosaicInfo ();
	
	dng_mosaic_info &info = *fMosaicInfo.Get ();
	
	ColorKeyCode color0 = (ColorKeyCode) info.fCFAPlaneColor [0];
	ColorKeyCode color1 = (ColorKeyCode) info.fCFAPlaneColor [1];
	ColorKeyCode color2 = (ColorKeyCode) info.fCFAPlaneColor [2];

	const uint32 patSize = 6;
	
	info.fCFAPatternSize = dng_point (patSize, patSize);

	info.fCFAPattern [0] [0] = color1;
	info.fCFAPattern [0] [1] = color2;
	info.fCFAPattern [0] [2] = color1;
	info.fCFAPattern [0] [3] = color1;
	info.fCFAPattern [0] [4] = color0;
	info.fCFAPattern [0] [5] = color1;

	info.fCFAPattern [1] [0] = color0;
	info.fCFAPattern [1] [1] = color1;
	info.fCFAPattern [1] [2] = color0;
	info.fCFAPattern [1] [3] = color2;
	info.fCFAPattern [1] [4] = color1;
	info.fCFAPattern [1] [5] = color2;

	info.fCFAPattern [2] [0] = color1;
	info.fCFAPattern [2] [1] = color2;
	info.fCFAPattern [2] [2] = color1;
	info.fCFAPattern [2] [3] = color1;
	info.fCFAPattern [2] [4] = color0;
	info.fCFAPattern [2] [5] = color1;

	info.fCFAPattern [3] [0] = color1;
	info.fCFAPattern [3] [1] = color0;
	info.fCFAPattern [3] [2] = color1;
	info.fCFAPattern [3] [3] = color1;
	info.fCFAPattern [3] [4] = color2;
	info.fCFAPattern [3] [5] = color1;

	info.fCFAPattern [4] [0] = color2;
	info.fCFAPattern [4] [1] = color1;
	info.fCFAPattern [4] [2] = color2;
	info.fCFAPattern [4] [3] = color0;
	info.fCFAPattern [4] [4] = color1;
	info.fCFAPattern [4] [5] = color0;

	info.fCFAPattern [5] [0] = color1;
	info.fCFAPattern [5] [1] = color0;
	info.fCFAPattern [5] [2] = color1;
	info.fCFAPattern [5] [3] = color1;
	info.fCFAPattern [5] [4] = color2;
	info.fCFAPattern [5] [5] = color1;

	DNG_REQUIRE (phase >= 0 && phase < patSize * patSize,
				 "Bad phase in SetFujiMosaic6x6.");

	if (phase > 0)
		{
		
		dng_mosaic_info temp = info;

		uint32 phaseRow = phase / patSize;

		uint32 phaseCol = phase - (phaseRow * patSize);

		for (uint32 dstRow = 0; dstRow < patSize; dstRow++)
			{
			
			uint32 srcRow = (dstRow + phaseRow) % patSize;
			
			for (uint32 dstCol = 0; dstCol < patSize; dstCol++)
				{

				uint32 srcCol = (dstCol + phaseCol) % patSize;
			
				temp.fCFAPattern [dstRow] [dstCol] = info.fCFAPattern [srcRow] [srcCol];

				}
			
			}

		info = temp;
		
		}
		
	info.fColorPlanes = 3;
	
	info.fCFALayout = 1;
			
	}

/******************************************************************************/

void dng_negative::SetQuadMosaic (uint32 pattern)
	{
	
	// The pattern of the four colors is assumed to be repeat at least every two
	// columns and eight rows.  The pattern is encoded as a 32-bit integer,
	// with every two bits encoding a color, in scan order for two columns and
	// eight rows (lsb is first).  The usual color coding is:
	//
	// 0 = Green
	// 1 = Magenta
	// 2 = Cyan
	// 3 = Yellow
	//
	// Examples:
	//
	//	PowerShot 600 uses 0xe1e4e1e4:
	//
	//	  0 1 2 3 4 5
	//	0 G M G M G M
	//	1 C Y C Y C Y
	//	2 M G M G M G
	//	3 C Y C Y C Y
	//
	//	PowerShot A5 uses 0x1e4e1e4e:
	//
	//	  0 1 2 3 4 5
	//	0 C Y C Y C Y
	//	1 G M G M G M
	//	2 C Y C Y C Y
	//	3 M G M G M G
	//
	//	PowerShot A50 uses 0x1b4e4b1e:
	//
	//	  0 1 2 3 4 5
	//	0 C Y C Y C Y
	//	1 M G M G M G
	//	2 Y C Y C Y C
	//	3 G M G M G M
	//	4 C Y C Y C Y
	//	5 G M G M G M
	//	6 Y C Y C Y C
	//	7 M G M G M G
	//
	//	PowerShot Pro70 uses 0x1e4b4e1b:
	//
	//	  0 1 2 3 4 5
	//	0 Y C Y C Y C
	//	1 M G M G M G
	//	2 C Y C Y C Y
	//	3 G M G M G M
	//	4 Y C Y C Y C
	//	5 G M G M G M
	//	6 C Y C Y C Y
	//	7 M G M G M G
	//
	//	PowerShots Pro90 and G1 use 0xb4b4b4b4:
	//
	//	  0 1 2 3 4 5
	//	0 G M G M G M
	//	1 Y C Y C Y C

	NeedMosaicInfo ();
	
	dng_mosaic_info &info = *fMosaicInfo.Get ();
							 
	if (((pattern >> 16) & 0x0FFFF) != (pattern & 0x0FFFF))
		{
		info.fCFAPatternSize = dng_point (8, 2);
		}
		
	else if (((pattern >> 8) & 0x0FF) != (pattern & 0x0FF))
		{
		info.fCFAPatternSize = dng_point (4, 2);
		}
		
	else
		{
		info.fCFAPatternSize = dng_point (2, 2);
		}
		
	for (int32 row = 0; row < info.fCFAPatternSize.v; row++)
		{
		
		for (int32 col = 0; col < info.fCFAPatternSize.h; col++)
			{
			
			uint32 index = (pattern >> ((((row << 1) & 14) + (col & 1)) << 1)) & 3;
			
			info.fCFAPattern [row] [col] = info.fCFAPlaneColor [index];
			
			}
			
		}

	info.fColorPlanes = 4;
	
	info.fCFALayout = 1;
			
	}
	
/******************************************************************************/

void dng_negative::SetGreenSplit (uint32 split)
	{
	
	NeedMosaicInfo ();
	
	dng_mosaic_info &info = *fMosaicInfo.Get ();
	
	info.fBayerGreenSplit = split;
	
	}

/*****************************************************************************/

void dng_negative::Parse (dng_host &host,
						  dng_stream &stream,
						  dng_info &info)
	{
	
	// Shared info.
	
	dng_shared &shared = *(info.fShared.Get ());
	
	// Find IFD holding the main raw information.
	
	dng_ifd &rawIFD = *info.fIFD [info.fMainIndex].Get ();
	
	// Model name.
	
	SetModelName (shared.fUniqueCameraModel.Get ());
	
	// Localized model name.
	
	SetLocalName (shared.fLocalizedCameraModel.Get ());
	
	// Base orientation.
	
		{
	
		uint32 orientation = info.fIFD [0]->fOrientation;
		
		if (orientation >= 1 && orientation <= 8)
			{
			
			SetBaseOrientation (dng_orientation::TIFFtoDNG (orientation));
						
			}
			
		}
		
	// Default crop rectangle.
	
	SetDefaultCropSize (rawIFD.fDefaultCropSizeH,
					    rawIFD.fDefaultCropSizeV);

	SetDefaultCropOrigin (rawIFD.fDefaultCropOriginH,
						  rawIFD.fDefaultCropOriginV);

	// Default user crop rectangle.

	SetDefaultUserCrop (rawIFD.fDefaultUserCropT,
						rawIFD.fDefaultUserCropL,
						rawIFD.fDefaultUserCropB,
						rawIFD.fDefaultUserCropR);
						        
	// Default scale.
		
	SetDefaultScale (rawIFD.fDefaultScaleH,
					 rawIFD.fDefaultScaleV);
	
	// Best quality scale.
	
	SetBestQualityScale (rawIFD.fBestQualityScale);
	
	// Baseline noise.

	SetBaselineNoise (shared.fBaselineNoise.As_real64 ());
	
	// NoiseReductionApplied.
	
	SetNoiseReductionApplied (shared.fNoiseReductionApplied);

	// NoiseProfile.

	SetNoiseProfile (shared.fNoiseProfile);
	
	// Baseline exposure.
	
	SetBaselineExposure (shared.fBaselineExposure.As_real64 ());

	// Baseline sharpness.
	
	SetBaselineSharpness (shared.fBaselineSharpness.As_real64 ());

	// Chroma blur radius.
	
	SetChromaBlurRadius (rawIFD.fChromaBlurRadius);

	// Anti-alias filter strength.
	
	SetAntiAliasStrength (rawIFD.fAntiAliasStrength);
		
	// Linear response limit.
	
	SetLinearResponseLimit (shared.fLinearResponseLimit.As_real64 ());
	
	// Shadow scale.
	
	SetShadowScale (shared.fShadowScale);
	
	// Colorimetric reference.
	
	SetColorimetricReference (shared.fColorimetricReference);
	
	// Color channels.
		
	SetColorChannels (shared.fCameraProfile.fColorPlanes);
	
	// Analog balance.
	
	if (shared.fAnalogBalance.NotEmpty ())
		{
		
		SetAnalogBalance (shared.fAnalogBalance);
		
		}

	// Camera calibration matrices

	if (shared.fCameraCalibration1.NotEmpty ())
		{
		
		SetCameraCalibration1 (shared.fCameraCalibration1);
		
		}
		
	if (shared.fCameraCalibration2.NotEmpty ())
		{
		
		SetCameraCalibration2 (shared.fCameraCalibration2);
		
		}
		
	if (shared.fCameraCalibration1.NotEmpty () ||
		shared.fCameraCalibration2.NotEmpty ())
		{
		
		SetCameraCalibrationSignature (shared.fCameraCalibrationSignature.Get ());
		
		}

	// Embedded camera profiles.
	
	if (shared.fCameraProfile.fColorPlanes > 1)
		{
	
		if (qDNGValidate || host.NeedsMeta () || host.NeedsImage ())
			{
			
			// Add profile from main IFD.
			
				{
			
				AutoPtr<dng_camera_profile> profile (new dng_camera_profile ());
				
				dng_camera_profile_info &profileInfo = shared.fCameraProfile;
				
				profile->Parse (stream, profileInfo);
				
				// The main embedded profile must be valid.
				
				if (!profile->IsValid (shared.fCameraProfile.fColorPlanes))
					{
					
					ThrowBadFormat ();
					
					}
				
				profile->SetWasReadFromDNG ();
				
				AddProfile (profile);
				
				}
				
			// Extra profiles.

			for (uint32 index = 0; index < (uint32) shared.fExtraCameraProfiles.size (); index++)
				{
				
				try
					{

					AutoPtr<dng_camera_profile> profile (new dng_camera_profile ());
					
					dng_camera_profile_info &profileInfo = shared.fExtraCameraProfiles [index];
					
					profile->Parse (stream, profileInfo);
					
					if (!profile->IsValid (shared.fCameraProfile.fColorPlanes))
						{
						
						ThrowBadFormat ();
						
						}
					
					profile->SetWasReadFromDNG ();
					
					AddProfile (profile);

					}
					
				catch (dng_exception &except)
					{
					
					// Don't ignore transient errors.
					
					if (host.IsTransientError (except.ErrorCode ()))
						{
						
						throw;
						
						}
				
					// Eat other parsing errors.
			
					#if qDNGValidate
					
					ReportWarning ("Unable to parse extra profile");
					
					#endif
					
					}
			
				}
			
			}
			
		// As shot profile name.
		
		if (shared.fAsShotProfileName.NotEmpty ())
			{
			
			SetAsShotProfileName (shared.fAsShotProfileName.Get ());
			
			}
			
		}
		
	// Raw image data digest.
	
	if (shared.fRawImageDigest.IsValid ())
		{
		
		SetRawImageDigest (shared.fRawImageDigest);
		
		}
			
	// New raw image data digest.
	
	if (shared.fNewRawImageDigest.IsValid ())
		{
		
		SetNewRawImageDigest (shared.fNewRawImageDigest);
		
		}
			
	// Raw data unique ID.
	
	if (shared.fRawDataUniqueID.IsValid ())
		{
		
		SetRawDataUniqueID (shared.fRawDataUniqueID);
		
		}
			
	// Original raw file name.
	
	if (shared.fOriginalRawFileName.NotEmpty ())
		{
		
		SetOriginalRawFileName (shared.fOriginalRawFileName.Get ());
		
		}
		
	// Original raw file data.
	
	if (shared.fOriginalRawFileDataCount)
		{
		
		SetHasOriginalRawFileData (true);
					
		if (host.KeepOriginalFile ())
			{
			
			uint32 count = shared.fOriginalRawFileDataCount;
			
			AutoPtr<dng_memory_block> block (host.Allocate (count));
			
			stream.SetReadPosition (shared.fOriginalRawFileDataOffset);
		
			stream.Get (block->Buffer (), count);
						
			SetOriginalRawFileData (block);
			
			SetOriginalRawFileDigest (shared.fOriginalRawFileDigest);
			
			ValidateOriginalRawFileDigest ();
			
			}
			
		}
			
	// DNG private data.
	
	if (shared.fDNGPrivateDataCount && (host.SaveDNGVersion () != dngVersion_None))
		{
		
		uint32 length = shared.fDNGPrivateDataCount;
		
		AutoPtr<dng_memory_block> block (host.Allocate (length));
		
		stream.SetReadPosition (shared.fDNGPrivateDataOffset);
			
		stream.Get (block->Buffer (), length);
							
		SetPrivateData (block);
			
		}
		
	// Hand off EXIF metadata to negative.
	
	ResetExif (info.fExif.Release ());
	
	// Parse linearization info.
	
	NeedLinearizationInfo ();
	
	fLinearizationInfo.Get ()->Parse (host,
								      stream,
								      info);
								      
	// Parse mosaic info.
	
	if (rawIFD.fPhotometricInterpretation == piCFA)
		{
	
		NeedMosaicInfo ();
		
		fMosaicInfo.Get ()->Parse (host,
							       stream,
							       info);
							  
		}
						  
	// Fill in original sizes.
	
	if (shared.fOriginalDefaultFinalSize.h > 0 &&
		shared.fOriginalDefaultFinalSize.v > 0)
		{
		
		SetOriginalDefaultFinalSize (shared.fOriginalDefaultFinalSize);
		
		SetOriginalBestQualityFinalSize (shared.fOriginalDefaultFinalSize);
		
		SetOriginalDefaultCropSize (dng_urational (shared.fOriginalDefaultFinalSize.h, 1),
									dng_urational (shared.fOriginalDefaultFinalSize.v, 1));
		
		}
		
	if (shared.fOriginalBestQualityFinalSize.h > 0 &&
		shared.fOriginalBestQualityFinalSize.v > 0)
		{
		
		SetOriginalBestQualityFinalSize (shared.fOriginalBestQualityFinalSize);
		
		}
		
	if (shared.fOriginalDefaultCropSizeH.As_real64 () >= 1.0 &&
		shared.fOriginalDefaultCropSizeV.As_real64 () >= 1.0)
		{
		
		SetOriginalDefaultCropSize (shared.fOriginalDefaultCropSizeH,
									shared.fOriginalDefaultCropSizeV);
		
		}
		
	}

/*****************************************************************************/

void dng_negative::SetDefaultOriginalSizes ()
	{
	
	// Fill in original sizes if we don't have them already.
	
	if (OriginalDefaultFinalSize () == dng_point ())
		{
		
		SetOriginalDefaultFinalSize (dng_point (DefaultFinalHeight (),
												DefaultFinalWidth  ()));
		
		}
		
	if (OriginalBestQualityFinalSize () == dng_point ())
		{
		
		SetOriginalBestQualityFinalSize (dng_point (BestQualityFinalHeight (),
													BestQualityFinalWidth  ()));
		
		}
		
	if (OriginalDefaultCropSizeH ().NotValid () ||
		OriginalDefaultCropSizeV ().NotValid ())
		{
		
		SetOriginalDefaultCropSize (DefaultCropSizeH (),
									DefaultCropSizeV ());
		
		}

	}

/*****************************************************************************/

void dng_negative::PostParse (dng_host &host,
						  	  dng_stream &stream,
						  	  dng_info &info)
	{
	
	// Shared info.
	
	dng_shared &shared = *(info.fShared.Get ());
	
	if (host.NeedsMeta ())
		{
		
		// Fill in original sizes if we don't have them already.
		
		SetDefaultOriginalSizes ();
				
		// MakerNote.
		
		if (shared.fMakerNoteCount)
			{
			
			// See if we know if the MakerNote is safe or not.
			
			SetMakerNoteSafety (shared.fMakerNoteSafety == 1);
			
			// If the MakerNote is safe, preserve it as a MakerNote.
			
			if (IsMakerNoteSafe ())
				{

				AutoPtr<dng_memory_block> block (host.Allocate (shared.fMakerNoteCount));
				
				stream.SetReadPosition (shared.fMakerNoteOffset);
					
				stream.Get (block->Buffer (), shared.fMakerNoteCount);
									
				SetMakerNote (block);
							
				}
			
			}
		
		// IPTC metadata.
		
		if (shared.fIPTC_NAA_Count)
			{
			
			AutoPtr<dng_memory_block> block (host.Allocate (shared.fIPTC_NAA_Count));
			
			stream.SetReadPosition (shared.fIPTC_NAA_Offset);
			
			uint64 iptcOffset = stream.PositionInOriginalFile();
			
			stream.Get (block->Buffer      (), 
						block->LogicalSize ());
			
			SetIPTC (block, iptcOffset);
							
			}
		
		// XMP metadata.
		
		if (shared.fXMPCount)
			{
			
			AutoPtr<dng_memory_block> block (host.Allocate (shared.fXMPCount));
			
			stream.SetReadPosition (shared.fXMPOffset);
			
			stream.Get (block->Buffer      (),
						block->LogicalSize ());
						
			Metadata ().SetEmbeddedXMP (host,
									    block->Buffer      (),
									    block->LogicalSize ());
										
			#if qDNGValidate
			
			if (!Metadata ().HaveValidEmbeddedXMP ())
				{
				ReportError ("The embedded XMP is invalid");
				}
			
			#endif
			
			}
				
		// Color info.
		
		if (!IsMonochrome ())
			{
			
			// If the ColorimetricReference is the ICC profile PCS,
			// then the data must be already be white balanced to the
			// ICC profile PCS white point.
			
			if (ColorimetricReference () == crICCProfilePCS)
				{
				
				ClearCameraNeutral ();
				
				SetCameraWhiteXY (PCStoXY ());
				
				}
				
			else
				{
				  		    
				// AsShotNeutral.
				
				if (shared.fAsShotNeutral.Count () == ColorChannels ())
					{
					
					SetCameraNeutral (shared.fAsShotNeutral);
										
					}
					
				// AsShotWhiteXY.
				
				if (shared.fAsShotWhiteXY.IsValid () && !HasCameraNeutral ())
					{
					
					SetCameraWhiteXY (shared.fAsShotWhiteXY);
					
					}
					
				}
				
			}
					
		}
		
	}
							
/*****************************************************************************/

bool dng_negative::SetFourColorBayer ()
	{
	
	if (ColorChannels () != 3)
		{
		return false;
		}
		
	if (!fMosaicInfo.Get ())
		{
		return false;
		}
		
	if (!fMosaicInfo.Get ()->SetFourColorBayer ())
		{
		return false;
		}
		
	SetColorChannels (4);
	
	if (fCameraNeutral.Count () == 3)
		{
		
		dng_vector n (4);
		
		n [0] = fCameraNeutral [0];
		n [1] = fCameraNeutral [1];
		n [2] = fCameraNeutral [2];
		n [3] = fCameraNeutral [1];
		
		fCameraNeutral = n;
		
		}

	fCameraCalibration1.Clear ();
	fCameraCalibration2.Clear ();
	
	fCameraCalibrationSignature.Clear ();
	
	for (uint32 index = 0; index < (uint32) fCameraProfile.size (); index++)
		{
		
		fCameraProfile [index]->SetFourColorBayer ();
		
		}
			
	return true;
	
	}
					
/*****************************************************************************/

const dng_image & dng_negative::RawImage () const
	{
	
	if (fRawImage.Get ())
		{
		return *fRawImage.Get ();
		}
		
	if (fStage1Image.Get ())
		{
		return *fStage1Image.Get ();
		}
		
	if (fUnflattenedStage3Image.Get ())
		{
		return *fUnflattenedStage3Image.Get ();
		}
		
	DNG_ASSERT (fStage3Image.Get (),
				"dng_negative::RawImage with no raw image");
		    
	return *fStage3Image.Get ();
	
	}

/*****************************************************************************/

const dng_jpeg_image * dng_negative::RawJPEGImage () const
	{

	return fRawJPEGImage.Get ();

	}

/*****************************************************************************/

void dng_negative::SetRawJPEGImage (AutoPtr<dng_jpeg_image> &jpegImage)
	{

	fRawJPEGImage.Reset (jpegImage.Release ());

	}

/*****************************************************************************/

void dng_negative::ClearRawJPEGImage ()
	{
	
	fRawJPEGImage.Reset ();

	}

/*****************************************************************************/

void dng_negative::FindRawJPEGImageDigest (dng_host &host) const
	{
	
	if (fRawJPEGImageDigest.IsNull ())
		{
		
		if (fRawJPEGImage.Get ())
			{
			
			#if qDNGValidate
			
			dng_timer timer ("FindRawJPEGImageDigest time");
			 
			#endif
			
			fRawJPEGImageDigest = fRawJPEGImage->FindDigest (host);
			
			}
			
		else
			{
			
			ThrowProgramError ("No raw JPEG image");
			
			}
		
		}
	
	}
		
/*****************************************************************************/

void dng_negative::ReadStage1Image (dng_host &host,
									dng_stream &stream,
									dng_info &info)
	{
	
	// Allocate image we are reading.
	
	dng_ifd &rawIFD = *info.fIFD [info.fMainIndex].Get ();
	
	fStage1Image.Reset (host.Make_dng_image (rawIFD.Bounds (),
											 rawIFD.fSamplesPerPixel,
											 rawIFD.PixelType ()));
					
	// See if we should grab the compressed JPEG data.
	
	AutoPtr<dng_jpeg_image> jpegImage;
	
	if (host.SaveDNGVersion () >= dngVersion_1_4_0_0 &&
		!host.PreferredSize () &&
		!host.ForPreview () &&
		rawIFD.fCompression == ccLossyJPEG)
		{
		
		jpegImage.Reset (new dng_jpeg_image);
		
		}
		
	// See if we need to compute the digest of the compressed JPEG data
	// while reading.
	
	bool needJPEGDigest = (RawImageDigest    ().IsValid () ||
						   NewRawImageDigest ().IsValid ()) &&
						  rawIFD.fCompression == ccLossyJPEG &&
						  jpegImage.Get () == NULL;
	
	dng_fingerprint jpegDigest;
	
	// Read the image.
	
	rawIFD.ReadImage (host,
					  stream,
					  *fStage1Image.Get (),
					  jpegImage.Get (),
					  needJPEGDigest ? &jpegDigest : NULL);
					  
	// Remember the raw floating point bit depth, if reading from
	// a floating point image.
	
	if (fStage1Image->PixelType () == ttFloat)
		{
		
		SetRawFloatBitDepth (rawIFD.fBitsPerSample [0]);
		
		}
					  
	// Remember the compressed JPEG data if we read it.
	
	if (jpegImage.Get ())
		{
				
		SetRawJPEGImage (jpegImage);
				
		}
		
	// Remember the compressed JPEG digest if we computed it.
	
	if (jpegDigest.IsValid ())
		{
		
		SetRawJPEGImageDigest (jpegDigest);
		
		}
					  
	// We are are reading the main image, we should read the opcode lists
	// also.
	
	if (rawIFD.fOpcodeList1Count)
		{
		
		#if qDNGValidate
		
		if (gVerbose)
			{
			printf ("\nParsing OpcodeList1: ");
			}
			
		#endif
		
		fOpcodeList1.Parse (host,
							stream,
							rawIFD.fOpcodeList1Count,
							rawIFD.fOpcodeList1Offset);
		
		}
		
	if (rawIFD.fOpcodeList2Count)
		{
		
		#if qDNGValidate
		
		if (gVerbose)
			{
			printf ("\nParsing OpcodeList2: ");
			}
			
		#endif
		
		fOpcodeList2.Parse (host,
							stream,
							rawIFD.fOpcodeList2Count,
							rawIFD.fOpcodeList2Offset);
		
		}

	if (rawIFD.fOpcodeList3Count)
		{
		
		#if qDNGValidate
		
		if (gVerbose)
			{
			printf ("\nParsing OpcodeList3: ");
			}
			
		#endif
		
		fOpcodeList3.Parse (host,
							stream,
							rawIFD.fOpcodeList3Count,
							rawIFD.fOpcodeList3Offset);
		
		}

	}
					
/*****************************************************************************/

void dng_negative::SetStage1Image (AutoPtr<dng_image> &image)
	{
	
	fStage1Image.Reset (image.Release ());
	
	}

/*****************************************************************************/

void dng_negative::SetStage2Image (AutoPtr<dng_image> &image)
	{
	
	fStage2Image.Reset (image.Release ());
	
	}

/*****************************************************************************/

void dng_negative::SetStage3Image (AutoPtr<dng_image> &image)
	{
	
	fStage3Image.Reset (image.Release ());
	
	}

/*****************************************************************************/

void dng_negative::DoBuildStage2 (dng_host &host)
	{
	
	dng_image &stage1 = *fStage1Image.Get ();
		
	dng_linearization_info &info = *fLinearizationInfo.Get ();
	
	uint32 pixelType = ttShort;
	
	if (stage1.PixelType () == ttLong ||
		stage1.PixelType () == ttFloat)
		{
		
		pixelType = ttFloat;
		
		}
	
	fStage2Image.Reset (host.Make_dng_image (info.fActiveArea.Size (),
											 stage1.Planes (),
											 pixelType));
								   
	info.Linearize (host,
					stage1,
					*fStage2Image.Get ());
							 
	}
		
/*****************************************************************************/

void dng_negative::DoPostOpcodeList2 (dng_host & /* host */)
	{
	
	// Nothing by default.
	
	}

/*****************************************************************************/

bool dng_negative::NeedDefloatStage2 (dng_host &host)
	{
	
	if (fStage2Image->PixelType () == ttFloat)
		{
		
		if (fRawImageStage >= rawImageStagePostOpcode2 &&
			host.SaveDNGVersion () != dngVersion_None  &&
			host.SaveDNGVersion () <  dngVersion_1_4_0_0)
			{
			
			return true;
			
			}
		
		}
	
	return false;
	
	}
		
/*****************************************************************************/

void dng_negative::DefloatStage2 (dng_host & /* host */)
	{
	
	ThrowNotYetImplemented ("dng_negative::DefloatStage2");
	
	}
		
/*****************************************************************************/

void dng_negative::BuildStage2Image (dng_host &host)
	{
	
	// If reading the negative to save in DNG format, figure out
	// when to grab a copy of the raw data.
	
	if (host.SaveDNGVersion () != dngVersion_None)
		{
		
		// Transparency masks are only supported in DNG version 1.4 and
		// later.  In this case, the flattening of the transparency mask happens
		// on the the stage3 image.  
		
		if (TransparencyMask () && host.SaveDNGVersion () < dngVersion_1_4_0_0)
			{
			fRawImageStage = rawImageStagePostOpcode3;
			}
		
		else if (fOpcodeList3.MinVersion (false) > host.SaveDNGVersion () ||
			     fOpcodeList3.AlwaysApply ())
			{
			fRawImageStage = rawImageStagePostOpcode3;
			}
			
		else if (host.SaveLinearDNG (*this))
			{
			
			// If the opcode list 3 has optional tags that are beyond the
			// the minimum version, and we are saving a linear DNG anyway,
			// then go ahead and apply them.
			
			if (fOpcodeList3.MinVersion (true) > host.SaveDNGVersion ())
				{
				fRawImageStage = rawImageStagePostOpcode3;
				}
				
			else
				{
				fRawImageStage = rawImageStagePreOpcode3;
				}
			
			}
			
		else if (fOpcodeList2.MinVersion (false) > host.SaveDNGVersion () ||
				 fOpcodeList2.AlwaysApply ())
			{
			fRawImageStage = rawImageStagePostOpcode2;
			}
			
		else if (fOpcodeList1.MinVersion (false) > host.SaveDNGVersion () ||
				 fOpcodeList1.AlwaysApply ())
			{
			fRawImageStage = rawImageStagePostOpcode1;
			}
			
		else
			{
			fRawImageStage = rawImageStagePreOpcode1;
			}
			
		// We should not save floating point stage1 images unless the target
		// DNG version is high enough to understand floating point images. 
		// We handle this by converting from floating point to integer if 
		// required after building stage2 image.
		
		if (fStage1Image->PixelType () == ttFloat)
			{
			
			if (fRawImageStage < rawImageStagePostOpcode2)
				{
				
				if (host.SaveDNGVersion () < dngVersion_1_4_0_0)
					{
					
					fRawImageStage = rawImageStagePostOpcode2;
					
					}
					
				}
				
			}
			
		}
		
	// Grab clone of raw image if required.
	
	if (fRawImageStage == rawImageStagePreOpcode1)
		{
		
		fRawImage.Reset (fStage1Image->Clone ());
		
		if (fTransparencyMask.Get ())
			{
			fRawTransparencyMask.Reset (fTransparencyMask->Clone ());
			}
		
		}

	else
		{
		
		// If we are not keeping the most raw image, we need
		// to recompute the raw image digest.
		
		ClearRawImageDigest ();
		
		// If we don't grab the unprocessed stage 1 image, then
		// the raw JPEG image is no longer valid.
		
		ClearRawJPEGImage ();
		
		// Nor is the digest of the raw JPEG data.
		
		ClearRawJPEGImageDigest ();
		
		// We also don't know the raw floating point bit depth.
		
		SetRawFloatBitDepth (0);
		
		}
		
	// Process opcode list 1.
	
	host.ApplyOpcodeList (fOpcodeList1, *this, fStage1Image);
	
	// See if we are done with the opcode list 1.
	
	if (fRawImageStage > rawImageStagePreOpcode1)
		{
		
		fOpcodeList1.Clear ();
		
		}
	
	// Grab clone of raw image if required.
	
	if (fRawImageStage == rawImageStagePostOpcode1)
		{
		
		fRawImage.Reset (fStage1Image->Clone ());
		
		if (fTransparencyMask.Get ())
			{
			fRawTransparencyMask.Reset (fTransparencyMask->Clone ());
			}
		
		}

	// Finalize linearization info.
	
		{
	
		NeedLinearizationInfo ();
	
		dng_linearization_info &info = *fLinearizationInfo.Get ();
		
		info.PostParse (host, *this);
		
		}
		
	// Perform the linearization.
	
	DoBuildStage2 (host);
		
	// Delete the stage1 image now that we have computed the stage 2 image.
	
	fStage1Image.Reset ();
	
	// Are we done with the linearization info.
	
	if (fRawImageStage > rawImageStagePostOpcode1)
		{
		
		ClearLinearizationInfo ();
		
		}
	
	// Process opcode list 2.
	
	host.ApplyOpcodeList (fOpcodeList2, *this, fStage2Image);
	
	// See if we are done with the opcode list 2.
	
	if (fRawImageStage > rawImageStagePostOpcode1)
		{
		
		fOpcodeList2.Clear ();
		
		}
		
	// Hook for any required processing just after opcode list 2.
	
	DoPostOpcodeList2 (host);
		
	// Convert from floating point to integer if required.
	
	if (NeedDefloatStage2 (host))
		{
		
		DefloatStage2 (host);
		
		}
		
	// Grab clone of raw image if required.
	
	if (fRawImageStage == rawImageStagePostOpcode2)
		{
		
		fRawImage.Reset (fStage2Image->Clone ());
		
		if (fTransparencyMask.Get ())
			{
			fRawTransparencyMask.Reset (fTransparencyMask->Clone ());
			}
		
		}
	
	}
									  
/*****************************************************************************/

void dng_negative::DoInterpolateStage3 (dng_host &host,
								        int32 srcPlane)
	{
	
	dng_image &stage2 = *fStage2Image.Get ();
		
	dng_mosaic_info &info = *fMosaicInfo.Get ();
	
	dng_point downScale = info.DownScale (host.MinimumSize   (),
										  host.PreferredSize (),
										  host.CropFactor    ());
	
	if (downScale != dng_point (1, 1))
		{
		SetIsPreview (true);
		}
	
	dng_point dstSize = info.DstSize (downScale);
			
	fStage3Image.Reset (host.Make_dng_image (dng_rect (dstSize),
											 info.fColorPlanes,
											 stage2.PixelType ()));

	if (srcPlane < 0 || srcPlane >= (int32) stage2.Planes ())
		{
		srcPlane = 0;
		}
				
	info.Interpolate (host,
					  *this,
					  stage2,
					  *fStage3Image.Get (),
					  downScale,
					  srcPlane);

	}
									   
/*****************************************************************************/

// Interpolate and merge a multi-channel CFA image.

void dng_negative::DoMergeStage3 (dng_host &host)
	{
	
	// The DNG SDK does not provide multi-channel CFA image merging code.
	// It just grabs the first channel and uses that.
	
	DoInterpolateStage3 (host, 0);
				   
	// Just grabbing the first channel would often result in the very
	// bright image using the baseline exposure value.
	
	fStage3Gain = pow (2.0, BaselineExposure ());
	
	}
									   
/*****************************************************************************/

void dng_negative::DoBuildStage3 (dng_host &host,
								  int32 srcPlane)
	{
	
	// If we don't have a mosaic pattern, then just move the stage 2
	// image on to stage 3.
	
	dng_mosaic_info *info = fMosaicInfo.Get ();

	if (!info || !info->IsColorFilterArray ())
		{
		
		fStage3Image.Reset (fStage2Image.Release ());
		
		}
		
	else
		{
		
		// Remember the size of the stage 2 image.
		
		dng_point stage2_size = fStage2Image->Size ();
		
		// Special case multi-channel CFA interpolation.
		
		if ((fStage2Image->Planes () > 1) && (srcPlane < 0))
			{
			
			DoMergeStage3 (host);
			
			}
			
		// Else do a single channel interpolation.
		
		else
			{
				
			DoInterpolateStage3 (host, srcPlane);
						   
			}
		
		// Calculate the ratio of the stage 3 image size to stage 2 image size.
		
		dng_point stage3_size = fStage3Image->Size ();
		
		fRawToFullScaleH = (real64) stage3_size.h / (real64) stage2_size.h;
		fRawToFullScaleV = (real64) stage3_size.v / (real64) stage2_size.v;
		
		}

	}
									   
/*****************************************************************************/

void dng_negative::BuildStage3Image (dng_host &host,
									 int32 srcPlane)
	{
	
	// Finalize the mosaic information.
	
	dng_mosaic_info *info = fMosaicInfo.Get ();
	
	if (info)
		{
		
		info->PostParse (host, *this);
		
		}
		
	// Do the interpolation as required.
	
	DoBuildStage3 (host, srcPlane);
	
	// Delete the stage2 image now that we have computed the stage 3 image.
	
	fStage2Image.Reset ();
	
	// Are we done with the mosaic info?
	
	if (fRawImageStage >= rawImageStagePreOpcode3)
		{
		
		ClearMosaicInfo ();
		
		// To support saving linear DNG files, to need to account for
		// and upscaling during interpolation.

		if (fRawToFullScaleH > 1.0)
			{
			
			uint32 adjust = Round_uint32 (fRawToFullScaleH);
			
			fDefaultCropSizeH  .n *= adjust;
			fDefaultCropOriginH.n *= adjust;
			fDefaultScaleH     .d *= adjust;
			
			fRawToFullScaleH /= (real64) adjust;
			
			}
		
		if (fRawToFullScaleV > 1.0)
			{
			
			uint32 adjust = Round_uint32 (fRawToFullScaleV);
			
			fDefaultCropSizeV  .n *= adjust;
			fDefaultCropOriginV.n *= adjust;
			fDefaultScaleV     .d *= adjust;
			
			fRawToFullScaleV /= (real64) adjust;
			
			}

		}
		
	// Resample the transparency mask if required.
	
	ResizeTransparencyToMatchStage3 (host);
			
	// Grab clone of raw image if required.
	
	if (fRawImageStage == rawImageStagePreOpcode3)
		{
		
		fRawImage.Reset (fStage3Image->Clone ());
		
		if (fTransparencyMask.Get ())
			{
			fRawTransparencyMask.Reset (fTransparencyMask->Clone ());
			}

		}
		
	// Process opcode list 3.
	
	host.ApplyOpcodeList (fOpcodeList3, *this, fStage3Image);
	
	// See if we are done with the opcode list 3.
	
	if (fRawImageStage > rawImageStagePreOpcode3)
		{
		
		fOpcodeList3.Clear ();
		
		}
		
	// Just in case the opcode list 3 changed the image size, resample the
	// transparency mask again if required.  This is nearly always going
	// to be a fast NOP operation.
	
	ResizeTransparencyToMatchStage3 (host);
 
	// Don't need to grab a copy of raw data at this stage since
	// it is kept around as the stage 3 image.
	
	}
		
/******************************************************************************/

class dng_gamma_encode_proxy : public dng_1d_function
	{
	
	private:
	
		real64 fBlack;
		real64 fWhite;
		
		bool fIsSceneReferred;
		
		real64 scale;
		real64 t1;
		
	public:
		
		dng_gamma_encode_proxy (real64 black,
							    real64 white,
							    bool isSceneReferred)
							   
			:	fBlack (black)
			,	fWhite (white)
			,	fIsSceneReferred (isSceneReferred)
			
			,	scale (1.0 / (fWhite - fBlack))
			,	t1 (1.0 / (27.0 * pow (5.0, 3.0 / 2.0)))
			
			{
			}
			
		virtual real64 Evaluate (real64 x) const
			{
			
			x = Pin_real64 (0.0, (x - fBlack) * scale, 1.0);
			
			real64 y;
			
			if (fIsSceneReferred)
				{
			
				real64 t = pow (sqrt (25920.0 * x * x + 1.0) * t1 + x * (8.0 / 15.0), 1.0 / 3.0);
				
				y = t - 1.0 / (45.0 * t);
				
				DNG_ASSERT (Abs_real64 (x - (y / 16.0 + y * y * y * 15.0 / 16.0)) < 0.0000001,
							"Round trip error");
							
				}
				
			else
				{
				
				y = (sqrt (960.0 * x + 1.0) - 1.0) / 30.0;
				
				DNG_ASSERT (Abs_real64 (x - (y / 16.0 + y * y * (15.0 / 16.0))) < 0.0000001,
							"Round trip error");
							
				}
				
			return y;
			
			}
	
	};

/*****************************************************************************/

class dng_encode_proxy_task: public dng_area_task
	{
	
	private:
	
		const dng_image &fSrcImage;
		
		dng_image &fDstImage;
		
		AutoPtr<dng_memory_block> fTable16 [kMaxColorPlanes];
			
	public:
	
		dng_encode_proxy_task (dng_host &host,
							   const dng_image &srcImage,
							   dng_image &dstImage,
							   const real64 *black,
							   const real64 *white,
							   bool isSceneReferred);
							 
		virtual dng_rect RepeatingTile1 () const
			{
			return fSrcImage.RepeatingTile ();
			}
			
		virtual dng_rect RepeatingTile2 () const
			{
			return fDstImage.RepeatingTile ();
			}
			
		virtual void Process (uint32 threadIndex,
							  const dng_rect &tile,
							  dng_abort_sniffer *sniffer);
								  
	private:
	
		// Hidden copy constructor and assignment operator.
	
		dng_encode_proxy_task (const dng_encode_proxy_task &task);
		
		dng_encode_proxy_task & operator= (const dng_encode_proxy_task &task);
	
	};

/*****************************************************************************/

dng_encode_proxy_task::dng_encode_proxy_task (dng_host &host,
											  const dng_image &srcImage,
										      dng_image &dstImage,
										      const real64 *black,
										      const real64 *white,
										      bool isSceneReferred)
										
	:	fSrcImage (srcImage)
	,	fDstImage (dstImage)
	
	{
	
	for (uint32 plane = 0; plane < fSrcImage.Planes (); plane++)
		{
		
		dng_gamma_encode_proxy gamma (black [plane],
									  white [plane],
									  isSceneReferred);
									  
		dng_1d_table table32;
		
		table32.Initialize (host.Allocator (), gamma);
		
		fTable16 [plane] . Reset (host.Allocate (0x10000 * sizeof (uint16)));
		
		table32.Expand16 (fTable16 [plane]->Buffer_uint16 ());
										 
		}
		
	}

/*****************************************************************************/

void dng_encode_proxy_task::Process (uint32 /* threadIndex */,
								     const dng_rect &tile,
							  	     dng_abort_sniffer * /* sniffer */)
	{
	
	dng_const_tile_buffer srcBuffer (fSrcImage, tile);
	dng_dirty_tile_buffer dstBuffer (fDstImage, tile);
	
	int32 sColStep = srcBuffer.fColStep;
	int32 dColStep = dstBuffer.fColStep;
	
	const uint16 *noise = dng_dither::Get ().NoiseBuffer16 ();
	
	for (uint32 plane = 0; plane < fSrcImage.Planes (); plane++)
		{
		
		const uint16 *map = fTable16 [plane]->Buffer_uint16 ();
		
		for (int32 row = tile.t; row < tile.b; row++)
			{
			
			const uint16 *sPtr = srcBuffer.ConstPixel_uint16 (row, tile.l, plane);
			
			uint8 *dPtr = dstBuffer.DirtyPixel_uint8 (row, tile.l, plane);
			
			const uint16 *rPtr = &noise [(row & dng_dither::kRNGMask) * dng_dither::kRNGSize];

			for (int32 col = tile.l; col < tile.r; col++)
				{
				
				uint32 x = *sPtr;
				
				uint32 r = rPtr [col & dng_dither::kRNGMask];
				
				x = map [x];
				
				x = (((x << 8) - x) + r) >> 16;
				
				*dPtr = (uint8) x;
				
				sPtr += sColStep;
				dPtr += dColStep;
				
				}
			
			}
			
		}
		
	}
								  
/******************************************************************************/

dng_image * dng_negative::EncodeRawProxy (dng_host &host,
										  const dng_image &srcImage,
										  dng_opcode_list &opcodeList) const
	{
	
	if (srcImage.PixelType () != ttShort)
		{
		return NULL;
		}
		
	real64 black [kMaxColorPlanes];
	real64 white [kMaxColorPlanes];
	
	bool isSceneReferred = (ColorimetricReference () == crSceneReferred);
	
		{
		
		const real64 kClipFraction = 0.00001;
	
		uint64 pixels = (uint64) srcImage.Bounds ().H () *
						(uint64) srcImage.Bounds ().W ();
						
		uint32 limit = Round_int32 ((real64) pixels * kClipFraction);
		
		AutoPtr<dng_memory_block> histData (host.Allocate (65536 * sizeof (uint32)));
		
		uint32 *hist = histData->Buffer_uint32 ();
			
		for (uint32 plane = 0; plane < srcImage.Planes (); plane++)
			{
			
			HistogramArea (host,
						   srcImage,
						   srcImage.Bounds (),
						   hist,
						   65535,
						   plane);
						   
			uint32 total = 0;

			uint32 upper = 65535;

			while (total + hist [upper] <= limit && upper > 255)
				{
				
				total += hist [upper];
				
				upper--;
				
				}
	
			total = 0;
			
			uint32 lower = 0;
			
			while (total + hist [lower] <= limit && lower < upper - 255)
				{
				
				total += hist [lower];
				
				lower++;
				
				}
			
			black [plane] = lower / 65535.0;
			white [plane] = upper / 65535.0;
		
			}
			
		}
		
	// Apply the gamma encoding, using dither when downsampling to 8-bit.
	
	AutoPtr<dng_image> dstImage (host.Make_dng_image (srcImage.Bounds (),
													  srcImage.Planes (),
													  ttByte));

		{
		
		dng_encode_proxy_task task (host,
							        srcImage,
									*dstImage,
									black,
									white,
									isSceneReferred);
		
		host.PerformAreaTask (task,
							  srcImage.Bounds ());
	
		}
				  
	// Add opcodes to undo the gamma encoding.
	
		{
	
		for (uint32 plane = 0; plane < srcImage.Planes (); plane++)
			{
			
			dng_area_spec areaSpec (srcImage.Bounds (),
									plane);
			
			real64 coefficient [4];
			
			coefficient [0] = 0.0;
			coefficient [1] = 1.0 / 16.0;
			
			if (isSceneReferred)
				{
				coefficient [2] = 0.0;
				coefficient [3] = 15.0 / 16.0;
				}
			else
				{
				coefficient [2] = 15.0 / 16.0;
				coefficient [3] = 0.0;
				}
			
			coefficient [0] *= white [plane] - black [plane];
			coefficient [1] *= white [plane] - black [plane];
			coefficient [2] *= white [plane] - black [plane];
			coefficient [3] *= white [plane] - black [plane];
			
			coefficient [0] += black [plane];
			
			AutoPtr<dng_opcode> opcode (new dng_opcode_MapPolynomial (areaSpec,
																	  isSceneReferred ? 3 : 2,
																	  coefficient));
																	  
			opcodeList.Append (opcode);
			
			}
			
		}
		
	return dstImage.Release ();
	
	}
									  
/******************************************************************************/

void dng_negative::AdjustProfileForStage3 ()
	{

	// For dng_sdk, the stage3 image's color space is always the same as the
	// raw image's color space.
	
	}
									  
/******************************************************************************/

void dng_negative::ConvertToProxy (dng_host &host,
								   dng_image_writer &writer,
								   uint32 proxySize,
								   uint64 proxyCount)
	{
	
	if (!proxySize)
		{
		proxySize = kMaxImageSide;
		}
	
	if (!proxyCount)
		{
		proxyCount = (uint64) proxySize * proxySize;
		}
	
	// Don't need to private data around in non-full size proxies.
	
	if (proxySize  < kMaxImageSide ||
		proxyCount < kMaxImageSide * kMaxImageSide)
		{
	
		ClearMakerNote ();
		
		ClearPrivateData ();
		
		}
	
	// See if we already have an acceptable proxy image.
	
	if (fRawImage.Get () &&
		fRawImage->PixelType () == ttByte &&
		fRawImage->Bounds () == DefaultCropArea () &&
		fRawImage->Bounds ().H () <= proxySize &&
		fRawImage->Bounds ().W () <= proxySize &&
		(uint64) fRawImage->Bounds ().H () *
		(uint64) fRawImage->Bounds ().W () <= proxyCount &&
		(!GetMosaicInfo () || !GetMosaicInfo ()->IsColorFilterArray ()) &&
		fRawJPEGImage.Get () &&
		(!RawTransparencyMask () || RawTransparencyMask ()->PixelType () == ttByte))
		{
		
		return;
		
		}
		
	if (fRawImage.Get () &&
		fRawImage->PixelType () == ttFloat &&
		fRawImage->Bounds ().H () <= proxySize &&
		fRawImage->Bounds ().W () <= proxySize &&
		(uint64) fRawImage->Bounds ().H () *
		(uint64) fRawImage->Bounds ().W () <= proxyCount &&
		RawFloatBitDepth () == 16 &&
		(!RawTransparencyMask () || RawTransparencyMask ()->PixelType () == ttByte))
		{
		
		return;
		
		}
	
	// Clear any grabbed raw image, since we are going to start
	// building the proxy with the stage3 image.

	fRawImage.Reset ();
	
	ClearRawJPEGImage ();
	
	SetRawFloatBitDepth (0);
	
	ClearLinearizationInfo ();
	
	ClearMosaicInfo ();
	
	fOpcodeList1.Clear ();
	fOpcodeList2.Clear ();
	fOpcodeList3.Clear ();
	
	// Adjust the profile to match the stage 3 image, if required.
	
	AdjustProfileForStage3 ();
	
	// Not saving the raw-most image, do the old raw digest is no
	// longer valid.
		
	ClearRawImageDigest ();
	
	ClearRawJPEGImageDigest ();
	
	// Trim off extra pixels outside the default crop area.
	
	dng_rect defaultCropArea = DefaultCropArea ();
	
	if (Stage3Image ()->Bounds () != defaultCropArea)
		{
		
		fStage3Image->Trim (defaultCropArea);
		
		if (fTransparencyMask.Get ())
			{
			fTransparencyMask->Trim (defaultCropArea);
			}
		
		fDefaultCropOriginH = dng_urational (0, 1);
		fDefaultCropOriginV = dng_urational (0, 1);
		
		}
		
	// Figure out the requested proxy pixel size.
	
	real64 aspectRatio = AspectRatio ();
	
	dng_point newSize (proxySize, proxySize);
	
	if (aspectRatio >= 1.0)
		{
		newSize.v = Max_int32 (1, Round_int32 (proxySize / aspectRatio));
		}
	else
		{
		newSize.h = Max_int32 (1, Round_int32 (proxySize * aspectRatio));
		}
		
	newSize.v = Min_int32 (newSize.v, DefaultFinalHeight ());
	newSize.h = Min_int32 (newSize.h, DefaultFinalWidth  ());
	
	if ((uint64) newSize.v *
	    (uint64) newSize.h > proxyCount)
		{

		if (aspectRatio >= 1.0)
			{
			
			newSize.h = (uint32) sqrt (proxyCount * aspectRatio);
			
			newSize.v = Max_int32 (1, Round_int32 (newSize.h / aspectRatio));
			
			}
			
		else
			{
			
			newSize.v = (uint32) sqrt (proxyCount / aspectRatio);
			
			newSize.h = Max_int32 (1, Round_int32 (newSize.v * aspectRatio));
												   
			}
															   
		}
		
	// If this is fewer pixels, downsample the stage 3 image to that size.
	
	dng_point oldSize = defaultCropArea.Size ();
	
	if ((uint64) newSize.v * (uint64) newSize.h <
		(uint64) oldSize.v * (uint64) oldSize.h)
		{
		
		const dng_image &srcImage (*Stage3Image ());
		
		AutoPtr<dng_image> dstImage (host.Make_dng_image (newSize,
														  srcImage.Planes (),
														  srcImage.PixelType ()));
														  
		host.ResampleImage (srcImage,
							*dstImage);
														 
		fStage3Image.Reset (dstImage.Release ());
		
		fDefaultCropSizeH = dng_urational (newSize.h, 1);
		fDefaultCropSizeV = dng_urational (newSize.v, 1);
		
		fDefaultScaleH = dng_urational (1, 1);
		fDefaultScaleV = dng_urational (1, 1);
		
		fBestQualityScale = dng_urational (1, 1);
		
		fRawToFullScaleH = 1.0;
		fRawToFullScaleV = 1.0;
		
		}
		
	// Convert 32-bit floating point images to 16-bit floating point to
	// save space.
	
	if (Stage3Image ()->PixelType () == ttFloat)
		{
		
		fRawImage.Reset (host.Make_dng_image (Stage3Image ()->Bounds (),
											  Stage3Image ()->Planes (),
											  ttFloat));
		
		LimitFloatBitDepth (host,
							*Stage3Image (),
							*fRawImage,
							16,
							32768.0f);
		
		SetRawFloatBitDepth (16);
		
		SetWhiteLevel (32768);
		
		}
		
	else
		{
		
		// Convert 16-bit deep images to 8-bit deep image for saving.
		
		fRawImage.Reset (EncodeRawProxy (host,
										 *Stage3Image (),
										 fOpcodeList2));
										 
		if (fRawImage.Get ())
			{
			
			SetWhiteLevel (255);
			
			// Compute JPEG compressed version.
			
			if (fRawImage->PixelType () == ttByte &&
				host.SaveDNGVersion () >= dngVersion_1_4_0_0)
				{
			
				AutoPtr<dng_jpeg_image> jpegImage (new dng_jpeg_image);
				
				jpegImage->Encode (host,
								   *this,
								   writer,
								   *fRawImage);
								   
				SetRawJPEGImage (jpegImage);
								   
				}
			
			}
			
		}
		
	// Deal with transparency mask.
	
	if (TransparencyMask ())
		{
		
		const bool convertTo8Bit = true;
		
		ResizeTransparencyToMatchStage3 (host, convertTo8Bit);
		
		fRawTransparencyMask.Reset (fTransparencyMask->Clone ());
		
		}
		
	// Recompute the raw data unique ID, since we changed the image data.
	
	RecomputeRawDataUniqueID (host);
			
	}

/*****************************************************************************/

dng_linearization_info * dng_negative::MakeLinearizationInfo ()
	{
	
	dng_linearization_info *info = new dng_linearization_info ();
	
	if (!info)
		{
		ThrowMemoryFull ();
		}
		
	return info;
	
	}

/*****************************************************************************/

void dng_negative::NeedLinearizationInfo ()
	{
	
	if (!fLinearizationInfo.Get ())
		{
	
		fLinearizationInfo.Reset (MakeLinearizationInfo ());
		
		}
	
	}

/*****************************************************************************/

dng_mosaic_info * dng_negative::MakeMosaicInfo ()
	{
	
	dng_mosaic_info *info = new dng_mosaic_info ();
	
	if (!info)
		{
		ThrowMemoryFull ();
		}
		
	return info;
	
	}

/*****************************************************************************/

void dng_negative::NeedMosaicInfo ()
	{
	
	if (!fMosaicInfo.Get ())
		{
	
		fMosaicInfo.Reset (MakeMosaicInfo ());
		
		}
	
	}

/*****************************************************************************/

void dng_negative::SetTransparencyMask (AutoPtr<dng_image> &image,
										uint32 bitDepth)
	{
	
	fTransparencyMask.Reset (image.Release ());
	
	fRawTransparencyMaskBitDepth = bitDepth;
	
	}

/*****************************************************************************/

const dng_image * dng_negative::TransparencyMask () const
	{
	
	return fTransparencyMask.Get ();
	
	}

/*****************************************************************************/

const dng_image * dng_negative::RawTransparencyMask () const
	{
	
	if (fRawTransparencyMask.Get ())
		{
		
		return fRawTransparencyMask.Get ();
		
		}
		
	return TransparencyMask ();
	
	}

/*****************************************************************************/

uint32 dng_negative::RawTransparencyMaskBitDepth () const
	{
	
	if (fRawTransparencyMaskBitDepth)
		{
	
		return fRawTransparencyMaskBitDepth;
		
		}
		
	const dng_image *mask = RawTransparencyMask ();
	
	if (mask)
		{
		
		switch (mask->PixelType ())
			{
			
			case ttByte:
				return 8;
				
			case ttShort:
				return 16;
				
			case ttFloat:
				return 32;
				
			default:
				ThrowProgramError ();
				
			}
		
		}
		
	return 0;
	
	}
									  
/*****************************************************************************/

void dng_negative::ReadTransparencyMask (dng_host &host,
									     dng_stream &stream,
									     dng_info &info)
	{
	
	if (info.fMaskIndex != -1)
		{
	
		// Allocate image we are reading.
		
		dng_ifd &maskIFD = *info.fIFD [info.fMaskIndex].Get ();
		
		fTransparencyMask.Reset (host.Make_dng_image (maskIFD.Bounds (),
													  1,
													  maskIFD.PixelType ()));
						
		// Read the image.
		
		maskIFD.ReadImage (host,
						   stream,
						   *fTransparencyMask.Get ());
						   
		// Remember the pixel depth.
		
		fRawTransparencyMaskBitDepth = maskIFD.fBitsPerSample [0];
						   
		}

	}

/*****************************************************************************/

void dng_negative::ResizeTransparencyToMatchStage3 (dng_host &host,
													bool convertTo8Bit)
	{
	
	if (TransparencyMask ())
		{
		
		if ((TransparencyMask ()->Bounds () != fStage3Image->Bounds ()) ||
			(TransparencyMask ()->PixelType () != ttByte && convertTo8Bit))
			{
			
			AutoPtr<dng_image> newMask (host.Make_dng_image (fStage3Image->Bounds (),
															 1,
															 convertTo8Bit ?
															 ttByte :
															 TransparencyMask ()->PixelType ()));
									
			host.ResampleImage (*TransparencyMask (),
								*newMask);
						   
			fTransparencyMask.Reset (newMask.Release ());
			
			if (!fRawTransparencyMask.Get ())
				{
				fRawTransparencyMaskBitDepth = 0;
				}
			
			}
			
		}
		
	}

/*****************************************************************************/

bool dng_negative::NeedFlattenTransparency (dng_host & /* host */)
	{
	
	if (TransparencyMask ())
		{
		
		return true;
	
		}
	
	return false;
		
	}
									  
/*****************************************************************************/

void dng_negative::FlattenTransparency (dng_host & /* host */)
	{
	
	ThrowNotYetImplemented ();
	
	}
									  
/*****************************************************************************/

const dng_image * dng_negative::UnflattenedStage3Image () const
	{
	
	if (fUnflattenedStage3Image.Get ())
		{
		
		return fUnflattenedStage3Image.Get ();
		
		}
		
	return fStage3Image.Get ();
		
	}

/*****************************************************************************/
