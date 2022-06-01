/*
File:   GfxUI.h
Author: J. Ian Lindsay
Date:   2022.05.28

These classes are built on top of the Image and graphics classes, and implement
  bi-directional button flows between the user and firmware. These classes
  only make sense if the mode of user input is a 2-axis surface, such as a
  mouse or touchscreen).
Touch and render coordinates are assumed to be isometric.
*/

#include "../Image/Image.h"
#include "../Image/ImageUtils.h"
#include "../ManuvrLink/ManuvrLink.h"
#include "../SensorFilter.h"

#ifndef __MANUVR_GFXUI_H
#define __MANUVR_GFXUI_H

/*******************************************************************************
* UIGfxWrapper flags
* Each object has 32-bits of flag space. The low 16-bits are reserved for the
*   base class.
*******************************************************************************/
#define GFXUI_FLAG_NEED_RERENDER              0x00000001   // Child classes mark this bit to demand a redraw.
#define GFXUI_FLAG_ALWAYS_REDRAW              0x00000002   // Child classes mark this bit to demand a redraw.
#define GFXUI_FLAG_DRAW_FRAME_U               0x00000004   // Easy way for the application to select framing.
#define GFXUI_FLAG_DRAW_FRAME_D               0x00000008   // Easy way for the application to select framing.
#define GFXUI_FLAG_DRAW_FRAME_L               0x00000010   // Easy way for the application to select framing.
#define GFXUI_FLAG_DRAW_FRAME_R               0x00000020   // Easy way for the application to select framing.
#define GFXUI_FLAG_FREE_THIS_ELEMENT          0x00000040   // This object ought to be freed when no longer needed.

#define GFXUI_BUTTON_FLAG_STATE               0x01000000   // Button state
#define GFXUI_BUTTON_FLAG_MOMENTARY           0x02000000   // Button reverts to off when released.

#define GFXUI_SENFILT_FLAG_SHOW_VALUE         0x01000000   //
#define GFXUI_SENFILT_FLAG_SHOW_RANGE         0x02000000   //

#define GFXUI_TXTAREA_FLAG_LINE_WRAP          0x01000000   //
#define GFXUI_TXTAREA_FLAG_WORD_WRAP          0x02000000   //
#define GFXUI_TXTAREA_FLAG_SCROLLABLE         0x04000000   //


#define GFXUI_SLIDER_FLAG_VERTICAL            0x01000000   // This slider is vertical.
#define GFXUI_SLIDER_FLAG_RENDER_VALUE        0x02000000   // Overlay the value on the slider bar.
#define GFXUI_SLIDER_FLAG_MARK_ONLY           0x04000000   // Do not fill the space under the mark.


#define GFXUI_FLAG_DRAW_FRAME_MASK  (GFXUI_FLAG_DRAW_FRAME_U | GFXUI_FLAG_DRAW_FRAME_D | GFXUI_FLAG_DRAW_FRAME_L | GFXUI_FLAG_DRAW_FRAME_R)


/*******************************************************************************
* UIGfxWrapper flags
*******************************************************************************/
/*
* These are the possible meanings of signals that might come in
*   from the user's plane.
*/
enum class GfxUIEvent : uint8_t {
  NONE        = 0x00,  //
  TOUCH       = 0x01,  // Usually a left-click initiate on a PC.
  RELEASE     = 0x02,  // Usually a left-click release on a PC.
  PRESSURE    = 0x03,  //
  DRAG        = 0x04,  // Usually a middle-click on a PC.
  HOVER       = 0x05,  // "Mouseover"
  SELECT      = 0x06,  // Usually a right-click on a PC.
  MOVE_UP     = 0x07,  // Usually a scrollwheel on a PC.
  MOVE_DOWN   = 0x08,  // Usually a scrollwheel on a PC.
  MOVE_LEFT   = 0x09,  // Usually a scrollwheel on a PC.
  MOVE_RIGHT  = 0x0A,  // Usually a scrollwheel on a PC.
  MOVE_IN     = 0x0B,  //
  MOVE_OUT    = 0x0C   //
};



/*******************************************************************************
* Base class that handles all touchable images.
*******************************************************************************/
class GfxUIElement {
  public:
    bool includesPoint(const uint32_t x, const uint32_t y) {
      return ((x >= _x) && (x < (_x + _w)) && (y >= _y) && (y < (_y + _h)));
    };

