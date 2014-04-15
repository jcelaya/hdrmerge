/*****************************************************************************/
// Copyright 2006-2012 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in
// accordance with the terms of the Adobe license agreement accompanying it.
/*****************************************************************************/

/* $Id: //mondo/dng_sdk_1_4/dng_sdk/source/dng_read_image.cpp#7 $ */ 
/* $DateTime: 2012/07/31 22:04:34 $ */
/* $Change: 840853 $ */
/* $Author: tknoll $ */

/*****************************************************************************/

#include "dng_read_image.h"

#include "dng_abort_sniffer.h"
#include "dng_area_task.h"
#include "dng_bottlenecks.h"
#include "dng_exceptions.h"
#include "dng_flags.h"
#include "dng_host.h"
#include "dng_image.h"
#include "dng_ifd.h"
#include "dng_jpeg_image.h"
#include "dng_lossless_jpeg.h"
#include "dng_mutex.h"
#include "dng_memory.h"
#include "dng_pixel_buffer.h"
#include "dng_tag_types.h"
#include "dng_tag_values.h"
#include "dng_utils.h"

#include "zlib.h"

#if qDNGUseLibJPEG
#include "jpeglib.h"
#include "jerror.h"
#endif
	
/******************************************************************************/

static void DecodeDelta8 (uint8 *dPtr,
						  uint32 rows,
						  uint32 cols,
						  uint32 channels)
	{
	
	const uint32 dRowStep = cols * channels;
	
	for (uint32 row = 0; row < rows; row++)
		{
		
		for (uint32 col = 1; col < cols; col++)
			{
			
			for (uint32 channel = 0; channel < channels; channel++)
				{
				
				dPtr [col * channels + channel] += dPtr [(col - 1) * channels + channel];
				
				}
			
			}
		
		dPtr += dRowStep;
		
		}

	}

/******************************************************************************/

static void DecodeDelta16 (uint16 *dPtr,
						   uint32 rows,
						   uint32 cols,
						   uint32 channels)
	{
	
	const uint32 dRowStep = cols * channels;
	
	for (uint32 row = 0; row < rows; row++)
		{
		
		for (uint32 col = 1; col < cols; col++)
			{
			
			for (uint32 channel = 0; channel < channels; channel++)
				{
				
				dPtr [col * channels + channel] += dPtr [(col - 1) * channels + channel];
				
				}
			
			}
		
		dPtr += dRowStep;
		
		}

	}
	
/******************************************************************************/

static void DecodeDelta32 (uint32 *dPtr,
						   uint32 rows,
						   uint32 cols,
						   uint32 channels)
	{
	
	const uint32 dRowStep = cols * channels;
	
	for (uint32 row = 0; row < rows; row++)
		{
		
		for (uint32 col = 1; col < cols; col++)
			{
			
			for (uint32 channel = 0; channel < channels; channel++)
				{
				
				dPtr [col * channels + channel] += dPtr [(col - 1) * channels + channel];
				
				}
			
			}
		
		dPtr += dRowStep;
		
		}

	}
	
/*****************************************************************************/

inline void DecodeDeltaBytes (uint8 *bytePtr, int32 cols, int32 channels)
	{
	
	if (channels == 1)
		{
		
		uint8 b0 = bytePtr [0];
		
		bytePtr += 1;
		
		for (int32 col = 1; col < cols; ++col)
			{
			
			b0 += bytePtr [0];
			
			bytePtr [0] = b0;
			
			bytePtr += 1;

			}
			
		}
	
	else if (channels == 3)
		{
		
		uint8 b0 = bytePtr [0];
		uint8 b1 = bytePtr [1];
		uint8 b2 = bytePtr [2];
		
		bytePtr += 3;
		
		for (int32 col = 1; col < cols; ++col)
			{
			
			b0 += bytePtr [0];
			b1 += bytePtr [1];
			b2 += bytePtr [2];
			
			bytePtr [0] = b0;
			bytePtr [1] = b1;
			bytePtr [2] = b2;
			
			bytePtr += 3;

			}
			
		}
		
	else if (channels == 4)
		{
		
		uint8 b0 = bytePtr [0];
		uint8 b1 = bytePtr [1];
		uint8 b2 = bytePtr [2];
		uint8 b3 = bytePtr [3];
		
		bytePtr += 4;
		
		for (int32 col = 1; col < cols; ++col)
			{
			
			b0 += bytePtr [0];
			b1 += bytePtr [1];
			b2 += bytePtr [2];
			b3 += bytePtr [3];
			
			bytePtr [0] = b0;
			bytePtr [1] = b1;
			bytePtr [2] = b2;
			bytePtr [3] = b3;
			
			bytePtr += 4;

			}
			
		}
		
	else
		{
		
		for (int32 col = 1; col < cols; ++col)
			{
			
			for (int32 chan = 0; chan < channels; ++chan)
				{
				
				bytePtr [chan + channels] += bytePtr [chan];
				
				}
				
			bytePtr += channels;

			}
			
		}

	}
						    
/*****************************************************************************/

static void DecodeFPDelta (uint8 *input,
						   uint8 *output,
						   int32 cols,
						   int32 channels,
						   int32 bytesPerSample)
	{
	
	DecodeDeltaBytes (input, cols * bytesPerSample, channels);
	
	int32 rowIncrement = cols * channels;
	
	if (bytesPerSample == 2)
		{
		
		#if qDNGBigEndian
		const uint8 *input0 = input;
		const uint8 *input1 = input + rowIncrement;
		#else
		const uint8 *input1 = input;
		const uint8 *input0 = input + rowIncrement;
		#endif
		
		for (int32 col = 0; col < rowIncrement; ++col)
			{
			
			output [0] = input0 [col];
			output [1] = input1 [col];
			
			output += 2;
				
			}
			
		}
		
	else if (bytesPerSample == 3)
		{
		
		const uint8 *input0 = input;
		const uint8 *input1 = input + rowIncrement;
		const uint8 *input2 = input + rowIncrement * 2;
		
		for (int32 col = 0; col < rowIncrement; ++col)
			{
			
			output [0] = input0 [col];
			output [1] = input1 [col];
			output [2] = input2 [col];
			
			output += 3;
				
			}
			
		}
		
	else
		{
		
		#if qDNGBigEndian
		const uint8 *input0 = input;
		const uint8 *input1 = input + rowIncrement;
		const uint8 *input2 = input + rowIncrement * 2;
		const uint8 *input3 = input + rowIncrement * 3;
		#else
		const uint8 *input3 = input;
		const uint8 *input2 = input + rowIncrement;
		const uint8 *input1 = input + rowIncrement * 2;
		const uint8 *input0 = input + rowIncrement * 3;
		#endif
		
		for (int32 col = 0; col < rowIncrement; ++col)
			{
			
			output [0] = input0 [col];
			output [1] = input1 [col];
			output [2] = input2 [col];
			output [3] = input3 [col];
			
			output += 4;
				
			}
			
		}
		
	}	
					    
/*****************************************************************************/

bool DecodePackBits (dng_stream &stream,
					 uint8 *dPtr,
					 int32 dstCount)
	{

	while (dstCount > 0)
		{
		
		int32 runCount = (int8) stream.Get_uint8 ();

		if (runCount >= 0)
			{
			
			++runCount;
			
			dstCount -= runCount;

			if (dstCount < 0)
				return false;
				
			stream.Get (dPtr, runCount);
			
			dPtr += runCount;
			
			}
			
		else
			{
			
			runCount = -runCount + 1;

			dstCount -= runCount;

			if (dstCount < 0)
				return false;
				
			uint8 x = stream.Get_uint8 ();
			
			while (runCount--)
				{
				
				*(dPtr++) = x;
				
				}

			}
			
		}

	return true;

	}

/******************************************************************************/

class dng_lzw_expander
	{
	
	private:
	
		enum
			{
			kResetCode = 256,
			kEndCode   = 257,
			kTableSize = 4096
			};
		
		struct LZWExpanderNode
			{
			int16 prefix;
			int16 final;
			int16 depth;
			int16 fake_for_padding;
			};

		dng_memory_data fBuffer;

		LZWExpanderNode *fTable;
		
		const uint8 *fSrcPtr;
		
		int32 fSrcCount;
		
		int32 fByteOffset;

		uint32 fBitBuffer;
		int32 fBitBufferCount;
		
		int32 fNextCode;
		
		int32 fCodeSize;
		
	public:
	
		dng_lzw_expander ();

		bool Expand (const uint8 *sPtr,
					 uint8 *dPtr,
					 int32 sCount,
					 int32 dCount);
	
	private:
	
		void InitTable ();
	
		void AddTable (int32 w, int32 k);
		
		bool GetCodeWord (int32 &code);

		// Hidden copy constructor and assignment operator.
	
		dng_lzw_expander (const dng_lzw_expander &expander);
		
		dng_lzw_expander & operator= (const dng_lzw_expander &expander);

	};

/******************************************************************************/

dng_lzw_expander::dng_lzw_expander ()

	:	fBuffer			 ()
	,	fTable			 (NULL)
	,	fSrcPtr			 (NULL)
	,	fSrcCount		 (0)
	,	fByteOffset		 (0)
	,	fBitBuffer		 (0)
	,	fBitBufferCount  (0)
	,	fNextCode		 (0)
	,	fCodeSize		 (0)
	
	{
	
	fBuffer.Allocate (kTableSize * sizeof (LZWExpanderNode));
	
	fTable = (LZWExpanderNode *) fBuffer.Buffer ();

	}

