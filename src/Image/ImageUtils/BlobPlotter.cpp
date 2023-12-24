/*
File:   BlobPlotter.cpp
Author: J. Ian Lindsay
Date:   2023.12.17

These classes are built on top of the GfxUI classes, and implement various
  renderings of raw binary data.
*/

#include "BlobPlotter.h"
#include "../../C3PValue/C3PValue.h"
#include "../../AbstractPlatform.h"   // Needed for logging.


/*******************************************************************************
* BlobStylerEntropyMap
*******************************************************************************/
int8_t BlobStylerEntropyMap::init(const uint8_t* PTR, const uint32_t LEN) {
  return 0;  // TODO: Take measurements of blob.
}

uint32_t BlobStylerEntropyMap::getColor(const uint8_t* PTR, const uint32_t LEN, const uint32_t OFFSET) {
  const uint32_t CURRENT_BYTE = (uint32_t) *(PTR + OFFSET);
  uint32_t color = (CURRENT_BYTE << 16) + (CURRENT_BYTE << 8) + (CURRENT_BYTE);
  return color;
}


/*******************************************************************************
* BlobStylerHeatMap
*******************************************************************************/
int8_t BlobStylerHeatMap::init(const uint8_t* PTR, const uint32_t LEN) {
  return 0;  // TODO: Take measurements of blob.
}

uint32_t BlobStylerHeatMap::getColor(const uint8_t* PTR, const uint32_t LEN, const uint32_t OFFSET) {
  /** Option (A): Straight luminosity gradient according to byte value. */
  const uint32_t CURRENT_BYTE = (uint32_t) *(PTR + OFFSET);
  uint32_t color = (CURRENT_BYTE << 16) + (CURRENT_BYTE << 8) + (CURRENT_BYTE);

  /** Option (B): porting of UIGfxWrapper::drawHeatMap(). */
  /*
  const uint8_t CURRENT_BYTE = *(PTR + OFFSET);
  uint8_t shift_value = 1;
  switch (_color_fmt) {
    // We need to make an intelligent choice about color granularity if we
    //   want the best results.
    default:                               shift_value = 1;    break;   // Anything else is unsupported. Monochrome shift.
    case ImgBufferFormat::GREY_8:          shift_value = 8;    break;   // Is it above the midpoint, or not?
    case ImgBufferFormat::R3_G3_B2:        shift_value = 5;    break;   // Middle value is midpoint, black is cold. White is hot.
    case ImgBufferFormat::GREY_16:         shift_value = 16;   break;   //
    case ImgBufferFormat::R5_G6_B5:        shift_value = 10;   break;   //
    case ImgBufferFormat::GREY_24:         shift_value = 24;   break;   //
    case ImgBufferFormat::R8_G8_B8:        shift_value = 16;   break;   //
    case ImgBufferFormat::R8_G8_B8_ALPHA:  shift_value = 16;   break;   //
  }
  uint8_t pix_intensity = strict_abs_delta((uint8_t) 127, CURRENT_BYTE);
  uint32_t color = (CURRENT_BYTE <= 127) ? pix_intensity : (pix_intensity << 11);

  //
  // TODO: These pieces are all that is left of void UIGfxWrapper::drawHeatMap().
  // The Stark Fist of Retrieval
  //const bool lock_range_to_absolute = (flags & GFXUI_FLAG_LOCK_RANGE_V) ? true : false;
  //const uint32_t MIN_ELEMENTS = strict_min((uint32_t) filt->windowSize(), (uint32_t) w * h);
  //const uint32_t PIXEL_SIZE   = strict_min((uint32_t) w, (uint32_t) h) / MIN_ELEMENTS;
  //const float    TEMP_MIN     = (range_lock_low == range_lock_hi) ? filt->minValue() : range_lock_low;
  //const float    TEMP_MAX     = (range_lock_low == range_lock_hi) ? filt->maxValue() : range_lock_hi;
  //const float    TEMP_RANGE   = TEMP_MAX - TEMP_MIN;
  //const float    BINSIZE_T    = TEMP_RANGE / (PIXEL_SIZE * 8);  // Allotted display area gives scale factor.
  //const float    MIDPOINT_T   = TEMP_RANGE / 2.0;
  //float* dataset = filt->memPtr();

  //for (uint32_t i = 0; i < MIN_ELEMENTS; i++) {
  //  uint32_t x = (i & 0x07) * PIXEL_SIZE;
  //  uint32_t y = (i >> 3) * PIXEL_SIZE;
  //  float pix_deviation = abs(MIDPOINT_T - dataset[i]);
  //  uint8_t pix_intensity = BINSIZE_T * (pix_deviation / (TEMP_MAX - MIDPOINT_T));
  //  uint32_t color = (dataset[i] <= MIDPOINT_T) ? pix_intensity : (pix_intensity << 11);
  //  _img->fillRect(x, y, PIXEL_SIZE, PIXEL_SIZE, color);
  //}
  //
  // TODO: /Stark Fist
  //
  */

  return color;
}



