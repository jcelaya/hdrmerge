/*****************************************************************************/
// Copyright 2006 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in
// accordance with the terms of the Adobe license agreement accompanying it.
/*****************************************************************************/

/* $Id: //mondo/dng_sdk_1_4/dng_sdk/source/dng_memory.cpp#1 $ */ 
/* $DateTime: 2012/05/30 13:28:51 $ */
/* $Change: 832332 $ */
/* $Author: tknoll $ */

/*****************************************************************************/

#include "dng_memory.h"

#include "dng_bottlenecks.h"
#include "dng_exceptions.h"

/*****************************************************************************/

dng_memory_data::dng_memory_data ()
	
	:	fBuffer (NULL)
	
	{
	
	}

/*****************************************************************************/

dng_memory_data::dng_memory_data (uint32 size)

	:	fBuffer (NULL)
	
	{
	
	Allocate (size);
	
	}

/*****************************************************************************/

dng_memory_data::~dng_memory_data ()
	{
	
	Clear ();
	
	}
				
/*****************************************************************************/

void dng_memory_data::Allocate (uint32 size)
	{
	
	Clear ();
	
	if (size)
		{
		
		fBuffer = (char*)malloc (size);
		
		if (!fBuffer)
			{
			
			ThrowMemoryFull ();
						 
			}
		
		}
	
	}
				
/*****************************************************************************/

void dng_memory_data::Clear ()
	{
	
	if (fBuffer)
		{
		
		free (fBuffer);
		
		fBuffer = NULL;
		
		}
		
	}
				
/*****************************************************************************/

dng_memory_block * dng_memory_block::Clone (dng_memory_allocator &allocator) const
	{
	
	uint32 size = LogicalSize ();
	
	dng_memory_block * result = allocator.Allocate (size);
	
	DoCopyBytes (Buffer (), result->Buffer (), size);
		
	return result;
	
	}

/*****************************************************************************/

class dng_malloc_block : public dng_memory_block
	{
	
	private:
	
		void *fMalloc;
	
	public:
	
		dng_malloc_block (uint32 logicalSize);
		
		virtual ~dng_malloc_block ();
		
	private:
	
		// Hidden copy constructor and assignment operator.
		
		dng_malloc_block (const dng_malloc_block &block);
		
		dng_malloc_block & operator= (const dng_malloc_block &block);
	
	};
	
/*****************************************************************************/

dng_malloc_block::dng_malloc_block (uint32 logicalSize)

	:	dng_memory_block (logicalSize)
	
	,	fMalloc (NULL)
	
	{

#if qLinux

	int err = ::posix_memalign( (void **) &fMalloc, 16, (size_t) PhysicalSize() );

	if (err)
		{

		ThrowMemoryFull ();

		}

#else

	fMalloc = (char*)malloc (PhysicalSize ());
	
	if (!fMalloc)
		{
		
		ThrowMemoryFull ();
					 
		}
		
#endif

	SetBuffer (fMalloc);
	
	}
		
/*****************************************************************************/

dng_malloc_block::~dng_malloc_block ()
	{
	
	if (fMalloc)
		{
		
		free (fMalloc);
		
		}
	
	}
		
/*****************************************************************************/

dng_memory_block * dng_memory_allocator::Allocate (uint32 size)
	{
	
	dng_memory_block *result = new dng_malloc_block (size);
	
	if (!result)
		{
		
		ThrowMemoryFull ();
		
		}
	
	return result;
	
	}

/*****************************************************************************/

dng_memory_allocator gDefaultDNGMemoryAllocator;

/*****************************************************************************/
