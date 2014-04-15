/*****************************************************************************/
// Copyright 2006-2012 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in
// accordance with the terms of the Adobe license agreement accompanying it.
/*****************************************************************************/

/* $Id: //mondo/dng_sdk_1_4/dng_sdk/source/dng_utils.h#3 $ */ 
/* $DateTime: 2012/06/14 20:24:41 $ */
/* $Change: 835078 $ */
/* $Author: tknoll $ */

/*****************************************************************************/

#ifndef __dng_utils__
#define __dng_utils__

/*****************************************************************************/

#include "dng_classes.h"
#include "dng_flags.h"
#include "dng_memory.h"
#include "dng_types.h"

/*****************************************************************************/

inline uint32 Abs_int32 (int32 x)
	{
	
	#if 0
	
	// Reference version.
	
	return (uint32) (x < 0 ? -x : x);
	
	#else
	
	// Branchless version.
	
    uint32 mask = (uint32) (x >> 31);
    
    return (uint32) (((uint32) x + mask) ^ mask);
    
	#endif
	
	}

inline int32 Min_int32 (int32 x, int32 y)
	{
	
	return (x <= y ? x : y);
	
	}

inline int32 Max_int32 (int32 x, int32 y)
	{
	
	return (x >= y ? x : y);
	
	}

inline int32 Pin_int32 (int32 min, int32 x, int32 max)
	{
	
	return Max_int32 (min, Min_int32 (x, max));
	
	}

inline int32 Pin_int32_between (int32 a, int32 x, int32 b)
	{
	
	int32 min, max;
	if (a < b) { min = a; max = b; }
	else { min = b; max = a; }
	
	return Pin_int32 (min, x, max);
	
	}

/*****************************************************************************/

inline uint16 Min_uint16 (uint16 x, uint16 y)
	{
	
	return (x <= y ? x : y);
	
	}

inline uint16 Max_uint16 (uint16 x, uint16 y)
	{
	
	return (x >= y ? x : y);
	
	}

inline int16 Pin_int16 (int32 x)
	{
	
	x = Pin_int32 (-32768, x, 32767);
	
	return (int16) x;
	
	}
	
/*****************************************************************************/

inline uint32 Min_uint32 (uint32 x, uint32 y)
	{
	
	return (x <= y ? x : y);
	
	}

inline uint32 Min_uint32 (uint32 x, uint32 y, uint32 z)
	{
	
	return Min_uint32 (x, Min_uint32 (y, z));
	
	}
	
inline uint32 Max_uint32 (uint32 x, uint32 y)
	{
	
	return (x >= y ? x : y);
	
	}
	
inline uint32 Max_uint32 (uint32 x, uint32 y, uint32 z)
	{
	
	return Max_uint32 (x, Max_uint32 (y, z));
	
	}
	
inline uint32 Pin_uint32 (uint32 min, uint32 x, uint32 max)
	{
	
	return Max_uint32 (min, Min_uint32 (x, max));
	
	}

/*****************************************************************************/

inline uint16 Pin_uint16 (int32 x)
	{
	
	#if 0
	
	// Reference version.
	
	x = Pin_int32 (0, x, 0x0FFFF);
	
	#else
	
	// Single branch version.
	
	if (x & ~65535)
		{
		
		x = ~x >> 31;
		
		}
		
	#endif
		
	return (uint16) x;
	
	}

/*****************************************************************************/

inline uint32 RoundUp2 (uint32 x)
	{
	
	return (x + 1) & (uint32) ~1;
	
	}

inline uint32 RoundUp4 (uint32 x)
	{
	
	return (x + 3) & (uint32) ~3;
	
	}

inline uint32 RoundUp8 (uint32 x)
	{
	
	return (x + 7) & (uint32) ~7;
	
	}

inline uint32 RoundUp16 (uint32 x)
	{
	
	return (x + 15) & (uint32) ~15;
	
	}

inline uint32 RoundUp4096 (uint32 x)
	{
	
	return (x + 4095) & (uint32) ~4095;
	
	}

/******************************************************************************/

