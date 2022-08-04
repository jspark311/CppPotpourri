/*
File:   ImageCaster.cpp
Author: J. Ian Lindsay
Date:   2022.08.02

A utility class that casts an Image over a Link.
*/

#include "../ImageUtils.h"


static uint32_t image_caster_id = 0;


/*******************************************************************************
* ImageCaster
*******************************************************************************/

/* Constructor */
ImageCaster::ImageCaster(ManuvrLink* l, Image* i_s, int x, int y, int w, int h) :
  _id(image_caster_id++), _link(l), _source(i_s), _s_x(x), _s_y(y)
{
  _s_w = (0 != w) ? w : (_source->x() - x);  // If not provided, assume the entire source image.
  _s_h = (0 != h) ? h : (_source->y() - y);  // If not provided, assume the entire source image.
}


bool ImageCaster::busy() {
  bool ret = false;
  return ret;
}


/*
* Copy the scaled image.
*/
int8_t ImageCaster::apply() {
  int8_t ret = -1;
  if (_link->isConnected()) {
    ret--;
    if (_source->allocated()) {
      KeyValuePair* msg_kvp = new KeyValuePair("IMG_CAST", "fxn");
      ret--;
      if (nullptr != msg_kvp) {
        msg_kvp->append(_id, "id");
        msg_kvp->append(_source, typecodeToStr(TCode::IMAGE));
        ret--;
        if (0 == _link->send(msg_kvp, false)) {
          ret = 0;
        }
      }
    }
  }
  return ret;
}
