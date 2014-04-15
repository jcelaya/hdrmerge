/*****************************************************************************/
// Copyright 2006 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in
// accordance with the terms of the Adobe license agreement accompanying it.
/*****************************************************************************/

/* $Id: //mondo/dng_sdk_1_4/dng_sdk/source/dng_tile_iterator.h#1 $ */ 
/* $DateTime: 2012/05/30 13:28:51 $ */
/* $Change: 832332 $ */
/* $Author: tknoll $ */

/*****************************************************************************/

#ifndef __dng_tile_iterator__
#define __dng_tile_iterator__

/*****************************************************************************/

#include "dng_classes.h"
#include "dng_point.h"
#include "dng_rect.h"
#include "dng_types.h"

/*****************************************************************************/

class dng_tile_iterator
	{
	
	private:
	
		dng_rect fArea;
		
		int32 fTileWidth;
		int32 fTileHeight;
		
		int32 fTileTop;
		int32 fTileLeft;
		
		int32 fRowLeft;
		
		int32 fLeftPage;
		int32 fRightPage;
		
		int32 fTopPage;
		int32 fBottomPage;
		
		int32 fHorizontalPage;
		int32 fVerticalPage;
		
	public:
	
		dng_tile_iterator (const dng_image &image,
						   const dng_rect &area);
						   
		dng_tile_iterator (const dng_point &tileSize,
						   const dng_rect &area);
						   
		dng_tile_iterator (const dng_rect &tile,
						   const dng_rect &area);
						   
		bool GetOneTile (dng_rect &tile);
		
	private:
	
		void Initialize (const dng_rect &tile,
						 const dng_rect &area);
	
	};
		
/*****************************************************************************/

#endif
	
/*****************************************************************************/
