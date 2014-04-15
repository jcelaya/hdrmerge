/*****************************************************************************/
// Copyright 2006-2007 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in
// accordance with the terms of the Adobe license agreement accompanying it.
/*****************************************************************************/

/* $Id: //mondo/dng_sdk_1_4/dng_sdk/source/dng_exceptions.cpp#2 $ */ 
/* $DateTime: 2012/06/06 12:08:58 $ */
/* $Change: 833617 $ */
/* $Author: tknoll $ */

/*****************************************************************************/

#include "dng_exceptions.h"

#include "dng_flags.h"
#include "dng_globals.h"

/*****************************************************************************/

#ifndef qDNGReportErrors
// assuming this isn't enable on Win, because it's using printf, but an app can redirect that to console
#define qDNGReportErrors ((qDNGDebug && qMacOS) || qDNGValidate)
#endif

/*****************************************************************************/

void ReportWarning (const char *message,
				    const char *sub_message)
	{
	
	
	#if qDNGReportErrors

	if (sub_message)
		fprintf (stderr, "*** Warning: %s (%s) ***\n", message, sub_message);
	else 
		fprintf (stderr, "*** Warning: %s ***\n", message);
	
	#else

	(void) message;
	(void) sub_message;
	
	#endif
	
	}

/*****************************************************************************/

void ReportError (const char *message,
				  const char *sub_message)
	{
	
	#if qDNGReportErrors

	if (sub_message)
		fprintf (stderr, "*** Error: %s (%s) ***\n", message, sub_message);
	else 
		fprintf (stderr, "*** Error: %s ***\n", message);
	
	#else

	(void) message;
	(void) sub_message;
	
	#endif
	
	}

/*****************************************************************************/

void Throw_dng_error (dng_error_code err,
					  const char *message,
					  const char *sub_message,
					  bool silent)
	{
	
	#if qDNGReportErrors
	
		{
		
		if (!message)
			{
		
			switch (err)
				{
				
				case dng_error_none:
				case dng_error_silent:
				case dng_error_user_canceled:
					{
					break;
					}
					
				case dng_error_not_yet_implemented:
					{
					message = "Not yet implemented";
					break;
					}
					
				case dng_error_host_insufficient:
					{
					message = "Host insufficient";
					break;
					}
				
				case dng_error_memory:
					{
					message = "Unable to allocate memory";
					break;
					}
					
				case dng_error_bad_format:
					{
					message = "File format is invalid";
					break;
					}
		
				case dng_error_matrix_math:
					{
					message = "Matrix math error";
					break;
					}
		
				case dng_error_open_file:
					{
					message = "Unable to open file";
					break;
					}
					
				case dng_error_read_file:
					{
					message = "File read error";
					break;
					}
					
				case dng_error_write_file:
					{
					message = "File write error";
					break;
					}
					
				case dng_error_end_of_file:
					{
					message = "Unexpected end-of-file";
					break;
					}
					
				case dng_error_file_is_damaged:
					{
					message = "File is damaged";
					break;
					}
					
				case dng_error_image_too_big_dng:
					{
					message = "Image is too big to save as DNG";
					break;
					}
					
				case dng_error_image_too_big_tiff:
					{
					message = "Image is too big to save as TIFF";
					break;
					}
					
				case dng_error_unsupported_dng:
					{
					message = "DNG version is unsupported";
					break;
					}
					
				default:
					{
					message = "Unknown error";
					break;
					}
					
				}
				
			}
			
		if (message && !silent)
			{
			ReportError (message, sub_message);
			}
		
		}
		
	#else
	
	(void) message;
	(void) sub_message;
	(void) silent;
	
	#endif
	
	throw dng_exception (err);
	
	}

/*****************************************************************************/
