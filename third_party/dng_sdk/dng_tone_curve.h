/*****************************************************************************/
// Copyright 2007 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in
// accordance with the terms of the Adobe license agreement accompanying it.
/*****************************************************************************/

/* $Id: //mondo/dng_sdk_1_4/dng_sdk/source/dng_tone_curve.h#2 $ */ 
/* $DateTime: 2012/07/31 22:04:34 $ */
/* $Change: 840853 $ */
/* $Author: tknoll $ */

/** \file
 * Representation of 1-dimensional tone curve.
 */

/*****************************************************************************/

#ifndef __dng_tone_curve__
#define __dng_tone_curve__

/*****************************************************************************/

#include "dng_classes.h"
#include "dng_point.h"

#include <vector>

/*****************************************************************************/

class dng_tone_curve
	{
	
	public:
		
		std::vector<dng_point_real64> fCoord;
		
	public:

		dng_tone_curve ();

		bool operator== (const dng_tone_curve &curve) const;
		
		bool operator!= (const dng_tone_curve &curve) const
			{
			return !(*this == curve);
			}
			
		void SetNull ();

		bool IsNull () const;
		
		void SetInvalid ();
		
		bool IsValid () const;
		
		void Solve (dng_spline_solver &solver) const;
		
	};

/*****************************************************************************/

#endif

/*****************************************************************************/
