/*
File:   ImageScaler.cpp
Author: J. Ian Lindsay
Date:   2022.05.27

An image transform that scales a given area from a source Image onto another at
  the given origin.
The source and target Images can be the same or different objects.
*/

#include "../ImageUtils.h"

/*******************************************************************************
* ImageScaler
*******************************************************************************/

/* Constructor */
ImageScaler::ImageScaler(Image* i_s, Image* i_t, float scale, PixUInt s_x, PixUInt s_y, PixUInt s_w, PixUInt s_h, PixUInt t_x, PixUInt t_y) :
  _source(i_s), _target(i_t), _scale(scale)
{
  _s_x = s_x;
  _s_y = s_y;
  _s_w = (0 != s_w) ? s_w : _source->x();  // If not provided, assume the entire source image.
  _s_h = (0 != s_h) ? s_h : _source->y();  // If not provided, assume the entire source image.
  _t_x = t_x;
  _t_y = t_y;
}


void ImageScaler::setParameters(float scale, PixUInt s_x, PixUInt s_y, PixUInt s_w, PixUInt s_h, PixUInt t_x, PixUInt t_y) {
  _scale = scale;
  _s_x = s_x;
  _s_y = s_y;
  _s_w = s_w;
  _s_h = s_h;
  _t_x = t_x;
  _t_y = t_y;
}


/*
* Copy the scaled image.
*/
int8_t ImageScaler::apply() {
  const PixUInt SRC_X_RANGE_START = _s_x;
  const PixUInt SRC_X_RANGE_END   = (_s_x + _s_w);
  const PixUInt SRC_Y_RANGE_START = _s_y;
  const PixUInt SRC_Y_RANGE_END   = (_s_y + _s_h);
  const PixUInt TGT_X_RANGE_START = _t_x;
  const PixUInt TGT_X_RANGE_END   = (_t_x + (_scale * _s_w));
  const PixUInt TGT_Y_RANGE_START = _t_y;
  const PixUInt TGT_Y_RANGE_END   = (_t_y + (_scale * _s_h));
  const PixUInt SQUARE_SIZE       = (PixUInt) _scale;

  int8_t ret = -1;
  if (_source->allocated() && _target->allocated()) {
    ret--;
    // If the source range is valid...
    if ((_source->x() >= SRC_X_RANGE_END) && (_source->y() >= SRC_Y_RANGE_END)) {
      ret--;
      // And the target range can contain the scaled result...
      if ((_target->x() >= TGT_X_RANGE_END) && (_target->y() >= TGT_Y_RANGE_END)) {
        ret = 0;
        if (_scale < 1.0f) {
          PixUInt source_x = SRC_X_RANGE_START;
          PixUInt source_y = SRC_Y_RANGE_START;
          // If source area is larger than target area, we will need to sample an
          //   area of the source, and write a single pixel to the target.
          // TODO: For now, we just take the leading pixel from the source area.
          //   It would be better to take the result of a transform of all pixels.
          for (PixUInt i = TGT_X_RANGE_START; i < TGT_X_RANGE_END; i++) {
            for (PixUInt j = TGT_Y_RANGE_START; j < TGT_Y_RANGE_END; j++) {
              uint32_t color = _source->getPixelAsFormat(source_x, source_y, _target->format());
              _target->setPixel(i, j, color);
              source_y += SQUARE_SIZE;
            }
            source_x += SQUARE_SIZE;
            source_y = SRC_Y_RANGE_START;
          }
        }
        else {
          //c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, "(%u %u)\t(%u %u)", SRC_X_RANGE_START, SRC_Y_RANGE_START, _s_x, _s_y);
          PixUInt target_x = TGT_X_RANGE_START;
          PixUInt target_y = TGT_Y_RANGE_START;
          // If source area is smaller than target area, we will need to sample a
          //   single pixel of the source, and write a rectangle to the target.
          for (PixUInt i = SRC_X_RANGE_START; i < SRC_X_RANGE_END; i++) {
            for (PixUInt j = SRC_Y_RANGE_START; j < SRC_Y_RANGE_END; j++) {
              uint32_t color = _source->getPixelAsFormat(i, j, _target->format());
              _target->fillRect(target_x, target_y, SQUARE_SIZE, SQUARE_SIZE, color);
              target_y += SQUARE_SIZE;
            }
            target_x += SQUARE_SIZE;
            target_y = TGT_Y_RANGE_START;
          }
        }
      }
    }
  }
  return ret;
}
