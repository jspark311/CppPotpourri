/*
File:   Vector3Render.cpp
Author: J. Ian Lindsay (LLM assisted)
Date:   2025.07.28

A small render class for vectors.
*/

#include "../ImageUtils.h"
#include <math.h>
#include <algorithm>



Vector3Render::Vector3Render(Image* i)
  : _img(i),
    _addr(0, 0),
    _width(0),
    _height(0),
    _vec_x(0.0f),
    _vec_y(0.0f),
    _vec_z(0.0f),
    _axis_color_x(0xFF0000u),
    _axis_color_y(0x00FF00u),
    _axis_color_z(0x0000FFu),
    _vector_color(0xFFFF00u),
    _background_color(0u),
    _x_grid_marks(0),
    _y_grid_marks(0),
    _z_grid_marks(0),
    _need_rerender(false),
    _draw_anchor_lines(true),
    _draw_text_value(false),
    _pitch(0.0f),
    _roll(0.0f),
    _sin_pitch(0.0f),
    _cos_pitch(1.0f),
    _sin_roll(0.0f),
    _cos_roll(1.0f)
{}


int8_t Vector3Render::setSourceFrame(const PixAddr A,
                                     const PixUInt W,
                                     const PixUInt H)
{
  int8_t STATUS = 0;
  if ((W == 0) || (H == 0) || (_img == (Image*)0)) {
    STATUS = -1;
  }
  else {
    _addr = A;
    _width = W;
    _height = H;
    _need_rerender = true;
  }
  return STATUS;
}


int8_t Vector3Render::setVector(float x, float y, float z)
{
  int8_t STATUS = 0;
  _vec_x = x;
  _vec_y = y;
  _vec_z = z;
  _need_rerender = true;
  return STATUS;
}

void Vector3Render::setColors(
  const uint32_t COLOR_X,
  const uint32_t COLOR_Y,
  const uint32_t COLOR_Z,
  const uint32_t COLOR_VECTOR,
  const uint32_t COLOR_BG
)
{
  _axis_color_x = COLOR_X;
  _axis_color_y = COLOR_Y;
  _axis_color_z = COLOR_Z;
  _vector_color = COLOR_VECTOR;
  _background_color = COLOR_BG;
  _need_rerender = true;
}

void Vector3Render::setgridMarks(
  const uint8_t MARKS_X,
  const uint8_t MARKS_Y,
  const uint8_t MARKS_Z
)
{
  _x_grid_marks = MARKS_X;
  _y_grid_marks = MARKS_Y;
  _z_grid_marks = MARKS_Z;
  _need_rerender = true;
}

void Vector3Render::setOrientation(const float PITCH,
                                   const float ROLL)
{
  _pitch = PITCH;
  _roll = ROLL;
  _sin_pitch = sinf(_pitch);
  _cos_pitch = cosf(_pitch);
  _sin_roll = sinf(_roll);
  _cos_roll = cosf(_roll);
  _need_rerender = true;
}

void Vector3Render::setOrientation(const Quaternion QUATERNION)
{
  Quaternion q = QUATERNION;
  q.normalize();
  _pitch = asinf(2.0f * (q.w * q.y - q.z * q.x));
  _roll = atan2f(
    2.0f * (q.w * q.x + q.y * q.z),
    1.0f - 2.0f * (q.x * q.x + q.y * q.y)
  );
  _sin_pitch = sinf(_pitch);
  _cos_pitch = cosf(_pitch);
  _sin_roll = sinf(_roll);
  _cos_roll = cosf(_roll);
  _need_rerender = true;
}


void Vector3Render::render(bool force) {
  if (force || _need_rerender) {
    _img->fillRect(_addr.x, _addr.y, _width, _height, _background_color);
    _draw_axes();
    _draw_vector();
    _need_rerender = false;
  }
}


