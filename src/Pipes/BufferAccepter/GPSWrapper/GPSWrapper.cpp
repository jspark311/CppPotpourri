/*
File:   GPSWrapper.cpp
Author: J. Ian Lindsay
Date:   2016.06.29

This class was an adaption based on Kosma Moczek's minmea. WTFPL license.
https://github.com/cloudyourcar/minmea

TODO: This intake and parsing class might make a great pair with this:
https://geographiclib.sourceforge.io/1.40/C/

*/

/*
* Copyright © 2014 Kosma Moczek <kosma@cloudyourcar.com>
* This program is free software. It comes without any warranty, to the extent
* permitted by applicable law. You can redistribute it and/or modify it under
* the terms of the Do What The Fuck You Want To Public License, Version 2, as
* published by Sam Hocevar. See the COPYING file for more details.
*/

#include "GPSWrapper.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef __cplusplus
}
#endif

/*******************************************************************************
* LocationFrame functions
*******************************************************************************/

void LocationFrame::printDebug(StringBuilder* output) {
  output->concatf("\tTimestamp: %u\n\tSat count: %u\n", timestamp, sat_count);
  output->concatf("\t(%.6f, %.6f) heading %.2f at %u m/s\n", (double) lat, (double) lon, (double) mag_bearing, (speed / 1000));
  output->concatf("\tDOP horiz: %.3f\n\tDOP vert:  %.3f\n", (double) dop_horiz, (double) dop_vert);
}

void LocationFrame::copyFrame(const LocationFrame* loc) {
  lat         = loc->lat;
  lon         = loc->lon;
  dop_horiz   = loc->dop_horiz;
  dop_vert    = loc->dop_vert;
  timestamp   = loc->timestamp;
  mag_bearing = loc->mag_bearing;
  speed       = loc->speed;
  altitude    = loc->altitude;
  sat_count   = loc->sat_count;
}


/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/

/**
* Implements the Haversine formula to get distance between two points on Earth.
* https://en.wikipedia.org/wiki/Haversine_formula
*
* @return Distance in meters.
*/
int32_t GPSWrapper::haversineDistance(LocationFrame* p0, LocationFrame* p1) {
  int32_t ret = 0;
  double theta0 = p0->lat * (double) (PI/180.0);
  double theta1 = p1->lat * (double) (PI/180.0);
  double delta_theta  = (p1->lat - p0->lat) * (double) (PI/180.0);
  double delta_lambda = (p1->lon - p0->lon) * (double) (PI/180.0);
  double half_delta_theta  = sin(delta_theta / 2);
  double half_delta_lambda = sin(delta_lambda / 2);
  double haversine_angle   = (half_delta_theta * half_delta_theta) + cos(theta0) * cos(theta1) * (half_delta_lambda * half_delta_lambda);
  double haversine_arctan  = atan2(sqrt(haversine_angle), sqrt(1 - haversine_angle));
  ret = (int32_t) (MEAN_RADIUS_OF_EARTH * haversine_arctan);
  return ret;
}

// TODO: Move to StringBuilder
static int hex2int(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}


static inline bool minmea_isfield(char c) {
  return isprint((unsigned char) c) && c != ',' && c != '*';
}


/**
* Rescale a fixed-point value to a different scale. Rounds towards zero.
*/
static inline int_least32_t minmea_rescale(struct minmea_float *f, int_least32_t new_scale) {
  if (f->scale == 0) {         return 0;   }
  if (f->scale == new_scale) { return f->value;   }
  if (f->scale > new_scale) {  return (f->value + ((f->value > 0) - (f->value < 0)) * f->scale/new_scale/2) / (f->scale/new_scale);    }
  else {                       return f->value * (new_scale/f->scale);   }
}


/**
* Convert a fixed-point value to a floating-point value.
* Returns NaN for "unknown" values.
*/
static inline float minmea_tofloat(struct minmea_float *f) {
  if (f->scale == 0) {  return NAN;   }
  return (float) f->value / (float) f->scale;
}


/**
* Convert a raw coordinate to a floating point DD.DDD... value.
* Returns NaN for "unknown" values.
*/
static inline float minmea_tocoord(struct minmea_float *f) {
  if (f->scale == 0) {     return NAN;     }
  int_least32_t degrees = f->value / (f->scale * 100);
  int_least32_t minutes = f->value % (f->scale * 100);
  return (float) degrees + (float) minutes / (60 * f->scale);
}



