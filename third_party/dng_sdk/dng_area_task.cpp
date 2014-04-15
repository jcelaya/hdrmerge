/*****************************************************************************/
// Copyright 2006-2012 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in
// accordance with the terms of the Adobe license agreement accompanying it.
/*****************************************************************************/

/* $Id: //mondo/dng_sdk_1_4/dng_sdk/source/dng_area_task.cpp#1 $ */ 
/* $DateTime: 2012/05/30 13:28:51 $ */
/* $Change: 832332 $ */
/* $Author: tknoll $ */

/*****************************************************************************/

#include "dng_area_task.h"

#include "dng_abort_sniffer.h"
#include "dng_flags.h"
#include "dng_sdk_limits.h"
#include "dng_tile_iterator.h"
#include "dng_utils.h"

#if qImagecore
extern bool gPrintTimings;
#endif

/*****************************************************************************/

dng_area_task::dng_area_task ()

	:	fMaxThreads   (kMaxMPThreads)
	
	,	fMinTaskArea  (256 * 256)
	
	,	fUnitCell	  (1, 1)
	
	,	fMaxTileSize  (256, 256)
	
	{
	
	}

/*****************************************************************************/

dng_area_task::~dng_area_task ()
	{
	
	}

/*****************************************************************************/

dng_rect dng_area_task::RepeatingTile1 () const
	{
	
	return dng_rect ();
	
	}
		
/*****************************************************************************/

dng_rect dng_area_task::RepeatingTile2 () const
	{
	
	return dng_rect ();
	
	}
		
/*****************************************************************************/

dng_rect dng_area_task::RepeatingTile3 () const
	{
	
	return dng_rect ();
	
	}
		
/*****************************************************************************/

void dng_area_task::Start (uint32 /* threadCount */,
						   const dng_point & /* tileSize */,
						   dng_memory_allocator * /* allocator */,
						   dng_abort_sniffer * /* sniffer */)
	{
	
	}

/*****************************************************************************/

void dng_area_task::Finish (uint32 /* threadCount */)
	{
	
	}
	
/*****************************************************************************/

dng_point dng_area_task::FindTileSize (const dng_rect &area) const
	{
	
	dng_rect repeatingTile1 = RepeatingTile1 ();
	dng_rect repeatingTile2 = RepeatingTile2 ();
	dng_rect repeatingTile3 = RepeatingTile3 ();
	
	if (repeatingTile1.IsEmpty ())
		{
		repeatingTile1 = area;
		}
	
	if (repeatingTile2.IsEmpty ())
		{
		repeatingTile2 = area;
		}
	
	if (repeatingTile3.IsEmpty ())
		{
		repeatingTile3 = area;
		}
		
	uint32 repeatV = Min_uint32 (Min_uint32 (repeatingTile1.H (),
											 repeatingTile2.H ()),
											 repeatingTile3.H ());
	
	uint32 repeatH = Min_uint32 (Min_uint32 (repeatingTile1.W (),
											 repeatingTile2.W ()),
											 repeatingTile3.W ());
	
	dng_point maxTileSize = MaxTileSize ();

	dng_point tileSize;
		
	tileSize.v = Min_int32 (repeatV, maxTileSize.v);
	tileSize.h = Min_int32 (repeatH, maxTileSize.h);
	
	// What this is doing is, if the smallest repeating image tile is larger than the 
	// maximum tile size, adjusting the tile size down so that the tiles are as small
	// as possible while still having the same number of tiles covering the
	// repeat area.  This makes the areas more equal in size, making MP
	// algorithms work better.
						
	// The image core team did not understand this code, and disabled it.
	// Really stupid idea to turn off code you don't understand!
	// I'm undoing this removal, because I think the code is correct and useful.

	uint32 countV = (repeatV + tileSize.v - 1) / tileSize.v;
	uint32 countH = (repeatH + tileSize.h - 1) / tileSize.h;
	
	tileSize.v = (repeatV + countV - 1) / countV;
	tileSize.h = (repeatH + countH - 1) / countH;
	
	// Round up to unit cell size.
	
	dng_point unitCell = UnitCell ();
	
	if (unitCell.h != 1 || unitCell.v != 1)
		{
		tileSize.v = ((tileSize.v + unitCell.v - 1) / unitCell.v) * unitCell.v;
		tileSize.h = ((tileSize.h + unitCell.h - 1) / unitCell.h) * unitCell.h;
		}
		
	// But if that is larger than maximum tile size, round down to unit cell size.
	
	if (tileSize.v > maxTileSize.v)
		{
		tileSize.v = (maxTileSize.v / unitCell.v) * unitCell.v;
		}

	if (tileSize.h > maxTileSize.h)
		{
		tileSize.h = (maxTileSize.h / unitCell.h) * unitCell.h;
		}
		
	#if qImagecore	
    if (gPrintTimings)
		{
        fprintf (stdout, "\nRender tile for below: %d x %d\n", (int32) tileSize.h, (int32) tileSize.v);
		}	
	#endif

	return tileSize;
	
	}
		
/*****************************************************************************/

void dng_area_task::ProcessOnThread (uint32 threadIndex,
									 const dng_rect &area,
									 const dng_point &tileSize,
									 dng_abort_sniffer *sniffer)
	{
	
	dng_rect repeatingTile1 = RepeatingTile1 ();
	dng_rect repeatingTile2 = RepeatingTile2 ();
	dng_rect repeatingTile3 = RepeatingTile3 ();
	
	if (repeatingTile1.IsEmpty ())
		{
		repeatingTile1 = area;
		}
	
	if (repeatingTile2.IsEmpty ())
		{
		repeatingTile2 = area;
		}
	
	if (repeatingTile3.IsEmpty ())
		{
		repeatingTile3 = area;
		}
		
	dng_rect tile1;
	
	dng_tile_iterator iter1 (repeatingTile3, area);
	
	while (iter1.GetOneTile (tile1))
		{
		
		dng_rect tile2;
		
		dng_tile_iterator iter2 (repeatingTile2, tile1);
		
		while (iter2.GetOneTile (tile2))
			{
			
			dng_rect tile3;
			
			dng_tile_iterator iter3 (repeatingTile1, tile2);
			
			while (iter3.GetOneTile (tile3))
				{
				
				dng_rect tile4;
				
				dng_tile_iterator iter4 (tileSize, tile3);
				
				while (iter4.GetOneTile (tile4))
					{
					
					dng_abort_sniffer::SniffForAbort (sniffer);
					
					Process (threadIndex, tile4, sniffer);
					
					}
					
				}
				
			}
		
		}
	
	}
		
/*****************************************************************************/

void dng_area_task::Perform (dng_area_task &task,
				  			 const dng_rect &area,
				  			 dng_memory_allocator *allocator,
				  			 dng_abort_sniffer *sniffer)
	{
	
	dng_point tileSize (task.FindTileSize (area));
		
	task.Start (1, tileSize, allocator, sniffer);
	
	task.ProcessOnThread (0, area, tileSize, sniffer);
			
	task.Finish (1);
	
	}

/*****************************************************************************/
