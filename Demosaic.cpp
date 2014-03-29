/*
  Copyright 2008-2013 LibRaw LLC (info@libraw.org)

LibRaw is free software; you can redistribute it and/or modify
it under the terms of the one of three licenses as you choose:

1. GNU LESSER GENERAL PUBLIC LICENSE version 2.1
   (See file LICENSE.LGPL provided in LibRaw distribution archive for details).

2. COMMON DEVELOPMENT AND DISTRIBUTION LICENSE (CDDL) Version 1.0
   (See file LICENSE.CDDL provided in LibRaw distribution archive for details).

3. LibRaw Software License 27032010
   (See file LICENSE.LibRaw.pdf provided in LibRaw distribution archive for details).

   This file is generated from Dave Coffin's dcraw.c
   dcraw.c -- Dave Coffin's raw photo decoder
   Copyright 1997-2010 by Dave Coffin, dcoffin a cybercom o net

   Look into dcraw homepage (probably http://cybercom.net/~dcoffin/dcraw/)
   for more information
*/

#include <cmath>
#include <ctime>
#include <cstdint>
#include <cstdlib>
#include "Demosaic.hpp"
using namespace std;
using namespace hdrmerge;

#define CLASS Demosaic::
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define LIM(x,min,max) MAX(min,MIN(x,max))
#define CLIP(x) LIM(x,0,65535)
#include "amaze_demosaic_RT.cc"


Demosaic::Demosaic(const ImageStack & stack)
: md(stack.getImage(0).getMetaData()), width(stack.getWidth()), height(stack.getHeight()), pre_mul(md.preMul) {
    image.reset(new float[width*height][4]);
}


void Demosaic::save(const std::string fileName) {

}
