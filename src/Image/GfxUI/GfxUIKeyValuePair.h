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

#include "../../C3PValue/C3PValue.h"
#include "../../C3PValue/KeyValuePair.h"

#define GFXUI_C3PVAL_FLAG_SHOW_TYPE_INFO     0x01000000   //
#define GFXUI_C3PVAL_FLAG_INHIBIT_REFRESH    0x02000000   //
#define GFXUI_C3PVAL_FLAG_OWNS_OBJECT        0x04000000   //
#define GFXUI_C3PVAL_FLAG_HOVER_RESPONSE     0x08000000   //
#define GFXUI_C3PVAL_FLAG_RESIZE_ON_RENDER   0x10000000   // If set, the GfxGUIElement will resize to fit upon rendering.

/*******************************************************************************
* Graphical rendering of the core classes in C3P's type-abstraction machinery.
*******************************************************************************/
/* Graphical rendering of a C3PType object. */
class GfxUIC3PType : public GfxUIElement {
  public:
    GfxUIC3PType(const TCode, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f = 0);
    ~GfxUIC3PType() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log);


  private:
    C3PType* _type;
};



/* Graphical rendering of a C3PValue object. */
class GfxUIC3PValue : public GfxUIElement {
  public:
    GfxUIC3PValue(C3PValue* value, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f = 0);
    ~GfxUIC3PValue();

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log);

    inline void showTypeInfo(bool x) {    _class_set_flag(GFXUI_C3PVAL_FLAG_SHOW_TYPE_INFO, x);   };
    inline bool showTypeInfo() {          return _class_flag(GFXUI_C3PVAL_FLAG_SHOW_TYPE_INFO);   };
    inline void inhibitRefresh(bool x) {  _class_set_flag(GFXUI_C3PVAL_FLAG_INHIBIT_REFRESH, x);  };
    inline bool inhibitRefresh() {        return _class_flag(GFXUI_C3PVAL_FLAG_INHIBIT_REFRESH);  };
    inline void hoverResponse(bool x) {   _class_set_flag(GFXUI_C3PVAL_FLAG_HOVER_RESPONSE, x);   };
    inline bool hoverResponse() {         return _class_flag(GFXUI_C3PVAL_FLAG_HOVER_RESPONSE);   };
    inline void reapObject(bool x) {      _class_set_flag(GFXUI_C3PVAL_FLAG_OWNS_OBJECT, x);      };
    inline bool reapObject() {            return _class_flag(GFXUI_C3PVAL_FLAG_OWNS_OBJECT);      };
    inline void resizeOnRender(bool x) {  _class_set_flag(GFXUI_C3PVAL_FLAG_RESIZE_ON_RENDER, x); };
    inline bool resizeOnRender() {        return _class_flag(GFXUI_C3PVAL_FLAG_RESIZE_ON_RENDER); };


  private:
    //GfxUIC3PType _type_render;
    C3PValue* _value;
    PixUInt _bounding_w;  // Area actually needed for last render. Accounts for border/margin.
    PixUInt _bounding_h;  // Area actually needed for last render. Accounts for border/margin.
    uint16_t _last_trace;
    bool _stacked_ir;
    bool _stacked_sti;
};



/* Graphical rendering of a KVP object. */
// TODO: Merge down into GfxUIC3PValue.
class GfxUIKeyValuePair : public GfxUIElement {
  public:
    GfxUIKeyValuePair(KeyValuePair* kvp, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f = 0);
    ~GfxUIKeyValuePair() {};

    inline void showTypeInfo(bool x) {    _class_set_flag(GFXUI_C3PVAL_FLAG_SHOW_TYPE_INFO, x);   };
    inline bool showTypeInfo() {          return _class_flag(GFXUI_C3PVAL_FLAG_SHOW_TYPE_INFO);   };
    inline void resizeOnRender(bool x) {  _class_set_flag(GFXUI_C3PVAL_FLAG_RESIZE_ON_RENDER, x); };
    inline bool resizeOnRender() {        return _class_flag(GFXUI_C3PVAL_FLAG_RESIZE_ON_RENDER); };

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log);

    int8_t showKVP(KeyValuePair*);

  private:
    KeyValuePair* _kvp;
    PixUInt _bounding_w;  // Area actually needed for last render. Accounts for border/margin.
    PixUInt _bounding_h;  // Area actually needed for last render. Accounts for border/margin.
    bool _kvp_loaded;

    //int8_t _calculate_bounding_box(PixUInt* w, PixUInt* h);
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
    virtual bool _notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log);

    int8_t setKVP(KeyValuePair*);
    int8_t setKVP(StringBuilder*, TCode);


  private:
    GfxUITextArea _txt_serialized;
    GfxUIKeyValuePair _kvp_view;
};

#endif  // __C3P_GFXUI_KIT_KVP_H
