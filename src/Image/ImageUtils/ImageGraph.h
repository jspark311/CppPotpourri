/*
File:   ImageGraph.h
Author: J. Ian Lindsay
Date:   2023.04.07

These classes are built on top of the GfxUI classes, and implement data graphing
  elements of a UI.

Tewmplates for abstracted rendering of cartesian graphs.
*/

#ifndef __C3P_IMG_GRAPHING_H
#define __C3P_IMG_GRAPHING_H

#include "../Image.h"
#include "../../SensorFilter.h"

/*
* A parameter class that defines a trace on a cartesian graph.
*/
class ImageGraphTrace {
  public:
  uint32_t color = 0x808080;
  float std_err_pos     = 0.0;  // Default is to not plot error bars.
  float std_err_neg     = 0.0;  // Default is to not plot error bars.
  uint32_t major_grid_x = 0;    // Default is off.
  uint32_t minor_grid_x = 0;    // Default is off.
  uint32_t major_grid_y = 0;    // Default is off.
  uint32_t minor_grid_y = 0;    // Default is off.
  //int32_t  scale_min_y  = 0;    // Default is off.
  //int32_t  scale_max_y  = 0;    // Default is off.
  float    v_scale       = 0.0; // The vertical scaling factor for the data.
  // TODO: Selections for discrete or joined dots, histogram mode.
  // TODO: Selections for discrete or joined dots.
  bool enabled      = false;
  bool autoscale_x  = false;
  bool autoscale_y  = true;
  bool show_x_range = false;
  bool show_y_range = true;
  bool show_value   = false;
  bool grid_lock_x  = false;   // Default is to allow the grid to scroll with the starting offset.
  bool grid_lock_y  = false;   // Default is to allow the grid to scroll with any range shift.

  uint32_t offset_x      = 0;  // Index 0 in the trace array is what index to be shown?
  uint32_t data_len      = 0;  // How long is the array?
  int32_t  accented_idx  = -1; // Isolates a single position in the data to be highlighted.
  uint32_t rend_offset_x = 0;  // What index in the trace array is first to be shown?

  // TODO: The values below are typed with more variety than uint32. Templatize.
  uint32_t* dataset   = nullptr;
  // TODO: The values below are derived. They need read-only accessors.
  uint32_t  max_value = 0;
  uint32_t  min_value = 0;
};


/*
* A class for rendering cartesian graphs.
* To facilitate building complex graphs, we don't force the feature-set into a
*   series of discrete API calls. Instead we use this object to retain state that
*   is costly to recalculate. We can then build up the state that we want and
*   render all in one pass.
*/
class ImageGraph {
  public:
    uint32_t fg_color;
    uint32_t bg_color;

    ImageGraphTrace trace0;
    ImageGraphTrace trace1;
    ImageGraphTrace trace2;

    ImageGraph(uint32_t w, uint32_t h) : fg_color(0), bg_color(0), _w(w), _h(h) {};
    ~ImageGraph() {};

    void drawGraph(Image*, const uint32_t x, const uint32_t y);  // Width and Height are implied.

    void setWidth(int w, int h);  //

    /* After options are applied, returns the size of the data that will exactly fill the window. */
    uint32_t frustum_width() {
      uint32_t ret = _w;
      uint32_t tmp = _w;
      const uint32_t INSET_X = (draw_ticks_x ? 3 : 1);
      if (tmp > INSET_X) {
        tmp -= INSET_X;   // Apply size of axis.
        ret = tmp;
      }
      return ret;
    }

    /* After options are applied, returns the pixel height where data will be shown. */
    uint32_t frustum_height() {
      uint32_t ret = _h;
      uint32_t tmp = _h;
      const uint32_t INSET_Y = (draw_ticks_y ? 3 : 1);
      if (tmp > INSET_Y) {
        tmp -= INSET_Y;   // Apply size of axis.
        ret = tmp;
      }
      return ret;
    }


  private:
    uint32_t _w;
    uint32_t _h;
    bool draw_ticks_x = false;
    bool draw_ticks_y = false;
};


#endif  // __C3P_IMG_GRAPHING_H