/******************************************************************************/

void dng_lzw_expander::InitTable ()
	{

	fCodeSize = 9;

	fNextCode = 258;
		
	LZWExpanderNode *node = &fTable [0];
	
	for (int32 code = 0; code < 256; code++)
		{
		
		node->prefix  = -1;
		node->final   = (int16) code;
		node->depth   = 1;
		
		node++;
		
		}
		
	}

/******************************************************************************/

void dng_lzw_expander::AddTable (int32 w, int32 k)
	{
	
	DNG_ASSERT ((w >= 0) && (w <= kTableSize),
				"bad w value in dng_lzw_expander::AddTable");

	LZWExpanderNode *parentNode = &fTable [w];
	
	int32 nextCode = fNextCode;
	
	fNextCode++;
	
	DNG_ASSERT ((nextCode >= 0) && (nextCode <= kTableSize),
				"bad fNextCode value in dng_lzw_expander::AddTable");
	
	LZWExpanderNode *node = &fTable [nextCode];
	
	node->prefix  = (int16) w;
	node->final   = (int16) k;
	node->depth   = 1 + parentNode->depth;
	
	if (nextCode + 1 == (1 << fCodeSize) - 1)
		{
		if (fCodeSize != 12)
			fCodeSize++;
		}
		
	}

/******************************************************************************/

bool dng_lzw_expander::GetCodeWord (int32 &code)
	{

	// The bit buffer has the current code in the most significant bits,
	// so shift off the low orders.
	
	int32 codeSize = fCodeSize;

	code = fBitBuffer >> (32 - codeSize);

	if (fBitBufferCount >= codeSize)
		{
		
		// Typical case; get the code from the bit buffer.

		fBitBuffer     <<= codeSize;
		fBitBufferCount -= codeSize;
		
		}

	else
		{
		
		// The buffer needs to be refreshed.

		const int32 bitsSoFar = fBitBufferCount;

		if (fByteOffset >= fSrcCount)
			return false;

		// Buffer a long word
		
		const uint8 *ptr = fSrcPtr + fByteOffset;

		#if qDNGBigEndian

		fBitBuffer = *((const uint32 *) ptr);

		#else
		
			{
			
			uint32 b0 = ptr [0];
			uint32 b1 = ptr [1];
			uint32 b2 = ptr [2];
			uint32 b3 = ptr [3];
			
			fBitBuffer = (((((b0 << 8) | b1) << 8) | b2) << 8) | b3;
			
			}

		#endif

		fBitBufferCount = 32;
		
		fByteOffset += 4;

		// Number of additional bits we need
		
		const int32 bitsUsed = codeSize - bitsSoFar;

		// Number of low order bits in the current buffer we don't care about
		
		const int32 bitsNotUsed = 32 - bitsUsed;

		code |= fBitBuffer >> bitsNotUsed;

		fBitBuffer     <<= bitsUsed;
		fBitBufferCount -= bitsUsed;

		}

	return true;

	}

/******************************************************************************/

bool dng_lzw_expander::Expand (const uint8 *sPtr,
						       uint8 *dPtr,
						       int32 sCount,
						       int32 dCount)
	{
	
	void *dStartPtr = dPtr;
	
	fSrcPtr = sPtr;
	
	fSrcCount = sCount;
	
	fByteOffset = 0;
	
	/* the master decode loop */
	
	while (true)
		{
		
		InitTable ();
			
		int32 code;
			
		do
			{
			
			if (!GetCodeWord (code)) 
				return false;
				
			DNG_ASSERT (code <= fNextCode,
						"Unexpected LZW code in dng_lzw_expander::Expand");
			
			}
		while (code == kResetCode);
		
		if (code == kEndCode) 
			return true;
		
		if (code > kEndCode) 
			return false;
		
		int32 oldCode = code;
		int32 inChar  = code;
		
		*(dPtr++) = (uint8) code;
		
		if (--dCount == 0) 
			return true;
		
		while (true)
			{
			
			if (!GetCodeWord (code)) 
				return false;

			if (code == kResetCode) 
				break;
			
			if (code == kEndCode) 
				return true;
			
			const int32 inCode = code;

			bool repeatLastPixel = false;
			
			if (code >= fNextCode)
				{
					
				// This is either a bad file or our code table is not big enough; we
				// are going to repeat the last code seen and attempt to muddle thru.
				
				code = oldCode;	

				repeatLastPixel = true;

				}
			
			// this can only happen if we hit 2 bad codes in a row
			
			if (code > fNextCode)
				return false;

			const int32 depth = fTable [code].depth;

			if (depth < dCount)
				{

				dCount -= depth;
				
				dPtr += depth;

				uint8 *ptr = dPtr;
				
				// give the compiler an extra hint to optimize these as registers
				
				const LZWExpanderNode *localTable = fTable;
				
				int32 localCode = code;
				
				// this is usually the hottest loop in LZW expansion
				
				while (localCode >= kResetCode)
					{
					
					if (ptr <= dStartPtr)
						return false;		// about to trash memory

					const LZWExpanderNode &node = localTable [localCode];
					
					uint8 tempFinal = (uint8) node.final;
					
					localCode = node.prefix;

					// Check for bogus table entry

					if (localCode < 0 || localCode > kTableSize)
						return false;

					*(--ptr) = tempFinal;

					}
				
				code = localCode;

				inChar = localCode;

				if (ptr <= dStartPtr)
					return false;		// about to trash memory

				*(--ptr) = (uint8) inChar;

				}
		
			else
				{
				
				// There might not be enough room for the full code
				// so skip the end of it.
				
				const int32 skip = depth - dCount;

				for (int32 i = 0; i < skip ; i++)
					{
					const LZWExpanderNode &node = fTable [code];
					code = node.prefix;
					}

				int32 depthUsed = depth - skip;

				dCount -= depthUsed;
				
				dPtr += depthUsed;

				uint8 *ptr = dPtr;

				while (code >= 0)
					{
					
					if (ptr <= dStartPtr)
						return false;		// about to trash memory

					const LZWExpanderNode &node = fTable [code];

					*(--ptr) = (uint8) node.final;

					code = node.prefix;
					
					// Check for bogus table entry

					if (code > kTableSize)
						return false;

					}

				return true;

				}

			if (repeatLastPixel)
				{
				
				*(dPtr++) = (uint8) inChar;
				
				if (--dCount == 0)
					return true;
	
				}
				
			if (fNextCode < kTableSize)
				{
				
				AddTable (oldCode, code);
				
				}
			
			oldCode = inCode;
			
			}
			
		}	

	return false;
	
	}

/*****************************************************************************/

dng_row_interleaved_image::dng_row_interleaved_image (dng_image &image,
													  uint32 factor)
													  
	:	dng_image (image.Bounds    (),
				   image.Planes    (),
				   image.PixelType ())
													  
	,	fImage  (image )
	,	fFactor (factor)
	
	{
	
	}
	
/*****************************************************************************/

int32 dng_row_interleaved_image::MapRow (int32 row) const
	{

	uint32 rows = Height ();
	
	int32 top = Bounds ().t;
	
	uint32 fieldRow = row - top;
	
	for (uint32 field = 0; true; field++)
		{
		
		uint32 fieldRows = (rows - field + fFactor - 1) / fFactor;
		
		if (fieldRow < fieldRows)
			{
			
			return fieldRow * fFactor + field + top;
			
			}
			
		fieldRow -= fieldRows;
		
		}
		
	ThrowProgramError ();
		
	return 0;
	
	}
	
/*****************************************************************************/

void dng_row_interleaved_image::DoGet (dng_pixel_buffer &buffer) const
	{
	
	dng_pixel_buffer tempBuffer (buffer);
	
	for (int32 row = buffer.fArea.t; row < buffer.fArea.b; row++)
		{
				
		tempBuffer.fArea.t = MapRow (row);
		
		tempBuffer.fArea.b = tempBuffer.fArea.t + 1;
		
		tempBuffer.fData = (void *) buffer.DirtyPixel (row,
										 			   buffer.fArea.l,
										 			   buffer.fPlane);
										 
		fImage.Get (tempBuffer);
		
		}
		
	}
	
/*****************************************************************************/

void dng_row_interleaved_image::DoPut (const dng_pixel_buffer &buffer)
	{
	
	dng_pixel_buffer tempBuffer (buffer);
	
	for (int32 row = buffer.fArea.t; row < buffer.fArea.b; row++)
		{
				
		tempBuffer.fArea.t = MapRow (row);
		
		tempBuffer.fArea.b = tempBuffer.fArea.t + 1;
		
		tempBuffer.fData = (void *) buffer.ConstPixel (row,
										 			   buffer.fArea.l,
										 			   buffer.fPlane);
										 
		fImage.Put (tempBuffer);
		
		}
		
	}
	
/*****************************************************************************/

