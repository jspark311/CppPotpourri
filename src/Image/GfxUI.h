/*
File:   GfxUI.h
Author: J. Ian Lindsay
Date:   2022.05.28

These classes are built on top of the Image and graphics classes, and implement
  bi-directional UI flows between the user and firmware. These classes
  only make sense if the mode of user input is a 2-axis surface, such as a
  mouse or touchscreen).

See the README.md fil the GfxUI directory for details.

This source file was never part of Adafruit's library. They are small graphics
  utilities that help implement simple UIs.
*/

#ifndef __C3P_GFXUI_H
#define __C3P_GFXUI_H

#include "EnumWrapper.h"
#include "FlagContainer.h"
#include "Image.h"
#include "ImageUtils.h"

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
#define GFXUI_FLAG_INACTIVE                   0x00000040   // Used to prevent suprious input at a class's discretion.
#define GFXUI_FLAG_MUTE_RENDER                0x00000080   // Used to suspend rendering that would otherwise happen. Makes element insensitive to notify().
#define GFXUI_FLAG_FREE_THIS_ELEMENT          0x00000100   // This object ought to be freed when no longer needed.
#define GFXUI_FLAG_CURRENT_FOCUS              0x00000200   // This is the element that is the focus of the pointer.
#define GFXUI_FLAG_CURRENT_SELECTION          0x00000400   // This is the element that was last selected (possibly not focused).
#define GFXUI_FLAG_UNDER_POINTER              0x00000800   // This element is underneath the pointer.
#define GFXUI_FLAG_DRAGGABLE                  0x00001000   // This element is draggable.
#define GFXUI_FLAG_DRAGGING                   0x00002000   // This element is undergoing a drag.
#define GFXUI_FLAG_TRACK_POINTER              0x00004000   //

// NOTE: This is thinking out-loud about "what" flow needs to accomplish.
//   Don't write against it.
// HTML was very well thought-out, and many people understand it. I should imagine
//   element flows and positioning/scaling rules should endeavor to follow that
//   pattern, without explicitly spending too many resources to do so. Mint tin.
#define GFXUI_FLAG_FLOW_POS_LOCK_U            0x00008000   // If set, auto-flow is inhibited in this direction.
#define GFXUI_FLAG_FLOW_POS_LOCK_D            0x00010000   // If set, auto-flow is inhibited in this direction.
#define GFXUI_FLAG_FLOW_POS_LOCK_L            0x00020000   // If set, auto-flow is inhibited in this direction.
#define GFXUI_FLAG_FLOW_POS_LOCK_R            0x00040000   // If set, auto-flow is inhibited in this direction.
#define GFXUI_FLAG_FLOW_POS_LOCK_PROPAGATE    0x00080000   // Propagate the locks?
#define GFXUI_FLAG_FLOW_SCALE_LOCK_X          0x00100000   // If set, auto-flow is inhibited on this axis.
#define GFXUI_FLAG_FLOW_SCALE_LOCK_Y          0x00200000   // If set, auto-flow is inhibited on this axis.
#define GFXUI_FLAG_FLOW_SCALE_LOCK_PROPAGATE  0x00400000   // Propagate the locks?

//////////////////////////////////
// TODO: Stark Fist of Removal
#define GFXUI_BUTTON_FLAG_STATE               0x01000000   // Button state
#define GFXUI_BUTTON_FLAG_MOMENTARY           0x02000000   // Button reverts to off when released.

#define GFXUI_MAGNIFIER_FLAG_SHOW_TRACERS     0x01000000   //
#define GFXUI_MAGNIFIER_FLAG_SHOW_FEED_FRAME  0x02000000   //

#define GFXUI_SENFILT_FLAG_SHOW_VALUE         0x01000000   //
#define GFXUI_SENFILT_FLAG_SHOW_RANGE         0x02000000   //

#define GFXUI_TXTAREA_FLAG_LINE_WRAP          0x01000000   //
#define GFXUI_TXTAREA_FLAG_WORD_WRAP          0x02000000   //
#define GFXUI_TXTAREA_FLAG_SCROLLABLE         0x04000000   //

#define GFXUI_SLIDER_FLAG_VERTICAL            0x01000000   // This slider is vertical.
#define GFXUI_SLIDER_FLAG_RENDER_VALUE        0x02000000   // Overlay the value on the slider bar.
#define GFXUI_SLIDER_FLAG_MARK_ONLY           0x04000000   // Do not fill the space under the mark.

