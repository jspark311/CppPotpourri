/*
File:   ImageUtils.cpp
Author: J. Ian Lindsay
Date:   2022.05.27

This source file was never part of Adafruit's library. They are small graphics
  utilities that help implement simple UIs.

TODO: This should ultimately condense into an instantiable class, to be bound
  to specific Image instances.
*/

#include "../ImageUtils.h"


const char* const getDataVisString(const DataVis e) {
  switch (e) {
    case DataVis::NONE:       return "NONE";
    case DataVis::GRAPH:      return "GRAPH";
    case DataVis::VECTOR:     return "VECTOR";
    case DataVis::COMPASS:    return "COMPASS";
    case DataVis::FIELD:      return "FIELD";
    case DataVis::TEXT:       return "TEXT";
  }
  return "UNKNOWN";
}


/*******************************************************************************
* Class boilerplate
*******************************************************************************/

UIGfxWrapper::UIGfxWrapper(Image* i) : _img(i) {
  _apply_color_map();
}



/*
* Set defaults for the color map.
*/
void UIGfxWrapper::_apply_color_map() {
  bg_color       = 0;
  fg_color       = _img->convertColor(0x00FFFFFF);
  active_color   = _img->convertColor(0x0000CCCC);
  inactive_color = _img->convertColor(0x00505050);
}


/*******************************************************************************
* Functions for rendering progress bars
*******************************************************************************/

/*
* Displays a progress bar that runs left to right.
* @param percent is in the range [0.0, 1.0]
*/
void UIGfxWrapper::drawProgressBarH(
  int x, int y, int w, int h, uint32_t color,
  bool draw_base, bool draw_val, float percent
) {
  if (draw_base) {   // Clear the way.
    _img->fillRect(x, y, w, h, bg_color);
  }
  uint32_t pix_width = percent * (w-2);
  int blackout_x = x+1+pix_width;
  int blackout_w = (w+2)-pix_width;

  _img->fillRoundRect(blackout_x, y+1, blackout_w, h-2, 3, bg_color);
  _img->fillRoundRect(x+1, y+1, pix_width, h-2, 3, color);
  _img->drawRoundRect(x, y, w, h, 3, fg_color);

  if (draw_val && ((h-4) >= 7)) {
    // If we have space to do so, and the application requested it, draw the
    //   progress value in the middle of the bar.
    int txt_x = x+3;
    int txt_y = y+3;
    StringBuilder temp_str;
    temp_str.concatf("%d%%", (int) (percent*100));
    _img->setCursor(txt_x, txt_y);
    _img->setTextColor(fg_color);
    _img->writeString((char*) temp_str.string());
  }
}


/*
* Displays a progress bar that runs bottom to top.
* @param percent is in the range [0.0, 1.0]
*/
void UIGfxWrapper::drawProgressBarV(
  int x, int y, int w, int h, uint32_t color,
  bool draw_base, bool draw_val, float percent
) {
  if (draw_base) {   // Clear the way.
    _img->fillRect(x, y, w, h, bg_color);
  }
  uint32_t pix_height = percent * (h-2);
  int blackout_h = y+(h-1)-pix_height;
  _img->fillRoundRect(x+1, y+1, w-2, blackout_h, 3, bg_color);
  _img->fillRoundRect(x+1, (y+h-1)-pix_height, w-2, pix_height, 3, color);
  _img->drawRoundRect(x, y, w, h, 3, fg_color);

  if (draw_val && ((w-4) >= 15)) {
    // If we have space to do so, and the application requested it, draw the
    //   progress value in the middle of the bar.
    int txt_x = x+2;
    // If there is not space under the line, draw above it.
    int txt_y = (9 < pix_height) ? ((y+2+h)-pix_height) : ((y+h)-(pix_height+8));
    StringBuilder temp_str;
    temp_str.concatf("%d%%", (int) (percent*100));
    _img->setCursor(txt_x, txt_y);
    _img->setTextColor(fg_color);
    _img->writeString((char*) temp_str.string());
  }
}