/*******************************************************************************
* BlobStylerExplicitFencing
*******************************************************************************/
int8_t BlobStylerExplicitFencing::init(const uint8_t* PTR, const uint32_t LEN) {
  return 0;  // TODO: Take measurements of blob.
}

uint32_t BlobStylerExplicitFencing::getColor(const uint8_t* PTR, const uint32_t LEN, const uint32_t OFFSET) {
  uint32_t ret = 0;  // Without fences, there will be only black.
  bool  keep_searching = true;
  int fence_position = 0;
  while (keep_searching) {
    if (fence_position < _fences.size()) {
      int fence_offset = _fences.getPriority(fence_position);
      if (fence_offset <= (int) OFFSET) {
        ret = _fences.get(fence_position);
        keep_searching = false;
      }
      fence_position++;
    }
    else {
      keep_searching = false;
    }
  }
  return ret;
}

/*
* Defines a new color for an offset.
* All subsequent bytes will share this color until/unless a new fence is
*   encountered during rendering. Thus, the final fence will define the color
*   until the end of the blob.
*
*/
int8_t BlobStylerExplicitFencing::addOffset(uint32_t offset, uint32_t color) {
  int8_t ret = -1;
  if (0 == _fences.getByPriority((int) offset)) {  // TODO: Technically not safe. Might be a valid color.
    ret = ((0 <= _fences.insert(color, (int) offset)) ? 0 : -2);
  }
  return ret;
}




/*******************************************************************************
* BlobPlotter
*******************************************************************************/
BlobPlotter::BlobPlotter(BlobStyler* styler, C3PValue* src_blob, Image* target, uint32_t x, uint32_t y, uint32_t w, uint32_t h) :
  _styler(styler),
  _src_blob(src_blob), _target(target), _t_x(x), _t_y(y), _t_w(w), _t_h(h),
  _offset_start(0), _offset_stop(0), _val_trace(0), _bytes_wide(0), _bytes_high(0),
  _force_render(false) {}


void BlobPlotter::setParameters(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
  _t_x = x;  _t_y = y;  _t_w = w;  _t_h = h;
  _force_render = true;
}


bool BlobPlotter::_needs_render() {
  bool dirty = false;
  if (nullptr != _src_blob) {
    dirty = ((_src_blob->trace() != _val_trace) | _force_render);
  }
  return dirty;
}


/**
* NOTE: This corpus of classes don't care about the TCode of the data contained
*   by the source C3PValue. All TCodes _should_ coerce peacefully into ptr/len.
* TODO: To the extent that they do NOT, it is a problem with C3PType. Fix it there.
* NOTE: A source array that is of zero-length will not fail this check, but such
*   a case (or nullptr for the array) will prevent the call to _curve_render(const uint8_t* PTR, const uint32_t LEN, const uint32_t T_SIZE).
*
* @return true if it is safe to proceed with the render. False if not.
*/
bool BlobPlotter::_able_to_render() {
  bool safe = false;
  if (nullptr != _styler) {
    if ((nullptr != _target) && _target->allocated()) {
      if ((0 < _t_w) & (0 < _t_h)) {
        const uint32_t X_EXTENT = (_t_x + _t_w);
        const uint32_t Y_EXTENT = (_t_y + _t_h);
        safe = ((_target->x() > X_EXTENT) & (_target->y() > Y_EXTENT));
      }
    }
  }
  return safe;
}