    void enableFrames(uint32_t frame_flags = GFXUI_FLAG_DRAW_FRAME_MASK) {
      _class_clear_flag(GFXUI_FLAG_DRAW_FRAME_MASK);
      _class_set_flag(frame_flags & GFXUI_FLAG_DRAW_FRAME_MASK);
      _need_redraw(true);
    };

    inline void shouldReap(bool x) {  _class_set_flag(GFXUI_FLAG_FREE_THIS_ELEMENT, x);   };
    inline bool shouldReap() {        return _class_flag(GFXUI_FLAG_FREE_THIS_ELEMENT);   };

    void reposition(uint32_t x, uint32_t y) {
      _x = x;
      _y = y;
      _need_redraw(true);
    };

    void resize(uint32_t w, uint32_t h) {
      _w = w;
      _h = h;
      _need_redraw(true);
    };

    /*
    * Top-level objects are the first to handle notify.
    * Iteration and recursion both stop on the first positive return value.
    */
    bool notify(const GfxUIEvent GFX_EVNT, const uint32_t x, const uint32_t y);

    /*
    * Top-level objects are the last to render.
    * Iteration and recursion both touch the entire tree.
    */
    int render(UIGfxWrapper* ui_gfx, bool force = false);


  protected:
    uint32_t _x;     // Location of the upper-left corner.
    uint32_t _y;     // Location of the upper-left corner.
    uint16_t _w;     // Size of the element.
    uint16_t _h;     // Size of the element.

    GfxUIElement(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t f) : _x(x), _y(y), _w(w), _h(h), _flags(f | GFXUI_FLAG_NEED_RERENDER) {};
    virtual ~GfxUIElement() {};

    /* These are the obligate overrides. */
    virtual bool _notify(const GfxUIEvent, const uint32_t x, const uint32_t y) =0;
    virtual int  _render(UIGfxWrapper*) =0;

    inline void _need_redraw(bool x) {  _class_set_flag(GFXUI_FLAG_NEED_RERENDER, x);   };
    inline bool _need_redraw() {        return _class_flag(GFXUI_FLAG_NEED_RERENDER | GFXUI_FLAG_ALWAYS_REDRAW);   };


    inline uint32_t _class_flags() {                return _flags;            };
    inline bool _class_flag(uint32_t _flag) {       return (_flags & _flag);  };
    inline void _class_flip_flag(uint32_t _flag) {  _flags ^= _flag;          };
    inline void _class_clear_flag(uint32_t _flag) { _flags &= ~_flag;         };
    inline void _class_set_flag(uint32_t _flag) {   _flags |= _flag;          };
    inline void _class_set_flag(uint32_t _flag, bool nu) {
      if (nu) _flags |= _flag;
      else    _flags &= ~_flag;
    };

  private:
    uint32_t _flags; // FlagContainer32 _flags;
    PriorityQueue<GfxUIElement*> _children;

    bool   _notify_children(const GfxUIEvent GFX_EVNT, const uint32_t x, const uint32_t y);
    int    _render_children(UIGfxWrapper*, bool force);
};



/*******************************************************************************
* Pure UI modules.
* These have no backing to deeper objects, and represent a simple shared state
*   between the user and the program.
*******************************************************************************/

/*******************************************************************************
* A graphical button
*******************************************************************************/
class GfxUIButton : public GfxUIElement {
  public:
    GfxUIButton(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t color, uint32_t f = 0) : GfxUIElement(x, y, w, h, f), _color_active_on(color) {};
    ~GfxUIButton() {};

    inline void pressed(bool x) {
      if (momentary()) {  _class_set_flag(GFXUI_BUTTON_FLAG_STATE, x);  }
      else if (x) {       _class_flip_flag(GFXUI_BUTTON_FLAG_STATE);    }
    };
    inline bool pressed() {          return _class_flag(GFXUI_BUTTON_FLAG_STATE);       };
    inline void momentary(bool x) {  _class_set_flag(GFXUI_BUTTON_FLAG_MOMENTARY, x);   };
    inline bool momentary() {        return _class_flag(GFXUI_BUTTON_FLAG_MOMENTARY);   };

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y);


  protected:
    uint32_t _color_active_on;   // The accent color of the element when active.
    uint32_t _color_active_off;  // The accent color of the element when active.
    uint32_t _color_inactive;    // The accent color of the element when inactive.
};