/*******************************************************************************
* GPS-specific functions                                                       *
*******************************************************************************/
#define GPSWRAPPER_MAX_ACCUMULATOR_LEN   1024

/**
* Takes a buffer from outside of this class. Typically a comm port.
* Always takes ownership of the buffer to avoid needless copy and heap-thrash.
*
* @param  buf    A pointer to the buffer.
* @return -1 to reject buffer, 0 to accept without claiming, 1 to accept with claim.
*/
int8_t GPSWrapper::pushBuffer(StringBuilder* buf) {
  _accumulator.concatHandoff(buf);
  const int32_t ACC_LEN = _accumulator.length();
  if (MINMEA_MAX_LENGTH < ACC_LEN) {
    _attempt_parse();
  }

  if (ACC_LEN > GPSWRAPPER_MAX_ACCUMULATOR_LEN) {
    _accumulator.cull(ACC_LEN - GPSWRAPPER_MAX_ACCUMULATOR_LEN);
  }
  return 1;
}


// NOTE: This object effectively has no buffer limit, since it is a sink. All
//   buffers given will end here. So just report a reasonable maximum size for
//   a sentence.
int32_t GPSWrapper::bufferAvailable() {
  int32_t ret = 0;
  const int32_t ACC_LEN = _accumulator.length();
  if (ACC_LEN <= GPSWRAPPER_MAX_ACCUMULATOR_LEN) {
    ret = (GPSWRAPPER_MAX_ACCUMULATOR_LEN - _accumulator.length());
  }
  return ret;
}


int8_t GPSWrapper::init() {
  _accumulator.clear();
  return 0;
}


const char* GPSWrapper::_get_string_by_sentence_id(enum minmea_sentence_id id) {
  switch (id) {
    case MINMEA_UNKNOWN:      return "UNKNOWN";
    case MINMEA_SENTENCE_RMC: return "RMC";
    case MINMEA_SENTENCE_GGA: return "GGA";
    case MINMEA_SENTENCE_GSA: return "GSA";
    case MINMEA_SENTENCE_GLL: return "GLL";
    case MINMEA_SENTENCE_GST: return "GST";
    case MINMEA_SENTENCE_GSV: return "GSV";
    case MINMEA_SENTENCE_VTG: return "VTG";
    default:
      break;
  }
  return "xxx";
}


enum minmea_sentence_id GPSWrapper::_sentence_id(const char *sentence, bool strict) {
  if (_check(sentence, strict)) {
    char type[6];
    if (_scan(sentence, "t", type)) {
      uint32_t int_sent_code = (type[2] << 16) + (type[3] << 8) + (type[4]);
      switch (int_sent_code) {
        case MINMEA_INT_SENTENCE_CODE_RMC:  return MINMEA_SENTENCE_RMC;
        case MINMEA_INT_SENTENCE_CODE_GGA:  return MINMEA_SENTENCE_GGA;
        case MINMEA_INT_SENTENCE_CODE_GSA:  return MINMEA_SENTENCE_GSA;
        case MINMEA_INT_SENTENCE_CODE_GLL:  return MINMEA_SENTENCE_GLL;
        case MINMEA_INT_SENTENCE_CODE_GST:  return MINMEA_SENTENCE_GST;
        case MINMEA_INT_SENTENCE_CODE_GSV:  return MINMEA_SENTENCE_GSV;
        case MINMEA_INT_SENTENCE_CODE_VTG:  return MINMEA_SENTENCE_VTG;
        default:                            return MINMEA_UNKNOWN;
      }
    }
  }
  return MINMEA_INVALID;
}


