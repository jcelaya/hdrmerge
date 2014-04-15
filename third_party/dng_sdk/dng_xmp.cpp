/*****************************************************************************/
// Copyright 2006-2008 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in
// accordance with the terms of the Adobe license agreement accompanying it.
/*****************************************************************************/

/* $Id: //mondo/dng_sdk_1_4/dng_sdk/source/dng_xmp.cpp#1 $ */ 
/* $DateTime: 2012/05/30 13:28:51 $ */
/* $Change: 832332 $ */
/* $Author: tknoll $ */

/*****************************************************************************/

#include "dng_xmp.h"

#include "dng_assertions.h"
#include "dng_date_time.h"
#include "dng_exceptions.h"
#include "dng_exif.h"
#include "dng_image_writer.h"
#include "dng_iptc.h"
#include "dng_negative.h"
#include "dng_string.h"
#include "dng_string_list.h"
#include "dng_utils.h"
#include "dng_xmp_sdk.h"

/*****************************************************************************/

dng_xmp::dng_xmp (dng_memory_allocator &allocator)

	:	fAllocator (allocator)
	
	,	fSDK (NULL)
	
	{
	
	fSDK = new dng_xmp_sdk ();
	
	if (!fSDK)
		{
		ThrowMemoryFull ();
		}
			
	}

/*****************************************************************************/

dng_xmp::dng_xmp (const dng_xmp &xmp)

	:	fAllocator (xmp.fAllocator)
	
	,	fSDK (NULL)
	
	{
	
	fSDK = new dng_xmp_sdk (*xmp.fSDK);
	
	if (!fSDK)
		{
		ThrowMemoryFull ();
		}
			
	}

/*****************************************************************************/

dng_xmp::~dng_xmp ()
	{
	
	if (fSDK)
		{
		
		delete fSDK;
		
		}
	
	}

/*****************************************************************************/

dng_xmp * dng_xmp::Clone () const
	{
	
	dng_xmp *result = new dng_xmp (*this);
	
	if (!result)
		{
		ThrowMemoryFull ();
		}
	
	return result;
	
	}

/*****************************************************************************/

void dng_xmp::TrimDecimal (char *s)
	{
	
	uint32 len = (uint32) strlen (s);
	
	while (len > 0)
		{
		
		if (s [len - 1] == '0')
			s [--len] = 0;
			
		else
			break;
			
		}
	
	if (len > 0)
		{
		
		if (s [len - 1] == '.')
			s [--len] = 0;
			
		}

	}
	
/*****************************************************************************/

dng_string dng_xmp::EncodeFingerprint (const dng_fingerprint &f,
									   bool allowInvalid)
	{
	
	dng_string result;
	
	if (f.IsValid () || allowInvalid)
		{
		
		char s [dng_fingerprint::kDNGFingerprintSize * 2 + 1];
		
		f.ToUtf8HexString (s);
			
		result.Set (s);
		
		}
	
	return result;
	
	}

/*****************************************************************************/

dng_fingerprint dng_xmp::DecodeFingerprint (const dng_string &s)
	{
	
	dng_fingerprint result;
	
	if (s.Length () == 32)
		result.FromUtf8HexString (s.Get ());
	
	return result;
	
	}

/*****************************************************************************/

dng_string dng_xmp::EncodeGPSVersion (uint32 version)
	{
	
	dng_string result;
	
	if (version)
		{
		
		uint8 b0 = (uint8) (version >> 24);
		uint8 b1 = (uint8) (version >> 16);
		uint8 b2 = (uint8) (version >>  8);
		uint8 b3 = (uint8) (version      );
		
		if (b0 <= 9 && b1 <= 9 && b2 <= 9 && b3 <= 9)
			{
			
   			char s [32];

			sprintf (s,
					 "%u.%u.%u.%u",
					 (unsigned) b0,
					 (unsigned) b1,
					 (unsigned) b2, 
					 (unsigned) b3);
					 
			result.Set (s);
			
			}
		
		}
	
	return result;
	
	}
		
/*****************************************************************************/

uint32 dng_xmp::DecodeGPSVersion (const dng_string &s)
	{
	
	uint32 result = 0;
	
	if (s.Length () == 7)
		{
		
		unsigned b0 = 0;
		unsigned b1 = 0;
		unsigned b2 = 0;
		unsigned b3 = 0;
		
		if (sscanf (s.Get (),
					"%u.%u.%u.%u",
					&b0,
					&b1,
					&b2,
					&b3) == 4)
			{
			
			result = (b0 << 24) |
					 (b1 << 16) |
					 (b2 <<  8) |
					 (b3      );
			
			}
		
		}
		
	return result;
	
	}
							   				   
/*****************************************************************************/

dng_string dng_xmp::EncodeGPSCoordinate (const dng_string &ref,
							    		 const dng_urational *coord)
	{
	
	dng_string result;
	
	if (ref.Length () == 1 && coord [0].IsValid () &&
							  coord [1].IsValid ())
		{
		
		char refChar = ForceUppercase (ref.Get () [0]);
		
		if (refChar == 'N' ||
			refChar == 'S' ||
			refChar == 'E' ||
			refChar == 'W')
			{
			
			char s [256];
			
			// Use the seconds case if all three values are
			// integers.
			
			if (coord [0].d == 1 &&
				coord [1].d == 1 &&
				coord [2].d == 1)
				{
								
				sprintf (s,
						 "%u,%u,%u%c",
						 (unsigned) coord [0].n,
						 (unsigned) coord [1].n,
						 (unsigned) coord [2].n,
						 refChar);
			
				}
				
			// Else we need to use the fractional minutes case.
				
			else
				{
				
				// Find value minutes.
				
				real64 x = coord [0].As_real64 () * 60.0 +
						   coord [1].As_real64 () +
						   coord [2].As_real64 () * (1.0 / 60.0);
						   
				// Round to fractional four decimal places.
				
				uint32 y = Round_uint32 (x * 10000.0);
				
				// Split into degrees and minutes.
				
				uint32 d = y / (60 * 10000);
				uint32 m = y % (60 * 10000);
				
				char min [32];
				
				sprintf (min, "%.4f", m * (1.0 / 10000.0));

				TrimDecimal (min);
				
				sprintf (s,
						 "%u,%s%c",
						 (unsigned) d,
						 min,
						 refChar);

				}
				
			result.Set (s);
				
			}
				
		}
	
	return result;
	
	}
		
/*****************************************************************************/

void dng_xmp::DecodeGPSCoordinate (const dng_string &s,
								   dng_string &ref,
								   dng_urational *coord)
	{
	
	ref.Clear ();
	
	coord [0].Clear ();
	coord [1].Clear ();
	coord [2].Clear ();
	
	if (s.Length () > 1)
		{
		
		char refChar = ForceUppercase (s.Get () [s.Length () - 1]);
		
		if (refChar == 'N' ||
			refChar == 'S' ||
			refChar == 'E' ||
			refChar == 'W')
			{
			
			dng_string ss (s);
			
			ss.Truncate (ss.Length () - 1);
			
			ss.NormalizeAsCommaSeparatedNumbers();
				
			int degrees = 0;
			
			real64 minutes = 0.0;
			real64 seconds = 0.0;
			
			int count = sscanf (ss.Get (),
								"%d,%lf,%lf",
								&degrees,
								&minutes,
								&seconds);
								
			if (count < 1)
				{
				return;
				}
				
			// The degree, minute, second values should always be positive.
			
			if (degrees < 0 || minutes < 0 || seconds < 0)
				{
				return;
				}
				
			coord [0] = dng_urational ((uint32) degrees, 1);
			
			if (count <= 2)
				{
				coord [1].Set_real64 (minutes, 10000);
				coord [2] = dng_urational (0, 1);
				}
			else
				{
				coord [1].Set_real64 (minutes, 1);
				coord [2].Set_real64 (seconds, 100);
				}
				
			char r [2];
			
			r [0] = refChar;
			r [1] = 0;
			
			ref.Set (r);
				
			}
		
		}
	
	}
											 
/*****************************************************************************/

dng_string dng_xmp::EncodeGPSDateTime (const dng_string &dateStamp,
									   const dng_urational *timeStamp)
	{
	
	dng_string result;
	
	if (timeStamp [0].IsValid () &&
		timeStamp [1].IsValid () &&
		timeStamp [2].IsValid ())
		{
  
 		char s [256];
 		
		char sec [32];
		
		sprintf (sec,
				 "%09.6f",
				 timeStamp [2].As_real64 ());
		
		TrimDecimal (sec);
		
		int year  = 0;
		int month = 0;
		int day   = 0;
		
		if (dateStamp.NotEmpty ())
			{
			
			sscanf (dateStamp.Get (), 
				    "%d:%d:%d",
				    &year,
				    &month,
				    &day);
			
			}
			
		if (year  >= 1 && year  <= 9999 &&
			month >= 1 && month <=   12 &&
			day   >= 1 && day   <=   31)
			{
			
			sprintf (s,
					 "%04d-%02d-%02dT%02u:%02u:%sZ",
					 year,
					 month,
					 day,
					 (unsigned) Round_uint32 (timeStamp [0].As_real64 ()),
					 (unsigned) Round_uint32 (timeStamp [1].As_real64 ()),
					 sec);
					 
			}
			
		else
			{
			
			sprintf (s,
					 "%02u:%02u:%sZ",
					 (unsigned) Round_uint32 (timeStamp [0].As_real64 ()),
					 (unsigned) Round_uint32 (timeStamp [1].As_real64 ()),
					 sec);
					 
			}
		
		result.Set (s);
		
		}
	
	return result;
	
	}
		
/*****************************************************************************/

void dng_xmp::DecodeGPSDateTime (const dng_string &s,
								 dng_string &dateStamp,
								 dng_urational *timeStamp)
	{
	
	dateStamp.Clear ();
	
	timeStamp [0].Clear ();
	timeStamp [1].Clear ();
	timeStamp [2].Clear ();
	
	if (s.NotEmpty ())
		{
		
		unsigned year   = 0;
		unsigned month  = 0;
		unsigned day    = 0;
		unsigned hour   = 0;
		unsigned minute = 0;
		
		double second = 0.0;
		
		if (sscanf (s.Get (),
					"%u-%u-%uT%u:%u:%lf",
					&year,
					&month,
					&day,
					&hour,
					&minute,
					&second) == 6)
			{
			
			if (year  >= 1 && year  <= 9999 &&
				month >= 1 && month <= 12   &&
				day   >= 1 && day   <= 31   )
				{
				
				char ss [64];
				
				sprintf (ss,
						 "%04u:%02u:%02u",
						 year,
						 month,
						 day);
						 
				dateStamp.Set (ss);
				
				}
			
			}
			
		else if (sscanf (s.Get (),
						 "%u:%u:%lf",
						 &hour,
				 		 &minute,
				 		 &second) != 3)
			{
			
			return;
			
			}
			
		timeStamp [0] = dng_urational ((uint32) hour  , 1);
		timeStamp [1] = dng_urational ((uint32) minute, 1);
		
		timeStamp [2].Set_real64 (second, 1000);
		
		}
	
	}
		
/*****************************************************************************/

void dng_xmp::Parse (dng_host &host,
					 const void *buffer,
				     uint32 count)
	{
	
	fSDK->Parse (host,
				 (const char *) buffer,
				 count);
	
	}
	
/*****************************************************************************/

dng_memory_block * dng_xmp::Serialize (bool asPacket,
									   uint32 targetBytes,
									   uint32 padBytes,
									   bool forJPEG,
									   bool compact) const
	{
	
	return fSDK->Serialize (fAllocator,
							asPacket,
							targetBytes,
							padBytes,
							forJPEG,
							compact);
	
	}
	