void Vector3Render::_draw_axes() {
  PointZ ORIG;
  _project_point(0.0f, 0.0f, 0.0f, ORIG);
  PointZ END;

  // X axis
  _project_point(_vec_x, 0.0f, 0.0f, END);
  _img->drawLine(ORIG.x, ORIG.y, END.x, END.y, _axis_color_x);
  if (_x_grid_marks > 0) {
    for (uint8_t I = 1; I <= _x_grid_marks; I++) {
      const float FRACT = (float)I / (float)(_x_grid_marks + 1);
      PointZ PT;
      _project_point(_vec_x * FRACT, 0.0f, 0.0f, PT);
      _img->drawCircle(PT.x, PT.y, 2, _axis_color_x);
    }
  }

  // Y axis
  _project_point(0.0f, _vec_y, 0.0f, END);
  _img->drawLine(ORIG.x, ORIG.y, END.x, END.y, _axis_color_y);
  if (_y_grid_marks > 0) {
    for (uint8_t I = 1; I <= _y_grid_marks; I++) {
      const float FRACT = (float)I / (float)(_y_grid_marks + 1);
      PointZ PT;
      _project_point(0.0f, _vec_y * FRACT, 0.0f, PT);
      _img->drawCircle(PT.x, PT.y, 2, _axis_color_y);
    }
  }

  // Z axis
  _project_point(0.0f, 0.0f, _vec_z, END);
  _img->drawLine(ORIG.x, ORIG.y, END.x, END.y, _axis_color_z);
  if (_z_grid_marks > 0) {
    for (uint8_t I = 1; I <= _z_grid_marks; I++) {
      const float FRACT = (float)I / (float)(_z_grid_marks + 1);
      PointZ PT;
      _project_point(0.0f, 0.0f, _vec_z * FRACT, PT);
      _img->drawCircle(PT.x, PT.y, 2, _axis_color_z);
    }
  }
}


void Vector3Render::_draw_vector() {
  PointZ ORIG;
  _project_point(0.0f, 0.0f, 0.0f, ORIG);
  PointZ TIP;
  _project_point(_vec_x, _vec_y, _vec_z, TIP);

  // Anchor lines to planes
  if (_draw_anchor_lines) {
    PointZ PXY;
    _project_point(_vec_x, _vec_y, 0.0f, PXY);
    _img->drawLine(TIP.x, TIP.y, PXY.x, PXY.y, 0x808080u);
    PointZ PXZ;
    _project_point(_vec_x, 0.0f, _vec_z, PXZ);
    _img->drawLine(TIP.x, TIP.y, PXZ.x, PXZ.y, 0x808080u);
    PointZ PYZ;
    _project_point(0.0f, _vec_y, _vec_z, PYZ);
    _img->drawLine(TIP.x, TIP.y, PYZ.x, PYZ.y, 0x808080u);
  }

  // Distance shading for vector
  const float DEP = TIP.z;
  const float CMP = DEP < -1.0f ? -1.0f : (DEP > 1.0f ? 1.0f : DEP);
  const float FACT = CMP * 0.5f + 0.5f;
  uint8_t RX = (uint8_t)((( _vector_color >> 16 ) & 0xFF) * FACT);
  uint8_t GX = (uint8_t)((( _vector_color >>  8 ) & 0xFF) * FACT);
  uint8_t BX = (uint8_t)((( _vector_color       ) & 0xFF) * FACT);
  const uint32_t COL = (RX << 16) | (GX << 8) | BX;
  _img->drawLine(ORIG.x, ORIG.y, TIP.x, TIP.y, COL);
  if (_draw_text_value) {
    StringBuilder buf;
    buf.concatf("<%.2f, %.2f, %.2f>", (double) _vec_x, (double) _vec_y, (double) _vec_z);
    _img->setCursor(TIP.x + 4, TIP.y + 4);
    _img->setTextSize(1);
    _img->setTextColor(_vector_color);
    _img->writeString(&buf);
  }
}


// Projects a 3D point onto 2D screen with given rotations
void Vector3Render::_project_point(float x0,
                    float y0,
                    float z0,
                    PointZ &out)
{
  float Y1 = y0 * _cos_pitch - z0 * _sin_pitch;
  float Z1 = y0 * _sin_pitch + z0 * _cos_pitch;
  float X2 = x0 * _cos_roll + Z1 * _sin_roll;
  float Z2 = -x0 * _sin_roll + Z1 * _cos_roll;
  out.x = _addr.x + (int)roundf(X2 * (_width < _height ? _width : _height) * 0.5f);
  out.y = _addr.y + (int)roundf((_height * 0.5f) - Y1 * (_width < _height ? _width : _height) * 0.5f);
}