bool GPSWrapper::_attempt_parse() {
  if (_accumulator.split("\n") == 0) return false;
  char* line = nullptr;
  bool local_success = false;
  bool emit_callback = false;

  // Always assume that the last token in the accumulator is an incomplete line.
  while (_accumulator.count() > 1) {
    line = _accumulator.position(0);
    enum minmea_sentence_id id = _sentence_id(line, false);
    switch (id) {
      case MINMEA_SENTENCE_GSA:
        {
          struct minmea_sentence_gsa frame;
          if (_parse_gsa(&frame, line)) {
            _loc_frame.dop_horiz = (double) minmea_tofloat(&frame.hdop);
            _loc_frame.dop_vert  = (double) minmea_tofloat(&frame.vdop);
            local_success = true;
          }
        }
        break;
      case MINMEA_SENTENCE_GLL:
        {
          struct minmea_sentence_gll frame;
          if (_parse_gll(&frame, line)) {
            local_success = true;
          }
        }
        break;
      case MINMEA_SENTENCE_RMC:
        {
          struct minmea_sentence_rmc frame;
          if (_parse_rmc(&frame, line)) {
            struct timespec ts;
            _loc_frame.lat = minmea_tocoord(&frame.latitude);
            _loc_frame.lon = minmea_tocoord(&frame.longitude);
            if (0 == _gettime(&ts, &frame.date, &frame.time)) {
              _loc_frame.timestamp = ts.tv_sec;
            }
            // _log.concatf("$xxRMC: raw coordinates and speed: (%d/%d,%d/%d) %d/%d\n",
            //         frame.latitude.value, frame.latitude.scale,
            //         frame.longitude.value, frame.longitude.scale,
            //         frame.speed.value, frame.speed.scale);
            // _log.concatf("$xxRMC fixed-point coordinates and speed scaled to three decimal places: (%d,%d) %d\n",
            //         minmea_rescale(&frame.latitude, 1000),
            //         minmea_rescale(&frame.longitude, 1000),
            //         minmea_rescale(&frame.speed, 1000));
            // _log.concatf("$xxRMC floating point degree coordinates and speed: (%f,%f) %f\n",
            //         (double) minmea_tocoord(&frame.latitude),
            //         (double) minmea_tocoord(&frame.longitude),
            //         (double) minmea_tofloat(&frame.speed));
            local_success = true;
          }
        }
        break;
      case MINMEA_SENTENCE_GGA:
        {
          struct minmea_sentence_gga frame;
          if (_parse_gga(&frame, line)) {
            _loc_frame.sat_count = frame.satellites_tracked;
            switch (frame.altitude_units) {
              case 'M': _loc_frame.altitude = (int) (minmea_tofloat(&frame.altitude) * 1000.0);   break;
              default:  break;
            }
            //_log.concatf("$xxGGA: fix quality: %d\n", frame.fix_quality);
            local_success = true;
          }
        }
        break;
      case MINMEA_SENTENCE_GST:
        {
          struct minmea_sentence_gst frame;
          if (_parse_gst(&frame, line)) {
            // _log.concatf("$xxGST: raw latitude,longitude and altitude error deviation: (%d/%d,%d/%d,%d/%d)\n",
            //         frame.latitude_error_deviation.value, frame.latitude_error_deviation.scale,
            //         frame.longitude_error_deviation.value, frame.longitude_error_deviation.scale,
            //         frame.altitude_error_deviation.value, frame.altitude_error_deviation.scale);
            // _log.concatf("$xxGST fixed point latitude,longitude and altitude error deviation"
            //        " scaled to one decimal place: (%d,%d,%d)\n",
            //         minmea_rescale(&frame.latitude_error_deviation, 10),
            //         minmea_rescale(&frame.longitude_error_deviation, 10),
            //         minmea_rescale(&frame.altitude_error_deviation, 10));
            // _log.concatf("$xxGST floating point degree latitude, longitude and altitude error deviation: (%f,%f,%f)",
            //         (double) minmea_tofloat(&frame.latitude_error_deviation),
            //         (double) minmea_tofloat(&frame.longitude_error_deviation),
            //         (double) minmea_tofloat(&frame.altitude_error_deviation));
            local_success = true;
          }
        }
        break;
      case MINMEA_SENTENCE_GSV:
        {
          struct minmea_sentence_gsv frame;
          if (_parse_gsv(&frame, line)) {
            // _log.concatf("$xxGSV: message %d of %d\n", frame.msg_nr, frame.total_msgs);
            // for (int i = 0; i < 4; i++)
            //     _log.concatf("$xxGSV: sat nr %d, elevation: %d, azimuth: %d, snr: %d dbm\n",
            //         frame.sats[i].nr,
            //         frame.sats[i].elevation,
            //         frame.sats[i].azimuth,
            //         frame.sats[i].snr);
            local_success = true;
          }
        }
        break;
      case MINMEA_SENTENCE_VTG:
        {
          struct minmea_sentence_vtg frame;
          if (_parse_vtg(&frame, line)) {
            _loc_frame.mag_bearing = minmea_tofloat(&frame.magnetic_track_degrees);
            _loc_frame.speed       = (minmea_tofloat(&frame.speed_kph) * 1000000) / 3600;  // Convert to mm/sec
            local_success = true;
            emit_callback = true;
          }
        }
        break;
      case MINMEA_INVALID:
      default:
        break;
    }
    if (local_success) {
      _sentences_parsed++;
      if (emit_callback & (nullptr != _callback)) {
        _callback(&_loc_frame);
      }
    }
    else {
      _sentences_rejected++;
      //_log.concatf("$xx%s sentence is not parsed: \n%s\n", _get_string_by_sentence_id(id), (const char*) line);
    }
    _accumulator.drop_position(0);
  }
  return true;
}