/*****************************************************************************/

void dng_xmp::PackageForJPEG (AutoPtr<dng_memory_block> &stdBlock,
							  AutoPtr<dng_memory_block> &extBlock,
							  dng_string &extDigest) const
	{
	
	fSDK->PackageForJPEG (fAllocator,
						  stdBlock,
						  extBlock,
						  extDigest);
	
	}
	
/*****************************************************************************/

void dng_xmp::MergeFromJPEG (const dng_xmp &xmp)
	{
	
	fSDK->MergeFromJPEG (xmp.fSDK);
	
	}

/*****************************************************************************/

bool dng_xmp::HasMeta () const
	{
	
	return fSDK->HasMeta ();
	
	}

/*****************************************************************************/

void * dng_xmp::GetPrivateMeta ()
	{
	
	return fSDK->GetPrivateMeta ();
	
	}

/*****************************************************************************/

bool dng_xmp::Exists (const char *ns,
					  const char *path) const
	{
 
	return fSDK->Exists (ns, path);
 
	}

/*****************************************************************************/

bool dng_xmp::HasNameSpace (const char *ns) const
	{
 
	return fSDK->HasNameSpace (ns);
 
	}

/*****************************************************************************/

bool dng_xmp::IteratePaths (IteratePathsCallback *callback,
						    void *callbackData,
							const char *ns,
							const char *path)
	{
	
	return fSDK->IteratePaths (callback, callbackData, ns, path);
	
	}
						   
/*****************************************************************************/

void dng_xmp::Remove (const char *ns,
				      const char *path)
	{
	
	fSDK->Remove (ns, path);
	
	}
	
/*****************************************************************************/

void dng_xmp::RemoveProperties (const char *ns)
	{
	
	fSDK->RemoveProperties (ns);
	
	}

/*****************************************************************************/

void dng_xmp::RemoveEmptyStringOrArray (const char *ns,
								        const char *path)
	{
	
	if (path == NULL || path [0] == 0)
		{
		return;
		}
	
	if (fSDK->IsEmptyString (ns, path) ||
		fSDK->IsEmptyArray  (ns, path))
		{
		
		Remove (ns, path);
		
		}
	
	}

/*****************************************************************************/

static bool RemoveEmptyStringsAndArraysCallback (const char *ns,
												 const char *path,
												 void *callbackData)
	{
	
	dng_xmp *xmp = (dng_xmp *) callbackData;
	
	xmp->RemoveEmptyStringOrArray (ns, path);
	
	return true;
	
	}

/*****************************************************************************/

void dng_xmp::RemoveEmptyStringsAndArrays (const char *ns)
	{
	
	IteratePaths (RemoveEmptyStringsAndArraysCallback,
				  (void *) this,
				  ns,
				  NULL);
	
	}
		
/*****************************************************************************/

void dng_xmp::Set (const char *ns,
				   const char *path,
				   const char *text)
	{
	
	fSDK->Set (ns, path, text);
	
	}
	
/*****************************************************************************/

bool dng_xmp::GetString (const char *ns,
						 const char *path,
						 dng_string &s) const
	{
	
	return fSDK->GetString (ns, path, s);
	
	}
		
/*****************************************************************************/

void dng_xmp::SetString (const char *ns,
						 const char *path,
						 const dng_string &s)
	{
	
	fSDK->SetString (ns, path, s);
	
	}
		
/*****************************************************************************/

bool dng_xmp::SyncString (const char *ns,
						  const char *path,
						  dng_string &s,
						  uint32 options)
	{
	
	bool isDefault = s.IsEmpty ();
	
	// Sync 1: Force XMP to match non-XMP.
	
	if (options & ignoreXMP)
		{
		
		if (isDefault || (options & removeXMP))
			{
			
			Remove (ns, path);
			
			}
			
		else
			{
			
			SetString (ns, path, s);
			
			}
		
		return false;
		
		}
	
	// Sync 2: From non-XMP to XMP if non-XMP is prefered.
	
	if ((options & preferNonXMP) && !isDefault)
		{
		
		if (options & removeXMP)
			{
			
			Remove (ns, path);
			
			}
			
		else
			{
		
			SetString (ns, path, s);
			
			}
				   
		return false;
		
		}
		
	// Sync 3: From XMP to non-XMP if XMP is prefered or default non-XMP.
		
	if ((options & preferXMP) || isDefault)
		{
		
		if (GetString (ns, path, s))
			{
			
			if (options & removeXMP)
				{
				
				Remove (ns, path);
				
				}

			return true;
						
			}
		
		}
		
	// Sync 4: From non-XMP to XMP.
	
	if (options & removeXMP)
		{
		
		Remove (ns, path);
		
		}
		
	else if (!isDefault)
		{
		
		SetString (ns, path, s);
		
		}
		
	return false;
	
	}

/*****************************************************************************/

bool dng_xmp::GetStringList (const char *ns,
						 	 const char *path,
						 	 dng_string_list &list) const
	{
	
	return fSDK->GetStringList (ns, path, list);
	
	}
		
/*****************************************************************************/

void dng_xmp::SetStringList (const char *ns,
						     const char *path,
						     const dng_string_list &list,
						     bool isBag)
	{
	
	fSDK->SetStringList (ns, path, list, isBag);
	
	}
		
/*****************************************************************************/

void dng_xmp::SyncStringList (const char *ns,
						      const char *path,
						      dng_string_list &list,
						      bool isBag,
						      uint32 options)
	{
	
	bool isDefault = (list.Count () == 0);
	
	// First make sure the XMP is not badly formatted, since
	// this breaks some Photoshop logic.
	
	ValidateStringList (ns, path);
	
	// Sync 1: Force XMP to match non-XMP.
	
	if (options & ignoreXMP)
		{
		
		if (isDefault)
			{
			
			Remove (ns, path);
			
			}
			
		else
			{
			
			SetStringList (ns, path, list, isBag);
			
			}
		
		return;
		
		}
	
	// Sync 2: From non-XMP to XMP if non-XMP is prefered.
	
	if ((options & preferNonXMP) && !isDefault)
		{
		
		SetStringList (ns, path, list, isBag);
				   
		return;
		
		}
		
	// Sync 3: From XMP to non-XMP if XMP is prefered or default non-XMP.
		
	if ((options & preferXMP) || isDefault)
		{
		
		if (GetStringList (ns, path, list))
			{
			
			return;
						
			}
			
		}
		
	// Sync 4: From non-XMP to XMP.
	
	if (!isDefault)
		{
		
		SetStringList (ns, path, list, isBag);
		
		}
		
	}

/*****************************************************************************/

void dng_xmp::SetStructField (const char *ns,
							  const char *path,
							  const char *fieldNS,
							  const char *fieldName,
							  const dng_string &s)
	{

	dng_string ss (s);
	
	ss.SetLineEndings ('\n');
	
	ss.StripLowASCII ();
	
	fSDK->SetStructField (ns, path, fieldNS, fieldName, ss.Get ());

	}

/*****************************************************************************/

void dng_xmp::SetStructField (const char *ns,
							  const char *path,
							  const char *fieldNS,
							  const char *fieldName,
							  const char *s)
	{

	fSDK->SetStructField (ns, path, fieldNS, fieldName, s);

	}

/*****************************************************************************/

void dng_xmp::DeleteStructField (const char *ns,
								 const char *path,
								 const char *fieldNS,
								 const char *fieldName)
	{
	
	fSDK->DeleteStructField (ns, path, fieldNS, fieldName);
	
	}
								
/*****************************************************************************/

bool dng_xmp::GetStructField (const char *ns,
							  const char *path,
							  const char *fieldNS,
							  const char *fieldName,
							  dng_string &s) const
	{
		
	return fSDK->GetStructField (ns, path, fieldNS, fieldName, s);

	}

/*****************************************************************************/

void dng_xmp::SetAltLangDefault (const char *ns,
								 const char *path,
								 const dng_string &s)
	{
		
	fSDK->SetAltLangDefault (ns, path, s);

	}

/*****************************************************************************/

bool dng_xmp::GetAltLangDefault (const char *ns,
								 const char *path,
								 dng_string &s) const
	{
			
	return fSDK->GetAltLangDefault (ns, path, s);

	}

/*****************************************************************************/

bool dng_xmp::SyncAltLangDefault (const char *ns,
								  const char *path,
								  dng_string &s,
								  uint32 options)
	{
	
	bool isDefault = s.IsEmpty ();
	
	// Sync 1: Force XMP to match non-XMP.
	
	if (options & ignoreXMP)
		{
		
		if (isDefault)
			{
			
			Remove (ns, path);
			
			}
			
		else
			{
			
			SetAltLangDefault (ns, path, s);
			
			}
		
		return false;
		
		}
	
	// Sync 2: From non-XMP to XMP if non-XMP is prefered.
	
	if ((options & preferNonXMP) && !isDefault)
		{
		
		SetAltLangDefault (ns, path, s);
				   
		return false;
		
		}
		
	// Sync 3: From XMP to non-XMP if XMP is prefered or default non-XMP.
		
	if ((options & preferXMP) || isDefault)
		{
		
		if (GetAltLangDefault (ns, path, s))
			{
							
			return true;
						
			}
		
		}
		
	// Sync 4: From non-XMP to XMP.
	
	if (!isDefault)
		{
		
		SetAltLangDefault (ns, path, s);
		
		}
		
	return false;
	
	}

/*****************************************************************************/

bool dng_xmp::GetBoolean (const char *ns,
					 	  const char *path,
					 	  bool &x) const
	{
	
	dng_string s;
	
	if (GetString (ns, path, s))
		{
	
		if (s.Matches ("True"))
			{
			
			x = true;
			
			return true;
			
			}
			
		if (s.Matches ("False"))
			{
			
			x = false;
			
			return true;
			
			}
			
		}
		
	return false;
	
	}
	
/*****************************************************************************/

void dng_xmp::SetBoolean (const char *ns,
					 	  const char *path,
					 	  bool x)
	{
	
	Set (ns, path, x ? "True" : "False");
	
	}
	
/*****************************************************************************/

bool dng_xmp::Get_int32 (const char *ns,
						 const char *path,
						 int32 &x) const
	{
	
	dng_string s;
	
	if (GetString (ns, path, s))
		{
	
		if (s.NotEmpty ())
			{
			
			int y = 0;
			
			if (sscanf (s.Get (), "%d", &y) == 1)
				{
				
				x = y;
				
				return true;
				
				}

			}
			
		}
		
	return false;
	
	}
	
/*****************************************************************************/

void dng_xmp::Set_int32 (const char *ns,
						 const char *path,
						 int32 x,
						 bool usePlus)
	{
	
	char s [64];
	
	if (x > 0 && usePlus)
		{
		sprintf (s, "+%d", (int) x);
		}
	else
		{
		sprintf (s, "%d", (int) x);
		}

	Set (ns, path, s);
	
	}
						 
/*****************************************************************************/

bool dng_xmp::Get_uint32 (const char *ns,
					 	  const char *path,
					 	  uint32 &x) const
	{
	
	dng_string s;
	
	if (GetString (ns, path, s))
		{
	
		if (s.NotEmpty ())
			{
			
			unsigned y = 0;
			
			if (sscanf (s.Get (), "%u", &y) == 1)
				{
				
				x = y;
				
				return true;
				
				}

			}
			
		}
		
	return false;
	
	}
	
/*****************************************************************************/

