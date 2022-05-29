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


#ifndef __MANUVR_GFXUI_H
#define __MANUVR_GFXUI_H

/*******************************************************************************
* UIGfxWrapper flags
*******************************************************************************/
#define GFXUI_BUTTON_FLAG_STATE               0x01   // Button state
#define GFXUI_BUTTON_FLAG_MOMENTARY           0x02   // Button reverts to off when released.

#define GFXUI_SLIDER_FLAG_VERTICAL            0x01   // This slider is vertical.
#define GFXUI_SLIDER_FLAG_RENDER_VALUE        0x02   // Overlay the value on the slider bar.
#define GFXUI_SLIDER_FLAG_MARK_ONLY           0x04   // Do not fill the space under the mark.


/*******************************************************************************
* UIGfxWrapper flags
*******************************************************************************/
/*
* These are the possible meanings of signals that might come in
*   from the user's plane.
*/
enum class GfxUIEvent : uint8_t {
  TOUCH       = 0x00,  //
  RELEASE     = 0x01,  //
  PRESSURE    = 0x02,  //
  DRAG        = 0x03,  //
};



/*******************************************************************************
* Baes class that handles all touchable images.
*******************************************************************************/
class GfxUIElement {
  public:
    bool includesPoint(uint32_t x, uint32_t y) {
      if ((x > _x) && (x < (_x + _w))) {
        if ((y > _y) && (y < (_y + _h))) {
          return true;
        }
      }
      return false;
    };

    virtual void render(UIGfxWrapper*) =0;
    virtual bool touch(uint32_t x, uint32_t y) =0;


  protected:
    uint32_t _x;     // Location of the upper-left corner.
    uint32_t _y;     // Location of the upper-left corner.
    uint16_t _w;     // Size of the element.
    uint16_t _h;     // Size of the element.
    uint8_t  _flags;
    uint16_t _id;    // ID code associated with the element.

    GfxUIElement(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint8_t f) : _x(x), _y(y), _w(w), _h(h), _flags(f) {};
    virtual ~GfxUIElement() {};

    inline uint8_t _class_flags() {                return _flags;            };
    inline bool _class_flag(uint8_t _flag) {       return (_flags & _flag);  };
    inline void _class_clear_flag(uint8_t _flag) { _flags &= ~_flag;         };
    inline void _class_set_flag(uint8_t _flag) {   _flags |= _flag;          };
    inline void _class_set_flag(uint8_t _flag, bool nu) {
      if (nu) _flags |= _flag;
      else    _flags &= ~_flag;
    };
};


/*******************************************************************************
* A graphical button
*******************************************************************************/
class GfxUIButton : public GfxUIElement {
  public:
    GfxUIButton(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t color, uint8_t f = 0) : GfxUIElement(x, y, w, h, f), _color_active_on(color) {};
    ~GfxUIButton() {};

    inline void pressed(bool x) {
      if (momentary()) {  _class_set_flag(GFXUI_BUTTON_FLAG_STATE, x);  }
      else if (x) {       _flags ^= GFXUI_BUTTON_FLAG_STATE;            }
    };
    inline void momentary(bool x) {  _class_set_flag(GFXUI_BUTTON_FLAG_MOMENTARY, x);   };
    inline bool pressed() {          return _class_flag(GFXUI_BUTTON_FLAG_STATE);       };
    inline bool momentary() {        return _class_flag(GFXUI_BUTTON_FLAG_MOMENTARY);   };

    void render(UIGfxWrapper* ui_gfx) {
      ui_gfx->drawButton(_x, _y, _w, _h, _color_active_on, pressed());
    };

    bool touch(uint32_t x, uint32_t y) {
      if (includesPoint(x, y)) {
        pressed(true);
        return true;
      }
      return false;
    };


  private:
    uint32_t _color_active_on;   // The accent color of the element when active.
    uint32_t _color_active_off;  // The accent color of the element when active.
    uint32_t _color_inactive;    // The accent color of the element when inactive.
};


/*******************************************************************************
* A graphical slider
*******************************************************************************/
class GfxUISlider : public GfxUIElement {
  public:
    GfxUISlider(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t color, uint8_t f = 0) : GfxUIElement(x, y, w, h, f), _color_marker(color) {};
    ~GfxUISlider() {};

    inline float value() {         return _percentage;    };
    inline void value(float x) {   _percentage = x;       };

    void render(UIGfxWrapper* ui_gfx) {
      if (_class_flag(GFXUI_SLIDER_FLAG_VERTICAL)) {
        ui_gfx->drawProgressBarV(
          _x, _y, _w, _h, _color_marker,
          true, _class_flag(GFXUI_SLIDER_FLAG_RENDER_VALUE),
          _percentage
        );
      }
      else {
        ui_gfx->drawProgressBarH(
          _x, _y, _w, _h, _color_marker,
          true, _class_flag(GFXUI_SLIDER_FLAG_RENDER_VALUE),
          _percentage
        );
      }
    };

    bool touch(uint32_t x, uint32_t y) {
      if (includesPoint(x, y)) {
        if (_class_flag(GFXUI_SLIDER_FLAG_VERTICAL)) {
          const float PIX_POS_REL = y - _y;
          _percentage = 1.0f - strict_min(1.0f, strict_max(0.0f, (PIX_POS_REL / (float)(_h-2))));
        }
        else {
          const float PIX_POS_REL = x - _x;
          _percentage = strict_min(1.0f, strict_max(0.0f, (PIX_POS_REL / (float)(_w-2))));
        }
        return true;
      }
      return false;
    };


  private:
    float    _percentage;        // The current position of the mark, as a fraction.
    uint32_t _color_marker;      // The accent color of the position mark.
    uint16_t _id;                // Function code associated with the button.
};


#endif  // __MANUVR_GFXUI_H
