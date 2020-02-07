/*
File:   SensorFilter.cpp
Author: J. Ian Lindsay
Date:   2020.01.30

Copyright 2020 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/


#include "SensorFilter.h"
#include <StringBuilder.h>

inline uint16_t strict_max(uint16_t a, uint16_t b) { return (a > b) ? a:b; };
inline uint16_t strict_min(uint16_t a, uint16_t b) { return (a > b) ? b:a; };


/******************************************************************************
* Statics
******************************************************************************/
extern const char* const FILTER_HEADER_STRING;

SensorFilter3* SensorFilter3::FilterFactory(FilteringStrategy x, int param0, int param1) {
  switch (x) {
    case FilteringStrategy::RAW:         return new NullFilter3(param0, param1);
    case FilteringStrategy::MOVING_AVG:  return new MeanFilter3(param0, param1);
    case FilteringStrategy::MOVING_MED:  return new MedianFilter3(param0, param1);
  }
  return nullptr;
}


const char* SensorFilter3::getFilterStr(FilteringStrategy x) {
  switch (x) {
    case FilteringStrategy::RAW:         return "NULL";
    case FilteringStrategy::MOVING_AVG:  return "MOVING_AVG";
    case FilteringStrategy::MOVING_MED:  return "MOVING_MED";
  }
  return "UNKNOWN";
}



/******************************************************************************
* NullFilter3
******************************************************************************/

NullFilter3::NullFilter3(int param0, int param1) {
  // Nothing to do here.
}

NullFilter3::~NullFilter3() {
  // Nothing to do here.
}

int8_t NullFilter3::feedFilter(double x, double y, double z) {
  last_value(x, y, z);
  return 1;   // Raw mode always returns a value when given one.
}


int8_t NullFilter3::purge() {
  last_value(0.0, 0.0, 0.0);
  return 0;
}


Vector3f64* NullFilter3::value() {
  return &last_value;
}


/* The null filter has no parameters */
int8_t NullFilter3::setParam0(int x) {  return -1;  }
int8_t NullFilter3::setParam1(int x) {  return -1;  }
int NullFilter3::getParam0() {          return 0;   }
int NullFilter3::getParam1() {          return 0;   }

FilteringStrategy NullFilter3::strategy() {
  return FilteringStrategy::RAW;
};


void NullFilter3::printFilter(StringBuilder* output, bool csv) {
  if (csv) {
    output->concatf(",%.4f,%.4f,%.4f", last_value.x, last_value.y, last_value.z);
  }
  else {
    output->concatf(FILTER_HEADER_STRING, getFilterStr(strategy()));
    output->concatf("\tValue = (%.4f, %.4f, %.4f)\n", last_value.x, last_value.y, last_value.z);
  }
}


/******************************************************************************
* MeanFilter3
******************************************************************************/
MeanFilter3::MeanFilter3(int param0, int param1) {
  setParam0(param0);
}

MeanFilter3::~MeanFilter3() {
  if (nullptr != samples) {
    free(samples);
    samples = nullptr;
  }
}

int8_t MeanFilter3::feedFilter(double x, double y, double z) {
  int8_t ret = 0;
  if (window_size > 1) {
    // Calculate the moving average...
    Vector3f64 input_vect(x, y, z);
    Vector3f64 temp_avg = (running_average * (window_size-1)) + input_vect;
    running_average = temp_avg / window_size;
    samples[sample_idx++](x, y, z);
    if (sample_idx >= window_size) {
      window_full = true;
      sample_idx = 0;
      _calculate_rms();    // These are expensive, and are calculated once per-window.
      _calculate_stdev();  // These are expensive, and are calculated once per-window.
    }
    ret = 1;
  }
  else {   // This is a null filter with extra steps.
    running_average(x, y, z);
    ret = 1;
  }
  return ret;
}


int8_t MeanFilter3::purge() {
  window_full     = false;
  sample_idx      = 0;
  running_average(0.0, 0.0, 0.0);
  rms(0.0, 0.0, 0.0);
  stdev(0.0, 0.0, 0.0);
  return 0;
}