void dng_xmp::Set_uint32 (const char *ns,
						  const char *path,
						  uint32 x)
	{

	char s [64];
	
	sprintf (s,
			 "%u",
			 (unsigned) x);
	
	Set (ns, path, s);
		
	}
	
/*****************************************************************************/

void dng_xmp::Sync_uint32 (const char *ns,
						   const char *path,
						   uint32 &x,
						   bool isDefault,
						   uint32 options)
	{
	
	// Sync 1: Force XMP to match non-XMP.
	
	if (options & ignoreXMP)
		{
		
		if (isDefault || (options & removeXMP))
			{
			
			Remove (ns, path);
			
			}
			
		else
			{
			
			Set_uint32 (ns, path, x);
			
			}
		
		return;
		
		}
	
	// Sync 2: From non-XMP to XMP if non-XMP is prefered.
	
	if ((options & preferNonXMP) && !isDefault)
		{
		
		if (options & removeXMP)
			{
			
			Remove (ns, path);
			
			}
			
		else
			{
			
			Set_uint32 (ns, path, x);
			
			}
				   
		return;
		
		}
		
	// Sync 3: From XMP to non-XMP if XMP is prefered or default non-XMP.
		
	if ((options & preferXMP) || isDefault)
		{
		
		if (Get_uint32 (ns, path, x))
			{
							
			if (options & removeXMP)
				{
				
				Remove (ns, path);
				
				}

			return;
						
			}
		
		}
		
	// Sync 4: From non-XMP to XMP.
	
	if (options & removeXMP)
		{
		
		Remove (ns, path);
		
		}
		
	else if (!isDefault)
		{
		
		Set_uint32 (ns, path, x);
		
		}
	
	}
						 
/*****************************************************************************/

void dng_xmp::Sync_uint32_array (const char *ns,
						   		 const char *path,
						   		 uint32 *data,
						   		 uint32 &count,
						   		 uint32 maxCount,
						   		 uint32 options)
	{
	
	dng_string_list list;
	
	for (uint32 j = 0; j < count; j++)
		{
		
		char s [32];
		
		sprintf (s, "%u", (unsigned) data [j]);
		
		dng_string ss;
		
		ss.Set (s);
		
		list.Append (ss);
		
		}
		
	SyncStringList (ns,
					path,
					list,
					false,
					options);
					
	count = 0;
	
	for (uint32 k = 0; k < maxCount; k++)
		{
		
		data [k] = 0;
		
		if (k < list.Count ())
			{
			
			unsigned x = 0;
			
			if (sscanf (list [k].Get (), "%u", &x) == 1)
				{
				
				data [count++] = x;
				
				}
			
			}
		
		}
	
	}

/*****************************************************************************/

bool dng_xmp::Get_real64 (const char *ns,
					  	  const char *path,
					  	  real64 &x) const
	{
	
	dng_string s;
	
	if (GetString (ns, path, s))
		{
	
		if (s.NotEmpty ())
			{
			
			double y = 0;
			
			if (sscanf (s.Get (), "%lf", &y) == 1)
				{
				
				x = y;
				
				return true;
				
				}

			}
			
		}
		
	return false;
	
	}
	
/*****************************************************************************/

void dng_xmp::Set_real64 (const char *ns,
					  	  const char *path,
					  	  real64 x,
					      uint32 places,
					      bool trim,
					      bool usePlus)
	{
	
	char s [64];
	
	if (x > 0.0 && usePlus)
		{
		sprintf (s, "+%0.*f", (unsigned) places, (double) x);
		}
	else
		{
		sprintf (s, "%0.*f", (unsigned) places, (double) x);
		}
	
	if (trim)
		{
		
		while (s [strlen (s) - 1] == '0')
			{
			s [strlen (s) - 1] = 0;
			}
			
		if (s [strlen (s) - 1] == '.')
			{
			s [strlen (s) - 1] = 0;
			}
			
		}
	
	Set (ns, path, s);
	
	}

/*****************************************************************************/

bool dng_xmp::Get_urational (const char *ns,
							 const char *path,
							 dng_urational &r) const
	{
	
	dng_string s;
	
	if (GetString (ns, path, s))
		{
	
		if (s.NotEmpty ())
			{
			
			unsigned n = 0;
			unsigned d = 0;
			
			if (sscanf (s.Get (), "%u/%u", &n, &d) == 2)
				{
				
				if (d != 0)
					{
				
					r = dng_urational (n, d);
					
					return true;
					
					}
				
				}
					
			}
			
		}

	return false;

	}

/*****************************************************************************/

void dng_xmp::Set_urational (const char *ns,
							 const char *path,
							 const dng_urational &r)
	{

	char s [64];
	
	sprintf (s,
			 "%u/%u",
			 (unsigned) r.n,
			 (unsigned) r.d);
	
	Set (ns, path, s);
		
	}

/*****************************************************************************/

void dng_xmp::Sync_urational (const char *ns,
							  const char *path,
							  dng_urational &r,
							  uint32 options)
	{
	
	bool isDefault = r.NotValid ();
	
	// Sync 1: Force XMP to match non-XMP.
	
	if (options & ignoreXMP)
		{
		
		if (isDefault || (options & removeXMP))
			{
			
			Remove (ns, path);
			
			}
			
		else
			{
			
			Set_urational (ns, path, r);
			
			}
		
		return;
		
		}
	
	// Sync 2: From non-XMP to XMP if non-XMP is prefered.
	
	if ((options & preferNonXMP) && !isDefault)
		{
		
		if (options & removeXMP)
			{
			
			Remove (ns, path);
			
			}
			
		else
			{
			
			Set_urational (ns, path, r);
			
			}
				   
		return;
		
		}
		
	// Sync 3: From XMP to non-XMP if XMP is prefered or default non-XMP.
		
	if ((options & preferXMP) || isDefault)
		{
		
		if (Get_urational (ns, path, r))
			{
							
			if (options & removeXMP)
				{
				
				Remove (ns, path);
				
				}

			return;
						
			}
		
		}
		
	// Sync 4: From non-XMP to XMP.
	
	if (options & removeXMP)
		{
		
		Remove (ns, path);
		
		}
		
	else if (!isDefault)
		{
		
		Set_urational (ns, path, r);
		
		}
	
	}
		
/*****************************************************************************/

bool dng_xmp::Get_srational (const char *ns,
							 const char *path,
							 dng_srational &r) const
	{
	
	dng_string s;
	
	if (GetString (ns, path, s))
		{
	
		if (s.NotEmpty ())
			{
			
			int n = 0;
			int d = 0;
			
			if (sscanf (s.Get (), "%d/%d", &n, &d) == 2)
				{
				
				if (d != 0)
					{
				
					r = dng_srational (n, d);
					
					return true;
					
					}
				
				}
					
			}
			
		}

	return false;

	}

/*****************************************************************************/

void dng_xmp::Set_srational (const char *ns,
							 const char *path,
							 const dng_srational &r)
	{

	char s [64];
	
	sprintf (s,
			 "%d/%d",
			 (int) r.n,
			 (int) r.d);
	
	Set (ns, path, s);
		
	}

/*****************************************************************************/

void dng_xmp::Sync_srational (const char *ns,
							  const char *path,
							  dng_srational &r,
							  uint32 options)
	{
	
	bool isDefault = r.NotValid ();
	
	// Sync 1: Force XMP to match non-XMP.
	
	if (options & ignoreXMP)
		{
		
		if (isDefault || (options & removeXMP))
			{
			
			Remove (ns, path);
			
			}
			
		else
			{
			
			Set_srational (ns, path, r);
			
			}
		
		return;
		
		}
	
	// Sync 2: From non-XMP to XMP if non-XMP is prefered.
	
	if ((options & preferNonXMP) && !isDefault)
		{
		
		if (options & removeXMP)
			{
			
			Remove (ns, path);
			
			}
			
		else
			{
			
			Set_srational (ns, path, r);
			
			}
				   
		return;
		
		}
		
	// Sync 3: From XMP to non-XMP if XMP is prefered or default non-XMP.
		
	if ((options & preferXMP) || isDefault)
		{
		
		if (Get_srational (ns, path, r))
			{
							
			if (options & removeXMP)
				{
				
				Remove (ns, path);
				
				}

			return;
						
			}
		
		}
		
	// Sync 4: From non-XMP to XMP.
	
	if (options & removeXMP)
		{
		
		Remove (ns, path);
		
		}
		
	else if (!isDefault)
		{
		
		Set_srational (ns, path, r);
		
		}
	
	}
		
/*****************************************************************************/

bool dng_xmp::GetFingerprint (const char *ns,
					 		  const char *path,
					    	  dng_fingerprint &print) const
	{
	
	dng_string s;
	
	if (GetString (ns, path, s))
		{
		
		dng_fingerprint temp = DecodeFingerprint (s);
		
		if (temp.IsValid ())
			{
			
			print = temp;
			
			return true;
			
			}
		
		}
		
	return false;
	
	}
	
/******************************************************************************/

void dng_xmp::SetFingerprint (const char *ns,
							  const char *tag,
							  const dng_fingerprint &print,
							  bool allowInvalid)
	{
	
	dng_string s = EncodeFingerprint (print, allowInvalid);
	
	if (s.IsEmpty ())
		{
		
		Remove (ns, tag);
		
		}
		
	else
		{
		
		SetString (ns, tag, s);
		
		}
	
	}

/******************************************************************************/

void dng_xmp::SetVersion2to4 (const char *ns,
							  const char *path,
							  uint32 version)
	{
	
	char buf [32];

	if (version & 0x000000ff)
		{
	
		// x.x.x.x
	
		sprintf (buf,
				 "%u.%u.%u.%u",
				 (unsigned) ((version >> 24) & 0xff),
				 (unsigned) ((version >> 16) & 0xff),
				 (unsigned) ((version >>  8) & 0xff),
				 (unsigned) ((version	   ) & 0xff));
	
		}

	else if (version & 0x0000ff00)
		{
	
		// x.x.x
	
		sprintf (buf,
				 "%u.%u.%u",
				 (unsigned) ((version >> 24) & 0xff),
				 (unsigned) ((version >> 16) & 0xff),
				 (unsigned) ((version >>  8) & 0xff));
	
		}

	else
		{
	
		// x.x
	
		sprintf (buf,
				 "%u.%u",
				 (unsigned) ((version >> 24) & 0xff),
				 (unsigned) ((version >> 16) & 0xff));
	
		}
		
	Set (ns, path, buf);
	
	}

/******************************************************************************/

dng_fingerprint dng_xmp::GetIPTCDigest () const
	{
	
	dng_fingerprint digest;
	
	if (GetFingerprint (XMP_NS_PHOTOSHOP,
						"LegacyIPTCDigest",
						digest))
		{
		
		return digest;
		
		}
		
	return dng_fingerprint ();
	
	}

/******************************************************************************/

void dng_xmp::SetIPTCDigest (dng_fingerprint &digest)
	{
	
	SetFingerprint (XMP_NS_PHOTOSHOP,
					"LegacyIPTCDigest",
					digest);
		
	}
		
/******************************************************************************/

void dng_xmp::ClearIPTCDigest ()
	{
	
	Remove (XMP_NS_PHOTOSHOP, "LegacyIPTCDigest");

	}
		
/*****************************************************************************/

