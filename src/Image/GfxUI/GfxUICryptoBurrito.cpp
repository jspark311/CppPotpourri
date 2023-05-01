/*
File:   GfxUICryptoBurrito.cpp
Author: J. Ian Lindsay
Date:   2023.04.08

*/

#include "../GfxUI.h"

#if defined(__HAS_CRYPT_WRAPPER)

/*******************************************************************************
* GfxUICryptoRNG
*******************************************************************************/

int GfxUICryptoRNG::_render(UIGfxWrapper* ui_gfx) {
  int ret = 0;
  if (!_rng_buffer.initialized()) {
    _rng_buffer.init();

  }
  if (_rng_buffer.initialized()) {
    if (_rng_buffer.windowSize() != _internal_Width()) {
      if (0 == _rng_buffer.windowSize(_internal_Width())) {
        random_fill((uint8_t*) _rng_buffer.memPtr(), _rng_buffer.memUsed());
        _rng_buffer.feedFilter();
      }
    }
    ret = _rng_buffer.dirty() ? 1 : 0;
  }
  return ret;
}


bool GfxUICryptoRNG::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
  bool ret = false;

  switch (GFX_EVNT) {
    case GfxUIEvent::TOUCH:
      if (_rng_buffer.initialized()) {
        random_fill((uint8_t*) _rng_buffer.memPtr(), _rng_buffer.memUsed());
        _rng_buffer.feedFilter();
        ret = true;
      }
      break;
    default:
      break;
  }

  if (ret) {
    _need_redraw(true);
  }
  return ret;
}


/*******************************************************************************
* CryptoBurrito
*******************************************************************************/

int GfxUICryptoBurrito::_render(UIGfxWrapper* ui_gfx) {
  return GfxUITabbedContentPane::_render(ui_gfx);
}

//bool GfxUICryptoBurrito::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
//  bool ret = false;
//
//  if (ret) {
//    _need_redraw(true);
//  }
//  return ret;
//}


#endif  // __HAS_CRYPT_WRAPPER