void GPSWrapper::printDebug(StringBuilder* output) {
  StringBuilder::styleHeader1(output, "GPSWrapper");
  output->concatf("\tParsed %u\n\tReject %u\n", _sentences_parsed, _sentences_rejected);
  output->concatf("\tAccumulator: %d bytes\n\n", _accumulator.length());
  _loc_frame.printDebug(output);
}


uint8_t GPSWrapper::_checksum(const char *sentence) {
  // Support senteces with or without the starting dollar sign.
  if (*sentence == '$') sentence++;
  uint8_t checksum = 0x00;
  // The optional checksum is an XOR of all bytes between "$" and "*".
  while (*sentence && *sentence != '*') checksum ^= *sentence++;
  return checksum;
}


bool GPSWrapper::_check(const char *sentence, bool strict) {
  // Sequence length is limited.
  if (strlen(sentence) <= MINMEA_MAX_LENGTH + 3) {
    // A valid sentence starts with "$".
    if (*sentence++ == '$') {
      // The optional checksum is an XOR of all bytes between "$" and "*".
      uint8_t checksum = 0x00;
      while (*sentence && *sentence != '*' && isprint((unsigned char) *sentence)) {
        checksum ^= *sentence++;
      }
      if (*sentence == '*') {
        // If checksum is present, extract and compare it.
        sentence++;
        int upper = hex2int(*sentence++);
        if (upper != -1) {
          int lower = hex2int(*sentence++);
          if (lower != -1) {
            int expected = upper << 4 | lower;
            // Check for checksum mismatch.
            return (checksum == expected);
          }
        }
      }
      else if (*sentence && (!strict) && ((*sentence == '\n') || (*sentence == '\r'))) {
        // The only stuff allowed at this point is a newline, and then only if
        //   strict mode is false.
        return true;
      }
    }
  }
  return false;
}




/*******************************************************************************
* Undigested GPS functions                                                     *
*******************************************************************************/

