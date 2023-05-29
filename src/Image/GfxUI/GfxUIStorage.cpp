/*
File:   GfxUIStorage.cpp
Author: J. Ian Lindsay
Date:   2022.06.19

*/

#include "../GfxUI.h"


/*******************************************************************************
* GfxUIStorage
*******************************************************************************/

GfxUIStorage::GfxUIStorage(Storage* storage, uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t f)
  : GfxUIElement(x, y, w, h, f), _storage(storage)
{}

int GfxUIStorage::_render(UIGfxWrapper* ui_gfx) {
  return 1;
}

bool GfxUIStorage::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
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
* GfxUIDataRecord
*******************************************************************************/

GfxUIDataRecord::GfxUIDataRecord(DataRecord* record, uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t f)
  : GfxUIElement(x, y, w, h, (f | GFXUI_FLAG_ALWAYS_REDRAW)), _record(record),
  _tab_bar(
    GfxUILayout(
      internalPosX(), internalPosY(), internalWidth(), 20,
      1, 1, 1, 0,
      0, 1, 0, 0               // Border_px(t, b, l, r)
    ),
    GfxUIStyle(0, // bg
      0xFFFFFF,   // border
      0xFFFFFF,   // header
      0x5555CC,   // active
      0xA0A0A0,   // inactive
      0xFFFFFF,   // selected
      0x202020,   // unselected
      1           // t_size
    ),
    (GFXUI_FLAG_DRAW_FRAME_D)
  ),
  _txt(
    GfxUILayout(
      internalPosX(), (internalPosY() + _tab_bar.elementHeight()), internalWidth(), (internalHeight() - _tab_bar.elementHeight()),
      1, 1, 1, 1,
      0, 0, 0, 0               // Border_px(t, b, l, r)
    ),
    GfxUIStyle(0, // bg
      0xFFFFFF,   // border
      0xFFFFFF,   // header
      0x5555CC,   // active
      0xA0A0A0,   // inactive
      0xFFFFFF,   // selected
      0x202020,   // unselected
      1
    )
  )
{
  // Note our subordinate objects...
  _tab_bar.addTab("Overview", true);
  _tab_bar.addTab("Blocks");
  _tab_bar.addTab("Storage");
  _add_child(&_tab_bar);
  _add_child(&_txt);
}


int GfxUIDataRecord::_render(UIGfxWrapper* ui_gfx) {
  StringBuilder _tmp_sbldr;
  _txt.clear();
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
  _txt.provideBuffer(&_tmp_sbldr);
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