static void ReorderSubTileBlocks (dng_host &host,
								  const dng_ifd &ifd,
								  dng_pixel_buffer &buffer,
								  AutoPtr<dng_memory_block> &tempBuffer)
	{
	
	uint32 tempBufferSize = buffer.fArea.W () *
							buffer.fArea.H () *
							buffer.fPlanes *
							buffer.fPixelSize;
							
	if (!tempBuffer.Get () || tempBuffer->LogicalSize () < tempBufferSize)
		{
		
		tempBuffer.Reset (host.Allocate (tempBufferSize));
		
		}
	
	uint32 blockRows = ifd.fSubTileBlockRows;
	uint32 blockCols = ifd.fSubTileBlockCols;
	
	uint32 rowBlocks = buffer.fArea.H () / blockRows;
	uint32 colBlocks = buffer.fArea.W () / blockCols;
	
	int32 rowStep = buffer.fRowStep * buffer.fPixelSize;
	int32 colStep = buffer.fColStep * buffer.fPixelSize;
	
	int32 rowBlockStep = rowStep * blockRows;
	int32 colBlockStep = colStep * blockCols;
	
	uint32 blockColBytes = blockCols * buffer.fPlanes * buffer.fPixelSize;
	
	const uint8 *s0 = (const uint8 *) buffer.fData;
	      uint8 *d0 = tempBuffer->Buffer_uint8 ();
	
	for (uint32 rowBlock = 0; rowBlock < rowBlocks; rowBlock++)
		{
		
		uint8 *d1 = d0;
		
		for (uint32 colBlock = 0; colBlock < colBlocks; colBlock++)
			{
			
			uint8 *d2 = d1;
			
			for (uint32 blockRow = 0; blockRow < blockRows; blockRow++)
				{
				
				for (uint32 j = 0; j < blockColBytes; j++)
					{
					
					d2 [j] = s0 [j];
					
					}
					
				s0 += blockColBytes;
				
				d2 += rowStep;
				
				}
			
			d1 += colBlockStep;
			
			}
			
		d0 += rowBlockStep;
		
		}
		
	// Copy back reordered pixels.
		
	DoCopyBytes (tempBuffer->Buffer (),
				 buffer.fData,
				 tempBufferSize);
	
	}

/*****************************************************************************/

class dng_image_spooler: public dng_spooler
	{
	
	private:
	
		dng_host &fHost;
		
		const dng_ifd &fIFD;
	
		dng_image &fImage;
	
		dng_rect fTileArea;
		
		uint32 fPlane;
		uint32 fPlanes;
		
		dng_memory_block &fBlock;
		
		AutoPtr<dng_memory_block> &fSubTileBuffer;
		
		dng_rect fTileStrip;
		
		uint8 *fBuffer;
		
		uint32 fBufferCount;
		uint32 fBufferSize;
		
	public:
	
		dng_image_spooler (dng_host &host,
						   const dng_ifd &ifd,
						   dng_image &image,
						   const dng_rect &tileArea,
						   uint32 plane,
						   uint32 planes,
						   dng_memory_block &block,
						   AutoPtr<dng_memory_block> &subTileBuffer);
		
		virtual ~dng_image_spooler ();
			
		virtual void Spool (const void *data,
							uint32 count);
							
	private:
	
		// Hidden copy constructor and assignment operator.
	
		dng_image_spooler (const dng_image_spooler &spooler);
		
		dng_image_spooler & operator= (const dng_image_spooler &spooler);
	
	};

/*****************************************************************************/

dng_image_spooler::dng_image_spooler (dng_host &host,
									  const dng_ifd &ifd,
									  dng_image &image,
									  const dng_rect &tileArea,
									  uint32 plane,
									  uint32 planes,
									  dng_memory_block &block,
									  AutoPtr<dng_memory_block> &subTileBuffer)

	:	fHost (host)
	,	fIFD (ifd)
	,	fImage (image)
	,	fTileArea (tileArea)
	,	fPlane (plane)
	,	fPlanes (planes)
	,	fBlock (block)
	,	fSubTileBuffer (subTileBuffer)
	
	,	fTileStrip ()
	,	fBuffer (NULL)
	,	fBufferCount (0)
	,	fBufferSize (0)
	
	{
	
	uint32 bytesPerRow = fTileArea.W () * fPlanes * (uint32) sizeof (uint16);
	
	uint32 stripLength = Pin_uint32 (ifd.fSubTileBlockRows,
									 fBlock.LogicalSize () / bytesPerRow,
									 fTileArea.H ());
									
	stripLength = stripLength / ifd.fSubTileBlockRows
							  * ifd.fSubTileBlockRows;
	
	fTileStrip   = fTileArea;
	fTileStrip.b = fTileArea.t + stripLength;
	
	fBuffer = (uint8 *) fBlock.Buffer ();
	
	fBufferCount = 0;
	fBufferSize  = bytesPerRow * stripLength;
				  
	}

/*****************************************************************************/

dng_image_spooler::~dng_image_spooler ()
	{
	
	}

/*****************************************************************************/

void dng_image_spooler::Spool (const void *data,
							   uint32 count)
	{
	
	while (count)
		{
		
		uint32 block = Min_uint32 (count, fBufferSize - fBufferCount);
		
		if (block == 0)
			{
			return;
			}
		
		DoCopyBytes (data,
					 fBuffer + fBufferCount,
				     block);
				
		data = ((const uint8 *) data) + block;
		
		count -= block;
		
		fBufferCount += block;
		
		if (fBufferCount == fBufferSize)
			{
			
			fHost.SniffForAbort ();
			
			dng_pixel_buffer buffer;
			
			buffer.fArea = fTileStrip;
			
			buffer.fPlane  = fPlane;
			buffer.fPlanes = fPlanes;
			
			buffer.fRowStep   = fPlanes * fTileStrip.W ();
			buffer.fColStep   = fPlanes;
			buffer.fPlaneStep = 1;
			
			buffer.fData = fBuffer;
			
			buffer.fPixelType = ttShort;
			buffer.fPixelSize = 2;
			
			if (fIFD.fSubTileBlockRows > 1)
				{
			
				ReorderSubTileBlocks (fHost,
									  fIFD,
									  buffer,
									  fSubTileBuffer);
									  
				}

			fImage.Put (buffer);
			
			uint32 stripLength = fTileStrip.H ();
			
			fTileStrip.t = fTileStrip.b;
			
			fTileStrip.b = Min_int32 (fTileStrip.t + stripLength,
									  fTileArea.b);
			
			fBufferCount = 0;
			
			fBufferSize = fTileStrip.W () *
						  fTileStrip.H () *
						  fPlanes * (uint32) sizeof (uint16);
	
			}

		}
	
	}

/*****************************************************************************/

dng_read_image::dng_read_image ()

	:	fJPEGTables ()
	
	{
	
	}

/*****************************************************************************/

dng_read_image::~dng_read_image ()
	{
	
	}

/*****************************************************************************/