bool GPSWrapper::_scan(const char *sentence, const char *format, ...) {
  bool result = false;
  bool optional = false;
  va_list ap;
  va_start(ap, format);

  const char *field = sentence;
#define next_field() \
    do { \
        /* Progress to the next field. */ \
        while (minmea_isfield(*sentence)) \
            sentence++; \
        /* Make sure there is a field there. */ \
        if (*sentence == ',') { \
            sentence++; \
            field = sentence; \
        } else { \
            field = nullptr; \
        } \
    } while (0)

    while (*format) {
        char type = *format++;

        if (type == ';') {
            // All further fields are optional.
            optional = true;
            continue;
        }

        if (!field && !optional) {
            // Field requested but we ran out if input. Bail out.
            goto parse_error;
        }

        switch (type) {
            case 'c': { // Single character field (char).
                char value = '\0';

                if (field && minmea_isfield(*field))
                    value = *field;

                *va_arg(ap, char *) = value;
            } break;

            case 'd': { // Single character direction field (int).
                int value = 0;

                if (field && minmea_isfield(*field)) {
                    switch (*field) {
                        case 'N':
                        case 'E':
                            value = 1;
                            break;
                        case 'S':
                        case 'W':
                            value = -1;
                            break;
                        default:
                            goto parse_error;
                    }
                }

                *va_arg(ap, int *) = value;
            } break;

            case 'f': { // Fractional value with scale (struct minmea_float).
                int sign = 0;
                int_least32_t value = -1;
                int_least32_t scale = 0;

                if (field) {
                    while (minmea_isfield(*field)) {
                        if (*field == '+' && !sign && value == -1) {
                            sign = 1;
                        } else if (*field == '-' && !sign && value == -1) {
                            sign = -1;
                        } else if (isdigit((unsigned char) *field)) {
                            int digit = *field - '0';
                            if (value == -1)
                                value = 0;
                            if (value > (INT_LEAST32_MAX-digit) / 10) {
                                /* we ran out of bits, what do we do? */
                                if (scale) {
                                    /* truncate extra precision */
                                    break;
                                } else {
                                    /* integer overflow. bail out. */
                                    goto parse_error;
                                }
                            }
                            value = (10 * value) + digit;
                            if (scale)
                                scale *= 10;
                        } else if (*field == '.' && scale == 0) {
                            scale = 1;
                        } else if (*field == ' ') {
                            /* Allow spaces at the start of the field. Not NMEA
                             * conformant, but some modules do this. */
                            if (sign != 0 || value != -1 || scale != 0)
                                goto parse_error;
                        } else {
                            goto parse_error;
                        }
                        field++;
                    }
                }

                if ((sign || scale) && value == -1)
                    goto parse_error;

                if (value == -1) {
                    /* No digits were scanned. */
                    value = 0;
                    scale = 0;
                } else if (scale == 0) {
                    /* No decimal point. */
                    scale = 1;
                }
                if (sign)
                    value *= sign;

                *va_arg(ap, struct minmea_float *) = (struct minmea_float) {value, scale};
            } break;

            case 'i': { // Integer value, default 0 (int).
                int value = 0;

                if (field) {
                    char *endptr;
                    value = strtol(field, &endptr, 10);
                    if (minmea_isfield(*endptr))
                        goto parse_error;
                }

                *va_arg(ap, int *) = value;
            } break;

            case 's': { // String value (char *).
                char *buf = va_arg(ap, char *);

                if (field) {
                    while (minmea_isfield(*field))
                        *buf++ = *field++;
                }

                *buf = '\0';
            } break;

            case 't': { // NMEA talker+sentence identifier (char *).
                // This field is always mandatory.
                if (!field)
                    goto parse_error;

                if (field[0] != '$')
                    goto parse_error;
                for (int f=0; f<5; f++)
                    if (!minmea_isfield(field[1+f]))
                        goto parse_error;

                char *buf = va_arg(ap, char *);
                memcpy(buf, field+1, 5);
                buf[5] = '\0';
            } break;

            case 'D': { // Date (int, int, int), -1 if empty.
                struct minmea_date *date = va_arg(ap, struct minmea_date *);

                int d = -1, m = -1, y = -1;

                if (field && minmea_isfield(*field)) {
                    // Always six digits.
                    for (int f=0; f<6; f++)
                        if (!isdigit((unsigned char) field[f]))
                            goto parse_error;

                    char dArr[] = {field[0], field[1], '\0'};
                    char mArr[] = {field[2], field[3], '\0'};
                    char yArr[] = {field[4], field[5], '\0'};
                    d = strtol(dArr, nullptr, 10);
                    m = strtol(mArr, nullptr, 10);
                    y = strtol(yArr, nullptr, 10);
                }

                date->day = d;
                date->month = m;
                date->year = y;
            } break;

            case 'T': { // Time (int, int, int, int), -1 if empty.
                struct minmea_time *time_ = va_arg(ap, struct minmea_time *);

                int h = -1, i = -1, s = -1, u = -1;

                if (field && minmea_isfield(*field)) {
                    // Minimum required: integer time.
                    for (int f=0; f<6; f++)
                        if (!isdigit((unsigned char) field[f]))
                            goto parse_error;

                    char hArr[] = {field[0], field[1], '\0'};
                    char iArr[] = {field[2], field[3], '\0'};
                    char sArr[] = {field[4], field[5], '\0'};
                    h = strtol(hArr, nullptr, 10);
                    i = strtol(iArr, nullptr, 10);
                    s = strtol(sArr, nullptr, 10);
                    field += 6;

                    // Extra: fractional time. Saved as microseconds.
                    if (*field++ == '.') {
                        int value = 0;
                        int scale = 1000000;
                        while (isdigit((unsigned char) *field) && scale > 1) {
                            value = (value * 10) + (*field++ - '0');
                            scale /= 10;
                        }
                        u = value * scale;
                    } else {
                        u = 0;
                    }
                }

                time_->hours = h;
                time_->minutes = i;
                time_->seconds = s;
                time_->microseconds = u;
            } break;

            case '_': { // Ignore the field.
            } break;

            default: { // Unknown.
                goto parse_error;
            } break;
        }

        next_field();
    }

    result = true;

parse_error:
    va_end(ap);
    return result;
}