void dng_xmp::SyncIPTC (dng_iptc &iptc,
					    uint32 options)
	{
	
	SyncAltLangDefault (XMP_NS_DC,
						"title",
						iptc.fTitle,
						options);

	SyncString (XMP_NS_PHOTOSHOP,
				"Category",
				iptc.fCategory,
				options);
				
		{
		
		uint32 x = 0xFFFFFFFF;
		
		if (iptc.fUrgency >= 0)
			{
			
			x = (uint32) iptc.fUrgency;
			
			}
			
		Sync_uint32 (XMP_NS_PHOTOSHOP,
					 "Urgency",
					 x,
					 x == 0xFFFFFFFF,
					 options);
					 
		if (x <= 9)
			{
			
			iptc.fUrgency = (int32) x;
			
			}
		
		}
		
	SyncStringList (XMP_NS_PHOTOSHOP,
					"SupplementalCategories",
					iptc.fSupplementalCategories,
					true,
					options);
				
	SyncStringList (XMP_NS_PHOTOSHOP,
					"Keywords",
					iptc.fKeywords,
					true,
					options);
				
	SyncString (XMP_NS_PHOTOSHOP,
			    "Instructions",
			    iptc.fInstructions,
			    options);
			    
		{
		
		dng_string s = iptc.fDateTimeCreated.Encode_ISO_8601 ();
		
		if (SyncString (XMP_NS_PHOTOSHOP,
						"DateCreated",
						s,
						options))
			{
			
			iptc.fDateTimeCreated.Decode_ISO_8601 (s.Get ());
			
			}
		
		}
		
		{
		
		dng_string s = iptc.fDigitalCreationDateTime.Encode_ISO_8601 ();
		
		if (SyncString (XMP_NS_EXIF,
						"DateTimeDigitized",
						s,
						options))
			{
			
			iptc.fDigitalCreationDateTime.Decode_ISO_8601 (s.Get ());
			
			}
					
		}
		
	SyncStringList (XMP_NS_DC,
			        "creator",
			        iptc.fAuthors,
					false,
					options);
			
	SyncString (XMP_NS_PHOTOSHOP,
			    "AuthorsPosition",
			    iptc.fAuthorsPosition,
			    options);
			
	SyncString (XMP_NS_PHOTOSHOP,
			    "City",
			    iptc.fCity,
			    options);
			
	SyncString (XMP_NS_PHOTOSHOP,
			    "State",
			    iptc.fState,
			    options);
			
	SyncString (XMP_NS_PHOTOSHOP,
			    "Country",
			    iptc.fCountry,
			    options);
			
	SyncString (XMP_NS_IPTC,
			    "CountryCode",
			    iptc.fCountryCode,
			    options);
			
	SyncString (XMP_NS_IPTC,
			    "Location",
			    iptc.fLocation,
			    options);
			
	SyncString (XMP_NS_PHOTOSHOP,
			    "TransmissionReference",
			    iptc.fTransmissionReference,
			    options);
			
	SyncString (XMP_NS_PHOTOSHOP,
			    "Headline",
			    iptc.fHeadline,
			    options);

	SyncString (XMP_NS_PHOTOSHOP,
			    "Credit",
			    iptc.fCredit,
			    options);

	SyncString (XMP_NS_PHOTOSHOP,
			    "Source",
			    iptc.fSource,
			    options);

	SyncAltLangDefault (XMP_NS_DC,
						"rights",
						iptc.fCopyrightNotice,
						options);
				
	SyncAltLangDefault (XMP_NS_DC,
						"description",
						iptc.fDescription,
						options);
				
	SyncString (XMP_NS_PHOTOSHOP,
			    "CaptionWriter",
			    iptc.fDescriptionWriter,
			    options);
			   
	}
		
/*****************************************************************************/

void dng_xmp::IngestIPTC (dng_metadata &metadata,
					      bool xmpIsNewer)
	{
	
	if (metadata.IPTCLength ())
		{
		
		// Parse the IPTC block.
	
		dng_iptc iptc;
		
		iptc.Parse (metadata.IPTCData   (),
					metadata.IPTCLength (),
					metadata.IPTCOffset ());
					
		// Compute fingerprint of IPTC data both ways, including and
		// excluding the padding data.
		
		dng_fingerprint iptcDigest1 = metadata.IPTCDigest (true );
		dng_fingerprint iptcDigest2 = metadata.IPTCDigest (false);
		
		// See if there is an IPTC fingerprint stored in the XMP.
			
		dng_fingerprint xmpDigest = GetIPTCDigest ();
		
		if (xmpDigest.IsValid ())
			{
			
			// If they match, the XMP was already synced with this
			// IPTC block, and we should not resync since it might
			// overwrite changes in the XMP data.
			
			if (iptcDigest1 == xmpDigest)
				{
				
				return;
				
				}
				
			// If it matches the incorrectly computed digest, skip
			// the sync, but fix the digest in the XMP.
			
			if (iptcDigest2 == xmpDigest)
				{
				
				SetIPTCDigest (iptcDigest1);
				
				return;
				
				}
				
			// Else the IPTC has changed, so force an update.
				
			xmpIsNewer = false;
			
			}
			
		else
			{
			
			// There is no IPTC digest.  Previously we would
			// prefer the IPTC in this case, but the MWG suggests
			// that we prefer the XMP in this case.
			
			xmpIsNewer = true;
			
			}
			
		// Remember the fingerprint of the IPTC we are syncing with.
			
		SetIPTCDigest (iptcDigest1);
					    
		// Find the sync options.
		
		uint32 options = xmpIsNewer ? preferXMP
									: preferNonXMP;
					
		// Synchronize the fields.
		
		SyncIPTC (iptc, options);
		
		}

	// After the IPTC data is moved to XMP, we don't need it anymore.
	
	metadata.ClearIPTC ();

	}
		
/*****************************************************************************/

void dng_xmp::RebuildIPTC (dng_metadata &metadata,
						   dng_memory_allocator &allocator,
						   bool padForTIFF)
	{
	
	// If there is no XMP, then there is no IPTC.
	
	if (!fSDK->HasMeta ())
		{
		return;
		}
		
	// Extract the legacy IPTC fields from the XMP data.
	
	dng_iptc iptc;
	
	SyncIPTC (iptc, preferXMP);
		
	// Build legacy IPTC record
	
	if (iptc.NotEmpty ())
		{
		
		AutoPtr<dng_memory_block> block (iptc.Spool (allocator,
													 padForTIFF));
		
		metadata.SetIPTC (block);
		
		}

	}
		
/*****************************************************************************/

void dng_xmp::SyncFlash (uint32 &flashState,
						 uint32 &flashMask,
						 uint32 options)
	{
	
	bool isDefault = (flashState == 0xFFFFFFFF);
	
	if ((options & ignoreXMP) || !isDefault)
		{
		
		Remove (XMP_NS_EXIF, "Flash");
		
		}
	
	if (!isDefault)
		{
		  
		fSDK->SetStructField (XMP_NS_EXIF,
							  "Flash",
							  XMP_NS_EXIF,
							  "Fired",
							  (flashState & 0x1) ? "True" : "False");
			
		if (((flashMask >> 1) & 3) == 3)
			{
			
			char s [8];
		
			sprintf (s, "%u", (unsigned) ((flashState >> 1) & 3));
			
			fSDK->SetStructField (XMP_NS_EXIF,
								  "Flash",
								  XMP_NS_EXIF,
								  "Return",
								  s);
			
			}
			
		if (((flashMask >> 3) & 3) == 3)
			{
		
			char s [8];
		
			sprintf (s, "%u", (unsigned) ((flashState >> 3) & 3));
			
			fSDK->SetStructField (XMP_NS_EXIF,
								  "Flash",
								  XMP_NS_EXIF,
								  "Mode",
								  s);
			
			}
			
		if ((flashMask & (1 << 5)) != 0)
			{
			
			fSDK->SetStructField (XMP_NS_EXIF,
								  "Flash",
								  XMP_NS_EXIF,
								  "Function",
								  (flashState & (1 << 5)) ? "True" : "False");
			
			}
			
		if ((flashMask & (1 << 6)) != 0)
			{
			
			fSDK->SetStructField (XMP_NS_EXIF,
								  "Flash",
								  XMP_NS_EXIF,
								  "RedEyeMode",
								  (flashState & (1 << 6)) ? "True" : "False");
			
			}
			
		}
		
	else if (fSDK->Exists (XMP_NS_EXIF, "Flash"))
		{
		
		dng_string s;
		
		if (fSDK->GetStructField (XMP_NS_EXIF,
								  "Flash",
								  XMP_NS_EXIF,
								  "Fired",
								  s))
			{
			
			flashState = 0;
			flashMask  = 1;
			
			if (s.Matches ("True"))
				{
				flashState |= 1;
				}
				
			if (fSDK->GetStructField (XMP_NS_EXIF,
									  "Flash",
									  XMP_NS_EXIF,
									  "Return",
									  s))
				{
				
				unsigned x = 0;
				
				if (sscanf (s.Get (), "%u", &x) == 1 && x <= 3)
					{
					
					flashState |= x << 1;
					flashMask  |= 3 << 1;
					
					}
				
				}
			
			if (fSDK->GetStructField (XMP_NS_EXIF,
									  "Flash",
									  XMP_NS_EXIF,
									  "Mode",
									  s))
				{
				
				unsigned x = 0;
				
				if (sscanf (s.Get (), "%u", &x) == 1 && x <= 3)
					{
					
					flashState |= x << 3;
					flashMask  |= 3 << 3;
					
					}
				
				}
			
			if (fSDK->GetStructField (XMP_NS_EXIF,
									  "Flash",
									  XMP_NS_EXIF,
									  "Function",
									  s))
				{
				
				flashMask |= 1 << 5;
				
				if (s.Matches ("True"))
					{
					flashState |= 1 << 5;
					}
					
				}
			
			if (fSDK->GetStructField (XMP_NS_EXIF,
									  "Flash",
									  XMP_NS_EXIF,
									  "RedEyeMode",
									  s))
				{
				
				flashMask |= 1 << 6;
				
				if (s.Matches ("True"))
					{
					flashState |= 1 << 6;
					}
					
				}
			
			}
		
		}
		
	}

/*****************************************************************************/

