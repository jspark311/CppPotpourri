/*
File:   GfxUIC3PScheduler.cpp
Author: J. Ian Lindsay
Date:   2023.05.18

*/

#include "../GfxUI.h"

/*******************************************************************************
* GfxUIC3PScheduler
*******************************************************************************/
/**
* Constructor
* The scheduler is a singleton class, and so we will not require references.
*/
GfxUIC3PScheduler::GfxUIC3PScheduler(const GfxUILayout lay, const GfxUIStyle sty, uint32_t f) :
  GfxUITabbedContentPane(lay, sty, (f | GFXUI_FLAG_ALWAYS_REDRAW)),
  _pane_info(0, 0, 0, 0),
  _pane_schedules(0, 0, 0, 0),
  _txt(
    GfxUILayout(
      internalPosX(), internalPosY(),
      internalWidth(), 64,
      1, 0, 0, 0,   // Margins_px(t, b, l, r)
      0, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    sty
  ),
  _sw_svc_loop(
    "Service Loop",
    &(C3PScheduler::getInstance()->profiler_service),
    GfxUILayout(
      internalPosX(), (_txt.elementPosY() + _txt.elementHeight()),
      internalWidth(), 32,
      1, 0, 0, 0,   // Margins_px(t, b, l, r)
      0, 0, 0, 1    // Border_px(t, b, l, r)
    ),
    sty
  ),
  _sw_deadband(
    "Deadband",
    &(C3PScheduler::getInstance()->profiler_service),
    GfxUILayout(
      internalPosX(), (_sw_svc_loop.elementPosY() + _sw_svc_loop.elementHeight()),
      internalWidth(), 32,
      1, 0, 0, 0,   // Margins_px(t, b, l, r)
      0, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    sty
  )
{
  // Note our subordinate objects...
  _pane_schedules.add_child(&_txt);
  _pane_schedules.add_child(&_sw_svc_loop);
  _pane_schedules.add_child(&_sw_deadband);
  addTab("Info", &_pane_info, true);
  addTab("Schedules", &_pane_schedules);
}


int GfxUIC3PScheduler::_render(UIGfxWrapper* ui_gfx) {
  StringBuilder tmp_str;
  C3PScheduler* scheduler = C3PScheduler::getInstance();
  if (scheduler) {
    tmp_str.concatf("Schedule count:   %u\n", scheduler->scheduleCount());
    //tmp_str.concatf("\tLoops (SVC/ISR):  %u / %u\n\n", profiler_service.executions(), _isr_count);
  }
  else {
    tmp_str.concat("Scheduler not created.\n");
  }
  _txt.clear();
  _txt.provideBuffer(&tmp_str);
  return GfxUITabbedContentPane::_render(ui_gfx);
}


bool GfxUIC3PScheduler::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
  bool ret = false;

  switch (GFX_EVNT) {
    default:
      break;
  }

  if (ret) {
    _need_redraw(true);
  }
  return ret;
}


/*******************************************************************************
* GfxUIC3PSchedule
*******************************************************************************/

/**
* Constructor
*/
GfxUIC3PSchedule::GfxUIC3PSchedule(const unsigned int ID, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f) :
  GfxUIElement(lay, sty, f),
  _SID(ID) {};


int GfxUIC3PSchedule::_render(UIGfxWrapper* ui_gfx) {
  int ret = 0;
  return ret;
}

bool GfxUIC3PSchedule::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
  bool ret = false;

  if (ret) {
    _need_redraw(true);
  }
  return ret;
}
