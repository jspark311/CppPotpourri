/*
File:   GfxUIKeyValuePair.h
Author: J. Ian Lindsay
Date:   2023.05.29

NOTE: Do not include this file directly. Only via GfxUI.h.

These classes are built on top of the GfxUI classes, and implement higher-level
  functional elements of a UI.
*/

#ifndef __C3P_GFXUI_KIT_KVP_H
#define __C3P_GFXUI_KIT_KVP_H

#include "../Image.h"
#include "../ImageUtils.h"
#include "../../KeyValuePair.h"

/*******************************************************************************
* Graphical tool for looking at KVP data.
*******************************************************************************/
class GfxUIKeyValuePair : public GfxUIElement {
  public:
    GfxUIKeyValuePair(KeyValuePair* kvp, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f = 0);
    ~GfxUIKeyValuePair() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);

    int8_t showKVP(KeyValuePair*);

  private:
    KeyValuePair* _kvp;
};


/*******************************************************************************
* Graphical tool for KVPs
*******************************************************************************/

class GfxUIKVPUtil : public GfxUIElement {
  public:
    GfxUIKVPUtil(const GfxUILayout lay, const GfxUIStyle sty, uint32_t f = 0);
    ~GfxUIKVPUtil();

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);

    int8_t setKVP(KeyValuePair*);
    int8_t setKVP(StringBuilder*, TCode);


  private:
    GfxUITextArea _txt_serialized;
    GfxUIKeyValuePair _kvp_view;
};

#endif  // __C3P_GFXUI_KIT_KVP_H