void dng_xmp::SyncExif (dng_exif &exif,
						const dng_exif *originalExif,
						bool doingUpdateFromXMP,
						bool removeFromXMP)
	{
	
	DNG_ASSERT (!doingUpdateFromXMP || originalExif,
				"Must have original EXIF if doingUpdateFromXMP");
	
	// Default synchronization options for the read-only fields.
			
	uint32 readOnly = doingUpdateFromXMP ? ignoreXMP
								         : preferNonXMP;
										 
	// Option for removable fields.
	
	uint32 removable = removeFromXMP ? removeXMP
									 : 0;
	
	// Make:
	
	SyncString (XMP_NS_TIFF,
				"Make",
				exif.fMake,
				readOnly + removable);
			
	// Model:
	
	SyncString (XMP_NS_TIFF,
			    "Model",
			    exif.fModel,
			    readOnly + removable);
			    
	// Exif version number:
	
		{
		
		dng_string exifVersion;
		
		if (exif.fExifVersion)
			{
			
			unsigned b0 = ((exif.fExifVersion >> 24) & 0x0FF) - '0';
			unsigned b1 = ((exif.fExifVersion >> 16) & 0x0FF) - '0';
			unsigned b2 = ((exif.fExifVersion >>  8) & 0x0FF) - '0';
			unsigned b3 = ((exif.fExifVersion      ) & 0x0FF) - '0';
			
			if (b0 <= 9 && b1 <= 9 && b2 <= 9 && b3 <= 9)
				{
				
				char s [5];
	
				sprintf (s,
						 "%1u%1u%1u%1u",
						 b0,
						 b1,
						 b2,
						 b3);
				
				exifVersion.Set (s);
				
				}
				
			}
			
		SyncString (XMP_NS_EXIF,
					"ExifVersion",
					exifVersion,
					readOnly);
					
		if (exifVersion.NotEmpty ())
			{
			
			unsigned b0;
			unsigned b1;
			unsigned b2;
			unsigned b3;
			
			if (sscanf (exifVersion.Get (),
						"%1u%1u%1u%1u",
						&b0,
						&b1,
						&b2,
						&b3) == 4)
				{
				
				if (b0 <= 9 && b1 <= 9 && b2 <= 9 && b3 <= 9)
					{
					
					b0 += '0';
					b1 += '0';
					b2 += '0';
					b3 += '0';
					
					exif.fExifVersion = (b0 << 24) |
										(b1 << 16) |
										(b2 <<  8) |
										(b3      );
					
					}
				
				}
			
			}
			
		// Provide default value for ExifVersion.
		
		if (!exif.fExifVersion)
			{
			
			exif.fExifVersion = DNG_CHAR4 ('0','2','2','1');
		
			Set (XMP_NS_EXIF,
				 "ExifVersion",
				 "0221");
					
			}
			
		if (removeFromXMP)
			{
			
			Remove (XMP_NS_EXIF, "ExifVersion");
			
			}
		
		}
		
	// ExposureTime / ShutterSpeedValue:
	
		{
		
		// Process twice in case XMP contains only one of the
		// two fields.
	
		for (uint32 pass = 0; pass < 2; pass++)
			{
			
			dng_urational et = exif.fExposureTime;
		
			Sync_urational (XMP_NS_EXIF,
							"ExposureTime",
							et,
							readOnly);
							
			if (et.IsValid ())
				{
				
				exif.SetExposureTime (et.As_real64 (), false);
				
				}
				
			dng_srational ss = exif.fShutterSpeedValue;
							
			Sync_srational (XMP_NS_EXIF,
						    "ShutterSpeedValue",
						    ss,
						    readOnly);
						    
			if (ss.IsValid ())
				{
				
				exif.SetShutterSpeedValue (ss.As_real64 ());
				
				}
						    
			}
			
		if (removeFromXMP)
			{
			
			Remove (XMP_NS_EXIF, "ExposureTime");
			
			Remove (XMP_NS_EXIF, "ShutterSpeedValue");
			
			}
			
		}
				    
	// FNumber / ApertureValue:
		
		{
		
		for (uint32 pass = 0; pass < 2; pass++)
			{
			
			dng_urational fs = exif.fFNumber;
			
			Sync_urational (XMP_NS_EXIF,
							"FNumber",
							fs,
							readOnly);
							
			if (fs.IsValid ())
				{
							
				exif.SetFNumber (fs.As_real64 ());
				
				}
			
			dng_urational av = exif.fApertureValue;
			
			Sync_urational (XMP_NS_EXIF,
							"ApertureValue",
							av,
							readOnly);
							
			if (av.IsValid ())
				{
							
				exif.SetApertureValue (av.As_real64 ());
				
				}
				
			}
			
		if (removeFromXMP)
			{
			
			Remove (XMP_NS_EXIF, "FNumber");
			
			Remove (XMP_NS_EXIF, "ApertureValue");
			
			}
			
		}
			
	// Exposure program:
	
	Sync_uint32 (XMP_NS_EXIF,
				 "ExposureProgram",
				 exif.fExposureProgram,
				 exif.fExposureProgram == 0xFFFFFFFF,
				 readOnly + removable);
	
	// ISO Speed Ratings:
	
		{
		
		uint32 isoSpeedRatingsCount = 0;

		uint32 isoSpeedRatingsOptions = readOnly;
		
		uint32 oldISOSpeedRatings [3];

		memcpy (oldISOSpeedRatings,
				exif.fISOSpeedRatings,
				sizeof (oldISOSpeedRatings));

		bool checkXMPForHigherISO = false;
		
		for (uint32 j = 0; j < 3; j++)
			{

			// Special case: the EXIF 2.2x standard represents ISO speed ratings with
			// 2 bytes, which cannot hold ISO speed ratings above 65535 (e.g.,
			// 102400). If the EXIF ISO speed rating value is 65535, prefer the XMP
			// ISOSpeedRatings tag value.

			if (exif.fISOSpeedRatings [j] == 65535)
				{

				isoSpeedRatingsOptions = preferXMP;

				checkXMPForHigherISO = true;

				isoSpeedRatingsCount = 0;

				break;

				}
			
			else if (exif.fISOSpeedRatings [j] == 0)
				{
				break;
				}
				
			isoSpeedRatingsCount++;
			
			}
			
		Sync_uint32_array (XMP_NS_EXIF,
						   "ISOSpeedRatings",
						   exif.fISOSpeedRatings,
						   isoSpeedRatingsCount,
						   3,
						   isoSpeedRatingsOptions);

		// If the EXIF ISO was 65535 and we failed to find anything meaningful in the
		// XMP, then we fall back to the EXIF ISO.

		if (checkXMPForHigherISO && (isoSpeedRatingsCount == 0))
			{
				
			memcpy (exif.fISOSpeedRatings,
					oldISOSpeedRatings,
					sizeof (oldISOSpeedRatings));
			
			}
			
		// Only remove the ISO tag if there are not ratings over 65535.
		
		if (removeFromXMP)
			{
			
			bool hasHighISO = false;
			
			for (uint32 j = 0; j < 3; j++)
				{
				
				if (exif.fISOSpeedRatings [j] == 0)
					{
					break;
					}

				hasHighISO = hasHighISO || (exif.fISOSpeedRatings [j] > 65535);
				
				}
				
			if (!hasHighISO)
				{
				
				Remove (XMP_NS_EXIF, "ISOSpeedRatings");
				
				}
			
			}
		
		}

	// SensitivityType:

	Sync_uint32 (XMP_NS_EXIF,
				 "SensitivityType",
				 exif.fSensitivityType,
				 exif.fSensitivityType == stUnknown,
				 readOnly + removable);
		
	// StandardOutputSensitivity:

	Sync_uint32 (XMP_NS_EXIF,
				 "StandardOutputSensitivity",
				 exif.fStandardOutputSensitivity,
				 exif.fStandardOutputSensitivity == 0,
				 readOnly + removable);
		
	// RecommendedExposureIndex:

	Sync_uint32 (XMP_NS_EXIF,
				 "RecommendedExposureIndex",
				 exif.fRecommendedExposureIndex,
				 exif.fRecommendedExposureIndex == 0,
				 readOnly + removable);
		
	// ISOSpeed:

	Sync_uint32 (XMP_NS_EXIF,
				 "ISOSpeed",
				 exif.fISOSpeed,
				 exif.fISOSpeed == 0,
				 readOnly + removable);
		
	// ISOSpeedLatitudeyyy:

	Sync_uint32 (XMP_NS_EXIF,
				 "ISOSpeedLatitudeyyy",
				 exif.fISOSpeedLatitudeyyy,
				 exif.fISOSpeedLatitudeyyy == 0,
				 readOnly + removable);
		
	// ISOSpeedLatitudezzz:

	Sync_uint32 (XMP_NS_EXIF,
				 "ISOSpeedLatitudezzz",
				 exif.fISOSpeedLatitudezzz,
				 exif.fISOSpeedLatitudezzz == 0,
				 readOnly + removable);
		
	// ExposureIndex:
	
	Sync_urational (XMP_NS_EXIF,
				    "ExposureIndex",
				    exif.fExposureIndex,
				    readOnly + removable);
				    
	// Brightness Value:
	
	Sync_srational (XMP_NS_EXIF,
				    "BrightnessValue",
				    exif.fBrightnessValue,
				    readOnly + removable);
				    
	// Exposure Bias:
	
	Sync_srational (XMP_NS_EXIF,
				    "ExposureBiasValue",
				    exif.fExposureBiasValue,
				    readOnly + removable);
				    
	// Max Aperture:
	
	Sync_urational (XMP_NS_EXIF,
				    "MaxApertureValue",
				    exif.fMaxApertureValue,
				    readOnly + removable);

	// Subject Distance:
	
	Sync_urational (XMP_NS_EXIF,
				    "SubjectDistance",
				    exif.fSubjectDistance,
				    readOnly + removable);
	
	// Metering Mode:
	
	Sync_uint32 (XMP_NS_EXIF,
				 "MeteringMode",
				 exif.fMeteringMode,
				 exif.fMeteringMode == 0xFFFFFFFF,
				 readOnly + removable);
	
	// Light Source:
	
	Sync_uint32 (XMP_NS_EXIF,
				 "LightSource",
				 exif.fLightSource,
				 exif.fLightSource > 0x0FFFF,
				 readOnly + removable);
	
	// Flash State:
	
	SyncFlash (exif.fFlash,
			   exif.fFlashMask,
			   readOnly);
			   
	if (removeFromXMP)
		{
		Remove (XMP_NS_EXIF, "Flash");
		}

	// Focal Length:
	
	Sync_urational (XMP_NS_EXIF,
					"FocalLength",
					exif.fFocalLength,
					readOnly + removable);
	
	// Sensing Method.
	
	Sync_uint32 (XMP_NS_EXIF,
				 "SensingMethod",
				 exif.fSensingMethod,
				 exif.fSensingMethod > 0x0FFFF,
				 readOnly + removable);
	
	// File Source.
	
	Sync_uint32 (XMP_NS_EXIF,
				 "FileSource",
				 exif.fFileSource,
				 exif.fFileSource > 0x0FF,
				 readOnly + removable);
	
	// Scene Type.
	
	Sync_uint32 (XMP_NS_EXIF,
				 "SceneType",
				 exif.fSceneType,
				 exif.fSceneType > 0x0FF,
				 readOnly + removable);
	
	// Focal Length in 35mm Film:
	
	Sync_uint32 (XMP_NS_EXIF,
				 "FocalLengthIn35mmFilm",
				 exif.fFocalLengthIn35mmFilm,
				 exif.fFocalLengthIn35mmFilm == 0,
				 readOnly + removable);
	
	// Custom Rendered:
	
	Sync_uint32 (XMP_NS_EXIF,
				 "CustomRendered",
				 exif.fCustomRendered,
				 exif.fCustomRendered > 0x0FFFF,
				 readOnly + removable);
	
	// Exposure Mode:
	
	Sync_uint32 (XMP_NS_EXIF,
				 "ExposureMode",
				 exif.fExposureMode,
				 exif.fExposureMode > 0x0FFFF,
				 readOnly + removable);
	
	// White Balance:
	
	Sync_uint32 (XMP_NS_EXIF,
				 "WhiteBalance",
				 exif.fWhiteBalance,
				 exif.fWhiteBalance > 0x0FFFF,
				 readOnly + removable);
	
	// Scene Capture Type:
	
	Sync_uint32 (XMP_NS_EXIF,
				 "SceneCaptureType",
				 exif.fSceneCaptureType,
				 exif.fSceneCaptureType > 0x0FFFF,
				 readOnly + removable);
	
	// Gain Control:
	
	Sync_uint32 (XMP_NS_EXIF,
				 "GainControl",
				 exif.fGainControl,
				 exif.fGainControl > 0x0FFFF,
				 readOnly + removable);
	
	// Contrast:
	
	Sync_uint32 (XMP_NS_EXIF,
				 "Contrast",
				 exif.fContrast,
				 exif.fContrast > 0x0FFFF,
				 readOnly + removable);
	
	// Saturation:
	
	Sync_uint32 (XMP_NS_EXIF,
				 "Saturation",
				 exif.fSaturation,
				 exif.fSaturation > 0x0FFFF,
				 readOnly + removable);
	
	// Sharpness:
	
	Sync_uint32 (XMP_NS_EXIF,
				 "Sharpness",
				 exif.fSharpness,
				 exif.fSharpness > 0x0FFFF,
				 readOnly + removable);
	
	// Subject Distance Range:
	
	Sync_uint32 (XMP_NS_EXIF,
				 "SubjectDistanceRange",
				 exif.fSubjectDistanceRange,
				 exif.fSubjectDistanceRange > 0x0FFFF,
				 readOnly + removable);
		
	// Subject Area:
	
	Sync_uint32_array (XMP_NS_EXIF,
					   "SubjectArea",
					   exif.fSubjectArea,
					   exif.fSubjectAreaCount,
					   sizeof (exif.fSubjectArea    ) /
					   sizeof (exif.fSubjectArea [0]),
					   readOnly);
					   
	if (removeFromXMP)
		{
		Remove (XMP_NS_EXIF, "SubjectArea");
		}
		
	// Digital Zoom Ratio:
	
	Sync_urational (XMP_NS_EXIF,
					"DigitalZoomRatio",
					exif.fDigitalZoomRatio,
					readOnly + removable);
	
	// Focal Plane Resolution:
	
	Sync_urational (XMP_NS_EXIF,
					"FocalPlaneXResolution",
					exif.fFocalPlaneXResolution,
					readOnly + removable);
	
	Sync_urational (XMP_NS_EXIF,
					"FocalPlaneYResolution",
					exif.fFocalPlaneYResolution,
					readOnly + removable);
	
	Sync_uint32 (XMP_NS_EXIF,
				 "FocalPlaneResolutionUnit",
				 exif.fFocalPlaneResolutionUnit,
				 exif.fFocalPlaneResolutionUnit > 0x0FFFF,
				 readOnly + removable);
		
	// ImageDescription:  (XMP is is always preferred)
	
	if (fSDK->GetAltLangDefault (XMP_NS_DC,
								 "description",
								 exif.fImageDescription))
		
		{
		
		}
		
	else if (doingUpdateFromXMP)
		{
		
		exif.fImageDescription.Clear ();
		
		if (originalExif->fImageDescription.NotEmpty ())
			{
			
			fSDK->SetAltLangDefault (XMP_NS_DC,
									 "description",
									 dng_string ());
									 
			}
		
		}
	
	else if (exif.fImageDescription.NotEmpty ())
		{
		
		fSDK->SetAltLangDefault (XMP_NS_DC,
								 "description",
								 exif.fImageDescription);
									 
		}
		
	// Artist:  (XMP is is always preferred)
	
		{
		
		dng_string_list xmpList;
		
		if (fSDK->GetStringList (XMP_NS_DC,
								 "creator",
								 xmpList))
			{
			
			exif.fArtist.Clear ();
							
			if (xmpList.Count () > 0)
				{
				
				uint32 j;
				
				uint32 bufferSize = xmpList.Count () * 4 + 1;
				
				for (j = 0; j < xmpList.Count (); j++)
					{
					
					bufferSize += xmpList [j].Length () * 2;
					
					}
					
				dng_memory_data temp (bufferSize);
				
				char *t = temp.Buffer_char ();
				
				for (j = 0; j < xmpList.Count (); j++)
					{
					
					const char *s = xmpList [j].Get ();
					
					bool needQuotes = xmpList [j].Contains ("; ") ||
									  s [0] == '\"';
									   
					if (needQuotes)
						{
						*(t++) = '\"';
						}
						
					while (s [0] != 0)
						{
						
						if (s [0] == '\"' && needQuotes)
							{
							*(t++) = '\"';
							}
							
						*(t++) = *(s++);
						
						}
						
					if (needQuotes)
						{
						*(t++) = '\"';
						}
						
					if (j != xmpList.Count () - 1)
						{
						*(t++) = ';';
						*(t++) = ' ';
						}
					else
						{
						*t = 0;
						}

					}
				
				exif.fArtist.Set (temp.Buffer_char ());
				
				}

			}
			
		else if (doingUpdateFromXMP)
			{
			
			exif.fArtist.Clear ();
			
			if (originalExif->fArtist.NotEmpty ())
				{
				
				dng_string_list fakeList;
				
				fakeList.Append (dng_string ());
				
				SetStringList (XMP_NS_DC,
							   "creator",
							   fakeList,
							   false);
										 
				}
			
			}
			
		else if (exif.fArtist.NotEmpty ())
			{
			
			dng_string_list newList;
		
			dng_memory_data temp (exif.fArtist.Length () + 1);
					
			const char *s = exif.fArtist.Get ();
			
			char *t = temp.Buffer_char ();
			
			bool first = true;
			
			bool quoted = false;
			
			bool valid = true;
				
			while (s [0] != 0 && valid)
				{
				
				if (first)
					{
					
					if (s [0] == '\"')
						{
						
						quoted = true;
						
						s++;
						
						}
					
					}
					
				first = false;
				
				if (quoted)
					{
					
					if (s [0] == '\"' &&
						s [1] == '\"')
						{
						
						s+= 2;
						
						*(t++) = '\"';
						
						}
							
					else if (s [0] == '\"')
						{
						
						s++;
						
						quoted = false;
						
						valid = valid && ((s [0] == 0) || ((s [0] == ';' && s [1] == ' ')));
				
						}
						
					else
						{
						
						*(t++) = *(s++);
						
						}
							
					}
						
				else if (s [0] == ';' &&
						 s [1] == ' ')
					{
					
					s += 2;
					
					t [0] = 0;
						
					dng_string ss;
					
					ss.Set (temp.Buffer_char ());
					
					newList.Append (ss);
					
					t = temp.Buffer_char ();
					
					first = true;
						
					}
					
				else
					{
					
					*(t++) = *(s++);
						
					}
					
				}
				
			if (quoted)
				{
				
				valid = false;
				
				}
				
			if (valid)
				{
				
				if (t != temp.Buffer_char ())
					{
					
					t [0] = 0;
						
					dng_string ss;
					
					ss.Set (temp.Buffer_char ());
					
					newList.Append (ss);
					
					}
					
				}
				
			else
				{
				
				newList.Clear ();
				
				newList.Append (exif.fArtist);
				
				}
				
			SetStringList (XMP_NS_DC,
						   "creator",
						   newList,
						   false);
										 
			}
			
		}
		
	// Software:  (XMP is is always preferred)
	
	if (fSDK->GetString (XMP_NS_XAP,
						 "CreatorTool",
						 exif.fSoftware))
		
		{
		
		}
		
	else if (doingUpdateFromXMP)
		{
		
		exif.fSoftware.Clear ();
		
		if (originalExif->fSoftware.NotEmpty ())
			{
			
			fSDK->SetString (XMP_NS_XAP,
							 "CreatorTool",
							 dng_string ());
									 
			}
		
		}
	
	else if (exif.fSoftware.NotEmpty ())
		{
		
		fSDK->SetString (XMP_NS_XAP,
						 "CreatorTool",
						 exif.fSoftware);
									 
		}
		
	// Copyright:  (XMP is is always preferred)
	
	if (fSDK->GetAltLangDefault (XMP_NS_DC,
								 "rights",
								 exif.fCopyright))
		
		{
		
		}
	
	else if (doingUpdateFromXMP)
		{
		
		exif.fCopyright.Clear ();
		
		if (originalExif->fCopyright.NotEmpty ())
			{
			
			fSDK->SetAltLangDefault (XMP_NS_DC,
									 "rights",
									 dng_string ());
									 
			}
		
		}
	
	else if (exif.fCopyright.NotEmpty ())
		{
		
		fSDK->SetAltLangDefault (XMP_NS_DC,
								 "rights",
								 exif.fCopyright);
				
		}
					
	// Camera serial number private tag:

	SyncString (XMP_NS_AUX,
				"SerialNumber",
				exif.fCameraSerialNumber,
				readOnly);
			
	// Lens Info:
	
		{
		
		dng_string s;
		
		if (exif.fLensInfo [0].IsValid ())
			{
			
			char ss [256];
			
			sprintf (ss,
					 "%u/%u %u/%u %u/%u %u/%u",
					 (unsigned) exif.fLensInfo [0].n,
					 (unsigned) exif.fLensInfo [0].d,
					 (unsigned) exif.fLensInfo [1].n,
					 (unsigned) exif.fLensInfo [1].d,
					 (unsigned) exif.fLensInfo [2].n,
					 (unsigned) exif.fLensInfo [2].d,
					 (unsigned) exif.fLensInfo [3].n,
					 (unsigned) exif.fLensInfo [3].d);
					 
			s.Set (ss);
			
			}
			
		SyncString (XMP_NS_AUX,
					"LensInfo",
				    s,
				    readOnly);
				    
		if (s.NotEmpty ())
			{
			
			unsigned n [4];
			unsigned d [4];
			
			if (sscanf (s.Get (),
						"%u/%u %u/%u %u/%u %u/%u",
						&n [0],
						&d [0],
						&n [1],
						&d [1],
						&n [2],
						&d [2],
						&n [3],
						&d [3]) == 8)
				{
				
				for (uint32 j = 0; j < 4; j++)
					{
					
					exif.fLensInfo [j] = dng_urational (n [j], d [j]);
					
					}
				
				}
						
			
			}

		}
		
	// Lens name:
	
		{
		
		// EXIF lens names are sometimes missing or wrong (esp. when non-OEM lenses
		// are used). So prefer the value from XMP.
		
		SyncString (XMP_NS_AUX,
					"Lens",
					exif.fLensName,
					preferXMP);

		// Generate default lens name from lens info if required.
		// Ignore names names that end in "f/0.0" due to third party bug.
		
		if ((exif.fLensName.IsEmpty () ||
			 exif.fLensName.EndsWith ("f/0.0")) && exif.fLensInfo [0].IsValid ())
			{
			
			char s [256];
							
			real64 minFL = exif.fLensInfo [0].As_real64 ();
			real64 maxFL = exif.fLensInfo [1].As_real64 ();
			
			// The f-stop numbers are optional.
			
			if (exif.fLensInfo [2].IsValid ())
				{
				
				real64 minFS = exif.fLensInfo [2].As_real64 ();
				real64 maxFS = exif.fLensInfo [3].As_real64 ();
				
				if (minFL == maxFL)
					sprintf (s, "%.1f mm f/%.1f", minFL, minFS);
					
				else if (minFS == maxFS)
					sprintf (s, "%.1f-%.1f mm f/%.1f", minFL, maxFL, minFS);
					
				else
					sprintf (s, "%.1f-%.1f mm f/%.1f-%.1f", minFL, maxFL, minFS, maxFS);
				
				}
				
			else
				{
				
				if (minFL == maxFL)
					sprintf (s, "%.1f mm", minFL);
					
				else
					sprintf (s, "%.1f-%.1f mm", minFL, maxFL);
				
				}
				
			exif.fLensName.Set (s);
				
			SetString (XMP_NS_AUX,
					   "Lens",
					   exif.fLensName);
			
			}

		}

	// Lens ID:
	
	SyncString (XMP_NS_AUX,
				"LensID",
				exif.fLensID,
				readOnly);
	
	// Lens Make:
	
	SyncString (XMP_NS_EXIF,
				"LensMake",
				exif.fLensMake,
				readOnly + removable);
		
	// Lens Serial Number:
	
	SyncString (XMP_NS_AUX,
				"LensSerialNumber",
				exif.fLensSerialNumber,
				readOnly);
		
	// Image Number:
	
	Sync_uint32 (XMP_NS_AUX, 
				 "ImageNumber",
				 exif.fImageNumber,
				 exif.fImageNumber == 0xFFFFFFFF,
				 readOnly);
		
	// User Comment:
	
	if (exif.fUserComment.NotEmpty ())
		{

		fSDK->SetAltLangDefault (XMP_NS_EXIF,
								 "UserComment",
								 exif.fUserComment);
		
		}
			
	else
		{
	
		(void) fSDK->GetAltLangDefault (XMP_NS_EXIF,
									  	"UserComment",
									  	exif.fUserComment);

		}
		
	if (removeFromXMP)
		{
		Remove (XMP_NS_EXIF, "UserComment");
		}
		
	// Approximate focus distance:

	SyncApproximateFocusDistance (exif,
								  readOnly);
	
	// Flash Compensation:
	
	Sync_srational (XMP_NS_AUX,
				    "FlashCompensation",
				    exif.fFlashCompensation,
				    readOnly);

	// Owner Name: (allow XMP updates)
	
	SyncString (XMP_NS_AUX,
				"OwnerName",
				exif.fOwnerName,
				preferXMP);
		
	// Firmware:
	
	SyncString (XMP_NS_AUX,
				"Firmware",
				exif.fFirmware,
				readOnly);
		
	// Image Unique ID:
	
		{
		
		dng_string s = EncodeFingerprint (exif.fImageUniqueID);
		
		SyncString (XMP_NS_EXIF,
				    "ImageUniqueID",
				    s,
				    readOnly + removable);
				    
		exif.fImageUniqueID = DecodeFingerprint (s);
		
		}
		
	// Allow EXIF GPS to be updated via updates from XMP.
	
	if (doingUpdateFromXMP)
		{
		
		// Require that at least one basic GPS field exist in the
		// XMP before overrriding the EXIF GPS fields.
		
		if (Exists (XMP_NS_EXIF, "GPSVersionID"       ) ||
			Exists (XMP_NS_EXIF, "GPSLatitude"        ) ||
			Exists (XMP_NS_EXIF, "GPSLongitude"       ) ||
			Exists (XMP_NS_EXIF, "GPSAltitude"        ) ||
			Exists (XMP_NS_EXIF, "GPSTimeStamp"       ) ||
			Exists (XMP_NS_EXIF, "GPSProcessingMethod"))
			{
			
			// Clear out the GPS info from the EXIF so it will
			// replaced by the GPS info from the XMP.
			
			dng_exif blankExif;
			
			exif.CopyGPSFrom (blankExif);
			
			}
		
		}
			
	// GPS Version ID:
	
		{
		
		dng_string s = EncodeGPSVersion (exif.fGPSVersionID);
		
		if (SyncString (XMP_NS_EXIF,
						"GPSVersionID",
						s,
						preferNonXMP + removable))
			{
					
			exif.fGPSVersionID = DecodeGPSVersion (s);
			
			}
	
		}
		
	// GPS Latitude:
	
		{
		
		dng_string s = EncodeGPSCoordinate (exif.fGPSLatitudeRef,
					  						exif.fGPSLatitude);
					  						
		if (SyncString (XMP_NS_EXIF,
						"GPSLatitude",
						s,
						preferNonXMP + removable))
			{
			
			DecodeGPSCoordinate (s,
								 exif.fGPSLatitudeRef,
								 exif.fGPSLatitude);
			
			}
					
		}
					  
	// GPS Longitude:
	
		{
	
		dng_string s = EncodeGPSCoordinate (exif.fGPSLongitudeRef,
					  						exif.fGPSLongitude);
					  						
		if (SyncString (XMP_NS_EXIF,
						"GPSLongitude",
						s,
						preferNonXMP + removable))
			{
			
			DecodeGPSCoordinate (s,
								 exif.fGPSLongitudeRef,
								 exif.fGPSLongitude);
			
			}
					
		}
					
	// Handle simple case of incorrectly written GPS altitude where someone didn't understand the GPSAltitudeRef and assumed the GPSAltitude RATIONAL is signed.
	// Only handle this case as we do not want to misinterpret e.g. a fixed point representation of very high GPS altitudes.
	
	uint32 &altitudeRef = exif.fGPSAltitudeRef;
	dng_urational &altitude = exif.fGPSAltitude;

	if (altitude.IsValid () &&
		(altitudeRef == 0 || altitudeRef == 0xFFFFFFFF))  // If the file contains a "below sea level" altitudeRef, assume the writing software is working according to the spec.
		{

		if ((altitude.n & (1U << 31)) &&
			altitude.d < 7) // As the denominator increases, large numerator values become possibly valid distances. Pick a limit on the conservative side (approx 33e6m) to prevent misinterpretation.
							// Noting that the normal case for this mistake has a denominator of 1
			{

			altitude.n = ~altitude.n + 1;
			altitudeRef = 1;

			}

		}

	// GPS Altitude Reference:
	
	Sync_uint32 (XMP_NS_EXIF,
				 "GPSAltitudeRef",
				 altitudeRef,
				 altitudeRef == 0xFFFFFFFF,
				 preferNonXMP + removable);
	
	// GPS Altitude:
	
	Sync_urational (XMP_NS_EXIF,
					"GPSAltitude",
					altitude,
					preferNonXMP + removable);
	
	// GPS Date/Time:
	
		{
		
		dng_string s = EncodeGPSDateTime (exif.fGPSDateStamp,
										  exif.fGPSTimeStamp);
										  
		if (SyncString (XMP_NS_EXIF,
						"GPSTimeStamp",
						s,
						preferNonXMP + removable))
			{
			
			DecodeGPSDateTime (s,
							   exif.fGPSDateStamp,
							   exif.fGPSTimeStamp);
			
			}
	
		}
		
	// GPS Satellites:
	
	SyncString (XMP_NS_EXIF,
				"GPSSatellites",
				exif.fGPSSatellites,
				preferNonXMP + removable);
	
	// GPS Status:

	SyncString (XMP_NS_EXIF,
				"GPSStatus",
				exif.fGPSStatus,
				preferNonXMP + removable);
	
	// GPS Measure Mode:
	
	SyncString (XMP_NS_EXIF,
				"GPSMeasureMode",
				exif.fGPSMeasureMode,
				preferNonXMP + removable);
	
	// GPS DOP:
	
	Sync_urational (XMP_NS_EXIF,
					"GPSDOP",
					exif.fGPSDOP,
					preferNonXMP + removable);
		
	// GPS Speed Reference:
	
	SyncString (XMP_NS_EXIF,
				"GPSSpeedRef",
				exif.fGPSSpeedRef,
				preferNonXMP + removable);
	
	// GPS Speed:
		
	Sync_urational (XMP_NS_EXIF,
					"GPSSpeed",
					exif.fGPSSpeed,
					preferNonXMP + removable);
		
	// GPS Track Reference:
	
	SyncString (XMP_NS_EXIF,
				"GPSTrackRef",
				exif.fGPSTrackRef,
				preferNonXMP + removable);
	
	// GPS Track:
		
	Sync_urational (XMP_NS_EXIF,
					"GPSTrack",
					exif.fGPSTrack,
					preferNonXMP + removable);
		
	// GPS Image Direction Reference:
	
	SyncString (XMP_NS_EXIF,
				"GPSImgDirectionRef",
				exif.fGPSImgDirectionRef,
				preferNonXMP + removable);
	
	// GPS Image Direction:
		
	Sync_urational (XMP_NS_EXIF,
					"GPSImgDirection",
					exif.fGPSImgDirection,
					preferNonXMP + removable);
		
	// GPS Map Datum:
	
	SyncString (XMP_NS_EXIF,
				"GPSMapDatum",
				exif.fGPSMapDatum,
				preferNonXMP + removable);
	
	// GPS Destination Latitude:
	
		{
		
		dng_string s = EncodeGPSCoordinate (exif.fGPSDestLatitudeRef,
					  						exif.fGPSDestLatitude);
					  						
		if (SyncString (XMP_NS_EXIF,
						"GPSDestLatitude",
						s,
						preferNonXMP + removable))
			{
			
			DecodeGPSCoordinate (s,
								 exif.fGPSDestLatitudeRef,
								 exif.fGPSDestLatitude);
			
			}
					
		}

	// GPS Destination Longitude:
	
		{
		
		dng_string s = EncodeGPSCoordinate (exif.fGPSDestLongitudeRef,
					  						exif.fGPSDestLongitude);
					  						
		if (SyncString (XMP_NS_EXIF,
						"GPSDestLongitude",
						s,
						preferNonXMP + removable))
			{
			
			DecodeGPSCoordinate (s,
								 exif.fGPSDestLongitudeRef,
								 exif.fGPSDestLongitude);
			
			}
					
		}

	// GPS Destination Bearing Reference:
	
	SyncString (XMP_NS_EXIF,
				"GPSDestBearingRef",
				exif.fGPSDestBearingRef,
				preferNonXMP + removable);
	
	// GPS Destination Bearing:
		
	Sync_urational (XMP_NS_EXIF,
					"GPSDestBearing",
					exif.fGPSDestBearing,
					preferNonXMP + removable);
		
	// GPS Destination Distance Reference:
	
	SyncString (XMP_NS_EXIF,
				"GPSDestDistanceRef",
				exif.fGPSDestDistanceRef,
				preferNonXMP + removable);
	
	// GPS Destination Distance:
		
	Sync_urational (XMP_NS_EXIF,
					"GPSDestDistance",
					exif.fGPSDestDistance,
					preferNonXMP + removable);
		
	// GPS Processing Method:
	
	SyncString (XMP_NS_EXIF,
				"GPSProcessingMethod",
				exif.fGPSProcessingMethod,
				preferNonXMP + removable);
	
	// GPS Area Information:
	
	SyncString (XMP_NS_EXIF,
				"GPSAreaInformation",
				exif.fGPSAreaInformation,
				preferNonXMP + removable);
	
	// GPS Differential:
	
	Sync_uint32 (XMP_NS_EXIF,
				 "GPSDifferential",
				 exif.fGPSDifferential,
				 exif.fGPSDifferential == 0xFFFFFFFF,
				 preferNonXMP + removable);
				 
	// GPS Horizontal Positioning Error:
	
	Sync_urational (XMP_NS_EXIF,
					"GPSHPositioningError",
					exif.fGPSHPositioningError,
					preferNonXMP + removable);
					
	// Sync date/times.
		
	UpdateExifDates (exif, removeFromXMP);
		
	// We are syncing EXIF and XMP, but we are not updating the
	// NativeDigest tags.  It is better to just delete them than leave
	// the stale values around.
	
	Remove (XMP_NS_EXIF, "NativeDigest");
	Remove (XMP_NS_TIFF, "NativeDigest");
	
	}