/*******************************************************************************
* Functions for rendering specific kinds of data.
*******************************************************************************/

/*
*/
void UIGfxWrapper::drawCompass(
  int x, int y, int w, int h,
  bool scale_needle, bool draw_val, float bearing_field, float bearing_true_north
) {
  int origin_x = x + (w >> 1);
  int origin_y = y + (h >> 1);
  const uint32_t RED   = _img->convertColor(0x000000FF);
  const uint32_t WHITE = _img->convertColor(0x00FFFFFF);
  int maximal_extent = (strict_min((int16_t) w, (int16_t) h) >> 1) - 1;
  const int NEEDLE_WIDTH = maximal_extent >> 3;
  _img->fillCircle(origin_x, origin_y, maximal_extent, bg_color);
  _img->drawCircle(origin_x, origin_y, maximal_extent, fg_color);
  int displacement_x = cos(bearing_field * (PI/180.0)) * maximal_extent;
  int displacement_y = sin(bearing_field * (PI/180.0)) * maximal_extent;
  int displacement_tri_x = cos((bearing_field + 90.0) * (PI/180.0)) * NEEDLE_WIDTH;
  int displacement_tri_y = sin((bearing_field + 90.0) * (PI/180.0)) * NEEDLE_WIDTH;

  int needle_tip_n_x = displacement_x + origin_x;
  int needle_tip_n_y = displacement_y + origin_y;
  int needle_tip_s_x = (displacement_x * -1) + origin_x;
  int needle_tip_s_y = (displacement_y * -1) + origin_y;
  int needle_x1 = displacement_tri_x + origin_x;
  int needle_y1 = displacement_tri_y + origin_y;
  int needle_x2 = (displacement_tri_x * -1) + origin_x;
  int needle_y2 = (displacement_tri_y * -1) + origin_y;
  _img->drawLine(origin_x, origin_y, needle_tip_s_x, needle_tip_s_y, WHITE);
  _img->drawLine(origin_x, origin_y, needle_tip_n_x, needle_tip_n_y, RED);
  //_img->fillTriangle(needle_tip_s_x, needle_tip_s_y, needle_x1, needle_y1, needle_x2, needle_y2, WHITE);
  //_img->fillTriangle(needle_tip_n_x, needle_tip_n_y, needle_x1, needle_y1, needle_x2, needle_y2, RED);
}


/*
* Draw the given data as a plane.
*/
void UIGfxWrapper::drawHeatMap(
  uint32_t x, uint32_t y, uint32_t w, uint32_t h,
  SensorFilter<float>* filt,
  uint32_t flags,
  float range_lock_low, float range_lock_hi
) {
  const bool lock_range_to_absolute = (flags & GFXUI_FLAG_LOCK_RANGE_V) ? true : false;
  const uint32_t MIN_ELEMENTS = strict_min((uint32_t) filt->windowSize(), (uint32_t) w * h);
  const uint32_t PIXEL_SIZE   = strict_min((uint32_t) w, (uint32_t) h) / MIN_ELEMENTS;
  const float    TEMP_MIN     = (range_lock_low == range_lock_hi) ? filt->minValue() : range_lock_low;
  const float    TEMP_MAX     = (range_lock_low == range_lock_hi) ? filt->maxValue() : range_lock_hi;
  const float    TEMP_RANGE   = TEMP_MAX - TEMP_MIN;
  const float    BINSIZE_T    = TEMP_RANGE / (PIXEL_SIZE * 8);  // Allotted display area gives scale factor.
  const float    MIDPOINT_T   = TEMP_RANGE / 2.0;

  uint8_t shift_value;
  switch (_img->format()) {
    // We need to make an intelligent choice about color granularity if we
    //   want the best results.
    case ImgBufferFormat::MONOCHROME:      shift_value = 1;    break;   // Is it above the midpoint, or not?
    case ImgBufferFormat::GREY_8:          shift_value = 8;    break;   // Middle value is midpoint, black is cold. White is hot.
    case ImgBufferFormat::R3_G3_B2:        shift_value = 5;    break;   //
    case ImgBufferFormat::GREY_16:         shift_value = 16;   break;   //
    case ImgBufferFormat::R5_G6_B5:        shift_value = 10;   break;   //
    case ImgBufferFormat::GREY_24:         shift_value = 24;   break;   //
    case ImgBufferFormat::R8_G8_B8:        shift_value = 16;   break;   //
    case ImgBufferFormat::R8_G8_B8_ALPHA:  shift_value = 16;   break;   //
    default: return;  // Anything else is unsupported.
  }
  float* dataset = filt->memPtr();

  //_img->setAddrWindow(x, y, w, h);
  for (uint32_t i = 0; i < MIN_ELEMENTS; i++) {
    uint32_t x = (i & 0x07) * PIXEL_SIZE;
    uint32_t y = (i >> 3) * PIXEL_SIZE;
    float pix_deviation = abs(MIDPOINT_T - dataset[i]);
    uint8_t pix_intensity = BINSIZE_T * (pix_deviation / (TEMP_MAX - MIDPOINT_T));
    uint32_t color = (dataset[i] <= MIDPOINT_T) ? pix_intensity : (pix_intensity << 11);
    _img->fillRect(x, y, PIXEL_SIZE, PIXEL_SIZE, color);
  }
  //_img->endWrite();
}