/**
* Super-class shim to conceal dirty and bounds-checking, and call the
*   operational function of the extending class if they pass.
*
* TODO: These return conventions rack disciprin.
* @return 0 on success, on negative on error.
*/
int8_t BlobPlotter::apply() {
  int8_t ret = 0;
  if (_needs_render()) {
    ret--;
    if (_able_to_render()) {
      ret--;
      int8_t fetch_success = 0;
      C3PBinBinder bin_binder = _src_blob->get_as_ptr_len(&fetch_success);
      if (0 < fetch_success) {
        ret--;
        if ((nullptr != bin_binder.buf) & (bin_binder.len > 0)) {
          ret--;
          // Hard-stop the offsets. If the caller has set them to something
          //   non-zero, they should be retained.
          // TODO: This will be buggy under some conditions. Re-think it.
          if ((_offset_stop  >= bin_binder.len) | (0 == _offset_stop)) {              _offset_stop  = bin_binder.len;  }
          if ((_offset_start >= bin_binder.len) | (_offset_start >= _offset_stop)) {  _offset_start = 0;               }
          const uint32_t DESIRED_RENDER_LEN = renderLength();
          if (_pixels_available() >= DESIRED_RENDER_LEN) {  // Enough area to hold all the data?
            ret--;
            _bytes_wide = 0;
            _bytes_high = 0;
            // Refresh the styler with the current state of the blob. Not all
            //   stylers care.
            // TODO: Take global offset as an option to this class.
            _styler->init((bin_binder.buf + _offset_start), DESIRED_RENDER_LEN);
            // Render the curve.
            if (0 == _curve_render(bin_binder.buf, _offset_start, DESIRED_RENDER_LEN)) {   // Render accomplished?
              _force_render = false;
              ret = 0;
            }
          }
        }
        else {
          // The C3PValue contained a nullptr, and/or a zero length.
          // TODO: Render an empty set.
          ret = 0;
        }
      }
    }
  }
  return ret;
}


/*
* Pad the LEN value such that it will produce a square output of squares.
*/
int8_t BlobPlotter::_calculate_square_size(const uint32_t LEN, const uint32_t T_SIZE, uint32_t* square_size) {
  const uint32_t STRICT_SQUARE_LIMIT = strict_min(_t_w, _t_h);   // Can't have a square larger than the bounding area...
  const double S_TO_FILL_AREA = sqrt(((double) T_SIZE / (double) LEN));
  if (1.0d > S_TO_FILL_AREA) return -1;
  const uint32_t ROUNDED_SQUARE_SIZE = (uint32_t) ceil(S_TO_FILL_AREA);
  const uint32_t SQUARE_SIZE    = strict_min(ROUNDED_SQUARE_SIZE, STRICT_SQUARE_LIMIT);
  const uint32_t BYTES_PER_ROW  = ((_t_w / SQUARE_SIZE) + 1);
  if (0 == BYTES_PER_ROW) return -1;
  const uint32_t TOTAL_ROWS     = ((LEN / BYTES_PER_ROW) + 1);
  const uint32_t UNSQUARE_BYTES = (LEN - (BYTES_PER_ROW * TOTAL_ROWS));   // Number of bytes surplus to the square.

  // Pad the length to yield a square that will include all the bytes in the
  //   range, and then recalculate the square size.
  const uint32_t PADDED_LEN       = (LEN + (BYTES_PER_ROW - UNSQUARE_BYTES));
  const uint32_t P_S_TO_FILL_AREA = (uint32_t) sqrt((double) T_SIZE / (double) PADDED_LEN);
  if (0 == P_S_TO_FILL_AREA) return -1;
  const uint32_t P_SQUARE_SIZE    = strict_min(P_S_TO_FILL_AREA, STRICT_SQUARE_LIMIT);
  const uint32_t P_BYTES_PER_ROW  = ((_t_w / P_SQUARE_SIZE) + 1);
  if (0 == P_BYTES_PER_ROW) return -1;
  const uint32_t P_TOTAL_ROWS     = ((LEN / P_BYTES_PER_ROW) + 1);
  *square_size = P_SQUARE_SIZE;
  return 0;
}



