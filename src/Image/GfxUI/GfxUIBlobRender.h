/*
File:   GfxUIBlobRender.h
Author: J. Ian Lindsay
Date:   2023.12.25

NOTE: Do not include this file directly. Only via GfxUI.h.

These classes are built on top of the GfxUI classes, and implement higher-level
  functional elements of a UI.
*/

#ifndef __C3P_GFXUI_KIT_BLOB_RENDER_H
#define __C3P_GFXUI_KIT_BLOB_RENDER_H

#include "../../C3PValue/C3PValue.h"
#include "../ImageUtils/BlobPlotter.h"

#define GFXUI_BLOB_RENDER_FLAG_INHIBIT_REFRESH    0x02000000   //

/*******************************************************************************
* Graphical rendering of binary blobs, with options.
*******************************************************************************/
class GfxUIBlobRender : public GfxUIElement {
  public:
    GfxUIBlobRender(C3PValue* value, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f = 0);
    ~GfxUIBlobRender();

    inline void inhibitRefresh(bool x) {  _class_set_flag(GFXUI_BLOB_RENDER_FLAG_INHIBIT_REFRESH, x);  };
    inline bool inhibitRefresh() {        return _class_flag(GFXUI_BLOB_RENDER_FLAG_INHIBIT_REFRESH);  };  // TODO: Promote?

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log);

    int8_t setPlotter(const BlobPlotterID);
    int8_t setStyler(const BlobStylerID);
    int8_t setMapFile(char* path);


  private:
    C3PValue*      _value;
    BlobPlotter*   _plotter;
    BlobStyler*    _styler;
    BlobPlotterID  _plotter_id;
    BlobStylerID   _styler_id;
    uint16_t       _last_trace;
    bool           _stacked_ir;

    GfxUITabBar   _plotter_selector;
    GfxUITabBar   _style_selector;
    GfxUITextArea _hex_txt;
    GfxUITextArea _map_txt;

    int8_t _check_plotter(Image*);
    int8_t _check_styler(Image*);
};


#endif  // __C3P_GFXUI_KIT_BLOB_RENDER_H