inline uint32 RoundDown2 (uint32 x)
	{
	
	return x & (uint32) ~1;
	
	}

inline uint32 RoundDown4 (uint32 x)
	{
	
	return x & (uint32) ~3;
	
	}

inline uint32 RoundDown8 (uint32 x)
	{
	
	return x & (uint32) ~7;
	
	}

inline uint32 RoundDown16 (uint32 x)
	{
	
	return x & (uint32) ~15;
	
	}

/******************************************************************************/

inline uint32 RoundUpForPixelSize (uint32 x, uint32 pixelSize)
	{
	
	switch (pixelSize)
		{
		
		case 1:
			return RoundUp16 (x);
			
		case 2:
			return RoundUp8 (x);
			
		case 4:
			return RoundUp4 (x);
			
		case 8:
			return RoundUp2 (x);
			
		default:
			return RoundUp16 (x);
					
		}
	
	}

/******************************************************************************/

inline uint64 Abs_int64 (int64 x)
	{
	
	return (uint64) (x < 0 ? -x : x);

	}

inline int64 Min_int64 (int64 x, int64 y)
	{
	
	return (x <= y ? x : y);
	
	}

inline int64 Max_int64 (int64 x, int64 y)
	{
	
	return (x >= y ? x : y);
	
	}

inline int64 Pin_int64 (int64 min, int64 x, int64 max)
	{
	
	return Max_int64 (min, Min_int64 (x, max));
	
	}

/******************************************************************************/

inline uint64 Min_uint64 (uint64 x, uint64 y)
	{
	
	return (x <= y ? x : y);
	
	}

inline uint64 Max_uint64 (uint64 x, uint64 y)
	{
	
	return (x >= y ? x : y);
	
	}

inline uint64 Pin_uint64 (uint64 min, uint64 x, uint64 max)
	{
	
	return Max_uint64 (min, Min_uint64 (x, max));
	
	}

/*****************************************************************************/

inline real32 Abs_real32 (real32 x)
	{
	
	return (x < 0.0f ? -x : x);
	
	}

inline real32 Min_real32 (real32 x, real32 y)
	{
	
	return (x < y ? x : y);
	
	}

inline real32 Max_real32 (real32 x, real32 y)
	{
	
	return (x > y ? x : y);
	
	}

inline real32 Pin_real32 (real32 min, real32 x, real32 max)
	{
	
	return Max_real32 (min, Min_real32 (x, max));
	
	}
	
inline real32 Pin_real32 (real32 x)
	{

	return Pin_real32 (0.0f, x, 1.0f);

	}

inline real32 Pin_real32_Overrange (real32 min, 
									real32 x, 
									real32 max)
	{
	
	// Normal numbers in (min,max). No change.
	
	if (x > min && x < max)
		{
		return x;
		}
		
	// Map large numbers (including positive infinity) to max.
		
	else if (x > min)
		{
		return max;
		}
		
	// Map everything else (including negative infinity and all NaNs) to min.
		
	return min;
	
	}

inline real32 Pin_Overrange (real32 x)
	{
	
	// Normal in-range numbers, except for plus and minus zero.
	
	if (x > 0.0f && x <= 1.0f)
		{
		return x;
		}
		
	// Large numbers, including positive infinity.
		
	else if (x > 0.5f)
		{
		return 1.0f;
		}
		
	// Plus and minus zero, negative numbers, negative infinity, and all NaNs.
		
	return 0.0f;
	
	}

inline real32 Lerp_real32 (real32 a, real32 b, real32 t)
	{
	
	return a + t * (b - a);
	
	}

/*****************************************************************************/

inline real64 Abs_real64 (real64 x)
	{
	
	return (x < 0.0 ? -x : x);
	
	}

inline real64 Min_real64 (real64 x, real64 y)
	{
	
	return (x < y ? x : y);
	
	}

inline real64 Max_real64 (real64 x, real64 y)
	{
	
	return (x > y ? x : y);
	
	}

inline real64 Pin_real64 (real64 min, real64 x, real64 max)
	{
	
	return Max_real64 (min, Min_real64 (x, max));
	
	}

