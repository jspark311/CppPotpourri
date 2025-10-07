/*
File:   GlobeRender.cpp
Author: J. Ian Lindsay (LLM assisted)
Date:   2025.07.24

A small render class for spheres with optional LAT/LON markers.
*/

#include "../ImageUtils.h"
#include <math.h>
#include <algorithm>


// Projects a 3D point onto 2D screen with given rotations
static void _project_point(float x0,
                         float y0,
                         float z0,
                         float sin_pitch,
                         float cos_pitch,
                         float sin_roll,
                         float cos_roll,
                         PixAddr center,
                         int radius,
                         PointZ &out)
{
    // Rotate around X-axis (pitch)
    float y1 = y0 * cos_pitch - z0 * sin_pitch;
    float z1 = y0 * sin_pitch + z0 * cos_pitch;

    // Rotate around Y-axis (roll)
    float x2 = x0 * cos_roll + z1 * sin_roll;
    float z2 = -x0 * sin_roll + z1 * cos_roll;

    // Orthographic projection
    out.x = center.x + static_cast<int>(roundf(x2 * radius));
    out.y = center.y - static_cast<int>(roundf(y1 * radius));
    out.z = z2;
}



// Initialize all member state explicitly
GlobeRender::GlobeRender(Image* i)
  : _img(i),
    _addr(0, 0), _center(0, 0),
    _width(0), _height(0),
    _radius(0),
    _sphere_color(0xFFFFFFu), _background_color(0u),
    _lat_lines(12), _lon_lines(12),
    _curve_segments(64),
    _need_rerender(false),
    _pitch(0.0f),     _roll(0.0f),
    _sin_pitch(0.0f), _cos_pitch(1.0f),
    _sin_roll(0.0f),  _cos_roll(1.0f)
{}



void GlobeRender::render(bool force) {
  if (!(force | _need_rerender)) {
    return;
  }
  _img->fillRect(_addr.x, _addr.y, _width, _height, _background_color);
  _img->drawCircle(_addr.x + _center.x, _addr.y + _center.y, _radius, _sphere_color);

    for (int lat = 1; lat < _lat_lines; lat++) {
      PointZ points[_curve_segments + 1];

        for (int seg = 0; seg <= _curve_segments; seg++) {
            float phi = (static_cast<float>(lat) / _lat_lines - 0.5f) * static_cast<float>(M_PI);
            float lambda = 2.0f * static_cast<float>(M_PI) * (static_cast<float>(seg) / _curve_segments);

            float x0 = cosf(phi) * cosf(lambda);
            float y0 = sinf(phi);
            float z0 = cosf(phi) * sinf(lambda);

            PointZ p;
            _project_point(x0, y0, z0,
                         _sin_pitch, _cos_pitch,
                         _sin_roll,  _cos_roll,
                         _center,    _radius, p);
            points[seg] = p;
        }

        for (int seg = 0; seg < _curve_segments; seg++) {
            PointZ A = points[seg];
            PointZ B = points[seg + 1];

            float depth = (A.z + B.z) * 0.5f;
            float clamped = depth < -1.0f ? -1.0f : (depth > 1.0f ? 1.0f : depth);
            uint8_t shade = static_cast<uint8_t>((clamped * 0.5f + 0.5f) * 255.0f);
            uint32_t color = (static_cast<uint32_t>(shade) << 16) |
                             (static_cast<uint32_t>(shade) <<  8) |
                              static_cast<uint32_t>(shade);

            _img->drawLine(_addr.x + A.x, _addr.y + A.y, _addr.x + B.x, _addr.y + B.y, color);
        }
    }

    for (int lon = 0; lon < _lon_lines; lon++) {
      PointZ points[_curve_segments + 1];
        for (int seg = 0; seg <= _curve_segments; seg++) {
            float phi = -0.5f * (float) M_PI + (float) seg / _curve_segments * (float) M_PI;
            float lambda = 2.0f * static_cast<float>(M_PI) * (static_cast<float>(lon) / _lon_lines);
            float x0 = cosf(phi) * cosf(lambda);
            float y0 = sinf(phi);
            float z0 = cosf(phi) * sinf(lambda);
            PointZ p;
            _project_point(x0, y0, z0,
                         _sin_pitch, _cos_pitch,
                         _sin_roll,  _cos_roll,
                         _center,    _radius, p);
            points[seg] = p;
        }

        for (int seg = 0; seg < _curve_segments; seg++) {
            PointZ A = points[seg];
            PointZ B = points[seg + 1];

            float depth = (A.z + B.z) * 0.5f;
            float clamped = depth < -1.0f ? -1.0f : (depth > 1.0f ? 1.0f : depth);
            uint8_t shade = static_cast<uint8_t>((clamped * 0.5f + 0.5f) * 255.0f);
            uint32_t color = (static_cast<uint32_t>(shade) << 16) |
                             (static_cast<uint32_t>(shade) <<  8) |
                              static_cast<uint32_t>(shade);
            _img->drawLine(_addr.x + A.x, _addr.y + A.y, _addr.x + B.x, _addr.y + B.y, color);
        }
    }
  _need_rerender = false;
}