#define GFXUI_TABBAR_FLAG_VERTICAL_TABS       0x01000000   // The tabs are stacked veritcally.
#define GFXUI_TABBAR_FLAG_SCROLL_CYCLES_TABS  0x02000000   // Scroll wheel cycles tabs.
// TODO: /Stark Fist
//////////////////////////////////


#define GFXUI_FLAG_DRAW_FRAME_MASK  (GFXUI_FLAG_DRAW_FRAME_U | GFXUI_FLAG_DRAW_FRAME_D | GFXUI_FLAG_DRAW_FRAME_L | GFXUI_FLAG_DRAW_FRAME_R)


/*******************************************************************************
* UIGfxWrapper types
*******************************************************************************/
class GfxUIElement;

/*
* These are the possible meanings of signals that might come in
*   from the user's plane.
*/
enum class GfxUIEvent : uint8_t {
  NONE         = 0x00,  //
  /* Direct results of user input */
  TOUCH        = 0x01,  // Usually a left-click initiate on a PC.
  RELEASE      = 0x02,  // Usually a left-click release on a PC.
  PRESSURE     = 0x03,  //
  DRAG         = 0x04,  // Usually a middle-click on a PC.
  HOVER_IN     = 0x05,  // "Mouseover" enters the element boundaries.
  HOVER_OUT    = 0x06,  // "Mouseover" exits the element boundaries.
  SELECT       = 0x07,  // Usually a right-click on a PC.
  UNSELECT     = 0x08,  // May be emulated or not.
  MOVE_UP      = 0x09,  // Usually a scrollwheel on a PC.
  MOVE_DOWN    = 0x0A,  // Usually a scrollwheel on a PC.
  MOVE_LEFT    = 0x0B,  // Usually a scrollwheel on a PC.
  MOVE_RIGHT   = 0x0C,  // Usually a scrollwheel on a PC.
  MOVE_IN      = 0x0D,  //
  MOVE_OUT     = 0x0E,  //
  KEY_PRESS    = 0x0F,  // Usually from a keyboard.

  /* Return path for notify() */
  IDENTIFY     = 0x1C,  // Uses the notify() API to get the inner-most element at given location.
  DRAG_START   = 0x1D,  // Subscribes an object to drag notification.
  DRAG_STOP    = 0x1E,  // Subscribes an object from drag notification.
  VALUE_CHANGE = 0x1F,  // A value-bearing UI element was updated by user action.
  INVALID      = 0x20   // We are bound by architecture to a maximum of 32 types of events.
};


/*
* This object functions as the styling and color pallate for an element.
*   Not all elements will use all of it,
* Absent initializing values, the color pallate will be monochrome
*   white-on-black, with a minimum text size.
*/
class GfxUIStyle {
  public:
    uint32_t color_bg;          // The background color.
    uint32_t color_border;      // Color of borders, if enabled.
    uint32_t color_header;      // Color of any leading elements.
    uint32_t color_active;      // Accent color for active elements.
    uint32_t color_inactive;    // Wash-out color for inactive elements.
    uint32_t color_selected;    // A blend to accentuate selected elements.
    uint32_t color_unselected;  // A blend to subdue unselected elements.
    uint8_t  text_size;         // How big should baseline text be?

    /* Default constructor. */
    GfxUIStyle() : color_bg(0),   color_border(0xFFFFFF),   color_header(0xFFFFFF),
      color_active(0xFFFFFF),     color_inactive(0xFFFFFF), color_selected(0xFFFFFF),
      color_unselected(0xFFFFFF), text_size(1) {};

    /* Fully-specified constructor. */
    GfxUIStyle(
      uint32_t bg,
      uint32_t border,
      uint32_t header,
      uint32_t active,
      uint32_t inactive,
      uint32_t selected,
      uint32_t unselected,
      uint8_t t_size = 1
    ) :
      color_bg(bg),
      color_border(border),
      color_header(header),
      color_active(active),
      color_inactive(inactive),
      color_selected(selected),
      color_unselected(unselected),
      text_size(t_size) {};

