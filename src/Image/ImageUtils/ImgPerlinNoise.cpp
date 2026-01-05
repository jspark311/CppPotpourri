/*
File:   ImgPerlinNoise.cpp
Author: J. Ian Lindsay
Date:   2025.06.26

Perlin noise generation.

TODO: Needs better integration with Image (format, color, etc).
*/

#include <cmath>
#include "Image/ImageUtils.h"

ImgPerlinNoise::ImgPerlinNoise(
  Image* target,
  PixUInt x,
  PixUInt y,
  PixUInt width,
  PixUInt height,
  float scale,
  int octaves,
  float persistence
) : PerlinNoise(width, height, scale, octaves, persistence),
    _target(target), _t_x(x), _t_y(y), _t_w(width), _t_h(height)
{}



int8_t ImgPerlinNoise::apply() {
  int8_t ret = -1;
  if (nullptr != _target) {
    ret--;
    float MAXCHANNEL = 1.0f;   // Maximum channel value.
    switch (_target->format()) {   // Observe image's format.
      case ImgBufferFormat::MONOCHROME:
        MAXCHANNEL = static_cast<float>((1 << 1) - 1);
        break;
      case ImgBufferFormat::GREY_24:
        MAXCHANNEL = static_cast<float>((1 << 24) - 1);
        break;
      case ImgBufferFormat::GREY_16:
        MAXCHANNEL = static_cast<float>((1 << 16) - 1);
        break;
      case ImgBufferFormat::GREY_8:
        MAXCHANNEL = static_cast<float>((1 << 8) - 1);
        break;
      case ImgBufferFormat::GREY_4:
        MAXCHANNEL = static_cast<float>((1 << 4) - 1);
        break;
      case ImgBufferFormat::R8_G8_B8_ALPHA:
      case ImgBufferFormat::R8_G8_B8:
        MAXCHANNEL = static_cast<float>((1 << 8) - 1);
        break;
      case ImgBufferFormat::R5_G6_B5:
        MAXCHANNEL = static_cast<float>((1 << 5) - 1);
        break;
      case ImgBufferFormat::R3_G3_B2:
        MAXCHANNEL = static_cast<float>((1 << 2) - 1);
        break;
      default:
        return ret;
    }
    ret--;

    if (0 == PerlinNoise::apply()) {
      ret--;
      for (PixUInt j = 0; j < _t_h; ++j) {
        for (PixUInt i = 0; i < _t_w; ++i) {
          const float NORMALIZED = valueAtPoint(i, j);
          const uint32_t ORIG_COLOR = (BlendMode::REPLACE == _blend_mode) ? 0 : _target->getPixel(_t_x + i, _t_y + j);
          uint32_t grey  = static_cast<uint32_t>(std::round(NORMALIZED * MAXCHANNEL));
          uint32_t color = 0;

          switch (_target->format()) {   // Observe image's format.
            case ImgBufferFormat::MONOCHROME:
            case ImgBufferFormat::GREY_24:
            case ImgBufferFormat::GREY_16:
            case ImgBufferFormat::GREY_8:
            case ImgBufferFormat::GREY_4:
              color = grey;
              switch (_blend_mode) {
                case BlendMode::ADD_SAT:  color = strict_range_bind((float) (ORIG_COLOR + color), 0.0f, MAXCHANNEL);  break;
                case BlendMode::SUB_SAT:  color = strict_range_bind((float) (ORIG_COLOR - color), 0.0f, MAXCHANNEL);  break;
                case BlendMode::SCALE:    color = strict_range_bind((float) (NORMALIZED * color), 0.0f, MAXCHANNEL);  break;
                case BlendMode::REPLACE:
                default:  break;
              }
              break;
            case ImgBufferFormat::R8_G8_B8_ALPHA:
            case ImgBufferFormat::R8_G8_B8:
              color = ((grey << 16) | (grey << 8) | grey);
              switch (_blend_mode) {
                case BlendMode::ADD_SAT:  color = strict_range_bind((float) (ORIG_COLOR + color), 0.0f, MAXCHANNEL);  break;
                case BlendMode::SUB_SAT:  color = strict_range_bind((float) (ORIG_COLOR - color), 0.0f, MAXCHANNEL);  break;
                case BlendMode::SCALE:    color = strict_range_bind((float) (NORMALIZED * color), 0.0f, MAXCHANNEL);  break;
                case BlendMode::REPLACE:
                default:  break;
              }
              break;
            case ImgBufferFormat::R5_G6_B5:
              color = ((grey << 11) | (grey << 5) | grey);
              break;
            case ImgBufferFormat::R3_G3_B2:
              color = ((grey << 5) | (grey << 2) | grey);
              break;

            default:
              break;
          }

          _target->setPixel(_t_x + i, _t_y + j, color, _blend_mode);
        }
      }
      ret = 0;
    }
  }
  return ret;
}
