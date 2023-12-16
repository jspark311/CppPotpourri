/*
File:   GfxUIKit.h
Author: J. Ian Lindsay
Date:   2023.03.11

NOTE: Do not include this file directly. Only via GfxUI.h.
TODO: This^ partly because this file is long and terribly hard to read, and I
  don't want you to make fun of me. But seriously, spread this out, tier it, or
  force it to undergo fission. Anything to prevent having to maintain _this_.


These classes are built on top of the GfxUI classes, and implement higher-level
  functional elements of a UI.
*/

#ifndef __C3P_GFXUI_KIT_H
#define __C3P_GFXUI_KIT_H

#include "../Image.h"
#include "../ImageUtils.h"
#include "../../M2MLink/M2MLink.h"
#include "../../Identity/Identity.h"
#include "../../Storage/Storage.h"
#include "../../Storage/RecordTypes/ConfRecord.h"
#include "../../Pipes/TripleAxisPipe/TripleAxisPipe.h"
#include "../../TimerTools/C3PScheduler.h"

/*******************************************************************************
* Non-interacting utility shims. These are invisibile classes used to
*   facilitate grouping, connection, and flow of GfxUIElements.
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

    inline int add_child(GfxUIElement* element) {     return _add_child(element);     };
    inline int remove_child(GfxUIElement* element) {  return _remove_child(element);  };

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
* Pure UI modules.
* These have no backing to deeper objects, and represent a simple shared state
*   between the user and the program.
*******************************************************************************/


/*******************************************************************************
* Graphical buttons
*******************************************************************************/
/* A button. */
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


/* A button with text. */
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
/* A button container with logic for acting as a single-selection tab bar. */
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


/* A tabbed content pane. */
class GfxUITabbedContentPane : public GfxUIElement {
  public:
    GfxUITabbedContentPane(const GfxUILayout lay, const GfxUIStyle sty, uint32_t f = 0);
    ~GfxUITabbedContentPane() {};

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
/* A graphical single-axis slider. */
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
* TODO: This class is unique, and should probably be in its own header, or at
*   least tiered differently within this one.
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
* A graphical text area that acts as a generic BufferPipe terminus
*******************************************************************************/
class GfxUITextArea : public GfxUIElement, public BufferAccepter {
  public:
    GfxUITextArea(const GfxUILayout lay, const GfxUIStyle sty, uint32_t f = 0) : GfxUIElement(lay, sty, f) {};
    ~GfxUITextArea() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);

    /* Implementation of BufferAccepter. */
    int8_t pushBuffer(StringBuilder*);
    int32_t bufferAvailable();

    inline void wrapLines(bool x) {  _class_set_flag(GFXUI_TXTAREA_FLAG_LINE_WRAP, x);   };
    inline bool wrapLines() {        return _class_flag(GFXUI_TXTAREA_FLAG_LINE_WRAP);   };
    inline void wrapWords(bool x) {  _class_set_flag(GFXUI_TXTAREA_FLAG_WORD_WRAP, x);   };
    inline bool wrapWords() {        return _class_flag(GFXUI_TXTAREA_FLAG_WORD_WRAP);   };
    inline void scrollable(bool x) { _class_set_flag(GFXUI_TXTAREA_FLAG_SCROLLABLE, x);  };
    inline bool scrollable() {       return _class_flag(GFXUI_TXTAREA_FLAG_SCROLLABLE);  };
    inline void scrollbackLength(uint32_t x) {  _max_scrollback_bytes = x;     };
    inline bool scrollbackLength() {            return _max_scrollback_bytes;  };
    void clear();


  private:
    uint32_t _cursor_x = 0;  // Location of the next character.
    uint32_t _cursor_y = 0;  // Location of the next character.
    uint32_t _max_scrollback_bytes = 600;   //
    uint32_t _max_cols = 0;  // Maximum number of columns that will fit in render area.
    uint16_t _max_rows = 0;  // Maximum number of lines that will fit in render area.
    uint16_t _top_line = 0;  // Which line index is at the top of the render?
    StringBuilder _scrollback;  //
};


/*******************************************************************************
* A graphical text area that is specialized for the logger.
*******************************************************************************/



/*******************************************************************************
* A graphical text area that acts as a TripleAxisPipe terminus
*******************************************************************************/
class GfxUI3AxisRender : public GfxUIElement, public TripleAxisPipe {
  public:
    GfxUI3AxisRender(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t color, uint32_t f = 0) : GfxUIElement(x, y, w, h, f) {};
    ~GfxUI3AxisRender() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);

    virtual int8_t pushVector(SpatialSense, Vector3f* data, Vector3f* error = nullptr);
    virtual void   printPipe(StringBuilder*, uint8_t stage, uint8_t verbosity);
};



/*******************************************************************************
* Graphical breakouts for basic C3P object representation.
* Although these can be used alone, they are mostly useful as compositional
*   elements of higher-order views.
*******************************************************************************/

/* A graphical representation of the StopWatch class. */
class GfxUIStopWatch : public GfxUIElement {
  public:
    GfxUIStopWatch(const char*, StopWatch*, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f = 0);
    ~GfxUIStopWatch() {};