    /* Copy constructor. */
    GfxUIStyle(const GfxUIStyle &obj) :
      color_bg(obj.color_bg),
      color_border(obj.color_border),
      color_header(obj.color_header),
      color_active(obj.color_active),
      color_inactive(obj.color_inactive),
      color_selected(obj.color_selected),
      color_unselected(obj.color_unselected),
      text_size(obj.text_size) {};

    /* Copy constructor. */
    GfxUIStyle(GfxUIStyle* obj) :
      color_bg(obj->color_bg),
      color_border(obj->color_border),
      color_header(obj->color_header),
      color_active(obj->color_active),
      color_inactive(obj->color_inactive),
      color_selected(obj->color_selected),
      color_unselected(obj->color_unselected),
      text_size(obj->text_size) {};
};


/*
* This class is intended to manage the parameters common to all elements. The
*   intent is to separate the concerns of (size, shape, and style) from those
*   of (behavior and specific content). Hierarchy might be greased with support
*   functions in this class for features like view flows.
* Creating one of these objects is sufficient to place and size any on-screen
*   element.
*/
class GfxUILayout {
  public:
    /* Constructor with optional global margins and borders. */
    GfxUILayout(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint8_t margin = 0, uint8_t border = 0) :
      _x(x), _y(y), _w(w), _h(h),
      _mrgn_t(margin), _mrgn_b(margin), _mrgn_l(margin), _mrgn_r(margin),
      _bordr_t(border), _bordr_b(border), _bordr_l(border), _bordr_r(border) {};

    /* Constructor that specifies each margin specifically. */
    GfxUILayout(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint8_t m_t, uint8_t m_b, uint8_t m_l, uint8_t m_r) :
      _x(x), _y(y), _w(w), _h(h), _mrgn_t(m_t), _mrgn_b(m_b), _mrgn_l(m_l), _mrgn_r(m_r) {};

    /* Fully-specified constructor. */
    GfxUILayout(
      uint32_t x, uint32_t y, uint16_t w, uint16_t h,
      uint8_t m_t, uint8_t m_b, uint8_t m_l, uint8_t m_r,
      uint8_t b_t, uint8_t b_b, uint8_t b_l, uint8_t b_r
    ) :
      _x(x), _y(y), _w(w), _h(h),
      _mrgn_t(m_t), _mrgn_b(m_b), _mrgn_l(m_l), _mrgn_r(m_r),
      _bordr_t(m_t), _bordr_b(m_b), _bordr_l(m_l), _bordr_r(m_r) {};

    /* Copy constructor. */
    GfxUILayout(const GfxUILayout &src) :
      _x(src._x), _y(src._y), _w(src._w), _h(src._h),
      _mrgn_t(src._mrgn_t), _mrgn_b(src._mrgn_b), _mrgn_l(src._mrgn_l), _mrgn_r(src._mrgn_r),
      _bordr_t(src._bordr_t), _bordr_b(src._bordr_b), _bordr_l(src._bordr_l), _bordr_r(src._bordr_r) {};

    /* Copy constructor. */
    GfxUILayout(GfxUILayout* src) :
      _x(src->_x), _y(src->_y), _w(src->_w), _h(src->_h),
      _mrgn_t(src->_mrgn_t), _mrgn_b(src->_mrgn_b), _mrgn_l(src->_mrgn_l), _mrgn_r(src->_mrgn_r),
      _bordr_t(src->_bordr_t), _bordr_b(src->_bordr_b), _bordr_l(src->_bordr_l), _bordr_r(src->_bordr_r) {};


    inline uint32_t elementPosX() {      return _x;    };
    inline uint32_t elementPosY() {      return _y;    };
    inline uint16_t elementWidth() {     return _w;    };
    inline uint16_t elementHeight() {    return _h;    };

    inline uint32_t xCornerUpperLeft() {     return _x;    };
    inline uint32_t xCornerUpperRight() {    return (_x + _w);    };
    inline uint32_t xCornerLowerLeft() {     return _x;    };
    inline uint32_t xCornerLowerRight() {    return (_x + _w);    };
    inline uint32_t yCornerUpperLeft() {     return _y;    };
    inline uint32_t yCornerUpperRight() {    return _y;    };
    inline uint32_t yCornerLowerLeft() {     return (_y + _h);    };
    inline uint32_t yCornerLowerRight() {    return (_y + _h);    };

