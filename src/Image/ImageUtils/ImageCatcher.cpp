/*
File:   ImageCatcher.cpp
Author: J. Ian Lindsay
Date:   2022.08.02

A utility class that accepts an Image from a Link, and inflates it.
*/

#include "../ImageUtils.h"


/*******************************************************************************
* ImageCatcher
*******************************************************************************/

/* Constructor */
ImageCatcher::ImageCatcher() :
  _id(0), _target(nullptr), _t_x(0), _t_y(0), _t_w_max(0), _t_h_max(0),
  _target_is_ours(true) {}


/* Constructor */
ImageCatcher::ImageCatcher(Image* i_t, PixUInt x, PixUInt y, PixUInt w, PixUInt h) :
  _id(0), _target(i_t), _t_x(x), _t_y(y), _t_w_max(w), _t_h_max(h),
  _target_is_ours(false)
{
  // If not provided, assume the entire source image.
  if (0 == _t_w_max) {  _t_w_max = _target->x();  }
  if (0 == _t_h_max) {  _t_h_max = _target->y();  }
}


/* Destructor */
ImageCatcher::~ImageCatcher() {
  if (_target_is_ours) {
    delete _target;
    _target = nullptr;
  }
}


/*
*
*/
int8_t ImageCatcher::apply(KeyValuePair* kvp) {
  int8_t ret = -1;
  if (nullptr != kvp) {
    uint32_t remote_id = 0;
    if (0 == kvp->valueWithKey("id", &remote_id)) {
      if (0 != remote_id) {
        if (0 == _id) {
          // If the ID isn't yet assigned, we take whatever is in the payload. Going
          //   forward, we will only respond to Images from the same ID.
          _id = remote_id;
        }
        if (remote_id == _id) {
          if (_target_is_ours) {
            // If the target Image is ours to define and allocate....
            if (nullptr == _target) {
              // TODO: Snatch the inflated image from the KVPs and mark it such that it
              //   won't be free'd at the end of the message cycle.
            }
          }
          else {
            // Otherwise, it was provided with strict unchangable boundaries.
            // TODO: If the image size fits within the specified width/height,
            //   copy the remote Image to the target.
            //if () {
            //}
            //else {
            //  // TODO: Log: Received Image is too big to fit in defined area.
            //}
          }
          ret = 0;
        }
        else {
          // TODO: We aren't going to accept this Image. It has the wrong ID.
        }
      }
      else {
        // TODO: Log: ID has illegal value.
      }
    }
    else {
      // TODO: Log: No ID in message.
    }
  }
  return ret;
}