Vector3f64* MeanFilter3::value() {
  return &running_average;
}


int8_t MeanFilter3::setParam0(int x) {
  if (x >= 0) {
    return _reallocate_sample_window((uint16_t) x);
  }
  return -1;
}

int MeanFilter3::getParam0() {          return window_size;   }


/* The mean filter has no parameter 1 */
int8_t MeanFilter3::setParam1(int x) {  return -1;  }
int MeanFilter3::getParam1() {          return 0;   }


FilteringStrategy MeanFilter3::strategy() {
  return FilteringStrategy::MOVING_AVG;
};


void MeanFilter3::printFilter(StringBuilder* output, bool csv) {
  if (csv) {
    output->concatf(",%.4f,%.4f,%.4f", running_average.x, running_average.y, running_average.z);
    output->concatf(",%.4f,%.4f,%.4f", rms.x, rms.y, rms.z);
    output->concatf(",%.4f,%.4f,%.4f", stdev.x, stdev.y, stdev.z);
  }
  else {
    output->concatf(FILTER_HEADER_STRING, getFilterStr(strategy()));
    output->concatf("\tSample window   = %u\n", window_size);
    output->concatf("\tRunning average = (%.4f, %.4f, %.4f)\n", running_average.x, running_average.y, running_average.z);
    output->concatf("\tRMS             = (%.4f, %.4f, %.4f)\n", rms.x, rms.y, rms.z);
    output->concatf("\tSTDEV           = (%.4f, %.4f, %.4f)\n", stdev.x, stdev.y, stdev.z);
  }
}


int8_t MeanFilter3::_reallocate_sample_window(uint16_t win) {
  uint16_t normd_win = strict_min(win, FILTER_MAX_ELEMENTS);
  if (normd_win != window_size) {
    window_size = normd_win;
    window_full = false;
    if (nullptr != samples) {
      free(samples);
      samples = nullptr;
    }
    if (window_size > 0) {
      samples = (Vector3f64*) malloc(window_size * sizeof(Vector3f64));
      if (nullptr != samples) {
        for (int i = 0; i < window_size; i++) {
          samples[i](0.0, 0.0, 0.0);
        }
      }
      sample_idx = 0;
    }
    else {
      stdev(0.0, 0.0, 0.0);
      rms(0.0, 0.0, 0.0);
    }
  }
  return 0;
}


/*
* Calulates the RMS over the entire sample window.
*/
int8_t MeanFilter3::_calculate_rms() {
  int8_t ret = -1;
  if ((window_size > 1) && (nullptr != samples)) {
    double squared_samples[3] = {0.0, 0.0, 0.0};
    for (uint16_t i = 0; i < window_size; i++) {
      squared_samples[0] += sq(samples[i].x);
      squared_samples[1] += sq(samples[i].y);
      squared_samples[2] += sq(samples[i].z);
    }
    rms(
      sqrt(squared_samples[0] / window_size),
      sqrt(squared_samples[1] / window_size),
      sqrt(squared_samples[2] / window_size)
    );
  }
  else {
    rms(0.0, 0.0, 0.0);
  }
  return ret;
}


/*
* Calulates the standard deviation of the samples.
*/
int8_t MeanFilter3::_calculate_stdev() {
  int8_t ret = -1;
  if ((window_size > 1) && (nullptr != samples)) {
    double deviation_sum[3] = {0.0, 0.0, 0.0};
    for (uint16_t i = 0; i < window_size; i++) {
      deviation_sum[0] += sq(samples[i].x - running_average.x);
      deviation_sum[1] += sq(samples[i].y - running_average.y);
      deviation_sum[2] += sq(samples[i].z - running_average.z);
    }
    stdev(
      sqrt(deviation_sum[0] / window_size),
      sqrt(deviation_sum[1] / window_size),
      sqrt(deviation_sum[2] / window_size)
    );
  }
  else {
    stdev(0.0, 0.0, 0.0);
  }
  return ret;
}



/******************************************************************************
* MedianFilter3
******************************************************************************/
MedianFilter3::MedianFilter3(int param0, int param1) {
  setParam0(param0);
}

