/*
File:   GPSWrapper.h
Author: J. Ian Lindsay
Date:   2016.06.29

This class was an adaption based on Kosma Moczek's minmea. WTFPL license.
https://github.com/cloudyourcar/minmea

This is a basic class for parsing NMEA sentences from a GPS and emitting
  them as annotated messages.

This class in unidirectional in the sense that it only reads from the
  associated transport. Hardware that has bidirectional capability for
  whatever reason can extend this class into something with a non-trivial
  fromCounterparty() method.
*/

/*
* Copyright © 2014 Kosma Moczek <kosma@cloudyourcar.com>
* This program is free software. It comes without any warranty, to the extent
* permitted by applicable law. You can redistribute it and/or modify it under
* the terms of the Do What The Fuck You Want To Public License, Version 2, as
* published by Sam Hocevar. See the COPYING file for more details.
*/


#ifndef __GPS_TOOLS_H__
#define __GPS_TOOLS_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <math.h>

#include "StringBuilder.h"
#include "CppPotpourri.h"


#define MINMEA_MAX_LENGTH          140

/* These are integer representations of the three-letter sentence IDs. */
#define MINMEA_INT_SENTENCE_CODE_RMC   0x00524d43
#define MINMEA_INT_SENTENCE_CODE_GGA   0x00474741
#define MINMEA_INT_SENTENCE_CODE_GSA   0x00475341
#define MINMEA_INT_SENTENCE_CODE_GLL   0x00474c4c
#define MINMEA_INT_SENTENCE_CODE_GST   0x00475354
#define MINMEA_INT_SENTENCE_CODE_GSV   0x00475356
#define MINMEA_INT_SENTENCE_CODE_VTG   0x00565447



/*******************************************************************************
* Undigested GPS types.
* TODO: Rework as classes with initializers and encapsulated logic.
*******************************************************************************/

enum minmea_sentence_id {
    MINMEA_INVALID = -1,
    MINMEA_UNKNOWN = 0,
    MINMEA_SENTENCE_RMC,
    MINMEA_SENTENCE_GGA,
    MINMEA_SENTENCE_GSA,
    MINMEA_SENTENCE_GLL,
    MINMEA_SENTENCE_GST,
    MINMEA_SENTENCE_GSV,
    MINMEA_SENTENCE_VTG,
};

enum minmea_gll_status {
    MINMEA_GLL_STATUS_DATA_VALID     = 'A',
    MINMEA_GLL_STATUS_DATA_NOT_VALID = 'V',
};

// FAA mode added to some fields in NMEA 2.3.
enum minmea_faa_mode {
    MINMEA_FAA_MODE_AUTONOMOUS   = 'A',
    MINMEA_FAA_MODE_DIFFERENTIAL = 'D',
    MINMEA_FAA_MODE_ESTIMATED    = 'E',
    MINMEA_FAA_MODE_MANUAL       = 'M',
    MINMEA_FAA_MODE_SIMULATED    = 'S',
    MINMEA_FAA_MODE_NOT_VALID    = 'N',
    MINMEA_FAA_MODE_PRECISE      = 'P',
};

enum minmea_gsa_mode {
    MINMEA_GPGSA_MODE_AUTO   = 'A',
    MINMEA_GPGSA_MODE_FORCED = 'M',
};

enum minmea_gsa_fix_type {
    MINMEA_GPGSA_FIX_NONE = 1,
    MINMEA_GPGSA_FIX_2D = 2,
    MINMEA_GPGSA_FIX_3D = 3,
};

struct minmea_float {
  int_least32_t value;
  int_least32_t scale;
};

struct minmea_date {
  int day;
  int month;
  int year;
};

struct minmea_time {
  int hours;
  int minutes;
  int seconds;
  int microseconds;
};

struct minmea_sentence_rmc {
  struct minmea_time time;
  bool valid;
  struct minmea_float latitude;
  struct minmea_float longitude;
  struct minmea_float speed;
  struct minmea_float course;
  struct minmea_date date;
  struct minmea_float variation;
};

struct minmea_sentence_gga {
  struct minmea_time time;
  struct minmea_float latitude;
  struct minmea_float longitude;
  int fix_quality;
  int satellites_tracked;
  struct minmea_float hdop;
  struct minmea_float altitude; char altitude_units;
  struct minmea_float height; char height_units;
  int dgps_age;
};

