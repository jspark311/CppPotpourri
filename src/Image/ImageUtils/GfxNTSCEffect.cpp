/*
File:   GfxNTSCEffect.cpp
Author: J. Ian Lindsay
Date:   2022.05.27

An image transform that applies synthetic NTSC distortions.

TODO: Port JSon's code.
*/

#include "../ImageUtils.h"


/*******************************************************************************
* GfxNTSCEffect
*******************************************************************************/

/* Constructor */
GfxNTSCEffect::GfxNTSCEffect(Image* i_s, Image* i_t) :
  _source(i_s), _target(i_t)
{
}



int8_t GfxNTSCEffect::apply() {
  return -1;
}