bool GPSWrapper::_talker_id(char talker[3], const char *sentence) {
  char type[6];
  if (!_scan(sentence, "t", type)) {
    return false;
  }
  talker[0] = type[0];
  talker[1] = type[1];
  talker[2] = '\0';
  return true;
}



bool GPSWrapper::_parse_rmc(struct minmea_sentence_rmc *frame, const char *sentence) {
  // $GPRMC,081836,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E*62
  char type[6];
  char validity;
  int latitude_direction;
  int longitude_direction;
  int variation_direction;
  if (!_scan(sentence, "tTcfdfdffDfd",
      type,
      &frame->time,
      &validity,
      &frame->latitude, &latitude_direction,
      &frame->longitude, &longitude_direction,
      &frame->speed,
      &frame->course,
      &frame->date,
      &frame->variation, &variation_direction)) {
    return false;
  }
  if (strcmp(type+2, "RMC")) {
    return false;
  }
  frame->valid = (validity == 'A');
  frame->latitude.value *= latitude_direction;
  frame->longitude.value *= longitude_direction;
  frame->variation.value *= variation_direction;
  return true;
}

bool GPSWrapper::_parse_gga(struct minmea_sentence_gga *frame, const char *sentence) {
  // $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
  char type[6];
  int latitude_direction;
  int longitude_direction;

  if (!_scan(sentence, "tTfdfdiiffcfci_",
      type,
      &frame->time,
      &frame->latitude, &latitude_direction,
      &frame->longitude, &longitude_direction,
      &frame->fix_quality,
      &frame->satellites_tracked,
      &frame->hdop,
      &frame->altitude, &frame->altitude_units,
      &frame->height, &frame->height_units,
      &frame->dgps_age)) {
    return false;
  }
  if (strcmp(type+2, "GGA")) {
    return false;
  }

  frame->latitude.value *= latitude_direction;
  frame->longitude.value *= longitude_direction;
  return true;
}

bool GPSWrapper::_parse_gsa(struct minmea_sentence_gsa *frame, const char *sentence) {
    // $GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39
    char type[6];

    if (!_scan(sentence, "tciiiiiiiiiiiiifff",
            type,
            &frame->mode,
            &frame->fix_type,
            &frame->sats[0],
            &frame->sats[1],
            &frame->sats[2],
            &frame->sats[3],
            &frame->sats[4],
            &frame->sats[5],
            &frame->sats[6],
            &frame->sats[7],
            &frame->sats[8],
            &frame->sats[9],
            &frame->sats[10],
            &frame->sats[11],
            &frame->pdop,
            &frame->hdop,
            &frame->vdop))
        return false;
    if (strcmp(type+2, "GSA"))
        return false;

    return true;
}

bool GPSWrapper::_parse_gll(struct minmea_sentence_gll *frame, const char *sentence) {
    // $GPGLL,3723.2475,N,12158.3416,W,161229.487,A,A*41$;
    char type[6];
    int latitude_direction;
    int longitude_direction;

    if (!_scan(sentence, "tfdfdTc;c",
            type,
            &frame->latitude, &latitude_direction,
            &frame->longitude, &longitude_direction,
            &frame->time,
            &frame->status,
            &frame->mode))
        return false;
    if (strcmp(type+2, "GLL"))
        return false;

    frame->latitude.value *= latitude_direction;
    frame->longitude.value *= longitude_direction;

    return true;
}

