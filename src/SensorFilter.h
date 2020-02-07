/*
File:   SensorFilter.h
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


#ifndef __SENSOR_FILTER_H__
#define __SENSOR_FILTER_H__

#include <inttypes.h>
#include <stdint.h>
#include <Arduino.h>
#include <StringBuilder.h>
#include "Vector3.h"

#define FILTER_MAX_ELEMENTS   8000

class StringBuilder;


enum class FilteringStrategy : uint8_t {
  RAW        = 0,  // No filtering
  MOVING_AVG = 1,  // Moving average with a given window size.
  MOVING_MED = 2   // Moving median with a given window size.
};


/*
* Filters for linear sequences of scalar values.
*/
template <typename T> class SensorFilter {
  public:
    SensorFilter(FilteringStrategy, int param0, int param1);
    virtual ~SensorFilter();

    int8_t feedFilter(T);
    int8_t purge();
    int8_t init();
    T      value();
    T      minValue();
    T      maxValue();

    int8_t setParam0(int);
    int8_t setParam1(int);
    int    getParam0();
    int    getParam1();

    inline FilteringStrategy strategy() {   return _strat;    };
    int8_t setStrategy(FilteringStrategy);
    void printFilter(StringBuilder*);

    inline T*       memPtr() {        return samples;         };
    inline uint16_t lastIndex() {     return sample_idx;      };
    inline uint16_t windowSize() {    return window_size;     };
    inline bool     dirty() {         return filter_dirty;    };
    inline bool     initialized() {   return filter_initd;    };
    inline T        rmsValue() {      return rms;             };
    inline T        stdevValue() {    return stdev;           };


    static const char* getFilterStr(FilteringStrategy);


  private:
    T*       samples         = nullptr;
    uint16_t sample_idx      = 0;
    uint16_t window_size     = 0;
    T        last_value      = T(0);
    T        min_value       = T(0);
    T        max_value       = T(0);
    T        rms             = T(0);
    T        stdev           = T(0);
    FilteringStrategy _strat = FilteringStrategy::RAW;
    bool     window_full     = false;
    bool     filter_dirty    = false;
    bool     filter_initd    = false;

    int8_t  _reallocate_sample_window(uint16_t);
    int8_t  _zero_samples();

    T  _calculate_rms();
    T  _calculate_stdev();
    T  _calculate_median();
};



/*
* Filters for linear sequences of vector values.
* Pure virtual interface.
* TODO: Generalize into SensorFilter.
*/
class SensorFilter3 {
  public:
    virtual ~SensorFilter3() {};

    virtual int8_t feedFilter(double x, double y, double z) =0;
    virtual int8_t purge()        =0;
    virtual Vector3f64* value()   =0;
    virtual int8_t setParam0(int) =0;
    virtual int8_t setParam1(int) =0;
    virtual int    getParam0()    =0;
    virtual int    getParam1()    =0;
    virtual FilteringStrategy strategy() =0;
    virtual void printFilter(StringBuilder*, bool csv) =0;

    inline void printFilter(StringBuilder* out) {   printFilter(out, false);  };

    static SensorFilter3* FilterFactory(FilteringStrategy, int param0, int param1);
    static const char* getFilterStr(FilteringStrategy);
};


/*
* A null filter. Basically a data accumulator and pass-through.
*/
class NullFilter3: public SensorFilter3 {
  public:
    NullFilter3(int param0, int param1);
    virtual ~NullFilter3();

    int8_t feedFilter(double x, double y, double z);
    int8_t purge();
    Vector3f64* value();

    int8_t setParam0(int);
    int8_t setParam1(int);
    int    getParam0();
    int    getParam1();

    FilteringStrategy strategy();
    void printFilter(StringBuilder*, bool csv);

  private:
    Vector3f64 last_value;
};


/*
* A moving average filter.
*/
class MeanFilter3: public SensorFilter3 {
  public:
    MeanFilter3(int param0, int param1);
    ~MeanFilter3();

    int8_t feedFilter(double x, double y, double z);
    int8_t purge();
    Vector3f64* value();

    int8_t setParam0(int);
    int8_t setParam1(int);
    int    getParam0();
    int    getParam1();

    FilteringStrategy strategy();
    void printFilter(StringBuilder*, bool csv);

    inline Vector3f64* rmsValue() {    return &rms;      };
    inline Vector3f64* stdevValue() {  return &stdev;    };


  private:
    Vector3f64* samples         = nullptr;
    Vector3f64  running_average;
    Vector3f64  rms;
    Vector3f64  stdev;
    uint16_t    sample_idx      = 0;
    uint16_t    window_size     = 0;
    bool        window_full     = false;

