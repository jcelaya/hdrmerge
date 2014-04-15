/*****************************************************************************/
// Copyright 2006-2012 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in
// accordance with the terms of the Adobe license agreement accompanying it.
/*****************************************************************************/

/* $Id: //mondo/dng_sdk_1_4/dng_sdk/source/dng_read_image.h#2 $ */ 
/* $DateTime: 2012/06/05 11:05:39 $ */
/* $Change: 833352 $ */
/* $Author: tknoll $ */

/** \file
 * Support for DNG image reading.
 */

/*****************************************************************************/

#ifndef __dng_read_image__
#define __dng_read_image__

/*****************************************************************************/

#include "dng_auto_ptr.h"
#include "dng_classes.h"
#include "dng_image.h"
#include "dng_memory.h"
#include "dng_types.h"

/******************************************************************************/

bool DecodePackBits (dng_stream &stream,
					 uint8 *dPtr,
					 int32 dstCount);

/*****************************************************************************/

class dng_row_interleaved_image: public dng_image
	{
	
	private:
	
		dng_image &fImage;
		
		uint32 fFactor;
		
	public:
	
		dng_row_interleaved_image (dng_image &image,
								   uint32 factor);
					  
		virtual void DoGet (dng_pixel_buffer &buffer) const;
			
		virtual void DoPut (const dng_pixel_buffer &buffer);
		
	private:
	
		int32 MapRow (int32 row) const;
	
	};

/*****************************************************************************/

/// \brief
///
///

class dng_read_image
	{
	
	friend class dng_read_tiles_task;
	
	protected:
	
		enum
			{
			
			// Target size for buffer used to copy data to the image.
			
			kImageBufferSize = 128 * 1024
			
			};
			
		AutoPtr<dng_memory_block> fJPEGTables;
	
	public:
	
		dng_read_image ();
		
		virtual ~dng_read_image ();
		
		///
		/// \param 

		virtual bool CanRead (const dng_ifd &ifd);
		
		///
		/// \param host Host used for memory allocation, progress updating, and abort testing.
		/// \param ifd
		/// \param stream Stream to read image data from.
		/// \param image Result image to populate.

		virtual void Read (dng_host &host,
						   const dng_ifd &ifd,
						   dng_stream &stream,
						   dng_image &image,
						   dng_jpeg_image *jpegImage,
						   dng_fingerprint *jpegDigest);
						   
	protected:
							    
		virtual bool ReadUncompressed (dng_host &host,
									   const dng_ifd &ifd,
									   dng_stream &stream,
									   dng_image &image,
									   const dng_rect &tileArea,
									   uint32 plane,
									   uint32 planes,
									   AutoPtr<dng_memory_block> &uncompressedBuffer,
									   AutoPtr<dng_memory_block> &subTileBlockBuffer);
									   
		virtual void DecodeLossyJPEG (dng_host &host,
									  dng_image &image,
									  const dng_rect &tileArea,
									  uint32 plane,
									  uint32 planes,
									  uint32 photometricInterpretation,
									  uint32 jpegDataSize,
									  uint8 *jpegDataInMemory);
	
		virtual bool ReadBaselineJPEG (dng_host &host,
									   const dng_ifd &ifd,
									   dng_stream &stream,
									   dng_image &image,
									   const dng_rect &tileArea,
									   uint32 plane,
									   uint32 planes,
									   uint32 tileByteCount,
									   uint8 *jpegDataInMemory);
	
		virtual bool ReadLosslessJPEG (dng_host &host,
									   const dng_ifd &ifd,
									   dng_stream &stream,
									   dng_image &image,
									   const dng_rect &tileArea,
									   uint32 plane,
									   uint32 planes,
									   uint32 tileByteCount,
									   AutoPtr<dng_memory_block> &uncompressedBuffer,
									   AutoPtr<dng_memory_block> &subTileBlockBuffer);
									   
		virtual bool CanReadTile (const dng_ifd &ifd);
		
		virtual bool NeedsCompressedBuffer (const dng_ifd &ifd);
	
		virtual void ByteSwapBuffer (dng_host &host,
									 dng_pixel_buffer &buffer);

		virtual void DecodePredictor (dng_host &host,
									  const dng_ifd &ifd,
						        	  dng_pixel_buffer &buffer);

		virtual void ReadTile (dng_host &host,
							   const dng_ifd &ifd,
							   dng_stream &stream,
							   dng_image &image,
							   const dng_rect &tileArea,
							   uint32 plane,
							   uint32 planes,
							   uint32 tileByteCount,
							   AutoPtr<dng_memory_block> &compressedBuffer,
							   AutoPtr<dng_memory_block> &uncompressedBuffer,
							   AutoPtr<dng_memory_block> &subTileBlockBuffer);
	
	};

/*****************************************************************************/

#endif
	
/*****************************************************************************/