/*****************************************************************************/

void dng_xmp::SyncApproximateFocusDistance (dng_exif &exif,
											const uint32 readOnly)
	{
	
	Sync_urational (XMP_NS_AUX,
					"ApproximateFocusDistance",
					exif.fApproxFocusDistance,
					readOnly);	
	
	}
							   
/******************************************************************************/
	
void dng_xmp::ValidateStringList (const char *ns,
							      const char *path)
	{
	
	fSDK->ValidateStringList (ns, path);

	}
							   
/******************************************************************************/
	
void dng_xmp::ValidateMetadata ()
	{
	
	// The following values should be arrays, but are not always.  So
	// fix them up because Photoshop sometimes has problems parsing invalid
	// tags.
	
	ValidateStringList (XMP_NS_DC, "creator");
							   
	ValidateStringList (XMP_NS_PHOTOSHOP, "Keywords");
	ValidateStringList (XMP_NS_PHOTOSHOP, "SupplementalCategories");
	
	}
		
/******************************************************************************/
	
bool dng_xmp::DateTimeIsDateOnly (const char *ns,
							      const char *path)
	{
	
	dng_string s;
	
	if (GetString (ns, path, s))
		{
		
		uint32 len = s.Length ();
		
		if (len)
			{
		
			for (uint32 j = 0; j < len; j++)
				{
				
				if (s.Get () [j] == 'T')
					{
					
					return false;
					
					}
					
				}
				
			return true;
			
			}
		
		}
	
	return false;
	
	}