/*******************************************************************************
* A graphical tab bar
*******************************************************************************/
class GfxUITabBar : public GfxUIElement {
  public:
    GfxUITabBar(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t color, uint32_t f = 0);
    ~GfxUITabBar() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y);


  protected:
    float    _percentage;        // The current position of the mark, as a fraction.
    uint32_t _color_marker;      // The accent color of the position mark.
    GfxUIElement* _buttons;      // A collection of buttons contained by this object.
};



/*******************************************************************************
* A graphical slider
*******************************************************************************/
class GfxUISlider : public GfxUIElement {
  public:
    GfxUISlider(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t color, uint32_t f = 0);
    ~GfxUISlider() {};

    inline float value() {         return _percentage;    };
    inline void value(float x) {   _percentage = x;       };

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y);


  protected:
    float    _percentage;        // The current position of the mark, as a fraction.
    uint32_t _color_marker;      // The accent color of the position mark.
};



/*******************************************************************************
* A graphical text area that acts as a BufferPipe terminus
*******************************************************************************/
class GfxUITextArea : public GfxUIElement, BufferAccepter {
  public:
    GfxUITextArea(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t color, uint32_t f = 0) : GfxUIElement(x, y, w, h, f), _color_text(color) {};
    ~GfxUITextArea() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y);

    /* Implementation of BufferAccepter. */
    int8_t provideBuffer(StringBuilder*);

    inline void wrapLines(bool x) {  _class_set_flag(GFXUI_TXTAREA_FLAG_LINE_WRAP, x);   };
    inline bool wrapLines() {        return _class_flag(GFXUI_TXTAREA_FLAG_LINE_WRAP);   };
    inline void wrapWords(bool x) {  _class_set_flag(GFXUI_TXTAREA_FLAG_WORD_WRAP, x);   };
    inline bool wrapWords() {        return _class_flag(GFXUI_TXTAREA_FLAG_WORD_WRAP);   };
    inline void scrollable(bool x) { _class_set_flag(GFXUI_TXTAREA_FLAG_SCROLLABLE, x);  };
    inline bool scrollable() {       return _class_flag(GFXUI_TXTAREA_FLAG_SCROLLABLE);  };
    inline void clear() {    _scrollback.clear();   };


  private:
    uint32_t _color_text;        // The accent color of the element when active.
    uint32_t _cursor_x = 0;
    uint32_t _cursor_y = 0;
    uint32_t _max_scrollback_bytes = 600;   // Tokenized strings
    uint32_t _max_cols = 0;
    uint32_t _max_rows = 0;
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
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y);


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
template <typename T> class GfxUISensorFilter : public GfxUIElement {
  public:
    GfxUISensorFilter(SensorFilter<T>* sf, uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t color, uint32_t f = 0) : GfxUIElement(x, y, w, h, f | GFXUI_FLAG_ALWAYS_REDRAW), _color(color), _filter(sf) {};
    ~GfxUISensorFilter() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y);

    inline void showValue(bool x) {  _class_set_flag(GFXUI_SENFILT_FLAG_SHOW_VALUE, x); };
    inline bool showValue() {        return _class_flag(GFXUI_SENFILT_FLAG_SHOW_VALUE); };

    inline void showRange(bool x) {  _class_set_flag(GFXUI_SENFILT_FLAG_SHOW_RANGE, x); };
    inline bool showRange() {        return _class_flag(GFXUI_SENFILT_FLAG_SHOW_RANGE); };


  private:
    uint32_t _color;        // The accent color of the element when active.
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
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y);


  private:
    uint32_t _color;        // The accent color of the element when active.
    KeyValuePair* _kvp;
};



/*******************************************************************************
* Graphical tools for using MLinks.
*******************************************************************************/
class GfxUIMLink : public GfxUIElement {
  public:
    GfxUIMLink(ManuvrLink* l, uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t f = 0) : GfxUIElement(x, y, w, h, f), _link(l) {};
    ~GfxUIMLink() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y);


  private:
    ManuvrLink* _link;
};


#endif  // __MANUVR_GFXUI_H
