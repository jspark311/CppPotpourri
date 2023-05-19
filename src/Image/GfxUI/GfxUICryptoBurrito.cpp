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
  _rng_buffer(_internal_Width(), FilteringStrategy::RAW),
  _vis_0(
    GfxUILayout(
      _internal_PosX(), _internal_PosY(),
      _internal_Width(), (_internal_Height() >> 1),
      1, 0, 0, 0,   // Margins_px(t, b, l, r)
      1, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    sty,
    &_rng_buffer,
    (GFXUI_FLAG_ALWAYS_REDRAW)
  ),
  _schedule_rng_update(55000, -1, true, this)
{
  _add_child(&_vis_0);
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


bool GfxUICryptoRNG::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
  bool ret = _rng_buffer.dirty();

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


PollingResult GfxUICryptoRNG::poll() {
  PollingResult ret = PollingResult::NO_ACTION;
  if (_rng_buffer.windowSize() != _internal_Width()) {
    if (0 != _rng_buffer.windowSize(_internal_Width())) {
      ret = PollingResult::ERROR;
    }
  }
  if (_rng_buffer.initialized() & (PollingResult::NO_ACTION == ret)) {
    random_fill((uint8_t*) _rng_buffer.memPtr(), _rng_buffer.memUsed());
    _rng_buffer.feedFilter();
    ret = PollingResult::ACTION;
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