void GlobeRender::renderWithMarker(float latitude, float longitude) {
  render(true);   // TODO: Always re-renders until this feature is moved to a child class.
  float x0 = cosf(latitude) * cosf(longitude);
  float y0 = sinf(latitude);
  float z0 = cosf(latitude) * sinf(longitude);

  PointZ p;
  _project_point(x0, y0, z0,
                 _sin_pitch, _cos_pitch,
                 _sin_roll, _cos_roll,
                 _center,    _radius, p);
  if (p.z < 0.0f) {
    return;
  }

    const uint32_t color = 0xFF0000u;
    const int ellipseRadiusX = 3;
    const int ellipseRadiusY = 3;
    int firstX = 0;
    int firstY = 0;
    int prevX = 0;
    int prevY = 0;
    bool isFirst = true;
    for (int angle = 0; angle < 360; angle += 15) {
      float rad = (angle * COFACTOR_DEGREE_TO_RADIAN);
      int dx = static_cast<int>(roundf(ellipseRadiusX * cosf(rad)));
      int dy = static_cast<int>(roundf(ellipseRadiusY * sinf(rad)));
      int px = p.x + dx;
      int py = p.y + dy;
      if (isFirst) {
        firstX = px;
        firstY = py;
        isFirst = false;
      }
      else {
        _img->drawLine(_addr.x + prevX, _addr.y + prevY, _addr.x + px, _addr.y + py, color);
      }
      prevX = px;
      prevY = py;
    }
    _img->drawLine(_addr.x + prevX, _addr.y + prevY, _addr.x + firstX, _addr.y + firstY, color);
}


bool GlobeRender::pixelToLatLon(const PixAddr ADDR,
                                  float &latitude,
                                  float &longitude)
{
  // Normalize pixel to unit sphere X and Y
  float x2 = (ADDR.x - _center.x) / (float) _radius;
  float y1 = (_center.y - ADDR.y) / (float) _radius;
  float r2sum = x2 * x2 + y1 * y1;
  if (r2sum > 1.0f) {
    return false;
  }

  float z2 = sqrtf(1.0f - r2sum);

  // Inverse roll
  float x0 = x2 * _cos_roll - z2 * _sin_roll;
  float z1 = x2 * _sin_roll + z2 * _cos_roll;

  // Inverse pitch
  float y0 = y1 * _cos_pitch + z1 * _sin_pitch;
  float z0 = -y1 * _sin_pitch + z1 * _cos_pitch;

  latitude = asinf(y0);
  longitude = atan2f(z0, x0);

  return true;
}


int8_t GlobeRender::setSourceFrame(const PixAddr A, const PixUInt W, const PixUInt H) {
  int8_t ret = -1;
  if (nullptr != _img) {
    const PixUInt T_WIDTH  = _img->width();
    const PixUInt T_HEIGHT = _img->height();
    const bool CONSTRAINED_W = (W + A.x) < (T_WIDTH);
    const bool CONSTRAINED_H = (H + A.y) < (T_HEIGHT);
    ret--;
    if (CONSTRAINED_W & CONSTRAINED_H) {
      _addr   = A;
      _width  = W;
      _height = H;
      _center = PixAddr((_width >> 1), (_height >> 1));
      _radius = ((_center.x < _center.y) ? _center.x : _center.y) - 1;
      ret = 0;
    }
  }
  _need_rerender = true;
  return ret;
}



void GlobeRender::setLatLonDivisions(const uint8_t LAT_DIVS, const uint8_t LON_DIVS) {
  _lat_lines = LAT_DIVS;
  _lon_lines = LON_DIVS;
  _need_rerender = true;
}


void GlobeRender::setColors(const uint32_t COLOR, const uint32_t BG_COLOR) {
  _sphere_color = COLOR;
  _background_color = BG_COLOR;
  _need_rerender = true;
}


void GlobeRender::setOrientation(const float PITCH, const float ROLL) {
  _pitch   = PITCH;
  _roll    = ROLL;
  _sin_pitch = sinf(_pitch);
  _cos_pitch = cosf(_pitch);
  _sin_roll  = sinf(_roll);
  _cos_roll  = cosf(_roll);
  _need_rerender = true;
}


void GlobeRender::setOrientation(const Quaternion QUATERNION) {
  Quaternion q = QUATERNION;
  q.normalize();

  // Convert normalized quaternion to Euler angles (pitch, roll)
  // pitch = asin(2*(w*y - z*x));
  _pitch = asinf(2.0f * (q.w * q.y - q.z * q.x));

  // roll = atan2(2*(w*x + y*z), 1 - 2*(x*x + y*y));
  _roll = atan2f(2.0f * (q.w * q.x + q.y * q.z),
                 1.0f - 2.0f * (q.x * q.x + q.y * q.y));

  _sin_pitch = sinf(_pitch);
  _cos_pitch = cosf(_pitch);
  _sin_roll  = sinf(_roll);
  _cos_roll  = cosf(_roll);
  _need_rerender = true;
}
