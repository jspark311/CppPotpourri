/*
File:   ImageGraph.cpp
Author: J. Ian Lindsay
Date:   2023.04.07

These classes are built on top of the GfxUI classes, and implement data graphing
  elements of a UI.

Templates for abstracted rendering of cartesian graphs.
*/

#include "ImageGraph.h"


/*******************************************************************************
* ImageGraph
*******************************************************************************/
/**
*
*/
template <> void ImageGraph<uint32_t>::drawGraph(Image* img, const PixUInt POS_X, const PixUInt POS_Y) {
  const PixUInt FRUS_W  = frustum_width();
  const PixUInt FRUS_H  = frustum_height();
  const PixUInt INSET_X = (_w - FRUS_W);
  const PixUInt INSET_Y = (_h - FRUS_H);
  const PixUInt GRAPH_X = (POS_X  + INSET_X);
  const PixUInt GRAPH_Y = (POS_Y  + INSET_Y);
  const PixUInt GRAPH_W = (FRUS_W - INSET_X);
  const PixUInt GRAPH_H = (FRUS_H - INSET_Y);

  if ((img->x() >= (POS_X + _w)) && (img->y() >= (POS_Y + _h))) {
    // Blank the space and draw the basic frame and axes.
    img->fillRect(POS_X, POS_Y, _w, _h, bg_color);
    img->drawFastVLine((GRAPH_X - 1), GRAPH_Y, FRUS_H, fg_color);
    img->drawFastHLine((GRAPH_X - 1), (GRAPH_Y + (FRUS_H - 1)), FRUS_W, fg_color);

    if (trace0.enabled) {
      trace0.findBounds((FRUS_W - INSET_X), (FRUS_H - INSET_Y));
      uint32_t* tmp_ptr = (trace0.dataset + trace0.rend_offset_x);
      uint32_t  tmp_len = (trace0.data_len - trace0.rend_offset_x);
      for (uint32_t i = 0; i < tmp_len; i++) {
        const uint32_t DATA_VALUE = *(tmp_ptr + i);
        const uint32_t DELTA_Y    = (DATA_VALUE / trace0.v_scale);
        const PixUInt PNT_X_POS  = (GRAPH_X + i);
        const PixUInt PNT_Y_POS  = ((GRAPH_Y + FRUS_H) - DELTA_Y);
        if ((int32_t) i != trace0.accented_idx) {
          // Draw a normal point on the curve.
          img->setPixel(PNT_X_POS, PNT_Y_POS, trace0.color);
        }
        else {
          // Draw an accented point on the curve.
          const PixUInt POINT_SIZE = 3;
          PixUInt point_x = (PNT_X_POS - POINT_SIZE);
          PixUInt point_y = (PNT_Y_POS - POINT_SIZE);
          PixUInt point_h = ((POINT_SIZE << 1) + 1);  // Ensure an odd number.
          PixUInt point_w = ((POINT_SIZE << 1) + 1);  // Ensure an odd number.

          if (point_x < GRAPH_X) {
            // Point overflowing the left-hand element boundary?
            point_w = point_w - (GRAPH_X - point_x);
            point_x = GRAPH_X;
          }
          else if ((point_x + point_w) > (GRAPH_X + GRAPH_W)) {
            // Point overflowing the right-hand element boundary?
            point_w = (GRAPH_X + GRAPH_W) - point_x;
          }
          if (point_y < GRAPH_Y) {
            // Point overflowing the upper element boundary?
            point_h = point_h - (GRAPH_Y - point_y);
            point_y = GRAPH_Y;
          }
          else if ((point_y + point_h) > (GRAPH_Y + GRAPH_H)) {
            // Point overflowing the lower element boundary?
            point_h = point_h - ((point_y + point_h) - (GRAPH_Y + GRAPH_H));
          }
          img->fillRect(point_x, point_y, point_w, point_h, trace0.color);
          img->drawFastVLine(PNT_X_POS, GRAPH_Y, GRAPH_H, trace0.color);   // Vertical rule crossing accented point.

          const uint16_t TXT_PIXEL_HEIGHT = img->getFontHeight();
          StringBuilder temp_txt;
          PixUInt txt_x = (point_x + point_w + 1);
          PixUInt txt_y = ((point_y - GRAPH_Y) > TXT_PIXEL_HEIGHT) ? (point_y - TXT_PIXEL_HEIGHT) : GRAPH_Y;
          PixUInt txt_w = 0;
          PixUInt txt_h = 0;
          temp_txt.concatf("%u: %u", (trace0.offset_x + trace0.rend_offset_x + i), DATA_VALUE);
          img->getTextBounds(&temp_txt, txt_x, txt_y, &txt_x, &txt_y, &txt_w, &txt_h);
          if ((txt_w + txt_x) > (GRAPH_X + GRAPH_W)) {
            // If the string would overflow the element's right-hand boundary,
            //   render it on the left-hand side of the indicator line.
            txt_x = point_x - txt_w;
            if (txt_x < GRAPH_X) {
              // If we adjusted the text offset, and it now lies outside the
              //   left-hand boundary of the element, set the text to be at the
              //   boundary, and what happens happens.
              txt_x = GRAPH_X;
            }
          }

          if (txt_y < GRAPH_Y) {
            // If the string would overflow the element's upper boundary,
            //   render it on the element's ceiling.
            txt_y = GRAPH_Y;
          }
          else if ((txt_h + txt_y) > (GRAPH_Y + GRAPH_H)) {
            // If the string would overflow the element's lower boundary,
            //   render it on the element's floor.
            txt_y = ((GRAPH_Y + GRAPH_H) - TXT_PIXEL_HEIGHT);
          }

          img->setCursor(txt_x, txt_y);
          img->writeString(&temp_txt);
        }
      }

      StringBuilder tmp_val_str;
      if (trace0.show_y_range) {
        uint16_t y_adv = img->getFontHeight();
        img->setCursor(GRAPH_X+1, GRAPH_Y);
        img->setTextColor(fg_color, bg_color);
        tmp_val_str.concatf("%u", trace0.maxValue());
        img->writeString(&tmp_val_str);
        tmp_val_str.clear();
        img->setCursor(GRAPH_X+1, (GRAPH_Y+FRUS_H) - y_adv);
        tmp_val_str.concatf("%u", trace0.minValue());
        img->writeString(&tmp_val_str);
        tmp_val_str.clear();
      }
      if (trace0.show_value) {
        const uint32_t FINAL_DATUM = *(tmp_ptr + (tmp_len-1));
        uint32_t tmp = (FINAL_DATUM / trace0.v_scale);
        //img->fillCircle(x+w, tmp+y, 1, color);
        img->setCursor(GRAPH_X, strict_min((PixUInt) ((GRAPH_Y+FRUS_H)-tmp), (PixUInt) (FRUS_H-1)));
        img->setTextColor(trace0.color, bg_color);
        tmp_val_str.concatf("%u", FINAL_DATUM);
        img->writeString(&tmp_val_str);
        tmp_val_str.clear();
      }
    }
  }
}



