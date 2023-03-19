/*
File:   GfxUIKit.h
Author: J. Ian Lindsay
Date:   2023.03.11

These classes are built on top of the GfxUI classes, and implement higher-level
  functional elements of a UI.
*/

#ifndef __MANUVR_GFXUI_KIT_H
#define __MANUVR_GFXUI_KIT_H

#include "../Image.h"
#include "../ImageUtils.h"
#include "../GfxUI.h"
#include "../../M2MLink/M2MLink.h"
#include "../../Identity/Identity.h"
#include "../../SensorFilter.h"
#include "../../Storage.h"


/*******************************************************************************
* Pure UI modules.
* These have no backing to deeper objects, and represent a simple shared state
*   between the user and the program.
*******************************************************************************/

/*******************************************************************************
* Non-interacting utility shims
*******************************************************************************/
/**
* A class to group UI elements into a single GfxUIElement. Only useful for
*   building complex views.
*/
class GfxUIGroup : public GfxUIElement {
  public:
    GfxUIGroup(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t f = 0) : GfxUIElement(x, y, w, h, f) {};
    GfxUIGroup(GfxUILayout layout, uint32_t f = 0) : GfxUIElement(&layout, f) {};
    GfxUIGroup(GfxUILayout* layout, uint32_t f = 0) : GfxUIElement(layout, f) {};
    ~GfxUIGroup() {};

    inline int add_child(GfxUIElement* element) {    return _add_child(element);  };

    /* Implementation of GfxUIElement. */
    // This class has no rendering tasks, and doesn't respond to user input.
    int  _render(UIGfxWrapper* ui_gfx) {    return 0;   };
    bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {  return false;   };
};


/**
* A special case of GfxUIGroup that functions as an optional root container.
* Applications that have a top-level tabbed interface probably won't want this.
*   But it helps in situations where you have dozens of elements in the view,
*   and you don't want to have to manage their rendering explicitly.
*/
class GfxUIRoot : public GfxUIGroup {
  public:
    GfxUIRoot(UIGfxWrapper* ui_gfx);
    ~GfxUIRoot() {};

    int render();  // Top-level call to use the built-in UIGfxWrapper.

  private:
    UIGfxWrapper* _ui_gfx;
};



/*******************************************************************************
* Graphical buttons
*******************************************************************************/
class GfxUIButton : public GfxUIElement {
  public:
    GfxUIButton(const GfxUILayout lay, const GfxUIStyle sty, uint32_t f = 0) : GfxUIElement(lay, sty, f) {};
    ~GfxUIButton() {};

    inline void buttonState(bool x) {  _class_set_flag(GFXUI_BUTTON_FLAG_STATE, x);  };
    inline void pressed(bool x) {
      if (momentary()) {  _class_set_flag(GFXUI_BUTTON_FLAG_STATE, x);  }
      else if (x) {       _class_flip_flag(GFXUI_BUTTON_FLAG_STATE);    }
    };
    inline bool pressed() {          return _class_flag(GFXUI_BUTTON_FLAG_STATE);       };
    inline void momentary(bool x) {  _class_set_flag(GFXUI_BUTTON_FLAG_MOMENTARY, x);   };
    inline bool momentary() {        return _class_flag(GFXUI_BUTTON_FLAG_MOMENTARY);   };

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);


  protected:
};


class GfxUITextButton : public GfxUIButton {
  public:
    GfxUITextButton(const GfxUILayout lay, const GfxUIStyle sty, const char* T, uint32_t f = 0) : GfxUIButton(lay, sty, f), _TXT(T) {};
    ~GfxUITextButton() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);

  protected:
    const char* _TXT;
};


/*******************************************************************************
* A graphical tab bar
*******************************************************************************/
class GfxUITabBar : public GfxUIElement {
  public:
    GfxUITabBar(const GfxUILayout lay, const GfxUIStyle sty, uint32_t f = 0) : GfxUIElement(lay, sty, f), _active_tab(0) {};
    ~GfxUITabBar() {};

    inline uint8_t activeTab() {     return _active_tab;    };
    inline uint8_t tabCount() {      return _children.size();    };
    int8_t addTab(const char* txt, bool selected = false);