bool GPSWrapper::_parse_gst(struct minmea_sentence_gst *frame, const char *sentence) {
    // $GPGST,024603.00,3.2,6.6,4.7,47.3,5.8,5.6,22.0*58
    char type[6];

    if (!_scan(sentence, "tTfffffff",
            type,
            &frame->time,
            &frame->rms_deviation,
            &frame->semi_major_deviation,
            &frame->semi_minor_deviation,
            &frame->semi_major_orientation,
            &frame->latitude_error_deviation,
            &frame->longitude_error_deviation,
            &frame->altitude_error_deviation))
        return false;
    if (strcmp(type+2, "GST"))
        return false;

    return true;
}

bool GPSWrapper::_parse_gsv(struct minmea_sentence_gsv *frame, const char *sentence) {
    // $GPGSV,3,1,11,03,03,111,00,04,15,270,00,06,01,010,00,13,06,292,00*74
    // $GPGSV,3,3,11,22,42,067,42,24,14,311,43,27,05,244,00,,,,*4D
    // $GPGSV,4,2,11,08,51,203,30,09,45,215,28*75
    // $GPGSV,4,4,13,39,31,170,27*40
    // $GPGSV,4,4,13*7B
    char type[6];

    if (!_scan(sentence, "tiii;iiiiiiiiiiiiiiii",
            type,
            &frame->total_msgs,
            &frame->msg_nr,
            &frame->total_sats,
            &frame->sats[0].nr,
            &frame->sats[0].elevation,
            &frame->sats[0].azimuth,
            &frame->sats[0].snr,
            &frame->sats[1].nr,
            &frame->sats[1].elevation,
            &frame->sats[1].azimuth,
            &frame->sats[1].snr,
            &frame->sats[2].nr,
            &frame->sats[2].elevation,
            &frame->sats[2].azimuth,
            &frame->sats[2].snr,
            &frame->sats[3].nr,
            &frame->sats[3].elevation,
            &frame->sats[3].azimuth,
            &frame->sats[3].snr
            )) {
        return false;
    }
    if (strcmp(type+2, "GSV"))
        return false;

    return true;
}

bool GPSWrapper::_parse_vtg(struct minmea_sentence_vtg *frame, const char *sentence) {
    // $GPVTG,054.7,T,034.4,M,005.5,N,010.2,K*48
    // $GPVTG,156.1,T,140.9,M,0.0,N,0.0,K*41
    // $GPVTG,096.5,T,083.5,M,0.0,N,0.0,K,D*22
    // $GPVTG,188.36,T,,M,0.820,N,1.519,K,A*3F
    char type[6];
    char c_true, c_magnetic, c_knots, c_kph, c_faa_mode;

    if (!_scan(sentence, "tfcfcfcfc;c",
            type,
            &frame->true_track_degrees,
            &c_true,
            &frame->magnetic_track_degrees,
            &c_magnetic,
            &frame->speed_knots,
            &c_knots,
            &frame->speed_kph,
            &c_kph,
            &c_faa_mode))
        return false;
    if (strcmp(type+2, "VTG"))
        return false;
    // check chars
    if (c_true != 'T' ||
        c_magnetic != 'M' ||
        c_knots != 'N' ||
        c_kph != 'K')
        return false;
    frame->faa_mode = (minmea_faa_mode) c_faa_mode;

    return true;
}


int GPSWrapper::_gettime(struct timespec *ts, const struct minmea_date* dval, const struct minmea_time* tval) {
  if (dval->year != -1 && tval->hours != -1) {
    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    tm.tm_year = 2000 + dval->year - 1900;
    tm.tm_mon  = dval->month - 1;
    tm.tm_mday = dval->day;
    tm.tm_hour = tval->hours;
    tm.tm_min  = tval->minutes;
    tm.tm_sec  = tval->seconds;

    //time_t timestamp = timegm(&tm); /* See README.md if your system lacks timegm(). */
    time_t timestamp = mktime(&tm); /* See README.md if your system lacks timegm(). */
    if (timestamp != -1) {
      ts->tv_sec = timestamp;
      ts->tv_nsec = tval->microseconds * 1000;
      return 0;
    }
  }
  return -1;
}
