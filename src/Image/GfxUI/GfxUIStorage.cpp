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

bool GfxUIStorage::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y) {
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
  _tab_bar(_internal_PosX(), _internal_PosY(), _internal_Width(), 20, 0x5555CC),
  _txt(_internal_PosX(), _internal_PosY() + _tab_bar.elementHeight(), _internal_Width(), h - _tab_bar.elementHeight(), 0x5555CC)
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

bool GfxUIDataRecord::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y) {
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