inline real64 Pin_real64 (real64 x)
	{
	
	return Pin_real64 (0.0, x, 1.0);
	
	}

inline real64 Pin_real64_Overrange (real64 min, 
									real64 x, 
									real64 max)
	{
	
	// Normal numbers in (min,max). No change.
	
	if (x > min && x < max)
		{
		return x;
		}
		
	// Map large numbers (including positive infinity) to max.
		
	else if (x > min)
		{
		return max;
		}
		
	// Map everything else (including negative infinity and all NaNs) to min.
		
	return min;
	
	}

inline real64 Lerp_real64 (real64 a, real64 b, real64 t)
	{
	
	return a + t * (b - a);
	
	}

/*****************************************************************************/

inline int32 Round_int32 (real32 x)
	{
	
	return (int32) (x > 0.0f ? x + 0.5f : x - 0.5f);
	
	}

inline int32 Round_int32 (real64 x)
	{
	
	return (int32) (x > 0.0 ? x + 0.5 : x - 0.5);
	
	}

inline uint32 Floor_uint32 (real32 x)
	{
	
	return (uint32) Max_real32 (0.0f, x);
	
	}

inline uint32 Floor_uint32 (real64 x)
	{
	
	return (uint32) Max_real64 (0.0, x);
	
	}

inline uint32 Round_uint32 (real32 x)
	{
	
	return Floor_uint32 (x + 0.5f);
	
	}

inline uint32 Round_uint32 (real64 x)
	{
	
	return Floor_uint32 (x + 0.5);
	
	}

/******************************************************************************/

inline int64 Round_int64 (real64 x)
	{
	
	return (int64) (x >= 0.0 ? x + 0.5 : x - 0.5);
	
	}

/*****************************************************************************/

const int64 kFixed64_One  = (((int64) 1) << 32);
const int64 kFixed64_Half = (((int64) 1) << 31);

/******************************************************************************/

inline int64 Real64ToFixed64 (real64 x)
	{
	
	return Round_int64 (x * (real64) kFixed64_One);
	
	}

/******************************************************************************/

inline real64 Fixed64ToReal64 (int64 x)
	{
	
	return x * (1.0 / (real64) kFixed64_One);
	
	}

/*****************************************************************************/

inline char ForceUppercase (char c)
	{
	
	if (c >= 'a' && c <= 'z')
		{
		
		c -= 'a' - 'A';
		
		}
		
	return c;
	
	}

/*****************************************************************************/

inline uint16 SwapBytes16 (uint16 x)
	{
	
	return (uint16) ((x << 8) |
					 (x >> 8));
	
	}

inline uint32 SwapBytes32 (uint32 x)
	{
	
	return (x << 24) +
		   ((x << 8) & 0x00FF0000) +
		   ((x >> 8) & 0x0000FF00) +
		   (x >> 24);
	
	}

/*****************************************************************************/

inline bool IsAligned16 (const void *p)
	{
	
	return (((uintptr) p) & 1) == 0;
	
	}

inline bool IsAligned32 (const void *p)
	{
	
	return (((uintptr) p) & 3) == 0;
	
	}

inline bool IsAligned64 (const void *p)
	{
	
	return (((uintptr) p) & 7) == 0;
	
	}

inline bool IsAligned128 (const void *p)
	{
	
	return (((uintptr) p) & 15) == 0;
	
	}

/******************************************************************************/

// Converts from RGB values (range 0.0 to 1.0) to HSV values (range 0.0 to
// 6.0 for hue, and 0.0 to 1.0 for saturation and value).

inline void DNG_RGBtoHSV (real32 r,
					      real32 g,
					      real32 b,
					      real32 &h,
					      real32 &s,
					      real32 &v)
	{
	
	v = Max_real32 (r, Max_real32 (g, b));

	real32 gap = v - Min_real32 (r, Min_real32 (g, b));
	
	if (gap > 0.0f)
		{

		if (r == v)
			{
			
			h = (g - b) / gap;
			
			if (h < 0.0f)
				{
				h += 6.0f;
				}
				
			}
			
		else if (g == v) 
			{
			h = 2.0f + (b - r) / gap;
			}
			
		else
			{
			h = 4.0f + (r - g) / gap;
			}
			
		s = gap / v;
		
		}
		
	else
		{
		h = 0.0f;
		s = 0.0f;
		}
	
	}