    inline StopWatch* stopwatch() {    return _stopwatch;  };

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);

  private:
    const char* _name;
    StopWatch* _stopwatch;
};


/*******************************************************************************
* Graphical representations for identities
*******************************************************************************/

/* A single-pane summary of an Identity. */
class GfxUIIdentity : public GfxUIElement {
  public:
    GfxUIIdentity(const GfxUILayout lay, const GfxUIStyle sty, Identity* id, uint32_t f = 0);
    ~GfxUIIdentity() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);


  private:
    Identity* _ident;
    GfxUITextArea _txt_handle;
    GfxUITextArea _txt_format;
    GfxUITextArea _txt_meta;
    GfxUIGroup    _flag_render;
};



/*******************************************************************************
* Graphical tools for using MLinks.
*******************************************************************************/
/*
* An MLink top-level representation.
*/
class GfxUIMLink : public GfxUITabbedContentPane {
  public:
    GfxUIMLink(const GfxUILayout lay, const GfxUIStyle sty, M2MLink* l, uint32_t f = 0);
    ~GfxUIMLink() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);


  private:
    M2MLink* _link;
    GfxUIGroup    _content_info;
    GfxUIGroup    _content_conf;
    GfxUIGroup    _content_msg;
    GfxUIGroup    _content_ses;
    GfxUITextButton   _btn_conf_syncast;
    GfxUITextButton   _btn_msg_send_sync;
    GfxUITextButton   _btn_ses_hangup;
    GfxUITextArea     _txt;
    //GfxUIIdentity     _iden_self;
};


/*******************************************************************************
* Graphical tools for C3PScheduler.
* This is mostly used to demo the Scheduler and verify features.
*******************************************************************************/
/* A discrete Schedule. */
class GfxUIC3PSchedule : public GfxUIElement {
  public:
    GfxUIC3PSchedule(C3PSchedule*, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f = 0);
    ~GfxUIC3PSchedule() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);

    inline C3PSchedule* getSchedule() {    return _sched;  };


  private:
    C3PSchedule*    _sched;
    GfxUIStopWatch  _gfx_profiler;
};


/* The Scheduler itself. */
class GfxUIC3PScheduler : public GfxUITabbedContentPane {
  public:
    GfxUIC3PScheduler(const GfxUILayout lay, const GfxUIStyle sty, uint32_t f = 0);
    ~GfxUIC3PScheduler() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);


  private:
    PriorityQueue<GfxUIC3PSchedule*> _dyn_elements;
    GfxUIGroup    _pane_info;
    GfxUIGroup    _pane_schedules;
    GfxUITextArea _txt;
    GfxUIStopWatch  _sw_svc_loop;
    GfxUIStopWatch  _sw_deadband;
};


/*******************************************************************************
* Storage objects.
* These act as state and control breakouts for classes in the Storage apparatus.
*******************************************************************************/

/* Storage */
//class GfxUIStorage : public GfxUIElement {
//  public:
//    GfxUIStorage(Storage* storage, uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t f = 0);
//    ~GfxUIStorage() {};
//
//    /* Implementation of GfxUIElement. */
//    virtual int  _render(UIGfxWrapper* ui_gfx);
//    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);
//
//
//  private:
//    Storage* _storage;
//};


/* DataRecord */
class GfxUIDataRecord : public GfxUITabbedContentPane {
  public:
    GfxUIDataRecord(SimpleDataRecord*, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f = 0);
    ~GfxUIDataRecord() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);


  private:
    SimpleDataRecord* _record;
};



/*******************************************************************************
* A graphical breakdown of a top-level configuration object.
*******************************************************************************/

class GfxUIOptionsView : public GfxUITabbedContentPane {
  public:
    GfxUIOptionsView(ConfRecord*, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f = 0);
    ~GfxUIOptionsView() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);


  protected:
    ConfRecord* _conf;
};



/*******************************************************************************
* SIUnit chooser
*******************************************************************************/
class GfxUISIUnitChooser : public GfxUIElement {
  public:
    GfxUISIUnitChooser(const GfxUILayout lay, const GfxUIStyle sty, uint32_t f = 0) :
      GfxUIElement(lay, sty, f),
      selection(SIUnit::UNITLESS), siu_string(nullptr) {};

    GfxUISIUnitChooser(const SIUnit UNIT, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f = 0) :
      GfxUIElement(lay, sty, f),
      selection(UNIT), siu_string(nullptr) {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);


  protected:
    SIUnit selection;
    SIUnit* siu_string;
};


/*******************************************************************************
* Tool for viewing profiling data.
*******************************************************************************/

class GfxUICPUProfiler : public GfxUIElement {
  public:
    GfxUICPUProfiler(const GfxUILayout lay, const GfxUIStyle sty, uint32_t f = 0);
    ~GfxUICPUProfiler() {};

    int addTimerData(const char* name, StopWatch*);   // Add a source of profiling data.
    int removeTimerData(const char*);   // Removal by name.
    int removeTimerData(StopWatch*);    // Removal by reference.

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log);


  private:
    PriorityQueue<StopWatch*> _stopwatches;
};


#endif  // __C3P_GFXUI_KIT_H