    inline void scrollCycle(bool x) {  _class_set_flag(GFXUI_TABBAR_FLAG_SCROLL_CYCLES_TABS, x);   };
    inline bool scrollCycle() {        return _class_flag(GFXUI_TABBAR_FLAG_SCROLL_CYCLES_TABS);   };

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);


  protected:
    uint8_t  _active_tab;

    int8_t _set_active_tab(uint8_t tab_idx);
};



class GfxUITabBarWithContent : public GfxUIElement {
  public:
    GfxUITabBarWithContent(const GfxUILayout lay, const GfxUIStyle sty, uint32_t f = 0);
    ~GfxUITabBarWithContent() {};

    //inline uint8_t activeTab() {     return _tab_bar.activeTab();    };
    inline uint8_t activeTab() {     return _active_tab;    };
    int8_t addTab(const char* txt, GfxUIElement* content, bool selected = false);

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);


  protected:
    GfxUITabBar _tab_bar;
    uint8_t  _active_tab;
};



/*******************************************************************************
* A graphical slider
*******************************************************************************/
class GfxUISlider : public GfxUIElement {
  public:
    GfxUISlider(const GfxUILayout lay, const GfxUIStyle sty, uint32_t f = 0) : GfxUIElement(lay, sty, f), _percentage(0.0f) {};
    ~GfxUISlider() {};

    inline float value() {         return _percentage;    };
    inline void value(float x) {   _percentage = x;       };

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);


  protected:
    float    _percentage;        // The current position of the mark, as a fraction.
};



/*******************************************************************************
* A magnifier that tracks the pointer while it is on-screen.
* NOTE: If this class is configured to draw pixels outside of its own bounds, it
*   is best used with an overlay image to avoid ghosting. This element should
*   render to the overlay, and take the source image as a constructor parameter.
*******************************************************************************/
class GfxUIMagnifier : public GfxUIElement {
  public:
    GfxUIMagnifier(Image* src_img, uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t color, uint32_t f = 0);
    ~GfxUIMagnifier() {};

    inline float scale() {         return _scale;    };
    inline void scale(float x) {   _scale = x;       };
    int setBoands(float min_mag, float max_mag);

    void pointerLocation(float x, float y) {
      _pointer_x = x;
      _pointer_y = y;
    };

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);


  protected:
    uint32_t _color;      // The accent color of the position mark.
    Image*   _src;        // The source image to magnify.
    uint32_t _pointer_x;  // The center of the feed.
    uint32_t _pointer_y;  // The center of the feed.
    float    _scale;      // The current scale factor to apply to the source.
    float    _min_mag;    // The current scale factor to apply to the source.
    float    _max_mag;    // The current scale factor to apply to the source.
};



/*******************************************************************************
* A graphical text area that acts as a BufferPipe terminus
*******************************************************************************/
class GfxUITextArea : public GfxUIElement, public BufferAccepter {
  public:
    GfxUITextArea(const GfxUILayout lay, const GfxUIStyle sty, uint32_t f = 0) : GfxUIElement(lay, sty, f) {};
    ~GfxUITextArea() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);

    /* Implementation of BufferAccepter. */
    int8_t provideBuffer(StringBuilder*);

    inline void wrapLines(bool x) {  _class_set_flag(GFXUI_TXTAREA_FLAG_LINE_WRAP, x);   };
    inline bool wrapLines() {        return _class_flag(GFXUI_TXTAREA_FLAG_LINE_WRAP);   };
    inline void wrapWords(bool x) {  _class_set_flag(GFXUI_TXTAREA_FLAG_WORD_WRAP, x);   };
    inline bool wrapWords() {        return _class_flag(GFXUI_TXTAREA_FLAG_WORD_WRAP);   };
    inline void scrollable(bool x) { _class_set_flag(GFXUI_TXTAREA_FLAG_SCROLLABLE, x);  };
    inline bool scrollable() {       return _class_flag(GFXUI_TXTAREA_FLAG_SCROLLABLE);  };
    void clear();


  private:
    uint32_t _cursor_x = 0;  // Location of the next character.
    uint32_t _cursor_y = 0;  // Location of the next character.
    uint32_t _max_scrollback_bytes = 600;   // Tokenized strings
    uint32_t _max_cols = 0;  // Maximum number of columns that will fit in render area.
    uint16_t _max_rows = 0;  // Maximum number of lines that will fit in render area.
    uint16_t _top_line = 0;  // Which line index is at the top of the render?
    StringBuilder _scrollback;  //
};