/*****************************************************************************/

// Converts from HSV values (range 0.0 to 6.0 for hue, and 0.0 to 1.0 for
// saturation and value) to RGB values (range 0.0 to 1.0).

inline void DNG_HSVtoRGB (real32 h,
						  real32 s,
						  real32 v,
						  real32 &r,
						  real32 &g,
						  real32 &b)
	{
	
	if (s > 0.0f)
		{
		
		if (h < 0.0f)
			h += 6.0f;
			
		if (h >= 6.0f)
			h -= 6.0f;
			
		int32  i = (int32) h;
		real32 f = h - (real32) i;
		
		real32 p = v * (1.0f - s);
		
		#define q	(v * (1.0f - s * f))
		#define t	(v * (1.0f - s * (1.0f - f)))
		
		switch (i)
			{
			case 0: r = v; g = t; b = p; break;
			case 1: r = q; g = v; b = p; break;
			case 2: r = p; g = v; b = t; break;
			case 3: r = p; g = q; b = v; break;
			case 4: r = t; g = p; b = v; break;
			case 5: r = v; g = p; b = q; break;
			}
			
		#undef q
		#undef t
		
		}
		
	else
		{
		r = v;
		g = v;
		b = v;
		}
	
	}

/******************************************************************************/

// High resolution timer, for code profiling.

real64 TickTimeInSeconds ();

// Lower resolution timer, but more stable.

real64 TickCountInSeconds ();

/******************************************************************************/

class dng_timer
	{

	public:

		dng_timer (const char *message);

		~dng_timer ();
		
	private:
	
		// Hidden copy constructor and assignment operator.
	
		dng_timer (const dng_timer &timer);
		
		dng_timer & operator= (const dng_timer &timer);

	private:

		const char *fMessage;
		
		real64 fStartTime;
		
	};

/*****************************************************************************/

// Returns the maximum squared Euclidean distance from the specified point to the
// specified rectangle rect.

real64 MaxSquaredDistancePointToRect (const dng_point_real64 &point,
									  const dng_rect_real64 &rect);

/*****************************************************************************/

// Returns the maximum Euclidean distance from the specified point to the specified
// rectangle rect.

real64 MaxDistancePointToRect (const dng_point_real64 &point,
							   const dng_rect_real64 &rect);

/*****************************************************************************/

inline uint32 DNG_HalfToFloat (uint16 halfValue)
	{

	int32 sign 	   = (halfValue >> 15) & 0x00000001;
	int32 exponent = (halfValue >> 10) & 0x0000001f;
	int32 mantissa =  halfValue		   & 0x000003ff;
   	
	if (exponent == 0)
		{
		
		if (mantissa == 0)
			{
			
			// Plus or minus zero

			return (uint32) (sign << 31);
			
			}
			
		else
			{
			
			// Denormalized number -- renormalize it

			while (!(mantissa & 0x00000400))
				{
				mantissa <<= 1;
				exponent -=  1;
				}

			exponent += 1;
			mantissa &= ~0x00000400;
			
			}
			
		}
		
	else if (exponent == 31)
		{
		
		if (mantissa == 0)
			{
			
			// Positive or negative infinity, convert to maximum (16 bit) values.
			
			return (uint32) ((sign << 31) | ((0x1eL + 127 - 15) << 23) |  (0x3ffL << 13));

			}
			
		else
			{
			
			// Nan -- Just set to zero.

			return 0;
			
			}
			
		}

	// Normalized number

	exponent += (127 - 15);
	mantissa <<= 13;

	// Assemble sign, exponent and mantissa.

	return (uint32) ((sign << 31) | (exponent << 23) | mantissa);
	
	}

/*****************************************************************************/

