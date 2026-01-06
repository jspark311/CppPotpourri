/*
File:   GfxCRTBloomEffect.cpp
Author: J. Ian Lindsay
Date:   2025.07.29

An image transform that attempts to mimic the appearance of a CRT.
*/

#include "../ImageUtils.h"

#if defined(CONFIG_C3P_IMG_SUPPORT)
/*******************************************************************************
* GfxCRTBloomEffect
*******************************************************************************/

/* Constructor */
GfxCRTBloomEffect::GfxCRTBloomEffect(Image* in, Image* out)
  : _source(in),
    _target(out),
    _src_addr(0, 0),
    _width(0),
    _height(0),
    _bloom_factor(0.5f),
    _edge_curvature(0.5f) {}


int8_t GfxCRTBloomEffect::setSourceFrame(const PixAddr A, const PixUInt W, const PixUInt H) {
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


int8_t GfxCRTBloomEffect::apply() {
  int8_t STATUS = 0;
  if ((_width == 0) || (_height == 0) ||
      (_source == (Image*)0) || (_target == (Image*)0)) {
    STATUS = -1;
  } else {
    const int RADIUS = (int)(_bloom_factor * 5.0f) + 1;
    const float INV_SUM = 1.0f / ((RADIUS * 2 + 1) * (RADIUS * 2 + 1));
    const float CX = _src_addr.x + _width * 0.5f;
    const float CY = _src_addr.y + _height * 0.5f;
    PixAddr pos;
    for (pos.y = _src_addr.y; pos.y < _src_addr.y + _height; pos.y++) {
      for (pos.x = _src_addr.x; pos.x < _src_addr.x + _width; pos.x++) {
        float sumR = 0.0f;
        float sumG = 0.0f;
        float sumB = 0.0f;
        for (int dy = -RADIUS; dy <= RADIUS; dy++) {
          for (int dx = -RADIUS; dx <= RADIUS; dx++) {
            PixAddr p = { (uint16_t)(pos.x + dx), (uint16_t)(pos.y + dy) };
            uint32_t c = _source->getPixel(p);
            sumR += (float)((c >> 16) & 0xFF);
            sumG += (float)((c >>  8) & 0xFF);
            sumB += (float)( c        & 0xFF);
          }
        }
        int32_t r = (int32_t)roundf(sumR * INV_SUM);
        int32_t g = (int32_t)roundf(sumG * INV_SUM);
        int32_t b = (int32_t)roundf(sumB * INV_SUM);

        // Edge curvature modulation
        float dx = (pos.x - CX) / (_width * 0.5f);
        float dy = (pos.y - CY) / (_height * 0.5f);
        float dist2 = dx * dx + dy * dy;
        float edge = 1.0f - _edge_curvature * dist2;
        if (edge < 0.0f) {
          edge = 0.0f;
        }
        r = (int32_t)(r * edge);
        g = (int32_t)(g * edge);
        b = (int32_t)(b * edge);

        uint8_t rC = _clip_to_byte(r);
        uint8_t gC = _clip_to_byte(g);
        uint8_t bC = _clip_to_byte(b);
        uint32_t outC = ((uint32_t)rC << 16) |
                        ((uint32_t)gC <<  8) |
                         (uint32_t)bC;
        _target->setPixel(pos, outC);
      }
    }
  }
  return STATUS;
}

uint8_t GfxCRTBloomEffect::_clip_to_byte(int32_t v) {
  uint8_t result;
  if (v < 0) {         result = 0;    }
  else if (v > 255) {  result = 255;  }
  else {               result = (uint8_t)v;  }
  return result;
}


#endif