/*******************************************************************************
* 3D projected shapes
*******************************************************************************/

/*
* Given a vector object, and parameters for the graph, draw the data to the
*   display. The given vector must be normalized.
*/
void UIGfxWrapper::drawVector(
  int x, int y, int w, int h, uint32_t color,
  bool draw_axes, bool draw_val, float vx, float vy, float vz
) {
  const int PERSPECTIVE_SCALE = 1;
  int origin_x = x + (w >> 1);
  int origin_y = y + (h >> 1);
  if (draw_axes) {   // Draw the axes? The origin is in the middle of the field.
    _img->drawFastVLine(origin_x, y, h, fg_color);
    _img->drawFastHLine(x, origin_y, w, fg_color);
    _img->drawLine(x, (y+h), w, y, fg_color);
    // Only 1/8 of a cube (all vector components are positive).
    //_img->drawFastVLine(x, y, h, fg_color);
    //_img->drawFastHLine(x, (y+h), w, fg_color);
    //_img->drawLine(x, (y+h), w>>1, y>>1, fg_color);
  }
  // Project the vector onto the x/y plane.
  // To give a sense of depth, we use a triangle where only a line is required.
  // We want the y-axis to be northward on the display. So we have to change the
  //   sign of that component.
  Vector3<int> projected(vx * (w >> 1), vy*(h >> 1) * -1, 0);   // TODO: z is unimplemented
  int x1 = origin_x + projected.x - PERSPECTIVE_SCALE;
  int y1 = origin_y + projected.y;
  int x2 = origin_x + projected.x;
  int y2 = origin_y + projected.y - PERSPECTIVE_SCALE;
  _img->fillTriangle(origin_x, origin_y, x1, y1, x2, y2, color);
}



/*******************************************************************************
* Functions for rendering common UI elements
*******************************************************************************/

void UIGfxWrapper::drawButton(int x, int y, int w, int h, uint32_t color, bool pressed) {
  const uint32_t ELEMENT_RADIUS = 4;
  if (pressed) {
    _img->fillRoundRect(x, y, w, h, ELEMENT_RADIUS, color);
    _img->drawRoundRect(x, y, w, h, ELEMENT_RADIUS, fg_color);
  }
  else {
    _img->fillRect(x, y, w, h, bg_color);
    _img->drawRoundRect(x, y, w, h, ELEMENT_RADIUS, fg_color);
  }
}


