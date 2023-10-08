/*
File:   GfxUIDataRecord.cpp
Author: J. Ian Lindsay
Date:   2022.06.19

This is a view that should be used to represent a single DataRecord, generally.
  It may or may not be included by a view that represents a specific type of
  DataRecord, although it probably would be, in some fashion.
*/

#include "../GfxUI.h"


/*******************************************************************************
* GfxUIDataRecord
*******************************************************************************/
GfxUIDataRecord::GfxUIDataRecord(SimpleDataRecord* record, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f)
  : GfxUITabbedContentPane(lay, sty, (f | GFXUI_FLAG_ALWAYS_REDRAW)), _record(record)
{
  // Note our subordinate objects...
  //addTab("Overview", true);
  //addTab("Blocks");
}


int GfxUIDataRecord::_render(UIGfxWrapper* ui_gfx) {
  StringBuilder _tmp_sbldr;
  switch (_tab_bar.activeTab()) {
    case 0:
      _record->printDebug(&_tmp_sbldr);
      break;
    case 1:
      _tmp_sbldr.concat("Nothing here yet.\n");
      break;
    case 2:
      _tmp_sbldr.concat("Nothing here yet, either.\n");
      break;
    default:
      break;
  }
  return 1;
}


bool GfxUIDataRecord::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
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