    int8_t  _reallocate_sample_window(uint16_t);
    int8_t  _calculate_rms();
    int8_t  _calculate_stdev();
};


/*
* A moving median filter.
*/
class MedianFilter3: public SensorFilter3 {
  public:
    MedianFilter3(int param0, int param1);
    virtual ~MedianFilter3();

    int8_t feedFilter(double x, double y, double z);
    int8_t purge();
    Vector3f64* value();

    int8_t setParam0(int);
    int8_t setParam1(int);
    int    getParam0();
    int    getParam1();

    FilteringStrategy strategy();
    void printFilter(StringBuilder*, bool csv);


  private:
    Vector3f64* samples        = nullptr;
    Vector3f64  running_median;
    uint16_t    sample_idx     = 0;
    uint16_t    window_size    = 0;
    bool        window_full    = false;

    int8_t  _reallocate_sample_window(uint16_t);
    int8_t  _calculate_median();
};







/******************************************************************************
* Statics
******************************************************************************/
static const char* const FILTER_HEADER_STRING = "\t%s filter\n\t-----------------------------\n";


template <typename T> const char* SensorFilter<T>::getFilterStr(FilteringStrategy x) {
  switch (x) {
    case FilteringStrategy::RAW:         return "NULL";
    case FilteringStrategy::MOVING_AVG:  return "MOVING_AVG";
    case FilteringStrategy::MOVING_MED:  return "MOVING_MED";
  }
  return "UNKNOWN";
}


/*******************************************************************************
* Linear filter class
*******************************************************************************/

/*
* Base constructor
*/
template <typename T> SensorFilter<T>::SensorFilter(FilteringStrategy s, int param0, int param1) {
  _strat      = s;
  window_size = param0;
}


/*
* Base destructor. Free sample memory.
*/
template <typename T> SensorFilter<T>::~SensorFilter() {
  if (nullptr != samples) {
    free(samples);
    samples = nullptr;
  }
}


template <typename T> int8_t SensorFilter<T>::init() {
  uint16_t tmp_window_size = window_size;
  window_size = 0;
  int8_t ret = _reallocate_sample_window(tmp_window_size);
  filter_initd = (0 == ret);
  return ret;
}


/*
* Reallocates the sample memory, freeing the prior allocation if necessary.
* Returns 0 of success, -1 otherwise.
*/
template <typename T> int8_t SensorFilter<T>::_reallocate_sample_window(uint16_t win) {
  int8_t ret = -1;
  uint16_t normd_win = (win > FILTER_MAX_ELEMENTS) ? FILTER_MAX_ELEMENTS : win;
  if (normd_win != window_size) {
    window_size = normd_win;
    window_full = false;
    if (nullptr != samples) {
      free(samples);
      samples = nullptr;
    }
    if (window_size > 0) {
      samples = (T*) malloc(window_size * sizeof(T));
      ret = _zero_samples();
      sample_idx = 0;
    }
    else {
      // Program asked for no sample depth.
      ret = 0;
    }
  }
  else {
    ret = _zero_samples();
  }
  return ret;
}


/*
* Zero's the content of the sample memory, whatever that means to this type.
* Returns 0 of success, -1 otherwise.
*/
template <typename T> int8_t SensorFilter<T>::_zero_samples() {
  int8_t ret = -1;
  last_value = T(0);
  rms        = T(0);
  stdev      = T(0);
  if (nullptr != samples) {
    if (window_size > 0) {
      ret = 0;
      for (int i = 0; i < window_size; i++) {
        samples[i] = T(0);
      }
    }
  }
  return ret;
}


template <typename T> int8_t SensorFilter<T>::setStrategy(FilteringStrategy s) {
  int8_t ret = -1;
  if (_strat != s) {
    _strat = s;
    last_value = T(0);
    ret = 0;
  }
  return ret;
}


template <typename T> void SensorFilter<T>::printFilter(StringBuilder* output) {
  output->concatf(FILTER_HEADER_STRING, SensorFilter<T>::getFilterStr(strategy()));
  switch (_strat) {
    case FilteringStrategy::RAW:
      output->concatf("\tValue           = %.8f\n", last_value);
      output->concatf("\tSample window   = %u\n",   window_size);
      break;
    case FilteringStrategy::MOVING_AVG:
      output->concatf("\tRunning average = %.8f\n", last_value);
      output->concatf("\tRMS             = %.8f\n", rmsValue());
      output->concatf("\tSTDEV           = %.8f\n", stdevValue());
      break;
    case FilteringStrategy::MOVING_MED:
      output->concatf("\tRunning median  = %.8f\n", last_value);
      break;
  }
}