inline uint16 DNG_FloatToHalf (uint32 i)
	{
	
	int32 sign     =  (i >> 16) & 0x00008000;
	int32 exponent = ((i >> 23) & 0x000000ff) - (127 - 15);
	int32 mantissa =   i		& 0x007fffff;

	if (exponent <= 0)
		{
		
		if (exponent < -10)
			{
			
			// Zero or underflow to zero.
			
			return (uint16)sign;
			
			}

		// E is between -10 and 0.  We convert f to a denormalized half.

		mantissa = (mantissa | 0x00800000) >> (1 - exponent);

		// Round to nearest, round "0.5" up.
		//
		// Rounding may cause the significand to overflow and make
		// our number normalized.  Because of the way a half's bits
		// are laid out, we don't have to treat this case separately;
		// the code below will handle it correctly.

		if (mantissa &  0x00001000)
			mantissa += 0x00002000;

		// Assemble the half from sign, exponent (zero) and mantissa.

		return (uint16)(sign | (mantissa >> 13));
		
		}
	
	else if (exponent == 0xff - (127 - 15))
		{
		
		if (mantissa == 0)
			{
			
			// F is an infinity; convert f to a half
			// infinity with the same sign as f.

			return (uint16)(sign | 0x7c00);
			
			}
			
		else
			{
			
			// F is a NAN; produce a half NAN that preserves
			// the sign bit and the 10 leftmost bits of the
			// significand of f.

			return (uint16)(sign | 0x7c00 | (mantissa >> 13));
			
			}
	
		}

	// E is greater than zero.  F is a normalized float.
	// We try to convert f to a normalized half.

	// Round to nearest, round "0.5" up

	if (mantissa & 0x00001000)
		{
		
		mantissa += 0x00002000;

		if (mantissa & 0x00800000)
			{
			mantissa =  0;		// overflow in significand,
			exponent += 1;		// adjust exponent
			}

		}

	// Handle exponent overflow

	if (exponent > 30)
		{
		return (uint16)(sign | 0x7c00);	// infinity with the same sign as f.
		}

	// Assemble the half from sign, exponent and mantissa.

	return (uint16)(sign | (exponent << 10) | (mantissa >> 13));
		
	}

/*****************************************************************************/

inline uint32 DNG_FP24ToFloat (const uint8 *input)
	{

	int32 sign     = (input [0] >> 7) & 0x01;
	int32 exponent = (input [0]     ) & 0x7F;
	int32 mantissa = (((int32) input [1]) << 8) | input[2];
   	
	if (exponent == 0)
		{
		
		if (mantissa == 0)
			{
			
			// Plus or minus zero
			
			return (uint32) (sign << 31);
			
			}

		else
			{
			
			// Denormalized number -- renormalize it

			while (!(mantissa & 0x00010000))
				{
				mantissa <<= 1;
				exponent -=  1;
				}

			exponent += 1;
			mantissa &= ~0x00010000;
			
			}
	
		}
		
	else if (exponent == 127)
		{
		
		if (mantissa == 0)
			{
			
			// Positive or negative infinity, convert to maximum (24 bit) values.
	
			return (uint32) ((sign << 31) | ((0x7eL + 128 - 64) << 23) |  (0xffffL << 7));
			
			}
			
		else
			{
			
			// Nan -- Just set to zero.

			return 0;
			
			}
			
		}
	
	// Normalized number

	exponent += (128 - 64);
	mantissa <<= 7;

	// Assemble sign, exponent and mantissa.

	return (uint32) ((sign << 31) | (exponent << 23) | mantissa);
	
	}

/*****************************************************************************/