MedianFilter3::~MedianFilter3() {
  if (nullptr != samples) {
    free(samples);
    samples = nullptr;
  }
}

int8_t MedianFilter3::feedFilter(double x, double y, double z) {
  int8_t ret = 0;
  if (window_size > 1) {
    // Calculate the moving median...
    samples[sample_idx++](x, y, z);
    if (sample_idx >= window_size) {
      window_full = true;
      sample_idx = 0;
    }
    if (window_full) {
      _calculate_median();
      ret = 1;
    }
  }
  else {   // This is a null filter with extra steps.
    running_median(x, y, z);
    ret = 1;
  }
  return ret;
}


int8_t MedianFilter3::purge() {
  window_full = false;
  sample_idx  = 0;
  running_median(0.0, 0.0, 0.0);
  return 0;
}


Vector3f64* MedianFilter3::value() {
  return &running_median;
}


int8_t MedianFilter3::setParam0(int x) {
  if (x >= 0) {
    return _reallocate_sample_window((uint16_t) x);
  }
  return -1;
}

int MedianFilter3::getParam0() {          return window_size;   }


/* The mean filter has no parameter 1 */
int8_t MedianFilter3::setParam1(int x) {  return -1;  }
int MedianFilter3::getParam1() {          return 0;   }


FilteringStrategy MedianFilter3::strategy() {
  return FilteringStrategy::MOVING_MED;
};


void MedianFilter3::printFilter(StringBuilder* output, bool csv) {
  if (csv) {
    output->concatf(",%.4f,%.4f,%.4f", running_median.x, running_median.y, running_median.z);
  }
  else {
    output->concatf(FILTER_HEADER_STRING, getFilterStr(strategy()));
    output->concatf("\tSample window   = %u\n", window_size);
    output->concatf("\tRunning median  = (%.4f, %.4f, %.4f)\n", running_median.x, running_median.y, running_median.z);
  }
}


int8_t MedianFilter3::_reallocate_sample_window(uint16_t win) {
  uint16_t normd_win = strict_min(win, FILTER_MAX_ELEMENTS);
  if (normd_win != window_size) {
    window_size = normd_win;
    window_full = false;
    if (nullptr != samples) {
      free(samples);
      samples = nullptr;
    }
    if (window_size > 0) {
      samples = (Vector3f64*) malloc(window_size * sizeof(Vector3f64));
      if (nullptr != samples) {
        for (int i = 0; i < window_size; i++) {
          samples[i](0.0, 0.0, 0.0);
        }
      }
      sample_idx = 0;
    }
  }
  return 0;
}


int8_t MedianFilter3::_calculate_median() {
  double sorted[3][window_size];
  for (uint16_t i = 0; i < window_size; i++) {
    sorted[0][i] = samples[i].x;
    sorted[1][i] = samples[i].y;
    sorted[2][i] = samples[i].z;
  }
  // Selection sort.
  uint16_t i = 0;
  double swap;
  for (uint8_t n = 0; n < 3; n++) {
    for (uint16_t c = 0; c < (window_size - 1); c++) {
      i = c;
      for (uint16_t d = c + 1; d < window_size; d++) {
        if (sorted[n][i] > sorted[n][d]) i = d;
      }
      if (i != c) {
        swap = sorted[n][c];
        sorted[n][c] = sorted[n][i];
        sorted[n][i] = swap;
      }
    }
  }

  if (window_size & 0x01) {
    // If there are an off number of samples...
    uint16_t offset = (window_size-1) >> 1;
    running_median.set(sorted[0][offset], sorted[1][offset], sorted[2][offset]);
  }
  else {
    // ...otherwise, we take the mean to the two middle values.
    uint16_t lower = (window_size-1) >> 1;
    uint16_t upper = lower + 1;
    running_median.set(
      ((sorted[0][upper] + sorted[0][lower]) / 2),
      ((sorted[1][upper] + sorted[1][lower]) / 2),
      ((sorted[2][upper] + sorted[2][lower]) / 2)
    );
  }
  return 0;
}