/*******************************************************************************
* A graphical text area that acts as a TripleAxisPipe terminus
*******************************************************************************/
class GfxUI3AxisRender : public GfxUIElement {
  public:
    GfxUI3AxisRender(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t color, uint32_t f = 0) : GfxUIElement(x, y, w, h, f), _color_accent(color) {};
    ~GfxUI3AxisRender() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);


  private:
    uint32_t _color_accent;        // The accent color of the element when active.
};



/*******************************************************************************
* Data and module control.
* These act as state and control breakouts for specific kinds of objects in C3P.
*******************************************************************************/

/*******************************************************************************
* Graphical tools for manipulating filters.
*******************************************************************************/
template <class T> class GfxUISensorFilter : public GfxUIElement {
  public:
    GfxUISensorFilter(const GfxUILayout lay, const GfxUIStyle sty, SensorFilter<T>* sf, uint32_t f = 0) : GfxUIElement(lay, sty, f | GFXUI_FLAG_ALWAYS_REDRAW), _filter(sf) {};
    ~GfxUISensorFilter() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);

    inline void showValue(bool x) {  _class_set_flag(GFXUI_SENFILT_FLAG_SHOW_VALUE, x); };
    inline bool showValue() {        return _class_flag(GFXUI_SENFILT_FLAG_SHOW_VALUE); };

    inline void showRange(bool x) {  _class_set_flag(GFXUI_SENFILT_FLAG_SHOW_RANGE, x); };
    inline bool showRange() {        return _class_flag(GFXUI_SENFILT_FLAG_SHOW_RANGE); };


  private:
    SensorFilter<T>* _filter;
};


/*******************************************************************************
* Graphical tool for looking at KVP data.
*******************************************************************************/
class GfxUIKeyValuePair : public GfxUIElement {
  public:
    GfxUIKeyValuePair(KeyValuePair* kvp, uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t color, uint32_t f = 0) : GfxUIElement(x, y, w, h, f | GFXUI_FLAG_ALWAYS_REDRAW), _color(color), _kvp(kvp) {};
    ~GfxUIKeyValuePair() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);


  private:
    uint32_t _color;        // The accent color of the element when active.
    KeyValuePair* _kvp;
};


/*******************************************************************************
* Graphical representations for identities
*******************************************************************************/
class GfxUIIdentity : public GfxUIElement {
  public:
    GfxUIIdentity(Identity* id, uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t color, uint32_t f = 0);
    ~GfxUIIdentity() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);


  private:
    uint32_t _color;        // The accent color of the element when active.
    Identity* _ident;
    GfxUITabBarWithContent _tab_bar;
    GfxUIGroup    _content_0;
    GfxUIGroup    _content_1;
    GfxUIGroup    _content_2;
    GfxUITextArea _txt0;
    GfxUITextArea _txt1;
    GfxUITextArea _txt2;
};



/*******************************************************************************
* Graphical tools for using MLinks.
*******************************************************************************/

class GfxUIMLink : public GfxUIElement {
  public:
    GfxUIMLink(M2MLink* l, uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t f = 0);
    ~GfxUIMLink() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);


  private:
    M2MLink* _link;
    GfxUITabBarWithContent _tab_bar;
    GfxUIGroup    _content_info;
    GfxUIGroup    _content_conf;
    GfxUIGroup    _content_msg;
    GfxUIGroup    _content_ses;
    GfxUITextButton   _btn_conf_syncast;
    GfxUITextButton   _btn_msg_send_sync;
    GfxUITextButton   _btn_ses_hangup;
    GfxUITextArea     _txt;
};


/*******************************************************************************
* Storage objects.
* These act as state and control breakouts for classes in the Storage apparatus.
*******************************************************************************/

/*******************************************************************************
* Graphical representation of a DataRecord.
*******************************************************************************/

/* Storage */
class GfxUIStorage : public GfxUIElement {
  public:
    GfxUIStorage(Storage* storage, uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t f = 0);
    ~GfxUIStorage() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);


  private:
    Storage* _storage;
};

/* DataRecord */
class GfxUIDataRecord : public GfxUIElement {
  public:
    GfxUIDataRecord(DataRecord* record, uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t f = 0);
    ~GfxUIDataRecord() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);


  private:
    DataRecord* _record;
    GfxUITabBar _tab_bar;
    GfxUITextArea _txt;
};



#endif  // __MANUVR_GFXUI_KIT_H