inline void DNG_FloatToFP24 (uint32 input, uint8 *output)
	{
	
	int32 exponent = (int32) ((input >> 23) & 0xFF) - 128;
	int32 mantissa = input & 0x007FFFFF;

	if (exponent == 127)	// infinity or NaN
		{
		
		// Will the NaN alais to infinity? 
		
		if (mantissa != 0x007FFFFF && ((mantissa >> 7) == 0xFFFF))
			{
			
			mantissa &= 0x003FFFFF;		// knock out msb to make it a NaN
			
			}
			
		}
		
	else if (exponent > 63)		// overflow, map to infinity
		{
		
		exponent = 63;
		mantissa = 0x007FFFFF;
		
		}
		
	else if (exponent <= -64) 
		{
		
		if (exponent >= -79)		// encode as denorm
			{
			mantissa = (mantissa | 0x00800000) >> (-63 - exponent);
			}
			
		else						// underflow to zero
			{
			mantissa = 0;
			}

		exponent = -64;
		
		}

	output [0] = (uint8)(((input >> 24) & 0x80) | (uint32) (exponent + 64));
	
	output [1] = (mantissa >> 15) & 0x00FF;
	output [2] = (mantissa >>  7) & 0x00FF;
	
	}

/******************************************************************************/

// The following code was from PSDivide.h in Photoshop.

// High order 32-bits of an unsigned 32 by 32 multiply.

#ifndef MULUH

#if defined(_X86_) && defined(_MSC_VER)

inline uint32 Muluh86 (uint32 x, uint32 y)
	{
	uint32 result;
	__asm
		{
		MOV		EAX, x
		MUL		y
		MOV		result, EDX
		}
	return (result);
	}

#define MULUH	Muluh86

#else

#define MULUH(x,y)	((uint32) (((x) * (uint64) (y)) >> 32))

#endif

#endif

// High order 32-bits of an signed 32 by 32 multiply.

#ifndef MULSH

#if defined(_X86_) && defined(_MSC_VER)

inline int32 Mulsh86 (int32 x, int32 y)
	{
	int32 result;
	__asm
		{
		MOV		EAX, x
		IMUL	y
		MOV		result, EDX
		}
	return (result);
	}

#define MULSH	Mulsh86

#else

#define MULSH(x,y)	((int32) (((x) * (int64) (y)) >> 32))

#endif

#endif

/******************************************************************************/

// Random number generator (identical to Apple's) for portable use.

// This implements the "minimal standard random number generator"
// as proposed by Park and Miller in CACM October, 1988.
// It has a period of 2147483647 (0x7fffffff)

// This is the ACM standard 30 bit generator:
// x' = (x * 16807) mod 2^31-1

inline uint32 DNG_Random (uint32 seed)
	{
	
	// high = seed / 127773
	
	uint32 temp = MULUH (0x069C16BD, seed);
	uint32 high = (temp + ((seed - temp) >> 1)) >> 16;
	
	// low = seed % 127773
	
	uint32 low = seed - high * 127773;
	
	// seed = (seed * 16807) % 2147483647
	
	seed = 16807 * low - 2836 * high;
	
	if (seed & 0x80000000)
		seed += 2147483647;
		
	return seed;
	
	}

/*****************************************************************************/

class dng_dither
	{
		
	public:

		static const uint32 kRNGBits = 7;

		static const uint32 kRNGSize = 1 << kRNGBits;

		static const uint32 kRNGMask = kRNGSize - 1;

		static const uint32 kRNGSize2D = kRNGSize * kRNGSize;

	private:

		dng_memory_data fNoiseBuffer;

	private:

		dng_dither ();

		// Hidden copy constructor and assignment operator.
	
		dng_dither (const dng_dither &);
		
		dng_dither & operator= (const dng_dither &);
		
	public:

		static const dng_dither & Get ();

	public:

		const uint16 *NoiseBuffer16 () const
			{
			return fNoiseBuffer.Buffer_uint16 ();
			}
		
	};

/*****************************************************************************/

void HistogramArea (dng_host &host,
					const dng_image &image,
					const dng_rect &area,
					uint32 *hist,
					uint32 histLimit,
					uint32 plane = 0);

/*****************************************************************************/

void LimitFloatBitDepth (dng_host &host,
						 const dng_image &srcImage,
						 dng_image &dstImage,
						 uint32 bitDepth,
						 real32 scale = 1.0f);

/*****************************************************************************/

#endif
	
/*****************************************************************************/
