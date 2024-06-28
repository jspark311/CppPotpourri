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
/*
* Constructor
*/
GfxUICryptoRNG::GfxUICryptoRNG(const GfxUILayout lay, const GfxUIStyle sty, uint32_t f) :
  GfxUIElement(lay, sty, f),
  _rng_buffer(256, FilteringStrategy::RAW),
  _vis_0(
    GfxUILayout(
      internalPosX(), internalPosY(),
      258, (internalHeight() >> 1),
      1, 0, 0, 0,   // Margins_px(t, b, l, r)
      1, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    sty,
    &_rng_buffer,
    (GFXUI_FLAG_ALWAYS_REDRAW | GFXUI_SENFILT_FLAG_DRAW_CURVE | GFXUI_SENFILT_FLAG_DRAW_GRID | GFXUI_SENFILT_FLAG_AUTOSCALE_Y)
  ),
  _schedule_rng_update("rng_update", 55000, -1, true, this)
{
  _add_child(&_vis_0);
  _vis_0.majorDivX(0);
  _vis_0.majorDivY(100);
  C3PScheduler::getInstance()->addSchedule(&_schedule_rng_update);
}


/*
* Destructor
*/
GfxUICryptoRNG::~GfxUICryptoRNG() {
  C3PScheduler::getInstance()->removeSchedule(&_schedule_rng_update);
}


int GfxUICryptoRNG::_render(UIGfxWrapper* ui_gfx) {
  int ret = 0;
  if (!_rng_buffer.initialized()) {
    _rng_buffer.init();

  }
  ret = _rng_buffer.dirty() ? 1 : 0;
  return ret;
}


bool GfxUICryptoRNG::_notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log) {
  bool ret = _rng_buffer.dirty();

  switch (GFX_EVNT) {
    case GfxUIEvent::TOUCH:
      if (_rng_buffer.initialized()) {
        uint8_t tmp_buffer[4096];
        random_fill(tmp_buffer, sizeof(tmp_buffer));
        _rng_buffer.purge();
        for (uint32_t i = 0; i < sizeof(tmp_buffer); i++) {
          *(_rng_buffer.memPtr() + tmp_buffer[i]) = *(_rng_buffer.memPtr() + tmp_buffer[i])+1;
        }
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


PollResult GfxUICryptoRNG::poll() {
  PollResult ret = PollResult::NO_ACTION;
  if (_rng_buffer.windowSize() != internalWidth()) {
    if (0 != _rng_buffer.windowSize(internalWidth())) {
      ret = PollResult::ERROR;
    }
  }
  if (_rng_buffer.initialized() & (PollResult::NO_ACTION == ret)) {
        uint8_t tmp_buffer[4096];
        random_fill(tmp_buffer, sizeof(tmp_buffer));
        _rng_buffer.purge();
        for (uint32_t i = 0; i < sizeof(tmp_buffer); i++) {
          *(_rng_buffer.memPtr() + tmp_buffer[i]) = *(_rng_buffer.memPtr() + tmp_buffer[i])+1;
        }
        _rng_buffer.feedFilter();
    ret = PollResult::ACTION;
  }
  return ret;
}


/*******************************************************************************
* CryptoBurrito
*******************************************************************************/

int GfxUICryptoBurrito::_render(UIGfxWrapper* ui_gfx) {
  return GfxUITabbedContentPane::_render(ui_gfx);
}



//bool GfxUICryptoBurrito::_notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log) {
//  bool ret = false;
//
//  if (ret) {
//    _need_redraw(true);
//  }
//  return ret;
//}


#endif  // __HAS_CRYPT_WRAPPER
