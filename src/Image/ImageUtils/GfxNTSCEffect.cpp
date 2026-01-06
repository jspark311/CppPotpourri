/*
File:   GfxNTSCEffect.cpp
Author: J. Ian Lindsay
Date:   2025.07.27

An image transform that applies synthetic NTSC distortions.
*/

#include "../ImageUtils.h"

#if defined(CONFIG_C3P_IMG_SUPPORT)

/*******************************************************************************
* GfxNTSCEffect
*******************************************************************************/

/* Constructor */
GfxNTSCEffect::GfxNTSCEffect(Image* i_s, Image* i_t) :
  _source(i_s),
  _target(i_t),
  _src_addr(0, 0),
  _width(0),
  _height(0) {}



uint8_t GfxNTSCEffect::_clip_to_byte(int32_t v) {
  uint8_t result;
  if (v < 0) {         result = 0;    }
  else if (v > 255) {  result = 255;  }
  else {               result = (uint8_t)v;  }
  return result;
}



int8_t GfxNTSCEffect::setSourceFrame(const PixAddr A, const PixUInt W, const PixUInt H) {
  int8_t ret = -1;
  if ((nullptr != _source) && (nullptr != _target)) {
    const PixUInt S_WIDTH  = _source->width();
    const PixUInt S_HEIGHT = _source->height();
    const PixUInt T_WIDTH  = _target->width();
    const PixUInt T_HEIGHT = _target->height();
    const bool CONSTRAINED_W = (W + A.x) < (T_WIDTH);
    const bool CONSTRAINED_H = (H + A.y) < (T_HEIGHT);
    ret--;
    if (CONSTRAINED_W & CONSTRAINED_H) {
      _src_addr = A;
      _width    = W;
      _height   = H;
      ret = 0;
    }
  }
  return ret;
}



int8_t GfxNTSCEffect::apply() {
  int8_t ret = -1;
  if (_source->allocated() && _target->allocated()) {
    // Noise parameters
    for (PixUInt y = 0; y < _height; y++) {
      for (PixUInt x = 0; x < _width;  x++) {
        // Fetch source pixel (TODO: R8G8B8 is the assumed format.)
        const uint32_t SRC_COLOR = _source->getPixel(_src_addr.x + x, _src_addr.y + y);
        const uint8_t  R   = static_cast<uint8_t>((SRC_COLOR >> 16) & 0xFFU);
        const uint8_t  G   = static_cast<uint8_t>((SRC_COLOR >>  8) & 0xFFU);
        const uint8_t  B   = static_cast<uint8_t>( SRC_COLOR        & 0xFFU);

        // RGB -> YIQ
        const float Y = (0.299 * R) + (0.587 * G) + (0.114 * B);
        const float I = (0.596 * R) - (0.275 * G) - (0.321 * B);
        const float Q = (0.212 * R) - (0.523 * G) + (0.311 * B);

        // Add analog noise to luma
        const float noise = (((float) (std::rand() % 2001) / 1000.0) - 1.0) * _noise_level;
        const float Yn = Y + noise * 255.0;

        // YIQ -> RGB
        const float rD = Yn + (0.956 * I) + (0.621 * Q);
        const float gD = Yn - (0.272 * I) - (0.647 * Q);
        const float bD = Yn - (1.106 * I) + (1.703 * Q);

        // Round and clip...
        const uint8_t RC = _clip_to_byte((int32_t) std::round(rD * 255.0));
        const uint8_t GC = _clip_to_byte((int32_t) std::round(gD * 255.0));
        const uint8_t BC = _clip_to_byte((int32_t) std::round(bD * 255.0));
        const uint32_t OUT_COLOR = ((uint32_t) RC << 16) | ((uint32_t) GC <<  8) | ((uint32_t) BC);

        _target->setPixel(_src_addr.x + x, _src_addr.y + y, OUT_COLOR);
      }
    }
  }
  return ret;
}

#endif
