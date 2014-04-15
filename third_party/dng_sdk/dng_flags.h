/*****************************************************************************/
// Copyright 2006-2012 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in
// accordance with the terms of the Adobe license agreement accompanying it.
/*****************************************************************************/

/* $Id: //mondo/dng_sdk_1_4/dng_sdk/source/dng_flags.h#5 $ */ 
/* $DateTime: 2012/07/31 22:04:34 $ */
/* $Change: 840853 $ */
/* $Author: tknoll $ */

/** \file
 * Conditional compilation flags for DNG SDK.
 *
 * All conditional compilation macros for the DNG SDK begin with a lowercase 'q'.
 */

/*****************************************************************************/

#ifndef __dng_flags__
#define __dng_flags__

/*****************************************************************************/

/// \def qMacOS 
/// 1 if compiling for Mac OS X.

/// \def qWinOS 
/// 1 if compiling for Windows.

// Make sure qMacOS and qWinOS are defined.

#if !defined(qMacOS) || !defined(qWinOS)
#include "RawEnvironment.h"
#endif

#if !defined(qMacOS) || !defined(qWinOS)
#error Unable to figure out platform
#endif

/*****************************************************************************/

// Platforms.

#ifndef qImagecore
#define qImagecore 0
#endif

#ifndef qiPhone
#define qiPhone 0
#endif

#ifndef qiPhoneSimulator
#define qiPhoneSimulator 0
#endif

#ifndef qAndroid
#define qAndroid 0
#endif

#ifndef qAndroidArm7
#define qAndroidArm7 0
#endif

/*****************************************************************************/

// Establish WIN32 and WIN64 definitions.

#if defined(_WIN32) && !defined(WIN32)
#define WIN32 1
#endif

#if defined(_WIN64) && !defined(WIN64)
#define WIN64 1
#endif

/*****************************************************************************/

/// \def qDNGDebug 
/// 1 if debug code is compiled in, 0 otherwise. Enables assertions and other debug
/// checks in exchange for slower processing.

// Figure out if debug build or not.

#ifndef qDNGDebug

#if defined(Debug)
#define qDNGDebug Debug

#elif defined(_DEBUG)
#define qDNGDebug _DEBUG

#else
#define qDNGDebug 0

#endif
#endif

/*****************************************************************************/

// Figure out byte order.

/// \def qDNGBigEndian 
/// 1 if this target platform is big endian (e.g. PowerPC Macintosh), else 0.
///
/// \def qDNGLittleEndian 
/// 1 if this target platform is little endian (e.g. x86 processors), else 0.

#ifndef qDNGBigEndian

#if defined(qDNGLittleEndian)
#define qDNGBigEndian !qDNGLittleEndian

#elif defined(__POWERPC__)
#define qDNGBigEndian 1

#elif defined(__INTEL__)
#define qDNGBigEndian 0

#elif defined(_M_IX86)
#define qDNGBigEndian 0

#elif defined(_M_X64) || defined(__amd64__)
#define qDNGBigEndian 0

#elif defined(__LITTLE_ENDIAN__)
#define qDNGBigEndian 0

#elif defined(__BIG_ENDIAN__)
#define qDNGBigEndian 1

#elif defined(_ARM_)
#define qDNGBigEndian 0

#else

#ifndef qXCodeRez
#error Unable to figure out byte order.
#endif

#endif
#endif

#ifndef qXCodeRez

#ifndef qDNGLittleEndian
#define qDNGLittleEndian !qDNGBigEndian
#endif

#endif

/*****************************************************************************/

/// \def qDNG64Bit 
/// 1 if this target platform uses 64-bit addresses, 0 otherwise.

#ifndef qDNG64Bit

#if qMacOS

#ifdef __LP64__
#if    __LP64__
#define qDNG64Bit 1
#endif
#endif

#elif qWinOS

#ifdef WIN64
#if    WIN64
#define qDNG64Bit 1
#endif
#endif

#endif

#ifndef qDNG64Bit
#define qDNG64Bit 0
#endif

#endif

/*****************************************************************************/

/// \def qDNGThreadSafe 
/// 1 if target platform has thread support and threadsafe libraries, 0 otherwise.

#ifndef qDNGThreadSafe
#define qDNGThreadSafe (qMacOS || qWinOS)
#endif

/*****************************************************************************/

/// \def qDNGValidateTarget 
/// 1 if dng_validate command line tool is being built, 0 otherwise.

#ifndef qDNGValidateTarget
#define qDNGValidateTarget 0
#endif

/*****************************************************************************/

/// \def qDNGValidate 
/// 1 if DNG validation code is enabled, 0 otherwise.

#ifndef qDNGValidate
#define qDNGValidate qDNGValidateTarget
#endif

/*****************************************************************************/

/// \def qDNGPrintMessages 
/// 1 if dng_show_message should use fprintf to stderr. 0 if it should use a platform
/// specific interrupt mechanism.

#ifndef qDNGPrintMessages
#define qDNGPrintMessages qDNGValidate
#endif

/*****************************************************************************/

/// \def qDNGCodec 
/// 1 to build the Windows Imaging Component Codec (e.g. for Vista).

#ifndef qDNGCodec
#define qDNGCodec 0
#endif

/*****************************************************************************/

// Experimental features -- work in progress for Lightroom 4.0 and Camera Raw 7.0.
// Turn this off for Lightroom 3.x & Camera Raw 6.x dot releases.

#ifndef qDNGExperimental
#define qDNGExperimental 1
#endif

/*****************************************************************************/

/// \def qDNGXMPFiles 
/// 1 to use XMPFiles.

#ifndef qDNGXMPFiles
#define qDNGXMPFiles 1
#endif

/*****************************************************************************/

/// \def qDNGXMPDocOps 
/// 1 to use XMPDocOps.

#ifndef qDNGXMPDocOps
#define qDNGXMPDocOps (!qDNGValidateTarget)
#endif

/*****************************************************************************/

/// \def qDNGUseLibJPEG
/// 1 to use open-source libjpeg for lossy jpeg processing.

#ifndef qDNGUseLibJPEG
#define qDNGUseLibJPEG qDNGValidateTarget
#endif

/*****************************************************************************/

#endif
	
/*****************************************************************************/
