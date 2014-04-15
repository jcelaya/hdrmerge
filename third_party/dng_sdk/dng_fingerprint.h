/*****************************************************************************/
// Copyright 2006-2007 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in
// accordance with the terms of the Adobe license agreement accompanying it.
/*****************************************************************************/

/* $Id: //mondo/dng_sdk_1_4/dng_sdk/source/dng_fingerprint.h#2 $ */ 
/* $DateTime: 2012/07/11 10:36:56 $ */
/* $Change: 838485 $ */
/* $Author: tknoll $ */

/** \file
 * Fingerprint (cryptographic hashing) support for generating strong hashes of image
 * data.
 */

/*****************************************************************************/

#ifndef __dng_fingerprint__
#define __dng_fingerprint__

/*****************************************************************************/

#include "dng_exceptions.h"
#include "dng_types.h"
#include "dng_stream.h"

#include <cstring>

/*****************************************************************************/

/// \brief Container fingerprint (MD5 only at present).

class dng_fingerprint
	{
	
	public:
	
		static const size_t kDNGFingerprintSize = 16;

		uint8 data [kDNGFingerprintSize];
		
	public:
	
		dng_fingerprint ();
		
		/// Check if fingerprint is all zeros.

		bool IsNull () const;

		/// Same as IsNull but expresses intention of testing validity.

		bool IsValid () const
			{
			return !IsNull ();
			}

		/// Set to all zeros, a value used to indicate an invalid fingerprint.

		void Clear ()
			{
			*this = dng_fingerprint ();
			}

		/// Test if two fingerprints are equal.
			
		bool operator== (const dng_fingerprint &print) const;
		
		/// Test if two fingerprints are not equal.

		bool operator!= (const dng_fingerprint &print) const
			{
			return !(*this == print);
			}
			
		/// Produce a 32-bit hash value from fingerprint used for faster hashing of
		/// fingerprints.
			
		uint32 Collapse32 () const; 

		/// Convert fingerprint to UTF-8 string.
		///
		/// \param resultStr The output array to which the UTF-8 encoding of the
		/// fingerprint will be written.

		void ToUtf8HexString (char resultStr [2 * kDNGFingerprintSize + 1]) const;

		/// Convert UTF-8 string to fingerprint. Returns true on success, false on
		/// failure.
		///
		/// \param inputStr The input array from which the UTF-8 encoding of the
		/// fingerprint will be read.
		///
		/// \retval True indicates success.

		bool FromUtf8HexString (const char inputStr [2 * kDNGFingerprintSize + 1]);

	};

/*****************************************************************************/

/// \brief Utility to compare fingerprints (e.g., for sorting).

struct dng_fingerprint_less_than
	{

	/// Less-than comparison.

	bool operator() (const dng_fingerprint &a, 
					 const dng_fingerprint &b) const
		{

		return memcmp (a.data, 
					   b.data, 
					   sizeof (a.data)) < 0;

		}

	};

/******************************************************************************/

// Derived from the RSA Data Security, Inc. MD5 Message-Digest Algorithm.

// Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
// rights reserved.
// 
// License to copy and use this software is granted provided that it
// is identified as the "RSA Data Security, Inc. MD5 Message-Digest
// Algorithm" in all material mentioning or referencing this software
// or this function.
// 
// License is also granted to make and use derivative works provided
// that such works are identified as "derived from the RSA Data
// Security, Inc. MD5 Message-Digest Algorithm" in all material
// mentioning or referencing the derived work.
// 
// RSA Data Security, Inc. makes no representations concerning either
// the merchantability of this software or the suitability of this
// software for any particular purpose. It is provided "as is"
// without express or implied warranty of any kind.
// 
// These notices must be retained in any copies of any part of this
// documentation and/or software.

/// \brief Class to hash binary data to a fingerprint using the MD5 Message-Digest
/// Algorithm.