    // These terrible inlines calculate the internal bounds of the renderable
    //   area after borders and margin have been taken into account.
    inline uint16_t internalPosX() {     return (_x + _mrgn_l + _bordr_l);  };
    inline uint16_t internalPosY() {     return (_y + _mrgn_t + _bordr_t);  };
    inline uint16_t internalWidth() {    return (_w - (_mrgn_r + _bordr_r + _mrgn_l + _bordr_l));   };
    inline uint16_t internalHeight() {   return (_h - (_mrgn_t + _bordr_t + _mrgn_b + _bordr_b));   };

    /* Does the given point fall on this region? */
    bool includesPoint(const uint32_t x, const uint32_t y) {
      return ((x >= _x) && (x < (_x + _w)) && (y >= _y) && (y < (_y + _h)));
    };

    // GfxUILayout flowInternal() {
    //   return GfxUILayout(
    //     internalPosX(), internalPosY(),
    //     internalWidth(), internalHeight(),
    //     0, 0, 0, 0,
    //     0, 0, 0, 0               // Border_px(t, b, l, r)
    //   );
    // };


    /* Static utility methods for automating flows during view construction. */
    static bool flowRight(GfxUILayout*, uint32_t spacing = 0);
    static bool flowDown(GfxUILayout*, uint32_t spacing = 0);


  protected:
    uint32_t _x;       // Location of the upper-left corner.
    uint32_t _y;       // Location of the upper-left corner.
    uint16_t _w;       // Size of the element.
    uint16_t _h;       // Size of the element.
    uint8_t  _mrgn_t;  // How many pixels inset should be the content?
    uint8_t  _mrgn_b;  // How many pixels inset should be the content?
    uint8_t  _mrgn_l;  // How many pixels inset should be the content?
    uint8_t  _mrgn_r;  // How many pixels inset should be the content?
    uint8_t  _bordr_t; // How many pixels should be the drawn border?
    uint8_t  _bordr_b; // How many pixels should be the drawn border?
    uint8_t  _bordr_l; // How many pixels should be the drawn border?
    uint8_t  _bordr_r; // How many pixels should be the drawn border?
};



/*
* This class is used to track responses to notify() on the exit-side of the
*   call stack.
*/
//class GfxUINoticeResponse {
//  //GfxUIElement* ptr;   // TODO: Going to try handling it with node priority. But a map would be best.
//  // TODO: How do I not have a hash-map object yet?
//  public:
//    GfxUIEvent event;
//
//  private:
//    //FlagContainer32 _event_mask;  // Bitmask derived from GfxUIEvent.
//};


/*******************************************************************************
* Base class that handles all touchable images.
*******************************************************************************/
class GfxUIElement : public GfxUILayout {
  public:
    virtual ~GfxUIElement() {};

    void enableFrames(uint32_t frame_flags = GFXUI_FLAG_DRAW_FRAME_MASK) {
      _class_clear_flag(GFXUI_FLAG_DRAW_FRAME_MASK);
      _class_set_flag(frame_flags & GFXUI_FLAG_DRAW_FRAME_MASK);
      _need_redraw(true);
    };

    void muteRender(bool x);
    inline bool muteRender() {           return _flags.value(GFXUI_FLAG_MUTE_RENDER);        };
    inline void shouldReap(bool x) {     _flags.set(GFXUI_FLAG_FREE_THIS_ELEMENT, x);        };
    inline bool shouldReap() {           return _flags.value(GFXUI_FLAG_FREE_THIS_ELEMENT);  };
    inline void elementActive(bool x) {  _flags.set(GFXUI_FLAG_INACTIVE, !x);                };
    inline bool elementActive() {        return !_flags.value(GFXUI_FLAG_INACTIVE);          };
    inline void isFocused(bool x) {      _flags.set(GFXUI_FLAG_CURRENT_FOCUS, x);            };
    inline bool isFocused() {            return _flags.value(GFXUI_FLAG_CURRENT_FOCUS);      };
    inline void isSelected(bool x) {     _flags.set(GFXUI_FLAG_CURRENT_SELECTION, x);        };
    inline bool isSelected() {           return _flags.value(GFXUI_FLAG_CURRENT_SELECTION);  };
    inline bool underPointer() {         return _flags.value(GFXUI_FLAG_UNDER_POINTER);      };
    inline void isDraggable(bool x) {    _flags.set(GFXUI_FLAG_DRAGGABLE, x);                };
    inline bool isDraggable() {          return _flags.value(GFXUI_FLAG_DRAGGABLE);          };
    inline void isDragging(bool x) {     _flags.set(GFXUI_FLAG_DRAGGING, x);                 };
    inline bool isDragging() {           return _flags.value(GFXUI_FLAG_DRAGGING);           };
    inline void trackPointer(bool x) {   _flags.set(GFXUI_FLAG_TRACK_POINTER, x);             };
    inline bool trackPointer() {         return _flags.value(GFXUI_FLAG_TRACK_POINTER);       };