struct minmea_sentence_gll {
  struct minmea_float latitude;
  struct minmea_float longitude;
  struct minmea_time time;
  char status;
  char mode;
};

struct minmea_sentence_gst {
  struct minmea_time time;
  struct minmea_float rms_deviation;
  struct minmea_float semi_major_deviation;
  struct minmea_float semi_minor_deviation;
  struct minmea_float semi_major_orientation;
  struct minmea_float latitude_error_deviation;
  struct minmea_float longitude_error_deviation;
  struct minmea_float altitude_error_deviation;
};

struct minmea_sentence_gsa {
  char mode;
  int fix_type;
  int sats[12];
  struct minmea_float pdop;
  struct minmea_float hdop;
  struct minmea_float vdop;
};

struct minmea_sat_info {
  int nr;
  int elevation;
  int azimuth;
  int snr;
};

struct minmea_sentence_gsv {
  int total_msgs;
  int msg_nr;
  int total_sats;
  struct minmea_sat_info sats[4];
};

struct minmea_sentence_vtg {
  struct minmea_float true_track_degrees;
  struct minmea_float magnetic_track_degrees;
  struct minmea_float speed_knots;
  struct minmea_float speed_kph;
  enum minmea_faa_mode faa_mode;
};


/*
* A BufferPipe that is specialized for parsing NMEA.
* After enough successful parsing, this class will emit messages into the
*   Kernel's general message queue containing framed high-level GPS data.
*/
class GPSWrapper : public BufferAccepter {
  public:
    GPSWrapper() {};
    ~GPSWrapper() {};

    /* Implementation of BufferAccepter. */
    int8_t provideBuffer(StringBuilder* buf);

    int8_t init();
    void printDebug(StringBuilder*);


  protected:
    uint32_t       _sentences_parsed   = 0;
    uint32_t       _sentences_rejected = 0;
    double         _last_lat   = 0.0;
    double         _last_lon   = 0.0;
    float          _last_speed = 0.0;
    StringBuilder  _accumulator;

    void _class_init();

    /**
    * Tries to empty the accumulator, parsing sentences iteratively.
    */
    bool _attempt_parse();

    /**
    * Calculate raw sentence checksum. Does not check sentence integrity.
    */
    uint8_t _checksum(const char *sentence);

    /**
    * Check sentence validity and checksum. Returns true for valid sentences.
    */
    bool _check(const char *sentence, bool strict);

    /**
    * Determine talker identifier.
    */
    bool _talker_id(char talker[3], const char *sentence);

    /**
    * Determine sentence identifier.
    */
    enum minmea_sentence_id _sentence_id(const char *sentence, bool strict);

    /*
    * Parse a specific type of sentence. Return true on success.
    */
    bool _parse_rmc(struct minmea_sentence_rmc *frame, const char *sentence);
    bool _parse_gga(struct minmea_sentence_gga *frame, const char *sentence);
    bool _parse_gsa(struct minmea_sentence_gsa *frame, const char *sentence);
    bool _parse_gll(struct minmea_sentence_gll *frame, const char *sentence);
    bool _parse_gst(struct minmea_sentence_gst *frame, const char *sentence);
    bool _parse_gsv(struct minmea_sentence_gsv *frame, const char *sentence);
    bool _parse_vtg(struct minmea_sentence_vtg *frame, const char *sentence);

    /**
    * Scanf-like processor for NMEA sentences. Supports the following formats:
    * c - single character (char *)
    * d - direction, returned as 1/-1, default 0 (int *)
    * f - fractional, returned as value + scale (int *, int *)
    * i - decimal, default zero (int *)
    * s - string (char *)
    * t - talker identifier and type (char *)
    * T - date/time stamp (int *, int *, int *)
    * Returns true on success. See library source code for details.
    */
    bool _scan(const char *sentence, const char *format, ...);

    const char* _get_string_by_sentence_id(enum minmea_sentence_id);

    /**
    * Convert GPS UTC date/time representation to a UNIX timestamp.
    */
    int _gettime(struct timespec *ts, const struct minmea_date *date, const struct minmea_time *time_);
};

#endif   // __GPS_TOOLS_H__