/******************************************************************************/
	
void dng_xmp::UpdateExifDates (dng_exif &exif,
							   bool removeFromXMP)
	{
	
	// For the following three date/time fields, we always prefer XMP to
	// the EXIF values.  This is to allow the user to correct the date/times
	// via changes in a sidecar XMP file, without modifying the original
	// raw file.
	
	// Kludge: The Nikon D4 is writing date only date/times into XMP, so
	// prefer the EXIF values if the XMP only contains a date.
	
	// Modification Date/Time:
	// exif.fDateTime
	// kXMP_NS_XMP:"ModifyDate" & kXMP_NS_TIFF:"DateTime" are aliased
		
		{
			
		dng_string s = exif.fDateTime.Encode_ISO_8601 ();
		
		bool dateOnly = DateTimeIsDateOnly (XMP_NS_TIFF, "DateTime");
		
		SyncString (XMP_NS_TIFF,
					"DateTime",
					s,
					dateOnly ? preferNonXMP : preferXMP);
		
		if (s.NotEmpty ())
			{
			
			exif.fDateTime.Decode_ISO_8601 (s.Get ());
			
			// Round trip again in case we need to add a fake time zone.
			
			s = exif.fDateTime.Encode_ISO_8601 ();
			
			SetString (XMP_NS_TIFF,
					   "DateTime",
					   s);
			
			}
			
		}
		
	// Original Date/Time:
	// exif.fDateTimeOriginal
	// IPTC: DateCreated
	// XMP_NS_EXIF:"DateTimeOriginal" & XMP_NS_PHOTOSHOP:"DateCreated"
	// Adobe has decided to keep the two XMP fields separate.
	
		{
			
		dng_string s = exif.fDateTimeOriginal.Encode_ISO_8601 ();
		
		bool dateOnly = DateTimeIsDateOnly (XMP_NS_EXIF, "DateTimeOriginal");
		
		SyncString (XMP_NS_EXIF,
					"DateTimeOriginal",
					s,
					dateOnly ? preferNonXMP : preferXMP);
		
		if (s.NotEmpty ())
			{
			
			exif.fDateTimeOriginal.Decode_ISO_8601 (s.Get ());
			
			// Round trip again in case we need to add a fake time zone.
			
			s = exif.fDateTimeOriginal.Encode_ISO_8601 ();
			
			SetString (XMP_NS_EXIF,
					   "DateTimeOriginal",
					   s);
			
			}
			
		// Sync the IPTC value to the EXIF value if only the EXIF
		// value exists.
		
		if (s.NotEmpty () && !Exists (XMP_NS_PHOTOSHOP, "DateCreated"))
			{
			
			SetString (XMP_NS_PHOTOSHOP, "DateCreated", s);
			
			}
			
		if (removeFromXMP)
			{
			
			Remove (XMP_NS_EXIF, "DateTimeOriginal");
			
			}
			
		}
		
	// Date Time Digitized:
	// XMP_NS_EXIF:"DateTimeDigitized" & kXMP_NS_XMP:"CreateDate" are aliased
	
		{
			
		dng_string s = exif.fDateTimeDigitized.Encode_ISO_8601 ();
		
		bool dateOnly = DateTimeIsDateOnly (XMP_NS_EXIF, "DateTimeDigitized");
		
		SyncString (XMP_NS_EXIF,
					"DateTimeDigitized",
					s,
					dateOnly ? preferNonXMP : preferXMP);
			
		if (s.NotEmpty ())
			{
				
			exif.fDateTimeDigitized.Decode_ISO_8601 (s.Get ());
							
			// Round trip again in case we need to add a fake time zone.
			
			s = exif.fDateTimeDigitized.Encode_ISO_8601 ();
			
			SetString (XMP_NS_EXIF,
					   "DateTimeDigitized",
					   s);
			
			}
			
		}
		
	}

