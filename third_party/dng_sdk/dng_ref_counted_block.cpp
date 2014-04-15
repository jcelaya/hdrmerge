/*****************************************************************************/
// Copyright 2006 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in
// accordance with the terms of the Adobe license agreement accompanying it.
/*****************************************************************************/

/* $Id: //mondo/dng_sdk_1_4/dng_sdk/source/dng_ref_counted_block.cpp#1 $ */ 
/* $DateTime: 2012/05/30 13:28:51 $ */
/* $Change: 832332 $ */
/* $Author: tknoll $ */

/*****************************************************************************/

#include <new>

#include "dng_ref_counted_block.h"

#include "dng_exceptions.h"

/*****************************************************************************/

dng_ref_counted_block::dng_ref_counted_block ()
	
	:	fBuffer (NULL)
	
	{
	
	}

/*****************************************************************************/

dng_ref_counted_block::dng_ref_counted_block (uint32 size)

	:	fBuffer (NULL)
	
	{
	
	Allocate (size);
	
	}

/*****************************************************************************/

dng_ref_counted_block::~dng_ref_counted_block ()
	{
	
	Clear ();
	
	}
				
/*****************************************************************************/

void dng_ref_counted_block::Allocate (uint32 size)
	{
	
	Clear ();
	
	if (size)
		{
		
		fBuffer = malloc (size + sizeof (header));
		
		if (!fBuffer)
			{
			
			ThrowMemoryFull ();
						 
			}
		
		new (fBuffer) header (size);

		}
	
	}
				
/*****************************************************************************/

void dng_ref_counted_block::Clear ()
	{
	
	if (fBuffer)
		{
		

		bool doFree = false;

		header *blockHeader = (struct header *)fBuffer;

			{
		
			dng_lock_mutex lock (&blockHeader->fMutex);

			if (--blockHeader->fRefCount == 0)
				doFree = true;
			}

		if (doFree)
			{
				
			blockHeader->~header ();

			free (fBuffer);

			}
		
		fBuffer = NULL;
		
		}
		
	}
				
/*****************************************************************************/

dng_ref_counted_block::dng_ref_counted_block (const dng_ref_counted_block &data)
	: fBuffer (NULL)
	{

	header *blockHeader = (struct header *)data.fBuffer;

	dng_lock_mutex lock (&blockHeader->fMutex);

	blockHeader->fRefCount++;

	fBuffer = blockHeader;

	}
		
/*****************************************************************************/

dng_ref_counted_block & dng_ref_counted_block::operator= (const dng_ref_counted_block &data)
	{

	if (this != &data)
		{
		Clear ();

		header *blockHeader = (struct header *)data.fBuffer;

		dng_lock_mutex lock (&blockHeader->fMutex);

		blockHeader->fRefCount++;

		fBuffer = blockHeader;

		}

	return *this;

	}

/*****************************************************************************/

void dng_ref_counted_block::EnsureWriteable ()
	{

	if (fBuffer)
		{

		header *possiblySharedHeader = (header *)fBuffer;

			{
			
			dng_lock_mutex lock (&possiblySharedHeader->fMutex);

			if (possiblySharedHeader->fRefCount > 1)
				{

				fBuffer = NULL;

				Allocate ((uint32)possiblySharedHeader->fSize);

				memcpy (Buffer (),
					((char *)possiblySharedHeader) + sizeof (struct header), // could just do + 1 w/o cast, but this makes the type mixing more explicit
					possiblySharedHeader->fSize);

				possiblySharedHeader->fRefCount--;

				}

			}

		}
	}

/*****************************************************************************/