void UIGfxWrapper::drawScrollbarH(int x, int y, int w, int h, uint32_t color, float pos) {
  //uint32_t slider_pix = 2 + (((1+sval) / 61.0f) * (_img->x()-5));
  //_img->fillRect(0, 55, _img->x()-1, 7, 0x0000);
  //_img->drawRoundRect(0, 54, _img->x(), 9, 3, 0xFFFF);
  //_img->fillRect(slider_pix-1, 55, 3, 7, 0xF800);
}


void UIGfxWrapper::drawScrollbarV(int x, int y, int w, int h, uint32_t color, float pos) {
  //uint32_t slider_pix = 2 + (((1+sval) / 61.0f) * (_img->x()-5));
  //_img->fillRect(0, 55, _img->x()-1, 7, 0x0000);
  //_img->drawRoundRect(0, 54, _img->x(), 9, 3, 0xFFFF);
  //_img->fillRect(slider_pix-1, 55, 3, 7, 0xF800);
}



/*
* Draw the data view selector widget.
*/
void UIGfxWrapper::draw_data_view_selector(
  int x, int y, int w, int h,
  DataVis opt0, DataVis opt1, DataVis opt2, DataVis opt3, DataVis opt4, DataVis opt5,
  DataVis selected
) {
  uint32_t offset = 0;
  //_img->setAddrWindow(x, y, w, h);
  _img->drawFastVLine(x, y, h, fg_color);
  _img->drawFastHLine(x, y, w, fg_color);
  _img->setCursor(x+2, y+2);
  _img->setTextColor(bg_color, fg_color);
  _img->writeString("VIS");
  offset += 9;
  _img->drawFastHLine(x, y + offset, w, fg_color);

  if (DataVis::NONE != opt0) {
    if (selected == opt0) {
      _img->setTextColor(bg_color, fg_color);
    }
    else {
      _img->setTextColor(fg_color, bg_color);
    }
    _img->setCursor(x+2, y+offset+2);
    _img->writeString(getDataVisString(opt0));
    offset += 10;
    _img->drawFastHLine(x, y + offset, w, fg_color);
  }
  if (DataVis::NONE != opt1) {
    if (selected == opt1) {
      _img->setTextColor(bg_color, fg_color);
    }
    else {
      _img->setTextColor(fg_color, bg_color);
    }
    _img->setCursor(x+2, y+offset+2);
    _img->writeString(getDataVisString(opt1));
    offset += 10;
    _img->drawFastHLine(x, y + offset, w, fg_color);
  }
  if (DataVis::NONE != opt2) {
    if (selected == opt2) {
      _img->setTextColor(bg_color, fg_color);
    }
    else {
      _img->setTextColor(fg_color, bg_color);
    }
    _img->setCursor(x+2, y+offset+2);
    _img->writeString(getDataVisString(opt2));
    offset += 10;
    _img->drawFastHLine(x, y + offset, w, fg_color);
  }
  if (DataVis::NONE != opt3) {
    if (selected == opt3) {
      _img->setTextColor(bg_color, fg_color);
    }
    else {
      _img->setTextColor(fg_color, bg_color);
    }
    _img->setCursor(x+2, y+offset+2);
    _img->writeString(getDataVisString(opt3));
    offset += 10;
    _img->drawFastHLine(x, y + offset, w, fg_color);
  }
  if (DataVis::NONE != opt4) {
    if (selected == opt4) {
      _img->setTextColor(bg_color, fg_color);
    }
    else {
      _img->setTextColor(fg_color, bg_color);
    }
    _img->setCursor(x+2, y+offset+2);
    _img->writeString(getDataVisString(opt4));
    offset += 10;
    _img->drawFastHLine(x, y + offset, w, fg_color);
  }
  if (DataVis::NONE != opt5) {
    if (selected == opt5) {
      _img->setTextColor(bg_color, fg_color);
    }
    else {
      _img->setTextColor(fg_color, bg_color);
    }
    _img->setCursor(x+2, y+offset+2);
    _img->writeString(getDataVisString(opt5));
    offset += 10;
    _img->drawFastHLine(x, y + offset, w, fg_color);
  }
  //_img->endWrite();
}