template <typename T> int8_t SensorFilter<T>::feedFilter(T val) {
  int8_t ret = 0;
  if (window_size > 1) {
    switch (_strat) {
      case FilteringStrategy::RAW:
        {
          last_value = val;
          samples[sample_idx++] = val;
          if (sample_idx >= window_size) {
            window_full = true;
            sample_idx = 0;
          }
          ret = 1;
        }
        break;
      case FilteringStrategy::MOVING_AVG:   // Calculate the moving average...
        {
          T temp_avg = (last_value * (window_size-1)) + val;
          last_value = temp_avg / window_size;
          samples[sample_idx++] = val;
          if (sample_idx >= window_size) {
            window_full = true;
            sample_idx = 0;
            rms   = _calculate_rms();    // These are expensive, and are calculated once per-window.
            stdev = _calculate_stdev();  // These are expensive, and are calculated once per-window.
          }
          ret = 1;
        }
        break;
      case FilteringStrategy::MOVING_MED:   // Calculate the moving median...
        {
          // Calculate the moving median...
          samples[sample_idx++] = val;
          if (sample_idx >= window_size) {
            window_full = true;
            sample_idx = 0;
          }
          if (window_full) {
            _calculate_median();
            ret = 1;
          }
        }
        break;
    }

  }
  else {   // This is a null filter with extra steps.
    last_value = val;
    ret = 1;
  }
  filter_dirty = true;
  return ret;
}


template <typename T> T SensorFilter<T>::value() {
  filter_dirty = false;
  return last_value;
};


/* Returns the smallest value in the current dataset. */
template <typename T> T SensorFilter<T>::minValue() {
  T ret = last_value;
  for (int i = 0; i < window_size; i++) {
    T ret = (samples[i] < ret) ? samples[i] : ret;
  }
  return ret;
};


/* Returns the largest value in the current dataset. */
template <typename T> T SensorFilter<T>::maxValue() {
  T ret = last_value;
  for (int i = 0; i < window_size; i++) {
    T ret = (samples[i] > ret) ? samples[i] : ret;
  }
  return ret;
};


template <typename T> int8_t SensorFilter<T>::purge() {
  return _zero_samples();
}


template <typename T> int8_t SensorFilter<T>::setParam0(int x) {
  if (x >= 0) {
    return _reallocate_sample_window((uint16_t) x);
  }
  return -1;
}


template <typename T> int SensorFilter<T>::getParam0() {  return window_size;  }


/* Parameter 1 is presently unused. */
template <typename T> int8_t SensorFilter<T>::setParam1(int x) {  return -1;  }
template <typename T> int SensorFilter<T>::getParam1() {          return 0;   }



/*
* Calulates the RMS over the entire sample window.
*/
template <typename T> T SensorFilter<T>::_calculate_rms() {
  T result;
  if ((window_size > 1) && (nullptr != samples)) {
    T squared_samples = T(0);
    for (int i = 0; i < window_size; i++) {
      T s_tmp = sq(samples[i]);
      squared_samples += s_tmp;
    }
    result = sqrt(squared_samples / window_size);
  }
  else {
    result = T(0);
  }
  return result;
}


/*
* Calulates the standard deviation of the samples.
*/
template <typename T> T SensorFilter<T>::_calculate_stdev() {
  T result;
  if ((window_size > 1) && (nullptr != samples)) {
    T deviation_sum = T(0);
    for (int i = 0; i < window_size; i++) {
      deviation_sum += sq(samples[i] - last_value);
    }
    result = sqrt(deviation_sum / window_size);
  }
  else {
    result = T(0);
  }
  return result;
}


/*
* Calulates the median value of the samples.
*/
template <typename T> T SensorFilter<T>::_calculate_median() {
  T sorted[window_size];
  for (uint16_t i = 0; i < window_size; i++) {
    sorted[i] = samples[i];
  }
  // Selection sort.
  uint16_t i  = 0;
  T swap;
  for (uint16_t c = 0; c < (window_size - 1); c++) {
    i = c;
    for (uint16_t d = c + 1; d < window_size; d++) {
      if (sorted[i] > sorted[d]) i = d;
    }
    if (i != c) {
      swap = sorted[c];
      sorted[c] = sorted[i];
      sorted[i] = swap;
    }
  }

  if (window_size & 0x01) {
    // If there are an off number of samples...
    last_value = sorted[(window_size-1) >> 1];
  }
  else {
    // ...otherwise, we take the mean to the two middle values.
    uint16_t lower = (window_size-1) >> 1;
    uint16_t upper = lower + 1;
    last_value = (sorted[upper] + sorted[lower]) / 2;
  }
  return 0;
}


#endif  // __SENSOR_FILTER_H__
