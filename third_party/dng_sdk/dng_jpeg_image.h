/*****************************************************************************/
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in
// accordance with the terms of the Adobe license agreement accompanying it.
/*****************************************************************************/

/* $Id: //mondo/dng_sdk_1_4/dng_sdk/source/dng_jpeg_image.h#1 $ */ 
/* $DateTime: 2012/05/30 13:28:51 $ */
/* $Change: 832332 $ */
/* $Author: tknoll $ */

/*****************************************************************************/

#ifndef __dng_jpeg_image__
#define __dng_jpeg_image__

/*****************************************************************************/

#include "dng_auto_ptr.h"
#include "dng_memory.h"
#include "dng_point.h"

/*****************************************************************************/

typedef AutoPtr<dng_memory_block> dng_jpeg_image_tile_ptr;

/*****************************************************************************/

class dng_jpeg_image
	{
	
	public:
	
		dng_point fImageSize;
		
		dng_point fTileSize;
		
		bool fUsesStrips;
		
		AutoPtr<dng_memory_block> fJPEGTables;
		
		AutoArray<dng_jpeg_image_tile_ptr> fJPEGData;
		
	public:
	
		dng_jpeg_image ();
		
		uint32 TilesAcross () const
			{
			if (fTileSize.h)
				{
				return (fImageSize.h + fTileSize.h - 1) / fTileSize.h;
				}
			else
				{
				return 0;
				}
			}
		
		uint32 TilesDown () const
			{
			if (fTileSize.v)
				{
				return (fImageSize.v + fTileSize.v - 1) / fTileSize.v;
				}
			else
				{
				return 0;
				}
			}
			
		uint32 TileCount () const
			{
			return TilesAcross () * TilesDown ();
			}
		
		void Encode (dng_host &host,
					 const dng_negative &negative,
					 dng_image_writer &writer,
					 const dng_image &image);
					 
		dng_fingerprint FindDigest (dng_host &host) const;
			
	};

/*****************************************************************************/

#endif
	
/*****************************************************************************/