class dng_md5_printer
	{
	
	public:
	
		dng_md5_printer ();
		
		virtual ~dng_md5_printer ()
			{
			}
		
		/// Reset the fingerprint.

		void Reset ();
		
		/// Append the data to the stream to be hashed.
		/// \param data The data to be hashed.
		/// \param inputLen The length of data, in bytes.

		void Process (const void *data,
					  uint32 inputLen);
					  
		/// Append the string to the stream to be hashed.
		/// \param text The string to be hashed.

		void Process (const char *text)
			{
			
			Process (text, (uint32) strlen (text));
			
			}
		
		/// Get the fingerprint (i.e., result of the hash).

		const dng_fingerprint & Result ();
		
	private:
	
		static void Encode (uint8 *output,
							const uint32 *input,
							uint32 len);

		static void Decode (uint32 *output,
							const uint8 *input,
							uint32 len);
							
		// F, G, H and I are basic MD5 functions.

		static inline uint32 F (uint32 x,
								uint32 y,
								uint32 z)
			{
			return (x & y) | (~x & z);
			}
			
		static inline uint32 G (uint32 x,
								uint32 y,
								uint32 z)
			{
			return (x & z) | (y & ~z);
			}
			
		static inline uint32 H (uint32 x,
								uint32 y,
								uint32 z)
			{
			return x ^ y ^ z;
			}
			
		static inline uint32 I (uint32 x,
								uint32 y,
								uint32 z)
			{
			return y ^ (x | ~z);
			}
			
		// FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
		
		static inline void FF (uint32 &a,
							   uint32 b,
							   uint32 c,
							   uint32 d,
							   uint32 x,
							   uint32 s,
							   uint32 ac)
			{
			a += F (b, c, d) + x + ac;
			a = (a << s) | (a >> (32 - s));
			a += b;
			}

		static inline void GG (uint32 &a,
							   uint32 b,
							   uint32 c,
							   uint32 d,
							   uint32 x,
							   uint32 s,
							   uint32 ac)
			{
			a += G (b, c, d) + x + ac;
			a = (a << s) | (a >> (32 - s));
			a += b;
			}

		static inline void HH (uint32 &a,
							   uint32 b,
							   uint32 c,
							   uint32 d,
							   uint32 x,
							   uint32 s,
							   uint32 ac)
			{
			a += H (b, c, d) + x + ac;
			a = (a << s) | (a >> (32 - s));
			a += b;
			}

		static inline void II (uint32 &a,
							   uint32 b,
							   uint32 c,
							   uint32 d,
							   uint32 x,
							   uint32 s,
							   uint32 ac)
			{
			a += I (b, c, d) + x + ac;
			a = (a << s) | (a >> (32 - s));
			a += b;
			}

		static void MD5Transform (uint32 state [4],
								  const uint8 block [64]);
		
	private:
	
	  	uint32 state [4];
	  	
	  	uint32 count [2];
	  	
	  	uint8 buffer [64];
		
		bool final;
		
		dng_fingerprint result;
		
	};

/*****************************************************************************/

/// \brief A dng_stream based interface to the MD5 printing logic.

class dng_md5_printer_stream : public dng_stream, dng_md5_printer
	{
	
	private:
	
		uint64 fNextOffset;

	public:

		/// Create an empty MD5 printer stream.

		dng_md5_printer_stream ()
		
			:	fNextOffset (0)
			
			{
			}

		virtual uint64 DoGetLength ()
			{
			
			return fNextOffset;
			
			}
	
		virtual void DoRead (void * /* data */,
							 uint32 /* count */,
							 uint64 /* offset */)
			{
			
			ThrowProgramError ();
			
			}
							 
		virtual void DoSetLength (uint64 length)
			{
			
			if (length != fNextOffset)
				{
				ThrowProgramError ();
				}
				
			}
							 
		virtual void DoWrite (const void *data,
							  uint32 count2,
							  uint64 offset)
			{
			
			if (offset != fNextOffset)
				{
				ThrowProgramError ();
				}
				
			Process (data, count2);
			
			fNextOffset += count2;
			
			}

		const dng_fingerprint & Result ()
			{
			
			Flush ();
			
			return dng_md5_printer::Result ();
			
			}

	};

/*****************************************************************************/

#endif
	
/*****************************************************************************/
