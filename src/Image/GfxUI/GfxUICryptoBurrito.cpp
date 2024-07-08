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
  _rng_buffer(1024),
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
  _vis_histogram(260, 260),
  _schedule_rng_update("rng_update", 200000, -1, true, this)
{
  _add_child(&_vis_0);
  _vis_0.majorDivX(0);
  _vis_0.majorDivY(100);
  _vis_histogram.fg_color = sty.color_active;
  _vis_histogram.bg_color = sty.color_bg;
  _vis_histogram.trace0.color       = sty.color_active;
  _vis_histogram.trace0.dataset     = _histo_data;
  _vis_histogram.trace0.data_len    = 256;
  _vis_histogram.trace0.offset_x    = 0;
  _vis_histogram.trace0.autoscale_y = true;
  _vis_histogram.trace0.enabled     = true;

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
  if (_rng_buffer.initialized() && _render_histo) {
    ret = 1;
    const PixUInt I_X = internalPosX();
    const PixUInt I_Y = internalPosY();
    const PixUInt I_W = internalWidth();
    const PixUInt I_H = internalHeight();
    _render_histo = false;
    _vis_histogram.drawGraph(ui_gfx->img(), (I_X + _vis_0.elementWidth()), I_Y);
    // Having just drawn the graph, the stats in the trace0 object will be
    //   fresh. Print them.
    StringBuilder img_print;
    const uint32_t SPREAD = (_vis_histogram.trace0.maxValue() - _vis_histogram.trace0.minValue());
    double var = (double) SPREAD / (double) _rng_buffer.totalSamples();

    img_print.concatf("%u / %u   var: %f", _vis_histogram.trace0.maxValue(), _vis_histogram.trace0.minValue(), var);

    _vis_histogram.drawGraph(ui_gfx->img(), (I_X + _vis_0.elementWidth()), I_Y);
    ui_gfx->img()->setCursor((260 + I_X + _vis_0.elementWidth()), I_Y);
    ui_gfx->img()->writeString(&img_print);
  }
  return ret;
}


int8_t GfxUICryptoRNG::_resample_rng() {
  int8_t ret = -1;
  if (!_rng_buffer.initialized()) {
    _rng_buffer.init();
  }
  if (_rng_buffer.initialized()) {
    const uint32_t RNG_BYTE_LEN = (_rng_buffer.windowSize() << 2);
    uint8_t tmp_buffer[RNG_BYTE_LEN];
    random_fill(tmp_buffer, RNG_BYTE_LEN);
    for (uint32_t i = 0; i < RNG_BYTE_LEN; i++) {
      *(((uint8_t*)_rng_buffer.memPtr()) + i) = tmp_buffer[i];
      _histo_data[tmp_buffer[i]]++;
    }
    _render_histo = true;
    _rng_buffer.feedSeries();   // Flash the entire TimeSeries state all at once.
    ret = 0;
  }
  return ret;
}


bool GfxUICryptoRNG::_notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log) {
  switch (GFX_EVNT) {
    case GfxUIEvent::TOUCH:
      _resample_rng();
      break;
    default:
      break;
  }

  bool ret = _rng_buffer.dirty();
  if (ret) {
    _need_redraw(true);
  }
  return ret;
}


PollResult GfxUICryptoRNG::poll() {
  PollResult ret = PollResult::NO_ACTION;
  //if (_rng_buffer.windowSize() != internalWidth()) {
  //  if (0 != _rng_buffer.windowSize(internalWidth())) {
  //    ret = PollResult::ERROR;
  //  }
  //}
  if (PollResult::NO_ACTION == ret) {
    _resample_rng();
    // TODO: Feed the SNR series.
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
