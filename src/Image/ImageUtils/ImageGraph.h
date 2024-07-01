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

/*******************************************************************************
* ImageGraphTrace
* A parameter class that defines a trace on a cartesian graph.
*******************************************************************************/
template <class T> class ImageGraphTrace {
  public:
    // TODO: Histogram mode.
    uint32_t color;         // The color motif for this object.
    PixUInt major_grid_x;   // Default is off.
    PixUInt minor_grid_x;   // Default is off.
    PixUInt major_grid_y;   // Default is off.
    PixUInt minor_grid_y;   // Default is off.
    bool enabled;           // This trace will only render if this is set to true.
    bool autoscale_x;       // Axis autoscaling.
    bool autoscale_y;       // Axis autoscaling.
    bool show_x_range;      // Show axis bounds.
    bool show_y_range;      // Show axis bounds.
    bool show_value;        // Render the value of the final datum in the set.
    bool draw_curve;        // Draw lines between successive points on the graph.
    bool draw_grid;         // Draw a grid on the graph.
    bool grid_lock_x;       // Default is to allow the grid to scroll with the starting offset.
    bool grid_lock_y;       // Default is to allow the grid to scroll with any range shift.
    T*       dataset;       // Pointer to the data to be graphed.
    uint32_t data_len;      // How long is the array?
    uint32_t offset_x;      // Index 0 in the trace array is what index in the data?
    int32_t  accented_idx;  // Isolates a single position in the data to be highlighted.

    // Constructors
    ImageGraphTrace();
    ImageGraphTrace(const ImageGraphTrace<T>& src) {  copyFrom(&src);  };

    bool copyFrom(const ImageGraphTrace<T>*);

    // Accessors to derived data.
    inline T maxValue() {        return _max_value;                 };
    inline T minValue() {        return _min_value;                 };
    inline T rangeInFrustum() {  return (_max_value - _min_value);  };

    void findBounds(const PixUInt w, const PixUInt h);

    bool drawVGrid() {   return (draw_grid && ((0 != major_grid_x) | (0 != minor_grid_x)));  };
    bool drawHGrid() {   return (draw_grid && ((0 != major_grid_y) | (0 != minor_grid_y)));  };
    float v_scale() {    return _v_scale;  };


  private:
    T     _max_value;
    T     _min_value;
    float _v_scale;        // The vertical scaling factor for the data.
};


/* Trivial constructor */
template <class T> ImageGraphTrace<T>::ImageGraphTrace() :
  color(0x808080),
  major_grid_x(0), minor_grid_x(0),
  major_grid_y(0), minor_grid_y(0),
  enabled(false),
  autoscale_x(false),  autoscale_y(false),
  show_x_range(false), show_y_range(false),
  show_value(false),   draw_curve(false),
  draw_grid(false),    grid_lock_x(false),
  grid_lock_y(false),  dataset(nullptr),
  data_len(0),         offset_x(0),
  accented_idx(-1),
  _max_value(T(0)), _min_value(T(0)),
  _v_scale(1.0) {}




template <class T> bool ImageGraphTrace<T>::copyFrom(const ImageGraphTrace<T>* SRC) {
  // NOTE: We can't simply...
  //   memcpy(this, (void*) SRC, sizeof(ImageGraphTrace));
  // ...because we don't want derived data coming along for the ride.
  color          = SRC->color;
  major_grid_x   = SRC->major_grid_x;
  minor_grid_x   = SRC->minor_grid_x;
  major_grid_y   = SRC->major_grid_y;
  minor_grid_y   = SRC->minor_grid_y;
  enabled        = SRC->enabled;
  autoscale_x    = SRC->autoscale_x;
  autoscale_y    = SRC->autoscale_y;
  show_x_range   = SRC->show_x_range;
  show_y_range   = SRC->show_y_range;
  show_value     = SRC->show_value;
  draw_curve     = SRC->draw_curve;
  draw_grid      = SRC->draw_grid;
  grid_lock_x    = SRC->grid_lock_x;
  grid_lock_y    = SRC->grid_lock_y;
  dataset        = SRC->dataset;
  data_len       = SRC->data_len;
  offset_x       = SRC->offset_x;
  accented_idx   = SRC->accented_idx;
  // Derived data is zeroed.
  _max_value = T(0);
  _min_value = T(0);
  _v_scale   = 1.0f;
  return (enabled & (data_len > 0));
}



/*
* Given a pixel width and height of a frustum, finds the min/max values in the
*   visible dataset, and recalculates any stored parameters that depend on them.
*
* NOTE: We phrase the algebra in such a way as to make use of v_scale a matter
*   of multiplication, rather than division. Sole safety check is here.
*/
template <class T> void ImageGraphTrace<T>::findBounds(const PixUInt W, const PixUInt H) {
  // Re-locate the bounds of the range within this frustum.
  // NOTE: We want autoscaling to work the same way for fully-negative renders.
  _max_value = (autoscale_y ? T(0) : *(dataset));
  _min_value = (autoscale_y ? *(dataset) : T(0));
  const uint32_t SAFE_WIDTH = strict_min((uint32_t) W, data_len);
  for (uint32_t i = 0; i < SAFE_WIDTH; i++) {
    T tmp = *(dataset + i);
    if (tmp > _max_value) {   _max_value = tmp;   }
    if (tmp < _min_value) {   _min_value = tmp;   }
  }
  const T RANGE = rangeInFrustum();
  _v_scale = (RANGE != T(0)) ? ((float) H / (float) RANGE) : 1.0f;
}




/*******************************************************************************
* ImageGraph
* To facilitate building complex graphs, we don't force the feature-set into a
*   series of discrete API calls. Instead we use this object to retain state
*   that is costly to recalculate. We can then build up the state that we want
*   and render all in one pass.
*******************************************************************************/

template <class T> class ImageGraph {
  public:
    uint32_t fg_color;
    uint32_t bg_color;

    ImageGraphTrace<T> trace0;
    ImageGraphTrace<T> trace1;
    ImageGraphTrace<T> trace2;

    ImageGraph(PixUInt w, PixUInt h) : fg_color(0), bg_color(0), _w(w), _h(h) {};
    ~ImageGraph() {};

    void drawGraph(Image*, const PixUInt x, const PixUInt y);  // Width and Height are implied.

    void setWidth(int w, int h);  //
    PixUInt frustum_width();
    PixUInt frustum_height();


  private:
    PixUInt _w;
    PixUInt _h;
    bool draw_ticks_x = false;
    bool draw_ticks_y = false;

    void _draw_accented_point(ImageGraphTrace<T>*);
};



/* After options are applied, returns the size of the data that will exactly fill the window. */
template <class T> PixUInt ImageGraph<T>::frustum_width() {
  PixUInt ret = _w;
  PixUInt tmp = _w;
  const PixUInt INSET_X = (draw_ticks_x ? 3 : 1);
  if (tmp > INSET_X) {
    tmp -= INSET_X;   // Apply size of axis.
    ret = tmp;
  }
  return ret;
}

/* After options are applied, returns the pixel height where data will be shown. */
template <class T> PixUInt ImageGraph<T>::frustum_height() {
  PixUInt ret = _h;
  PixUInt tmp = _h;
  const PixUInt INSET_Y = (draw_ticks_y ? 3 : 1);
  if (tmp > INSET_Y) {
    tmp -= INSET_Y;   // Apply size of axis.
    ret = tmp;
  }
  return ret;
}


template <class T> void ImageGraph<T>::_draw_accented_point(ImageGraphTrace<T>* trace) {
}

#endif  // __C3P_IMG_GRAPHING_H