    inline void setMargins(uint8_t t, uint8_t b, uint8_t l, uint8_t r) {
      _mrgn_t = t;
      _mrgn_b = b;
      _mrgn_l = l;
      _mrgn_r = r;
      _need_redraw(true);
    };

    inline void fill(UIGfxWrapper* ui_gfx, uint32_t color) {
      ui_gfx->img()->fillRect(internalPosX(), internalPosY(), internalWidth(), internalHeight(), color);
    };

    void reposition(uint32_t x, uint32_t y);

    void resize(uint32_t w, uint32_t h) {
      _w = w;
      _h = h;
      _need_redraw(true);
    };

    /*
    * Top-level objects are the first to handle notify.
    * Iteration and recursion both stop on the first positive return value.
    */
    bool notify(const GfxUIEvent GFX_EVNT, const uint32_t x, const uint32_t y, PriorityQueue<GfxUIElement*>* change_log);

    /*
    * Top-level objects are the last to render.
    * Iteration and recursion both touch the entire tree.
    */
    int render(UIGfxWrapper* ui_gfx, bool force = false);


  protected:
    GfxUIStyle _style;
    PriorityQueue<GfxUIElement*> _children;
    uint32_t _pointer_x;
    uint32_t _pointer_y;

    GfxUIElement(const GfxUILayout layout, const GfxUIStyle style, uint32_t f);
    GfxUIElement(GfxUILayout* layout, GfxUIStyle* style, uint32_t f);
    GfxUIElement(GfxUILayout* layout, uint32_t f) : GfxUIElement(layout, nullptr, f) {};
    GfxUIElement(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t f);

    int _add_child(GfxUIElement*);
    int _remove_child(GfxUIElement*);
    //int8_t _reflow(GfxUILayout);   // TODO: Now that laytouts are working well, automate their flows.

    /* These are the obligate overrides. */
    virtual bool _notify(const GfxUIEvent, const uint32_t x, const uint32_t y, PriorityQueue<GfxUIElement*>* change_log) =0;
    virtual int  _render(UIGfxWrapper*) =0;

    inline void _need_redraw(bool x) {  _class_set_flag(GFXUI_FLAG_NEED_RERENDER, x);   };
    inline bool _need_redraw() {        return _class_flag(GFXUI_FLAG_NEED_RERENDER | GFXUI_FLAG_ALWAYS_REDRAW);   };

    inline uint32_t _class_flags() {                   return _flags.raw;       };
    inline bool _class_flag(uint32_t f) {              return _flags.value(f);  };
    inline void _class_clear_flag(uint32_t f) {        _flags.clear(f);         };
    inline void _class_flip_flag(uint32_t f) {         _flags.set(f, _flags.value(f));  };
    inline void _class_set_flag(uint32_t f, bool x = true) {  _flags.set(f, x);        };

  private:
    FlagContainer32 _flags;  // If set, the step has been done.

    bool   _notify_children(const GfxUIEvent GFX_EVNT, const uint32_t x, const uint32_t y, PriorityQueue<GfxUIElement*>* change_log);
    int    _render_children(UIGfxWrapper*, bool force);
};



/*******************************************************************************
* Tail-inclusions to bring in the specific modules written around the API above.
*******************************************************************************/
#include "GfxUI/GfxUIKit.h"
#include "GfxUI/GfxUIKeyValuePair.h"
#include "GfxUI/GfxUIBlobRender.h"
#include "GfxUI/GfxUIGraphing.h"
#include "GfxUI/GfxUICryptoBurrito.h"


#endif  // __C3P_GFXUI_H