/*******************************************************************************
* BlobPlotterHilbertCurve
*******************************************************************************/

/* Gray code converter fxn taken from Wikipedia. */
uint32_t BlobPlotterHilbertCurve::_bin_to_reflected_gray(uint32_t idx) {
  return (idx ^ (idx >> 1));
}

/* Gray code converter fxn taken from Wikipedia. */
uint32_t BlobPlotterHilbertCurve::_reflected_gray_to_idx(uint32_t gray) {
  gray ^= (gray >> 16);
  gray ^= (gray >>  8);
  gray ^= (gray >>  4);
  gray ^= (gray >>  2);
  gray ^= (gray >>  1);
  return gray;
}

/*
* Superclass gives us a reasonable square size, so we don't need to worry about
*   scaling the curve to fit within the area. We only need to worry about
*   rendering the correct bytes to the correct square locations to preserve
*   locallity.
*
* Algorithm for the Hilbert curve came from...
* Programming the Hilbert curve (John Skilling)
* Citation: AIP Conference Proceedings 707, 381 (2004); doi: 10.1063/1.1751381
* View online: http://dx.doi.org/10.1063/1.1751381
*/
int8_t BlobPlotterHilbertCurve::_curve_render(const uint8_t* PTR, const uint32_t OFFSET, const uint32_t LEN) {
  int8_t ret = -1;
  // We need to find the ratio of H/W for this dataset, and we need to do
  //   it while (ideally) keeping a power-of-2 on the X-axis.
  uint32_t crude_scale = _pixels_available() / LEN;
  uint32_t square_size = 1;
  while (crude_scale > 3) {
    square_size = square_size * 2;    // Mul-2
    crude_scale = crude_scale >> 2;   // Div-4
  }
  const uint16_t BYTES_PER_ROW = (_t_w / square_size);
  const uint16_t BYTES_PER_COL = (_t_h / square_size);

  uint16_t x0_bits = 0;
  uint16_t x1_bits = 0;
  while (BYTES_PER_ROW >= pow(2, (x0_bits + 1))) {    x0_bits++;  }
  while (BYTES_PER_COL >= pow(2, (x1_bits + 1))) {    x1_bits++;  }
  if ((x0_bits == 0) | (x1_bits == 0) | (32 < (x0_bits + x1_bits))) {
    c3p_log(LOG_LEV_ERROR, "BlobPlotterHilbertCurve", "Bailout. Bits (x0/x1): %u/%u\t SS: %u", x0_bits, x1_bits, _bytes_wide, _bytes_high, square_size);
    return ret;
  }
  _bytes_wide = pow(2, x0_bits);        // Now we know.
  _bytes_high = (LEN / _bytes_wide);    // This is also certain to be properly scaled, but...
  if ((_bytes_wide * _bytes_high) < LEN) {  _bytes_high++;  }  // ...account for possible rounding error...
  while (_bytes_high >= pow(2, x1_bits)) {    x1_bits++;  }    // ...and re-seek the height bit-count, if needed.

  // In light of the resulting true field sizes, scale-back the per-byte render
  //   to fill the available area while still remaining within it.
  while ((_t_w < (square_size * _bytes_wide)) || (_t_h < (square_size * _bytes_high))) {
    square_size--;
  }
  // Bailout condition. Not enough area to fully-render in terms of
  //   perfect-squares on the X-axis.
  // NOTE: This is technically not a deal-breaker for the algorithm, but
  //   considering what we are rendering, pow(2) offsets make comprehension
  //   much easier. So we insist on it.
  if (square_size == 0) {
    c3p_log(LOG_LEV_ERROR, "BlobPlotterHilbertCurve", "Bailout. Bits (x0/x1): %u/%u\t (w/h): %u/%u\t SS: %u", x0_bits, x1_bits, _bytes_wide, _bytes_high, square_size);
    return ret;
  }

  // Generate bitmasks for each coordinate.
  const uint32_t X0_BIT_MASK_BASE = (0xAAAAAAAA >> (32-(x0_bits << 1)));
  const uint32_t X1_BIT_MASK_BASE = (0x55555555 >> (32-(x1_bits << 1)));

  for (uint32_t i = 0; i < LEN; i++) {
    uint32_t graycode = _bin_to_reflected_gray(i);
    // This step preserves curve continuity. Straight-up graycode would break
    //   it up into same-oriented tiles. Mutate the graycode to account for the
    //   reflections in the curve,
    // LSB first. The bottom two bits are just taken as-is. All subsequent bits
    //   impact the transform of the inferior bits.
    const uint16_t BITS_TO_LOOP = (strict_max(x0_bits, x1_bits) << 1);
    for (uint16_t n = 2; n < (BITS_TO_LOOP+1); n++) {
      const bool BIT_VALUE = ((graycode >> n) & 0x00000001);
      const uint32_t LOWER_BIT_MASK = (0xFFFFFFFF >> (31 - (n & 0xFE)));
      const uint32_t X0_BIT_MASK    = (X0_BIT_MASK_BASE & LOWER_BIT_MASK);
      const uint32_t X1_BIT_MASK    = (X1_BIT_MASK_BASE & LOWER_BIT_MASK);
      if (BIT_VALUE) {   // Invert x0
        graycode = (graycode & ~X0_BIT_MASK) | (~graycode & X0_BIT_MASK);
      }
      else if (0 == (n & 1)) {
        // Exchange if we are on an even bit. Otherwise there is no need.
        const uint32_t NEW_X1_BITS = ((graycode & X0_BIT_MASK) >> 1);
        const uint32_t NEW_X0_BITS = ((graycode & X1_BIT_MASK) << 1);
        graycode = (graycode & ~LOWER_BIT_MASK) | NEW_X0_BITS | NEW_X1_BITS;
      }
    }

    // Demux X/Y from Hilbert index.
    uint16_t geo_coord[2] = {0, 0};
    for (uint32_t n = 0; n < 32; n++) {
      // Odd bits are x0, which we construe as Cartesian-X.
      // Even bits are x1, which we construe as Cartesian-Y.
      const uint8_t COORD_IDX = ((n & 1) ? 0 : 1);
      const bool    BIT_VALUE = ((graycode >> n) & 0x00000001);
      geo_coord[COORD_IDX] = (geo_coord[COORD_IDX] >> 1) + (BIT_VALUE ? 0x8000 : 0);
    }
    // Draw the byte.
    uint32_t target_x = _t_x + (geo_coord[0] * square_size);
    uint32_t target_y = _t_y + (geo_coord[1] * square_size);
    if (((target_x + square_size) <= (_t_x + _t_w)) & ((target_y + square_size) <= (_t_y + _t_h))) {
      uint32_t color = _styler->getColor(PTR, LEN, (i+OFFSET));
      _target->fillRect(target_x, target_y, square_size, square_size, color);
    }
    else {
      c3p_log(LOG_LEV_ERROR, "BlobPlotterHilbertCurve", "Boundary violation: (%u)  %u   %u", i, target_x, target_y);
    }
  }
  ret = 0;
  return ret;
}



/*******************************************************************************
* BlobPlotterLinear
*******************************************************************************/
int8_t BlobPlotterLinear::_curve_render(const uint8_t* PTR, const uint32_t OFFSET, const uint32_t LEN) {
  int8_t ret = -1;
  uint32_t square_size = 0;
  const uint32_t PIX_AVAILABLE = (_t_h * _t_w);
  if (0 == _calculate_square_size(LEN, PIX_AVAILABLE, &square_size)) {
    //const uint32_t BYTES_PER_ROW  = (_t_w / square_size);
    uint32_t target_x = _t_x;
    uint32_t target_y = _t_y;
    for (uint32_t i = 0; i < LEN; i++) {
      uint32_t color = _styler->getColor(PTR, LEN, (i+OFFSET));
      if (0 < i) {  // TODO: Letting the compiler optimize this slop away bugs me. Just write it cleaner.
        //if (0 == (i % BYTES_PER_ROW)) {
        if ((target_x + square_size) >= (_t_x + _t_w)) {
          target_x  = _t_x;
          target_y += square_size;
        }
        else {
          target_x += square_size;
        }
      }
      _target->fillRect(target_x, target_y, square_size, square_size, color);
    }
    ret = 0;
  }
  return ret;
}
