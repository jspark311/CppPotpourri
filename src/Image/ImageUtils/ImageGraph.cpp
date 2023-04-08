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
template <> void ImageGraph<uint32_t>::drawGraph(Image* img, const uint32_t POS_X, const uint32_t POS_Y) {
  const uint32_t FRUS_W  = frustum_width();
  const uint32_t FRUS_H  = frustum_height();
  const uint32_t INSET_X = (_w - FRUS_W);
  const uint32_t INSET_Y = (_h - FRUS_H);
  const uint32_t GRAPH_X = POS_X + INSET_X;
  const uint32_t GRAPH_Y = POS_Y + INSET_Y;
  uint32_t v_max = 0;
  uint32_t v_min = 0;

  if (trace0.enabled) {
    uint32_t  tmp_len = trace0.data_len;
    uint32_t* tmp_ptr = trace0.dataset;
    if (FRUS_W < trace0.data_len) {
      // TODO: X-axis autoscaling.
      // Without X-axis auto-scaling, just show the tail of the array if the
      //   frustum isn't wide enough for all of it.
      trace0.rend_offset_x = (trace0.data_len - FRUS_W);
      tmp_ptr += trace0.rend_offset_x;
      tmp_len = FRUS_W;
    }
    // Re-locate the bounds of the range within this frustum.
    trace0.max_value = *tmp_ptr;
    trace0.min_value = 0; //*tmp_ptr;

    for (uint32_t i = 0; i < tmp_len; i++) {
      uint32_t tmp = *(tmp_ptr + i);
      if (tmp > trace0.max_value) {
        trace0.max_value = tmp;
      }
      if (tmp < trace0.min_value) {
        trace0.min_value = tmp;
      }
      //c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, "v_max \t %u", trace0.max_value);
    }
    trace0.v_scale   = (float) (trace0.max_value - trace0.min_value) / (float) FRUS_H;
    //float h_scale = data_len / w;
    //v_max = strict_max(v_max, trace0.max_value);  // TODO: Implies global graph scaling.
    v_max = trace0.max_value;
    //v_min = strict_min(v_min, trace0.min_value);  // TODO: Implies global graph scaling.
    v_min = trace0.min_value;
  }


  if ((img->x() >= (POS_X + _w)) && (img->y() >= (POS_Y + _h))) {
    // Blank the space and draw the basic frame and axes.
    img->fillRect(POS_X, POS_Y, _w, _h, bg_color);
    img->drawFastVLine((GRAPH_X - 1), GRAPH_Y, FRUS_H, fg_color);
    img->drawFastHLine((GRAPH_X - 1), (GRAPH_Y + (FRUS_H - 1)), FRUS_W, fg_color);

    if (trace0.enabled) {
      uint32_t  tmp_len = (trace0.data_len - trace0.rend_offset_x);
      uint32_t* tmp_ptr = (trace0.dataset + trace0.rend_offset_x);
      for (uint32_t i = 0; i < tmp_len; i++) {
        uint32_t tmp = *(tmp_ptr + i) / trace0.v_scale;
        img->setPixel((GRAPH_X + i), ((GRAPH_Y + FRUS_H) - tmp), trace0.color);
      }

      StringBuilder tmp_val_str;
      if (trace0.show_y_range) {
        uint16_t y_adv = img->getFontHeight();
        img->setCursor(GRAPH_X+1, GRAPH_Y);
        img->setTextColor(fg_color, bg_color);
        tmp_val_str.concatf("%u", v_max);
        img->writeString(&tmp_val_str);
        tmp_val_str.clear();
        img->setCursor(GRAPH_X+1, (GRAPH_Y+FRUS_H) - y_adv);
        tmp_val_str.concatf("%u", v_min);
        img->writeString(&tmp_val_str);
        tmp_val_str.clear();
      }
      if (trace0.show_value) {
        const uint32_t FINAL_DATUM = *(tmp_ptr + (tmp_len-1));
        uint32_t tmp = (FINAL_DATUM / trace0.v_scale);
        //img->fillCircle(x+w, tmp+y, 1, color);
        img->setCursor(GRAPH_X, strict_min((uint32_t) ((GRAPH_Y+FRUS_H)-tmp), (uint32_t) (FRUS_H-1)));
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
template <> void ImageGraph<float>::drawGraph(Image* img, const uint32_t POS_X, const uint32_t POS_Y) {
  const uint32_t FRUS_W  = frustum_width();
  const uint32_t FRUS_H  = frustum_height();
  const uint32_t INSET_X = (_w - FRUS_W);
  const uint32_t INSET_Y = (_h - FRUS_H);
  const uint32_t GRAPH_X = POS_X + INSET_X;
  const uint32_t GRAPH_Y = POS_Y + INSET_Y;
  uint32_t v_max = 0;
  uint32_t v_min = 0;

  if (trace0.enabled) {
    uint32_t  tmp_len = trace0.data_len;
    float*    tmp_ptr = trace0.dataset;
    if (FRUS_W < trace0.data_len) {
      // TODO: X-axis autoscaling.
      // Without X-axis auto-scaling, just show the tail of the array if the
      //   frustum isn't wide enough for all of it.
      trace0.rend_offset_x = (trace0.data_len - FRUS_W);
      tmp_ptr += trace0.rend_offset_x;
      tmp_len = FRUS_W;
    }
    // Re-locate the bounds of the range within this frustum.
    trace0.max_value = *tmp_ptr;
    trace0.min_value = 0; //*tmp_ptr;

    for (uint32_t i = 0; i < tmp_len; i++) {
      float tmp = *(tmp_ptr + i);
      if (tmp > trace0.max_value) {
        trace0.max_value = tmp;
      }
      if (tmp < trace0.min_value) {
        trace0.min_value = tmp;
      }
      //c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, "v_max \t %u", trace0.max_value);
    }
    trace0.v_scale   = (float) (trace0.max_value - trace0.min_value) / (float) FRUS_H;
    //float h_scale = data_len / w;
    //v_max = strict_max(v_max, trace0.max_value);  // TODO: Implies global graph scaling.
    v_max = trace0.max_value;
    //v_min = strict_min(v_min, trace0.min_value);  // TODO: Implies global graph scaling.
    v_min = trace0.min_value;
  }


  if ((img->x() >= (POS_X + _w)) && (img->y() >= (POS_Y + _h))) {
    // Blank the space and draw the basic frame and axes.
    img->fillRect(POS_X, POS_Y, _w, _h, bg_color);
    img->drawFastVLine((GRAPH_X - 1), GRAPH_Y, FRUS_H, fg_color);
    img->drawFastHLine((GRAPH_X - 1), (GRAPH_Y + (FRUS_H - 1)), FRUS_W, fg_color);

    if (trace0.enabled) {
      uint32_t  tmp_len = (trace0.data_len - trace0.rend_offset_x);
      float*    tmp_ptr = (trace0.dataset + trace0.rend_offset_x);
      for (uint32_t i = 0; i < tmp_len; i++) {
        float tmp = *(tmp_ptr + i) / trace0.v_scale;
        img->setPixel((GRAPH_X + i), ((GRAPH_Y + FRUS_H) - tmp), trace0.color);
      }

      StringBuilder tmp_val_str;
      if (trace0.show_y_range) {
        uint16_t y_adv = img->getFontHeight();
        img->setCursor(GRAPH_X+1, GRAPH_Y);
        img->setTextColor(fg_color, bg_color);
        tmp_val_str.concatf("%.2f", v_max);
        img->writeString(&tmp_val_str);
        tmp_val_str.clear();
        img->setCursor(GRAPH_X+1, (GRAPH_Y+FRUS_H) - y_adv);
        tmp_val_str.concatf(".2f", v_min);
        img->writeString(&tmp_val_str);
        tmp_val_str.clear();
      }
      if (trace0.show_value) {
        const uint32_t FINAL_DATUM = *(tmp_ptr + (tmp_len-1));
        uint32_t tmp = (FINAL_DATUM / trace0.v_scale);
        //img->fillCircle(x+w, tmp+y, 1, color);
        img->setCursor(GRAPH_X, strict_min((uint32_t) ((GRAPH_Y+FRUS_H)-tmp), (uint32_t) (FRUS_H-1)));
        img->setTextColor(trace0.color, bg_color);
        tmp_val_str.concatf(".3f", FINAL_DATUM);
        img->writeString(&tmp_val_str);
        tmp_val_str.clear();
      }
    }
  }
}