/******************************************************************************/

void dng_xmp::UpdateDateTime (const dng_date_time_info &dt)
	{
	
	dng_string s = dt.Encode_ISO_8601 ();
								   
	SetString (XMP_NS_TIFF,
			   "DateTime",
			   s);
					
	}

/******************************************************************************/

void dng_xmp::UpdateMetadataDate (const dng_date_time_info &dt)
	{
	
	dng_string s = dt.Encode_ISO_8601 ();
								   
	SetString (XMP_NS_XAP,
			   "MetadataDate",
			   s);
					
	}

/*****************************************************************************/

bool dng_xmp::HasOrientation () const
	{
	
	uint32 x = 0;
	
	if (Get_uint32 (XMP_NS_TIFF,
					"Orientation",
					x))
		{
		
		return (x >= 1) && (x <= 8);
		
		}
		
	return false;
		
	}
		
/*****************************************************************************/

dng_orientation dng_xmp::GetOrientation () const
	{
	
	dng_orientation result;
	
	uint32 x = 0;
	
	if (Get_uint32 (XMP_NS_TIFF,
					"Orientation",
					x))
		{
		
		if ((x >= 1) && (x <= 8))
			{
			
			result.SetTIFF (x);
			
			}
						
		}
	
	return result;
	
	}
		
/******************************************************************************/

void dng_xmp::ClearOrientation ()
	{
	
	fSDK->Remove (XMP_NS_TIFF, "Orientation");
	
	}
				  
/******************************************************************************/

void dng_xmp::SetOrientation (const dng_orientation &orientation)
	{
	
	Set_uint32 (XMP_NS_TIFF,
			    "Orientation",
				orientation.GetTIFF ());
		
	}
		
/*****************************************************************************/

void dng_xmp::SyncOrientation (dng_negative &negative,
					   		   bool xmpIsMaster)
	{
	
	SyncOrientation (negative.Metadata (), xmpIsMaster);
	
	}
	
/*****************************************************************************/

void dng_xmp::SyncOrientation (dng_metadata &metadata,
					   		   bool xmpIsMaster)
	{
			
	// See if XMP contains the orientation.
	
	bool xmpHasOrientation = HasOrientation ();

	// See if XMP is the master value.
	
	if (xmpHasOrientation && (xmpIsMaster || !metadata.HasBaseOrientation ()))
		{
		
		metadata.SetBaseOrientation (GetOrientation ());
		
		}
		
	else
		{
		
		SetOrientation (metadata.BaseOrientation ());
		
		}

	}
	
/******************************************************************************/

void dng_xmp::ClearImageInfo ()
	{
	
	Remove (XMP_NS_TIFF, "ImageWidth" );
	Remove (XMP_NS_TIFF, "ImageLength");
	
	Remove (XMP_NS_EXIF, "PixelXDimension");
	Remove (XMP_NS_EXIF, "PixelYDimension");
		
	Remove (XMP_NS_TIFF, "BitsPerSample");
	
	Remove (XMP_NS_TIFF, "Compression");
		
	Remove (XMP_NS_TIFF, "PhotometricInterpretation");
	
	// "Orientation" is handled separately.
	
	Remove (XMP_NS_TIFF, "SamplesPerPixel");
	
	Remove (XMP_NS_TIFF, "PlanarConfiguration");
	
	Remove (XMP_NS_TIFF, "XResolution");
	Remove (XMP_NS_TIFF, "YResolution");
		
	Remove (XMP_NS_TIFF, "ResolutionUnit");
	
	Remove (XMP_NS_PHOTOSHOP, "ColorMode" );
	Remove (XMP_NS_PHOTOSHOP, "ICCProfile");
	
	}
	
/******************************************************************************/

void dng_xmp::SetImageSize (const dng_point &size)
	{
	
	Set_uint32 (XMP_NS_TIFF, "ImageWidth" , size.h);
	Set_uint32 (XMP_NS_TIFF, "ImageLength", size.v);
	
	// Mirror these values to the EXIF tags.
	
	Set_uint32 (XMP_NS_EXIF, "PixelXDimension" , size.h);
	Set_uint32 (XMP_NS_EXIF, "PixelYDimension" , size.v);
	
	}
	
/******************************************************************************/

void dng_xmp::SetSampleInfo (uint32 samplesPerPixel,
							 uint32 bitsPerSample)
	{
	
	Set_uint32 (XMP_NS_TIFF, "SamplesPerPixel", samplesPerPixel);
	
	char s [32];
	
	sprintf (s, "%u", (unsigned) bitsPerSample);
	
	dng_string ss;
	
	ss.Set (s);
	
	dng_string_list list;
	
	for (uint32 j = 0; j < samplesPerPixel; j++)
		{
		list.Append (ss);
		}
	
	SetStringList (XMP_NS_TIFF, "BitsPerSample", list, false);
	
	}
	
/******************************************************************************/

void dng_xmp::SetPhotometricInterpretation (uint32 pi)
	{
	
	Set_uint32 (XMP_NS_TIFF, "PhotometricInterpretation", pi);
	
	}
		
/******************************************************************************/

void dng_xmp::SetResolution (const dng_resolution &res)
	{
	
 	Set_urational (XMP_NS_TIFF, "XResolution", res.fXResolution);
	Set_urational (XMP_NS_TIFF, "YResolution", res.fYResolution);
		
    Set_uint32 (XMP_NS_TIFF, "ResolutionUnit", res.fResolutionUnit);
	
	}

/*****************************************************************************/

void dng_xmp::ComposeArrayItemPath (const char *ns,
									const char *arrayName,
									int32 itemNumber,
									dng_string &s) const
	{
	
	fSDK->ComposeArrayItemPath (ns, arrayName, itemNumber, s);

	}
		
/*****************************************************************************/

void dng_xmp::ComposeStructFieldPath (const char *ns,
									  const char *structName,
									  const char *fieldNS,
									  const char *fieldName,
									  dng_string &s) const
	{
	
	fSDK->ComposeStructFieldPath (ns, structName, fieldNS, fieldName, s);

	}

/*****************************************************************************/

int32 dng_xmp::CountArrayItems (const char *ns,
							    const char *path) const
	{
	
	return fSDK->CountArrayItems (ns, path);

	}

/*****************************************************************************/

void dng_xmp::AppendArrayItem (const char *ns,
							   const char *arrayName,
							   const char *itemValue,
							   bool isBag,
							   bool propIsStruct)
	{

	fSDK->AppendArrayItem (ns,
						   arrayName,
						   itemValue,
						   isBag,
						   propIsStruct);
	}

/*****************************************************************************/

#if qDNGXMPDocOps

/*****************************************************************************/

void dng_xmp::DocOpsOpenXMP (const char *srcMIMI)
	{
	
	fSDK->DocOpsOpenXMP (srcMIMI);
	
	}

/*****************************************************************************/

void dng_xmp::DocOpsPrepareForSave (const char *srcMIMI,
									const char *dstMIMI,
									bool newPath)
	{
	
	fSDK->DocOpsPrepareForSave (srcMIMI,
								dstMIMI,
								newPath);
	
	}

/*****************************************************************************/

void dng_xmp::DocOpsUpdateMetadata (const char *srcMIMI)
	{
	
	fSDK->DocOpsUpdateMetadata (srcMIMI);
	
	}

/*****************************************************************************/

#endif

/*****************************************************************************/