/**
*
*/
template <> void ImageGraph<float>::drawGraph(Image* img, const PixUInt POS_X, const PixUInt POS_Y) {
  const PixUInt FRUS_W  = frustum_width();
  const PixUInt FRUS_H  = frustum_height();
  const PixUInt INSET_X = (_w - FRUS_W);
  const PixUInt INSET_Y = (_h - FRUS_H);
  const PixUInt GRAPH_X = (POS_X  + INSET_X);
  const PixUInt GRAPH_Y = (POS_Y  + INSET_Y);
  const PixUInt GRAPH_W = (FRUS_W - INSET_X);
  const PixUInt GRAPH_H = (FRUS_H - INSET_Y);

  if ((img->x() >= (POS_X + _w)) && (img->y() >= (POS_Y + _h))) {
    // Blank the space and draw the basic frame and axes.
    img->fillRect(POS_X, POS_Y, _w, _h, bg_color);
    img->drawFastVLine((GRAPH_X - 1), GRAPH_Y, FRUS_H, fg_color);
    img->drawFastHLine((GRAPH_X - 1), (GRAPH_Y + (FRUS_H - 1)), FRUS_W, fg_color);

    if (trace0.enabled) {
      trace0.findBounds((FRUS_W - INSET_X), (FRUS_H - INSET_Y));
      float*    tmp_ptr = (trace0.dataset + trace0.rend_offset_x);
      uint32_t  tmp_len = (trace0.data_len - trace0.rend_offset_x);
      for (uint32_t i = 0; i < tmp_len; i++) {
        const float    DATA_VALUE = *(tmp_ptr + i);
        const uint32_t DELTA_Y    = (DATA_VALUE / trace0.v_scale);
        const PixUInt PNT_X_POS  = (GRAPH_X + i);
        const PixUInt PNT_Y_POS  = ((GRAPH_Y + FRUS_H) - DELTA_Y);
        if ((int32_t) i != trace0.accented_idx) {
          // Draw a normal point on the curve.
          img->setPixel(PNT_X_POS, PNT_Y_POS, trace0.color);
        }
        else {
          // Draw an accented point on the curve.
          const PixUInt POINT_SIZE = 3;
          PixUInt point_x = (PNT_X_POS - POINT_SIZE);
          PixUInt point_y = (PNT_Y_POS - POINT_SIZE);
          PixUInt point_h = ((POINT_SIZE << 1) + 1);  // Ensure an odd number.
          PixUInt point_w = ((POINT_SIZE << 1) + 1);  // Ensure an odd number.

          if (point_x < GRAPH_X) {
            // Point overflowing the left-hand element boundary?
            point_w = point_w - (GRAPH_X - point_x);
            point_x = GRAPH_X;
          }
          else if ((point_x + point_w) > (GRAPH_X + GRAPH_W)) {
            // Point overflowing the right-hand element boundary?
            point_w = (GRAPH_X + GRAPH_W) - point_x;
          }
          if (point_y < GRAPH_Y) {
            // Point overflowing the upper element boundary?
            point_h = point_h - (GRAPH_Y - point_y);
            point_y = GRAPH_Y;
          }
          else if ((point_y + point_h) > (GRAPH_Y + GRAPH_H)) {
            // Point overflowing the lower element boundary?
            point_h = point_h - ((point_y + point_h) - (GRAPH_Y + GRAPH_H));
          }
          img->fillRect(point_x, point_y, point_w, point_h, trace0.color);
          img->drawFastVLine(PNT_X_POS, GRAPH_Y, GRAPH_H, trace0.color);   // Vertical rule crossing accented point.

          const uint16_t TXT_PIXEL_HEIGHT = img->getFontHeight();
          StringBuilder temp_txt;
          PixUInt txt_x = (point_x + point_w + 1);
          PixUInt txt_y = ((point_y - GRAPH_Y) > TXT_PIXEL_HEIGHT) ? (point_y - TXT_PIXEL_HEIGHT) : GRAPH_Y;
          PixUInt txt_w = 0;
          PixUInt txt_h = 0;
          temp_txt.concatf("%u: %.3f", (trace0.offset_x + trace0.rend_offset_x + i), (double) DATA_VALUE);
          img->getTextBounds(&temp_txt, txt_x, txt_y, &txt_x, &txt_y, &txt_w, &txt_h);
          if ((txt_w + txt_x) > (GRAPH_X + GRAPH_W)) {
            // If the string would overflow the element's right-hand boundary,
            //   render it on the left-hand side of the indicator line.
            txt_x = point_x - txt_w;
            if (txt_x < GRAPH_X) {
              // If we adjusted the text offset, and it now lies outside the
              //   left-hand boundary of the element, set the text to be at the
              //   boundary, and what happens happens.
              txt_x = GRAPH_X;
            }
          }

          if (txt_y < GRAPH_Y) {
            // If the string would overflow the element's upper boundary,
            //   render it on the element's ceiling.
            txt_y = GRAPH_Y;
          }
          else if ((txt_h + txt_y) > (GRAPH_Y + GRAPH_H)) {
            // If the string would overflow the element's lower boundary,
            //   render it on the element's floor.
            txt_y = ((GRAPH_Y + GRAPH_H) - TXT_PIXEL_HEIGHT);
          }

          img->setCursor(txt_x, txt_y);
          img->writeString(&temp_txt);
        }
      }

      StringBuilder tmp_val_str;
      if (trace0.show_y_range) {
        uint16_t y_adv = img->getFontHeight();
        img->setCursor(GRAPH_X+1, GRAPH_Y);
        img->setTextColor(fg_color, bg_color);
        tmp_val_str.concatf("%.2f", (double) trace0.maxValue());
        img->writeString(&tmp_val_str);
        tmp_val_str.clear();
        img->setCursor(GRAPH_X+1, (GRAPH_Y+FRUS_H) - y_adv);
        tmp_val_str.concatf("%.2f", (double) trace0.minValue());
        img->writeString(&tmp_val_str);
        tmp_val_str.clear();
      }
      if (trace0.show_value) {
        const float FINAL_DATUM = *(tmp_ptr + (tmp_len-1));
        float tmp = (FINAL_DATUM / trace0.v_scale);
        //img->fillCircle(x+w, tmp+y, 1, color);
        img->setCursor(GRAPH_X, strict_min((PixUInt) ((GRAPH_Y+FRUS_H)-tmp), (PixUInt) (FRUS_H-1)));
        img->setTextColor(trace0.color, bg_color);
        tmp_val_str.concatf("%.3f", (double) FINAL_DATUM);
        img->writeString(&tmp_val_str);
        tmp_val_str.clear();
      }
    }
  }
}