bool dng_read_image::ReadUncompressed (dng_host &host,
									   const dng_ifd &ifd,
									   dng_stream &stream,
									   dng_image &image,
									   const dng_rect &tileArea,
									   uint32 plane,
									   uint32 planes,
									   AutoPtr<dng_memory_block> &uncompressedBuffer,
									   AutoPtr<dng_memory_block> &subTileBlockBuffer)
	{
	
	uint32 rows          = tileArea.H ();
	uint32 samplesPerRow = tileArea.W ();
	
	if (ifd.fPlanarConfiguration == pcRowInterleaved)
		{
		rows *= planes;
		}
	else
		{
		samplesPerRow *= planes;
		}
	
	uint32 samplesPerTile = samplesPerRow * rows;
	
	dng_pixel_buffer buffer;
	
	buffer.fArea = tileArea;
	
	buffer.fPlane  = plane;
	buffer.fPlanes = planes;
	
	buffer.fRowStep = planes * tileArea.W ();
	
	if (ifd.fPlanarConfiguration == pcRowInterleaved)
		{
		buffer.fColStep   = 1;
		buffer.fPlaneStep = tileArea.W ();
		}
		
	else
		{
		buffer.fColStep   = planes;
		buffer.fPlaneStep = 1;
		}
		
	if (uncompressedBuffer.Get () == NULL)
		{

		#if qDNGValidate
		
		ReportError ("Fuzz: Missing uncompressed buffer");
		
		#endif

		ThrowBadFormat ();
		
		}
	
	buffer.fData = uncompressedBuffer->Buffer ();
	
	uint32 bitDepth = ifd.fBitsPerSample [plane];
	
	if (bitDepth == 8)
		{
		
		buffer.fPixelType = ttByte;
		buffer.fPixelSize = 1;
				
		stream.Get (buffer.fData, samplesPerTile);
		
		}
		
	else if (bitDepth == 16 && ifd.fSampleFormat [0] == sfFloatingPoint)
		{
		
		buffer.fPixelType = ttFloat;
		buffer.fPixelSize = 4;
		
		uint32 *p_uint32 = (uint32 *) buffer.fData;
			
		for (uint32 j = 0; j < samplesPerTile; j++)
			{
			
			p_uint32 [j] = DNG_HalfToFloat (stream.Get_uint16 ());
											
			}
		
		}
	
	else if (bitDepth == 24 && ifd.fSampleFormat [0] == sfFloatingPoint)
		{
		
		buffer.fPixelType = ttFloat;
		buffer.fPixelSize = 4;
		
		uint32 *p_uint32 = (uint32 *) buffer.fData;
			
		for (uint32 j = 0; j < samplesPerTile; j++)
			{
			
			uint8 input [3];
			
			if (stream.LittleEndian ())
				{
				input [2] = stream.Get_uint8 ();
				input [1] = stream.Get_uint8 ();
				input [0] = stream.Get_uint8 ();
				}
				
			else
				{
				input [0] = stream.Get_uint8 ();
				input [1] = stream.Get_uint8 ();
				input [2] = stream.Get_uint8 ();
				}
			
			p_uint32 [j] = DNG_FP24ToFloat (input);
											
			}
		
		}
	
	else if (bitDepth == 16)
		{
		
		buffer.fPixelType = ttShort;
		buffer.fPixelSize = 2;
		
		stream.Get (buffer.fData, samplesPerTile * 2);
		
		if (stream.SwapBytes ())
			{
			
			DoSwapBytes16 ((uint16 *) buffer.fData,
						   samplesPerTile);
						
			}
				
		}
		
	else if (bitDepth == 32)
		{
		
		buffer.fPixelType = image.PixelType ();
		buffer.fPixelSize = 4;
		
		stream.Get (buffer.fData, samplesPerTile * 4);
		
		if (stream.SwapBytes ())
			{
			
			DoSwapBytes32 ((uint32 *) buffer.fData,
						   samplesPerTile);
						
			}
				
		}
		
	else if (bitDepth == 12)
		{
		
		buffer.fPixelType = ttShort;
		buffer.fPixelSize = 2;
		
		uint16 *p = (uint16 *) buffer.fData;
			
		uint32 evenSamples = samplesPerRow >> 1;
		
		for (uint32 row = 0; row < rows; row++)
			{
			
			for (uint32 j = 0; j < evenSamples; j++)
				{
				
				uint32 b0 = stream.Get_uint8 ();
				uint32 b1 = stream.Get_uint8 ();
				uint32 b2 = stream.Get_uint8 ();
								
				p [0] = (uint16) ((b0 << 4) | (b1 >> 4));
				p [1] = (uint16) (((b1 << 8) | b2) & 0x0FFF);
				
				p += 2;
				
				}
				
			if (samplesPerRow & 1)
				{
				
				uint32 b0 = stream.Get_uint8 ();
				uint32 b1 = stream.Get_uint8 ();
				
				p [0] = (uint16) ((b0 << 4) | (b1 >> 4));
				
				p += 1;
				
				}
			
			}
		
		}
		
	else if (bitDepth > 8 && bitDepth < 16)
		{
		
		buffer.fPixelType = ttShort;
		buffer.fPixelSize = 2;
		
		uint16 *p = (uint16 *) buffer.fData;
			
		uint32 bitMask = (1 << bitDepth) - 1;
							
		for (uint32 row = 0; row < rows; row++)
			{
			
			uint32 bitBuffer  = 0;
			uint32 bufferBits = 0;
			
			for (uint32 j = 0; j < samplesPerRow; j++)
				{
				
				while (bufferBits < bitDepth)
					{
					
					bitBuffer = (bitBuffer << 8) | stream.Get_uint8 ();
					
					bufferBits += 8;
					
					}
									
				p [j] = (uint16) ((bitBuffer >> (bufferBits - bitDepth)) & bitMask);
				
				bufferBits -= bitDepth;
				
				}
				
			p += samplesPerRow;
			
			}
			
		}
		
	else if (bitDepth > 16 && bitDepth < 32)
		{
		
		buffer.fPixelType = ttLong;
		buffer.fPixelSize = 4;
		
		uint32 *p = (uint32 *) buffer.fData;
			
		uint32 bitMask = (1 << bitDepth) - 1;
							
		for (uint32 row = 0; row < rows; row++)
			{
			
			uint64 bitBuffer  = 0;
			uint32 bufferBits = 0;
			
			for (uint32 j = 0; j < samplesPerRow; j++)
				{
				
				while (bufferBits < bitDepth)
					{
					
					bitBuffer = (bitBuffer << 8) | stream.Get_uint8 ();
					
					bufferBits += 8;
					
					}
									
				p [j] = ((uint32) (bitBuffer >> (bufferBits - bitDepth))) & bitMask;
				
				bufferBits -= bitDepth;
				
				}
				
			p += samplesPerRow;
			
			}
			
		}
		
	else
		{
		
		return false;
		
		}
		
	if (ifd.fSampleBitShift)
		{
		
		buffer.ShiftRight (ifd.fSampleBitShift);
		
		}
		
	if (ifd.fSubTileBlockRows > 1)
		{
		
		ReorderSubTileBlocks (host,
							  ifd,
							  buffer,
							  subTileBlockBuffer);
		
		}
		
	image.Put (buffer);
	
	return true;
		
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

#endif

/*****************************************************************************/

void dng_read_image::DecodeLossyJPEG (dng_host &host,
									  dng_image &image,
									  const dng_rect &tileArea,
									  uint32 plane,
									  uint32 planes,
									  uint32 /* photometricInterpretation */,
									  uint32 jpegDataSize,
									  uint8 *jpegDataInMemory)
	{
	
	#if qDNGUseLibJPEG
	
	struct jpeg_decompress_struct cinfo;
	
	// Setup the error manager.
	
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error (&jerr);
	
	jerr.error_exit     = dng_error_exit;
	jerr.output_message = dng_output_message;
	
	try
		{
		
		// Create the decompression context.

		jpeg_create_decompress (&cinfo);
		
		// Setup the data source.
		
		jpeg_mem_src (&cinfo,
					  jpegDataInMemory,
					  jpegDataSize);
			
		// Read the JPEG header.
			
		jpeg_read_header (&cinfo, TRUE);
		
		// Check header.
		
		if ((uint32) cinfo.image_width	  != tileArea.W () ||
			(uint32) cinfo.image_height	  != tileArea.H () ||
			(uint32) cinfo.num_components != planes        )
			{
			ThrowBadFormat ();
			}
			
		// Start the compression.
		
		jpeg_start_decompress (&cinfo);
		
		// Setup a one-scanline size buffer.
		
		dng_pixel_buffer buffer;
		
		buffer.fArea   = tileArea;
		buffer.fArea.b = tileArea.t + 1;
		buffer.fPlane  = plane;
		buffer.fPlanes = planes;
		
		buffer.fColStep   = planes;
		buffer.fRowStep   = tileArea.W () * planes;
		buffer.fPlaneStep = 1;
		
		buffer.fPixelType = ttByte;
		buffer.fPixelSize = 1;
		
		buffer.fDirty = true;
		
		AutoPtr<dng_memory_block> bufferData (host.Allocate (buffer.fRowStep));
		
		buffer.fData = bufferData->Buffer ();
		
		uint8 *sampArray [1];
		
		sampArray [0] = bufferData->Buffer_uint8 ();
		
		// Read each scanline and save to image.
			
		while (buffer.fArea.t < tileArea.b)
			{
			
			jpeg_read_scanlines (&cinfo, sampArray, 1);
			
			image.Put (buffer);
			
			buffer.fArea.t = buffer.fArea.b;
			buffer.fArea.b = buffer.fArea.t + 1;
			
			}
			
		// Cleanup.
			
		jpeg_finish_decompress (&cinfo);

		jpeg_destroy_decompress (&cinfo);
			
		}
		
	catch (...)
		{
		
		jpeg_destroy_decompress (&cinfo);
		
		throw;
		
		}
	
	#else
				
	// The dng_sdk does not include a lossy JPEG decoder.  Override this
	// this method to add lossy JPEG support.
	
	(void) host;
	(void) image;
	(void) tileArea;
	(void) plane;
	(void) planes;
	(void) jpegDataSize;
	(void) jpegDataInMemory;
	
	ThrowProgramError ("Missing lossy JPEG decoder");
	
	#endif
	
	}
	
/*****************************************************************************/

static dng_memory_block * ReadJPEGDataToBlock (dng_host &host,
											   dng_stream &stream,
											   dng_memory_block *tablesBlock,
											   uint64 tileOffset,
											   uint32 tileByteCount,
											   bool patchFirstByte)
	{
	
	if (tileByteCount <= 2)
		{
		ThrowEndOfFile ();
		}
		
	uint32 tablesByteCount = tablesBlock ? tablesBlock->LogicalSize () : 0;
	
	if (tablesByteCount && tablesByteCount < 4)
		{
		ThrowEndOfFile ();
		}
		
	// The JPEG tables start with a two byte SOI marker, and
	// and end with a two byte EOI marker.  The JPEG tile
	// data also starts with a two byte SOI marker.  We can
	// convert this combination a normal JPEG stream removing
	// the last two bytes of the JPEG tables and the first two
	// bytes of the tile data, and then concatenating them.

	if (tablesByteCount)
		{
		
		tablesByteCount -= 2;
		
		tileOffset    += 2;
		tileByteCount -= 2;
		
		}
		
	// Allocate buffer.
	
	AutoPtr<dng_memory_block> buffer (host.Allocate (tablesByteCount + tileByteCount));
													  
	// Read in table.
	
	if (tablesByteCount)
		{
		
		DoCopyBytes (tablesBlock->Buffer (),
					 buffer->Buffer (),
					 tablesByteCount);
		
		}
		
	// Read in tile data.
	
	stream.SetReadPosition (tileOffset);
	
	stream.Get (buffer->Buffer_uint8 () + tablesByteCount, tileByteCount);
		
	// Patch first byte, if required.
		
	if (patchFirstByte)
		{
		
		buffer->Buffer_uint8 () [0] = 0xFF;
		
		}
		
	// Return buffer.
	
	return buffer.Release ();
	
	}

/*****************************************************************************/

bool dng_read_image::ReadBaselineJPEG (dng_host &host,
									   const dng_ifd &ifd,
									   dng_stream &stream,
									   dng_image &image,
									   const dng_rect &tileArea,
									   uint32 plane,
									   uint32 planes,
									   uint32 tileByteCount,
									   uint8 *jpegDataInMemory)
	{
	
	// Setup the data source.
	
	if (fJPEGTables.Get () || !jpegDataInMemory)
		{
		
		AutoPtr<dng_memory_block> jpegDataBlock;
	
		jpegDataBlock.Reset (ReadJPEGDataToBlock (host,
												  stream,
												  fJPEGTables.Get (),
												  stream.Position (),
												  tileByteCount,
												  ifd.fPatchFirstJPEGByte));
												  
		DecodeLossyJPEG (host,
						 image,
						 tileArea,
						 plane,
						 planes,
						 ifd.fPhotometricInterpretation,
						 jpegDataBlock->LogicalSize (),
						 jpegDataBlock->Buffer_uint8 ());
		
		}
		
	else
		{
	
		if (ifd.fPatchFirstJPEGByte && tileByteCount)
			{
			jpegDataInMemory [0] = 0xFF;
			}

		DecodeLossyJPEG (host,
						 image,
						 tileArea,
						 plane,
						 planes,
						 ifd.fPhotometricInterpretation,
						 tileByteCount,
						 jpegDataInMemory);
		
		}
				
	return true;
	
	}
	
/*****************************************************************************/

bool dng_read_image::ReadLosslessJPEG (dng_host &host,
									   const dng_ifd &ifd,
									   dng_stream &stream,
									   dng_image &image,
									   const dng_rect &tileArea,
									   uint32 plane,
									   uint32 planes,
									   uint32 tileByteCount,
									   AutoPtr<dng_memory_block> &uncompressedBuffer,
									   AutoPtr<dng_memory_block> &subTileBlockBuffer)
	{
	
	uint32 bytesPerRow = tileArea.W () * planes * (uint32) sizeof (uint16);
	
	uint32 rowsPerStrip = Pin_uint32 (ifd.fSubTileBlockRows,
									  kImageBufferSize / bytesPerRow,
									  tileArea.H ());
									  
	rowsPerStrip = rowsPerStrip / ifd.fSubTileBlockRows
								* ifd.fSubTileBlockRows;
									  
	uint32 bufferSize = bytesPerRow * rowsPerStrip;
	
	if (uncompressedBuffer.Get () &&
		uncompressedBuffer->LogicalSize () < bufferSize)
		{
		
		uncompressedBuffer.Reset ();
		
		}
		
	if (uncompressedBuffer.Get () == NULL)
		{
		
		uncompressedBuffer.Reset (host.Allocate (bufferSize));
									
		}
	
	dng_image_spooler spooler (host,
							   ifd,
							   image,
							   tileArea,
							   plane,
							   planes,
							   *uncompressedBuffer.Get (),
							   subTileBlockBuffer);
							   
	uint32 decodedSize = tileArea.W () *
						 tileArea.H () *
						 planes * (uint32) sizeof (uint16);
							
	bool bug16 = ifd.fLosslessJPEGBug16;
	
	uint64 tileOffset = stream.Position ();
	
	DecodeLosslessJPEG (stream,
					    spooler,
					    decodedSize,
					    decodedSize,
						bug16);
						
	if (stream.Position () > tileOffset + tileByteCount)
		{
		ThrowBadFormat ();
		}

	return true;
	
	}
	
/*****************************************************************************/

bool dng_read_image::CanReadTile (const dng_ifd &ifd)
	{
	
	if (ifd.fSampleFormat [0] != sfUnsignedInteger &&
		ifd.fSampleFormat [0] != sfFloatingPoint)
		{
		return false;
		}

	switch (ifd.fCompression)
		{
		
		case ccUncompressed:
			{
			
			if (ifd.fSampleFormat [0] == sfFloatingPoint)
				{
				
				return (ifd.fBitsPerSample [0] == 16 ||
						ifd.fBitsPerSample [0] == 24 ||
						ifd.fBitsPerSample [0] == 32);
						
				}
				
			return ifd.fBitsPerSample [0] >= 8 &&
				   ifd.fBitsPerSample [0] <= 32;
			
			}
			
		case ccJPEG:
			{
			
			if (ifd.fSampleFormat [0] != sfUnsignedInteger)
				{
				return false;
				}
				
			if (ifd.IsBaselineJPEG ())
				{
				
				// Baseline JPEG.
				
				return true;
				
				}
				
			else
				{
				
				// Lossless JPEG.
				
				return ifd.fBitsPerSample [0] >= 8 &&
					   ifd.fBitsPerSample [0] <= 16;
				
				}
				
			break;
			
			}

		case ccLZW:
		case ccDeflate:
		case ccOldDeflate:
		case ccPackBits:
			{
			
			if (ifd.fSampleFormat [0] == sfFloatingPoint)
				{
				
				if (ifd.fCompression == ccPackBits)
					{
					return false;
					}
				
				if (ifd.fPredictor != cpNullPredictor   &&
					ifd.fPredictor != cpFloatingPoint   &&
					ifd.fPredictor != cpFloatingPointX2 &&
					ifd.fPredictor != cpFloatingPointX4)
					{
					return false;
					}
					
				if (ifd.fBitsPerSample [0] != 16 &&
					ifd.fBitsPerSample [0] != 24 &&
					ifd.fBitsPerSample [0] != 32)
					{
					return false;
					}
				
				}
				
			else
				{

				if (ifd.fPredictor != cpNullPredictor		   &&
					ifd.fPredictor != cpHorizontalDifference   &&
					ifd.fPredictor != cpHorizontalDifferenceX2 &&
					ifd.fPredictor != cpHorizontalDifferenceX4)
					{
					return false;
					}
					
				if (ifd.fBitsPerSample [0] != 8  &&
					ifd.fBitsPerSample [0] != 16 &&
					ifd.fBitsPerSample [0] != 32)
					{
					return false;
					}
					
				}
					
			return true;
			
			}
			
		default:
			{
			break;
			}
			
		}
		
	return false;

	}
	
/*****************************************************************************/

bool dng_read_image::NeedsCompressedBuffer (const dng_ifd &ifd)
	{
	
	if (ifd.fCompression == ccLZW        ||
		ifd.fCompression == ccDeflate    ||
		ifd.fCompression == ccOldDeflate ||
		ifd.fCompression == ccPackBits)
		{
		return true;
		}
		
	return false;
	
	}
	
/*****************************************************************************/

void dng_read_image::ByteSwapBuffer (dng_host & /* host */,
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

void dng_read_image::DecodePredictor (dng_host & /* host */,
									  const dng_ifd &ifd,
						        	  dng_pixel_buffer &buffer)
	{
	
	switch (ifd.fPredictor)
		{
		
		case cpNullPredictor:
			{
			
			return;
			
			}
		
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
					
					DecodeDelta8 ((uint8 *) buffer.fData,
								  buffer.fArea.H (),
								  buffer.fArea.W () / xFactor,
								  buffer.fPlanes    * xFactor);
					
					return;
					
					}
					
				case ttShort:
					{
					
					DecodeDelta16 ((uint16 *) buffer.fData,
								   buffer.fArea.H (),
								   buffer.fArea.W () / xFactor,
								   buffer.fPlanes    * xFactor);
					
					return;
					
					}
					
				case ttLong:
					{
					
					DecodeDelta32 ((uint32 *) buffer.fData,
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
			
		default:
			break;
		
		}
		
	ThrowBadFormat ();
	
	}
						    
/*****************************************************************************/

void dng_read_image::ReadTile (dng_host &host,
						       const dng_ifd &ifd,
						       dng_stream &stream,
						       dng_image &image,
						       const dng_rect &tileArea,
						       uint32 plane,
						       uint32 planes,
						       uint32 tileByteCount,
							   AutoPtr<dng_memory_block> &compressedBuffer,
							   AutoPtr<dng_memory_block> &uncompressedBuffer,
							   AutoPtr<dng_memory_block> &subTileBlockBuffer)
	{
	
	switch (ifd.fCompression)
		{
		
		case ccLZW:
		case ccDeflate:
		case ccOldDeflate:
		case ccPackBits:
			{
			
			// Figure out uncompressed size.
			
			uint32 bytesPerSample = (ifd.fBitsPerSample [0] >> 3);
			
			uint32 sampleCount = planes * tileArea.W () * tileArea.H ();
			
			uint32 uncompressedSize = sampleCount * bytesPerSample;
			
			// Setup pixel buffer to hold uncompressed data.
			
			dng_pixel_buffer buffer;
			
			if (ifd.fSampleFormat [0] == sfFloatingPoint)
				{
				buffer.fPixelType = ttFloat;
				}
			
			else if (ifd.fBitsPerSample [0] == 8)
				{
				buffer.fPixelType = ttByte;
				}
				
			else if (ifd.fBitsPerSample [0] == 16)
				{
				buffer.fPixelType = ttShort;
				}
				
			else if (ifd.fBitsPerSample [0] == 32)
				{
				buffer.fPixelType = ttLong;
				}
				
			else
				{
				ThrowBadFormat ();
				}
				
			buffer.fPixelSize = bytesPerSample;
			
			buffer.fArea = tileArea;
			
			buffer.fPlane  = plane;
			buffer.fPlanes = planes;
			
			buffer.fPlaneStep = 1;

			buffer.fColStep = planes;
			buffer.fRowStep = planes * tileArea.W ();
			
			uint32 bufferSize = uncompressedSize;
			
			// If we are using the floating point predictor, we need an extra
			// buffer row.
			
			if (ifd.fPredictor == cpFloatingPoint   ||
				ifd.fPredictor == cpFloatingPointX2 ||
				ifd.fPredictor == cpFloatingPointX4)
				{
				bufferSize += buffer.fRowStep * buffer.fPixelSize;
				}
				
			// If are processing less than full size floating point data,
			// we need space to expand the data to full floating point size.
			
			if (buffer.fPixelType == ttFloat)
				{
				bufferSize = Max_uint32 (bufferSize, sampleCount * 4);
				}
			
			// Sometimes with multi-threading and planar image using strips, 
			// we can process a small tile before a large tile on a thread. 
			// Simple fix is to just reallocate the buffer if it is too small.
			
			if (uncompressedBuffer.Get () &&
				uncompressedBuffer->LogicalSize () < bufferSize)
				{
				
				uncompressedBuffer.Reset ();
				
				}
				
			if (uncompressedBuffer.Get () == NULL)
				{

				uncompressedBuffer.Reset (host.Allocate (bufferSize));
											
				}
				
			buffer.fData = uncompressedBuffer->Buffer ();
			
			// If using floating point predictor, move buffer pointer to second row.
			
			if (ifd.fPredictor == cpFloatingPoint   ||
				ifd.fPredictor == cpFloatingPointX2 ||
				ifd.fPredictor == cpFloatingPointX4)
				{
				
				buffer.fData = (uint8 *) buffer.fData +
							   buffer.fRowStep * buffer.fPixelSize;
							   
				}
			
			// Decompress the data.
			
			if (ifd.fCompression == ccLZW)
				{
				
				dng_lzw_expander expander;
				
				if (!expander.Expand (compressedBuffer->Buffer_uint8 (),
									  (uint8 *) buffer.fData,
									  tileByteCount,
									  uncompressedSize))
					{
					ThrowBadFormat ();
					}
				
				}
				
			else if (ifd.fCompression == ccPackBits)
				{
				
				dng_stream subStream (compressedBuffer->Buffer_uint8 (),
									  tileByteCount);
											 
				if (!DecodePackBits (subStream,
									 (uint8 *) buffer.fData,
									 uncompressedSize))
					{
					ThrowBadFormat ();
					}
				
				}
			
			else
				{
				
				uLongf dstLen = uncompressedSize;
							
				int err = uncompress ((Bytef *) buffer.fData,
									  &dstLen,
									  (const Bytef *) compressedBuffer->Buffer (),
									  tileByteCount);
									  
				if (err != Z_OK)
					{
					
					if (err == Z_MEM_ERROR)
						{
						ThrowMemoryFull ();
						}
						
					else if (err == Z_DATA_ERROR)
						{
						// Most other TIFF readers do not fail for this error
						// so we should not either, even if it means showing
						// a corrupted image to the user.  Watson #2530216
						// - tknoll 12/20/11
						}
						
					else
						{
						ThrowBadFormat ();
						}
						
					}
					
				if (dstLen != uncompressedSize)
					{
					ThrowBadFormat ();
					}
				
				}
				
			// The floating point predictor is byte order independent.
			
			if (ifd.fPredictor == cpFloatingPoint   ||
				ifd.fPredictor == cpFloatingPointX2 ||
				ifd.fPredictor == cpFloatingPointX4)
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
				
				for (int32 row = tileArea.t; row < tileArea.b; row++)
					{
					
					uint8 *srcPtr = (uint8 *) buffer.DirtyPixel (row    , tileArea.l, plane);
					uint8 *dstPtr = (uint8 *) buffer.DirtyPixel (row - 1, tileArea.l, plane);
					
					DecodeFPDelta (srcPtr,
								   dstPtr,
								   tileArea.W () / xFactor,
								   planes        * xFactor,
								   bytesPerSample);
					
					}
				
				buffer.fData = (uint8 *) buffer.fData -
							   buffer.fRowStep * buffer.fPixelSize;
							   
				}
				
			else
				{
				
				// Both these compression algorithms are byte based.
				
				if (stream.SwapBytes ())
					{
					
					ByteSwapBuffer (host,
									buffer);
									
					}
					
				// Undo the predictor.
				
				DecodePredictor (host,
								 ifd,
								 buffer);
								 
				}
				
			// Expand floating point data, if needed.
			
			if (buffer.fPixelType == ttFloat && buffer.fPixelSize == 2)
				{
				
				uint16 *srcPtr = (uint16 *) buffer.fData;
				uint32 *dstPtr = (uint32 *) buffer.fData;
				
				for (int32 index = sampleCount - 1; index >= 0; index--)
					{
					
					dstPtr [index] = DNG_HalfToFloat (srcPtr [index]);
					
					}
					
				buffer.fPixelSize = 4;
				
				}
				
			else if (buffer.fPixelType == ttFloat && buffer.fPixelSize == 3)
				{
				
				uint8  *srcPtr = ((uint8  *) buffer.fData) + (sampleCount - 1) * 3;
				uint32 *dstPtr = ((uint32 *) buffer.fData) + (sampleCount - 1);
				
				if (stream.BigEndian () || ifd.fPredictor == cpFloatingPoint   ||
										   ifd.fPredictor == cpFloatingPointX2 ||
										   ifd.fPredictor == cpFloatingPointX4)
					{
				
					for (uint32 index = 0; index < sampleCount; index++)
						{
						
						*(dstPtr--) = DNG_FP24ToFloat (srcPtr);
						
						srcPtr -= 3;
						
						}
						
					}
					
				else
					{
					
					for (uint32 index = 0; index < sampleCount; index++)
						{
						
						uint8 input [3];
						
						input [2] = srcPtr [0];
						input [1] = srcPtr [1];
						input [0] = srcPtr [2];
						
						*(dstPtr--) = DNG_FP24ToFloat (input);
						
						srcPtr -= 3;
						
						}
						
					}
					
				buffer.fPixelSize = 4;
				
				}
			
			// Save the data.
			
			image.Put (buffer);
			
			return;
			
			}
	
		case ccUncompressed:
			{
			
			if (ReadUncompressed (host,
								  ifd,
								  stream,
								  image,
								  tileArea,
								  plane,
								  planes,
								  uncompressedBuffer,
								  subTileBlockBuffer))
				{
				
				return;
				
				}
				
			break;
			
			}
			
		case ccJPEG:
			{
			
			if (ifd.IsBaselineJPEG ())
				{
				
				// Baseline JPEG.
				
				if (ReadBaselineJPEG (host,
									  ifd,
									  stream,
									  image,
									  tileArea,
									  plane,
									  planes,
									  tileByteCount,
									  compressedBuffer.Get () ? compressedBuffer->Buffer_uint8 () : NULL))
					{
					
					return;
					
					}
				
				}
				
			else
				{
				
				// Otherwise is should be lossless JPEG.
				
				if (ReadLosslessJPEG (host,
									  ifd,
									  stream,
									  image,
									  tileArea,
									  plane,
									  planes,
									  tileByteCount,
									  uncompressedBuffer,
									  subTileBlockBuffer))
					{
					
					return;
					
					}
				
				}
			
			break;
			
			}
			
		case ccLossyJPEG:
			{
			
			if (ReadBaselineJPEG (host,
								  ifd,
								  stream,
								  image,
								  tileArea,
								  plane,
								  planes,
								  tileByteCount,
								  compressedBuffer.Get () ? compressedBuffer->Buffer_uint8 () : NULL))
				{
				
				return;
				
				}
							
			break;
			
			}
			
		default:
			break;
			
		}
		
	ThrowBadFormat ();
		
	}

/*****************************************************************************/

bool dng_read_image::CanRead (const dng_ifd &ifd)
	{
	
	if (ifd.fImageWidth  < 1 ||
		ifd.fImageLength < 1)
		{
		return false;
		}
		
	if (ifd.fSamplesPerPixel < 1)
		{
		return false;
		}
		
	if (ifd.fBitsPerSample [0] < 1)
		{
		return false;
		}
	
	for (uint32 j = 1; j < Min_uint32 (ifd.fSamplesPerPixel,
									   kMaxSamplesPerPixel); j++)
		{
		
		if (ifd.fBitsPerSample [j] !=
			ifd.fBitsPerSample [0])
			{
			return false;
			}
			
		if (ifd.fSampleFormat [j] !=
			ifd.fSampleFormat [0])
			{
			return false;
			}

		}
		
	if ((ifd.fPlanarConfiguration != pcInterleaved   ) &&
		(ifd.fPlanarConfiguration != pcPlanar        ) &&
		(ifd.fPlanarConfiguration != pcRowInterleaved))
		{
		return false;
		}
		
	if (ifd.fUsesStrips == ifd.fUsesTiles)
		{
		return false;
		}
		
	uint32 tileCount = ifd.TilesPerImage ();
	
	if (tileCount < 1)
		{
		return false;
		}
		
	bool needTileByteCounts = (ifd.TileByteCount (ifd.TileArea (0, 0)) == 0);
		
	if (tileCount == 1)
		{
		
		if (needTileByteCounts)
			{
			
			if (ifd.fTileByteCount [0] < 1)
				{
				return false;
				}
			
			}
		
		}
		
	else
		{
		
		if (ifd.fTileOffsetsCount != tileCount)
			{
			return false;
			}
			
		if (needTileByteCounts)
			{
			
			if (ifd.fTileByteCountsCount != tileCount)
				{
				return false;
				}
			
			}
		
		}
		
	if (!CanReadTile (ifd))
		{
		return false;
		}
		
	return true;
	
	}
	
/*****************************************************************************/

class dng_read_tiles_task : public dng_area_task
	{
	
	private:
	
		dng_read_image &fReadImage;
		
		dng_host &fHost;
		
		const dng_ifd &fIFD;
		
		dng_stream &fStream;
		
		dng_image &fImage;
		
		dng_jpeg_image *fJPEGImage;
		
		dng_fingerprint *fJPEGTileDigest;
		
		uint32 fOuterSamples;
		
		uint32 fInnerSamples;
		
		uint32 fTilesDown;
		
		uint32 fTilesAcross;
		
		uint64 *fTileOffset;
		
		uint32 *fTileByteCount;
		
		uint32 fCompressedSize;
		
		uint32 fUncompressedSize;
		
		dng_mutex fMutex;
		
		uint32 fNextTileIndex;
		
	public:
	
		dng_read_tiles_task (dng_read_image &readImage,
							 dng_host &host,
							 const dng_ifd &ifd,
							 dng_stream &stream,
							 dng_image &image,
							 dng_jpeg_image *jpegImage,
							 dng_fingerprint *jpegTileDigest,
							 uint32 outerSamples,
							 uint32 innerSamples,
							 uint32 tilesDown,
							 uint32 tilesAcross,
							 uint64 *tileOffset,
							 uint32 *tileByteCount,
							 uint32 compressedSize,
							 uint32 uncompressedSize)
		
			:	fReadImage        (readImage)
			,	fHost		      (host)
			,	fIFD		      (ifd)
			,	fStream		      (stream)
			,	fImage		      (image)
			,	fJPEGImage		  (jpegImage)
			,	fJPEGTileDigest   (jpegTileDigest)
			,	fOuterSamples     (outerSamples)
			,	fInnerSamples     (innerSamples)
			,	fTilesDown        (tilesDown)
			,	fTilesAcross	  (tilesAcross)
			,	fTileOffset		  (tileOffset)
			,	fTileByteCount	  (tileByteCount)
			,	fCompressedSize   (compressedSize)
			,	fUncompressedSize (uncompressedSize)
			,	fMutex			  ("dng_read_tiles_task")
			,	fNextTileIndex	  (0)
			
			{
			
			fMinTaskArea = 16 * 16;
			fUnitCell    = dng_point (16, 16);
			fMaxTileSize = dng_point (16, 16);
			
			}
	
		void Process (uint32 /* threadIndex */,
					  const dng_rect & /* tile */,
					  dng_abort_sniffer *sniffer)
			{
			
			AutoPtr<dng_memory_block> compressedBuffer;
			AutoPtr<dng_memory_block> uncompressedBuffer;
			AutoPtr<dng_memory_block> subTileBlockBuffer;
			
			if (!fJPEGImage)
				{
				compressedBuffer.Reset (fHost.Allocate (fCompressedSize));
				}
			
			if (fUncompressedSize)
				{
				uncompressedBuffer.Reset (fHost.Allocate (fUncompressedSize));
				}
			
			while (true)
				{
				
				uint32 tileIndex;
				uint32 byteCount;
				
					{
					
					dng_lock_mutex lock (&fMutex);
					
					if (fNextTileIndex == fOuterSamples * fTilesDown * fTilesAcross)
						{
						return;
						}
						
					tileIndex = fNextTileIndex++;
					
					TempStreamSniffer noSniffer (fStream, NULL);

					fStream.SetReadPosition (fTileOffset [tileIndex]);
					
					byteCount = fTileByteCount [tileIndex];
					
					if (fJPEGImage)
						{
						
						fJPEGImage->fJPEGData [tileIndex] . Reset (fHost.Allocate (byteCount));
						
						}
					
					fStream.Get (fJPEGImage ? fJPEGImage->fJPEGData [tileIndex]->Buffer ()
											: compressedBuffer->Buffer (),
								 byteCount);
					
					}
					
				dng_abort_sniffer::SniffForAbort (sniffer);
				
				if (fJPEGTileDigest)
					{
					
					dng_md5_printer printer;
					
					printer.Process (compressedBuffer->Buffer (),
									 byteCount);
									 
					fJPEGTileDigest [tileIndex] = printer.Result ();
					
					}
					
				dng_stream tileStream (fJPEGImage ? fJPEGImage->fJPEGData [tileIndex]->Buffer ()
												  : compressedBuffer->Buffer (),
									   byteCount);
									   
				tileStream.SetLittleEndian (fStream.LittleEndian ());
							
				uint32 plane = tileIndex / (fTilesDown * fTilesAcross);
				
				uint32 rowIndex = (tileIndex - plane * fTilesDown * fTilesAcross) / fTilesAcross;
				
				uint32 colIndex = tileIndex - (plane * fTilesDown + rowIndex) * fTilesAcross;
				
				dng_rect tileArea = fIFD.TileArea (rowIndex, colIndex);
				
				dng_host host (&fHost.Allocator (),
							   sniffer);				// Cannot use sniffer attached to main host
				
				fReadImage.ReadTile (host,
									 fIFD,
									 tileStream,
									 fImage,
									 tileArea,
									 plane,
									 fInnerSamples,
									 byteCount,
									 fJPEGImage ? fJPEGImage->fJPEGData [tileIndex]
												: compressedBuffer,
									 uncompressedBuffer,
									 subTileBlockBuffer);
					
				}
			
			}
		
	private:

		// Hidden copy constructor and assignment operator.

		dng_read_tiles_task (const dng_read_tiles_task &);

		dng_read_tiles_task & operator= (const dng_read_tiles_task &);
		
	};

/*****************************************************************************/

void dng_read_image::Read (dng_host &host,
						   const dng_ifd &ifd,
						   dng_stream &stream,
						   dng_image &image,
						   dng_jpeg_image *jpegImage,
						   dng_fingerprint *jpegDigest)
	{
	
	uint32 tileIndex;
	
	// Deal with row interleaved images.
	
	if (ifd.fRowInterleaveFactor > 1 &&
		ifd.fRowInterleaveFactor < ifd.fImageLength)
		{
		
		dng_ifd tempIFD (ifd);
		
		tempIFD.fRowInterleaveFactor = 1;
		
		dng_row_interleaved_image tempImage (image,
											 ifd.fRowInterleaveFactor);
		
		Read (host,
			  tempIFD,
			  stream,
			  tempImage,
			  jpegImage,
			  jpegDigest);
			  
		return;
		
		}
	
	// Figure out inner and outer samples.
	
	uint32 innerSamples = 1;
	uint32 outerSamples = 1;
	
	if (ifd.fPlanarConfiguration == pcPlanar)
		{
		outerSamples = ifd.fSamplesPerPixel;
		}
	else
		{
		innerSamples = ifd.fSamplesPerPixel;
		}
	
	// Calculate number of tiles to read.
	
	uint32 tilesAcross = ifd.TilesAcross ();
	uint32 tilesDown   = ifd.TilesDown   ();
	
	uint32 tileCount = tilesAcross * tilesDown * outerSamples;
	
	// Find the tile offsets.
		
	dng_memory_data tileOffsetData (tileCount * (uint32) sizeof (uint64));
	
	uint64 *tileOffset = tileOffsetData.Buffer_uint64 ();
	
	if (tileCount <= dng_ifd::kMaxTileInfo)
		{
		
		for (tileIndex = 0; tileIndex < tileCount; tileIndex++)
			{
			
			tileOffset [tileIndex] = ifd.fTileOffset [tileIndex];
			
			}
		
		}
		
	else
		{
		
		stream.SetReadPosition (ifd.fTileOffsetsOffset);
		
		for (tileIndex = 0; tileIndex < tileCount; tileIndex++)
			{
			
			tileOffset [tileIndex] = stream.TagValue_uint32 (ifd.fTileOffsetsType);
			
			}
		
		}
		
	// Quick validity check on tile offsets.
	
	for (tileIndex = 0; tileIndex < tileCount; tileIndex++)
		{
		
		#if qDNGValidate
		
		if (tileOffset [tileIndex] < 8)
			{
			
			ReportWarning ("Tile/Strip offset less than 8");
			
			}
		
		#endif
		
		if (tileOffset [tileIndex] >= stream.Length ())
			{
			
			ThrowBadFormat ();
			
			}
		
		}
		
	// Buffer to hold the tile byte counts, if needed.
	
	dng_memory_data tileByteCountData;
	
	uint32 *tileByteCount = NULL;
	
	// If we can compute the number of bytes needed to store the
	// data, we can split the read for each tile into sub-tiles.
		
	uint32 uncompressedSize = 0;

	uint32 subTileLength = ifd.fTileLength;
	
	if (ifd.TileByteCount (ifd.TileArea (0, 0)) != 0)
		{
		
		uint32 bytesPerPixel = TagTypeSize (ifd.PixelType ());
		
		uint32 bytesPerRow = ifd.fTileWidth * innerSamples * bytesPerPixel;
				
		subTileLength = Pin_uint32 (ifd.fSubTileBlockRows,
									kImageBufferSize / bytesPerRow, 
									ifd.fTileLength);
									
		subTileLength = subTileLength / ifd.fSubTileBlockRows
									  * ifd.fSubTileBlockRows;
									  
		uncompressedSize = subTileLength * bytesPerRow;
									
		}
		
	// Else we need to know the byte counts.
	
	else
		{
		
		tileByteCountData.Allocate (tileCount * (uint32) sizeof (uint32));
		
		tileByteCount = tileByteCountData.Buffer_uint32 ();
		
		if (tileCount <= dng_ifd::kMaxTileInfo)
			{
			
			for (tileIndex = 0; tileIndex < tileCount; tileIndex++)
				{
				
				tileByteCount [tileIndex] = ifd.fTileByteCount [tileIndex];
				
				}
			
			}
			
		else
			{
			
			stream.SetReadPosition (ifd.fTileByteCountsOffset);
			
			for (tileIndex = 0; tileIndex < tileCount; tileIndex++)
				{
				
				tileByteCount [tileIndex] = stream.TagValue_uint32 (ifd.fTileByteCountsType);
				
				}
			
			}
			
		// Quick validity check on tile byte counts.
		
		for (tileIndex = 0; tileIndex < tileCount; tileIndex++)
			{
			
			if (tileByteCount [tileIndex] < 1 ||
				tileByteCount [tileIndex] > stream.Length ())
				{
				
				ThrowBadFormat ();
				
				}
			
			}
		
		}
		
	// Find maximum tile size, if possible.
		
	uint32 maxTileByteCount = 0;
	
	if (tileByteCount)
		{
		
		for (tileIndex = 0; tileIndex < tileCount; tileIndex++)
			{
			
			maxTileByteCount = Max_uint32 (maxTileByteCount,
										   tileByteCount [tileIndex]);
										   
			}
			
		}
		
	// Do we need a compressed data buffer?

	uint32 compressedSize = 0;
	
	bool needsCompressedBuffer = NeedsCompressedBuffer (ifd);
	
	if (needsCompressedBuffer)
		{
		
		if (!tileByteCount)
			{
			ThrowBadFormat ();
			}
		
		compressedSize = maxTileByteCount;
		
		}
		
	// Are we keeping the compressed JPEG image data?
	
	if (jpegImage)
		{
		
		if (ifd.IsBaselineJPEG ())
			{
			
			jpegImage->fImageSize.h = ifd.fImageWidth;
			jpegImage->fImageSize.v = ifd.fImageLength;
			
			jpegImage->fTileSize.h = ifd.fTileWidth;
			jpegImage->fTileSize.v = ifd.fTileLength;
			
			jpegImage->fUsesStrips = ifd.fUsesStrips;
			
			jpegImage->fJPEGData.Reset (new dng_jpeg_image_tile_ptr [tileCount]);
						
			}
			
		else
			{
			
			jpegImage = NULL;
			
			}
		
		}
		
	// Do we need to read the JPEG tables?
	
	if (ifd.fJPEGTablesOffset && ifd.fJPEGTablesCount)
		{
	
		if (ifd.IsBaselineJPEG ())
			{
			
			fJPEGTables.Reset (host.Allocate (ifd.fJPEGTablesCount));
			
			stream.SetReadPosition (ifd.fJPEGTablesOffset);
			
			stream.Get (fJPEGTables->Buffer      (),
						fJPEGTables->LogicalSize ());
			
			}
			
		}
		
	AutoArray<dng_fingerprint> jpegTileDigest;
	
	if (jpegDigest)
		{
		
		jpegTileDigest.Reset (new dng_fingerprint [tileCount + (fJPEGTables.Get () ? 1 : 0)]);
		
		}
			
	// Don't read planes we are not actually saving.
	
	outerSamples = Min_uint32 (image.Planes (), outerSamples);
		
	// See if we can do this read using multiple threads.
	
	bool useMultipleThreads = (outerSamples * tilesDown * tilesAcross >= 2) &&
							  (host.PerformAreaTaskThreads () > 1) &&
							  (maxTileByteCount > 0 && maxTileByteCount <= 1024 * 1024) &&
							  (subTileLength == ifd.fTileLength) &&
							  (ifd.fCompression != ccUncompressed);
	
#if qImagecore
	useMultipleThreads = false;	
#endif
		
	if (useMultipleThreads)
		{
		
		uint32 threadCount = Min_uint32 (outerSamples * tilesDown * tilesAcross,
										 host.PerformAreaTaskThreads ());
		
		dng_read_tiles_task task (*this,
								  host,
								  ifd,
								  stream,
								  image,
								  jpegImage,
								  jpegTileDigest.Get (),
								  outerSamples,
								  innerSamples,
								  tilesDown,
								  tilesAcross,
								  tileOffset,
								  tileByteCount,
								  maxTileByteCount,
								  uncompressedSize);
								  
		host.PerformAreaTask (task,
							  dng_rect (0, 0, 16, 16 * threadCount));
		
		}
		
	// Else use a single thread to read all the tiles.
	
	else
		{
		
		AutoPtr<dng_memory_block> compressedBuffer;
		AutoPtr<dng_memory_block> uncompressedBuffer;
		AutoPtr<dng_memory_block> subTileBlockBuffer;
		
		if (uncompressedSize)
			{
			uncompressedBuffer.Reset (host.Allocate (uncompressedSize));
			}
			
		if (compressedSize && !jpegImage)
			{
			compressedBuffer.Reset (host.Allocate (compressedSize));
			}
			
		else if (jpegDigest)
			{
			compressedBuffer.Reset (host.Allocate (maxTileByteCount));
			}
			
		tileIndex = 0;
		
		for (uint32 plane = 0; plane < outerSamples; plane++)
			{
			
			for (uint32 rowIndex = 0; rowIndex < tilesDown; rowIndex++)
				{
				
				for (uint32 colIndex = 0; colIndex < tilesAcross; colIndex++)
					{
					
					stream.SetReadPosition (tileOffset [tileIndex]);
					
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
						
						uint32 subByteCount;
						
						if (tileByteCount)
							{
							subByteCount = tileByteCount [tileIndex];
							}
						else
							{
							subByteCount = ifd.TileByteCount (subArea);
							}
							
						if (jpegImage)
							{
							
							jpegImage->fJPEGData [tileIndex].Reset (host.Allocate (subByteCount));
							
							stream.Get (jpegImage->fJPEGData [tileIndex]->Buffer (), subByteCount);
							
							stream.SetReadPosition (tileOffset [tileIndex]);
							
							}
							
						else if ((needsCompressedBuffer || jpegDigest) && subByteCount)
							{
							
							stream.Get (compressedBuffer->Buffer (), subByteCount);
							
							if (jpegDigest)
								{
								
								dng_md5_printer printer;
								
								printer.Process (compressedBuffer->Buffer (),
												 subByteCount);
												 
								jpegTileDigest [tileIndex] = printer.Result ();
								
								}
							
							}
							
						ReadTile (host,
								  ifd,
								  stream,
								  image,
								  subArea,
								  plane,
								  innerSamples,
								  subByteCount,
								  jpegImage ? jpegImage->fJPEGData [tileIndex] : compressedBuffer,
								  uncompressedBuffer,
								  subTileBlockBuffer);
								  
						}
					
					tileIndex++;
					
					}
					
				}

			}
			
		}
		
	// Finish up JPEG digest computation, if needed.
	
	if (jpegDigest)
		{
		
		if (fJPEGTables.Get ())
			{
			
			dng_md5_printer printer;
			
			printer.Process (fJPEGTables->Buffer      (),
							 fJPEGTables->LogicalSize ());
							 
			jpegTileDigest [tileCount] = printer.Result ();
			
			}
			
		dng_md5_printer printer2;
		
		for (uint32 j = 0; j < tileCount + (fJPEGTables.Get () ? 1 : 0); j++)
			{
			
			printer2.Process (jpegTileDigest [j].data,
							  dng_fingerprint::kDNGFingerprintSize);
							  
			}
			
		*jpegDigest = printer2.Result ();
		
		}
		
	// Keep the JPEG table in the jpeg image, if any.
	
	if (jpegImage)
		{
		
		jpegImage->fJPEGTables.Reset (fJPEGTables.Release ());
		
		}

	}

/*****************************************************************************/
