/*
File:   TimeSeries.h
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


This header file contains the libraries means of handling time-series data.
*/


#ifndef __C3P_TIMESERIES_H__
#define __C3P_TIMESERIES_H__

#include <inttypes.h>
#include <stdint.h>
#include "../Meta/Rationalizer.h"
#include "../Vector3.h"
#include "../StringBuilder.h"
#include "../EnumeratedTypeCodes.h"
#include "../FlagContainer.h"
#include "../C3PValue/KeyValuePair.h"

#define TIMESERIES_FLAG_FILTER_INITD       0x00000001  // Timeseries is initialized and ready.
#define TIMESERIES_FLAG_WINDOW_FULL        0x00000002  // Window is filled.
#define TIMESERIES_FLAG_SELF_ALLOC         0x00000004  // Memory holding the values is owned by this class.
#define TIMESERIES_FLAG_VALID_MINMAX       0x00000008  // Statistical measurement is valid.
#define TIMESERIES_FLAG_VALID_MEAN         0x00000010  // Statistical measurement i s valid.
#define TIMESERIES_FLAG_VALID_RMS          0x00000020  // Statistical measurement is valid.
#define TIMESERIES_FLAG_VALID_STDEV        0x00000040  // Statistical measurement is valid.
#define TIMESERIES_FLAG_VALID_MEDIAN       0x00000080  // Statistical measurement is valid.
#define TIMESERIES_FLAG_VALID_SNR          0x00000100  // Statistical measurement is valid.

// Flag compounds
#define TIMESERIES_FLAG_MASK_ALL_STATS ( \
  TIMESERIES_FLAG_VALID_MINMAX | TIMESERIES_FLAG_VALID_MEAN | \
  TIMESERIES_FLAG_VALID_RMS | TIMESERIES_FLAG_VALID_STDEV | \
  TIMESERIES_FLAG_VALID_MEDIAN | TIMESERIES_FLAG_VALID_SNR)

#define TIMESERIES_FLAG_MASK_SENDABLE_RESULT ( \
  TIMESERIES_FLAG_WINDOW_FULL | TIMESERIES_FLAG_FILTER_INITD)


/*
* Virtual base class that handles the basic flags and meta for a timeseries.
* The primary purpose here is to control template bloat, rather than provide a
*   generic interface to timeseries data.
*/
class TimeSeriesBase {
  public:
    inline int8_t   purge() {                  return _zero_samples();                  };
    inline int8_t   windowSize(uint32_t x) {   return _reallocate_sample_window(x);     };
    inline uint32_t windowSize() {         return (initialized() ? _window_size : 0);   };
    inline uint32_t windowFull() {         return _flags.value(TIMESERIES_FLAG_WINDOW_FULL);   };
    inline bool     initialized() {        return _flags.value(TIMESERIES_FLAG_FILTER_INITD);  };
    inline uint32_t lastIndex() {          return _sample_idx;                            };
    inline uint32_t totalSamples() {       return _samples_total;                         };
    inline char*    name() {               return (_name ? _name : (char*) "");           };
    inline SIUnit*  units() {              return (_units ? _units : nullptr);            };
    inline void     invalidateStats() {    _flags.clear(TIMESERIES_FLAG_MASK_ALL_STATS);  };

    inline C3PValue* valueObj() {          return &_value;                                };
    inline TCode     tcode() {             return _value.tcode();                         };
    inline bool      dirty() {             return (_value.trace() == _last_trace);        };
    inline void      markClean() {         _last_trace = _value.trace();                  };

    int8_t name(char*);
    int8_t units(SIUnit*);
    int8_t serialize(StringBuilder*, TCode);
    int8_t deserialize(StringBuilder*, TCode);


  protected:
    C3PValue        _value;          // Allocated and supplied by the child class.
    uint32_t        _window_size;    // The present size of the window.
    uint32_t        _samples_total;  // Total number of samples that have been ingested since purge().
    uint32_t        _sample_idx;     // The present sample index in the underlying memory pool.
    FlagContainer32 _flags;

    TimeSeriesBase(const TCode, uint32_t ws, uint32_t flgs = 0);
    ~TimeSeriesBase();

    inline bool   _self_allocated() {  return _flags.value(TIMESERIES_FLAG_SELF_ALLOC);       };
    inline bool   _stale_minmax() {    return !(_flags.value(TIMESERIES_FLAG_VALID_MINMAX));  };
    inline bool   _stale_mean() {      return !(_flags.value(TIMESERIES_FLAG_VALID_MEAN));    };
    inline bool   _stale_rms() {       return !(_flags.value(TIMESERIES_FLAG_VALID_RMS));     };
    inline bool   _stale_stdev() {     return !(_flags.value(TIMESERIES_FLAG_VALID_STDEV));   };
    inline bool   _stale_median() {    return !(_flags.value(TIMESERIES_FLAG_VALID_MEDIAN));  };
    inline bool   _stale_snr() {       return !(_flags.value(TIMESERIES_FLAG_VALID_SNR));     };

    void _print_series_base(StringBuilder*);

    /* Mandatory overrides for a child class. */
    virtual int8_t _reallocate_sample_window(uint32_t) =0;
    virtual int8_t _zero_samples() =0;
    #if defined(__BUILD_HAS_CBOR)
      virtual void   _serialize_value(cbor::encoder*, uint32_t idx)    =0;
      virtual void   _deserialize_value(cbor::encoder*, uint32_t idx)  =0;
    #endif


  private:
    char*      _name;
    SIUnit*    _units;
    uint16_t   _last_trace;
};


/*
* Filters for linear sequences of scalar values.
*/
template <class T> class TimeSeries : public TimeSeriesBase {
  public:
    TimeSeries(T* buf, uint32_t ws);
    TimeSeries(uint32_t ws) : TimeSeries(nullptr, ws) {};
    virtual ~TimeSeries();

    int8_t feedSeries(T);
    int8_t feedSeries();
    int8_t init();
    T      value();

    void printSeries(StringBuilder*);

    /* Value accessor inlines */
    inline T        minValue() {   if (_stale_minmax()) _calculate_minmax();  return _min_value;  };
    inline T        maxValue() {   if (_stale_minmax()) _calculate_minmax();  return _max_value;  };
    inline double   mean() {       if (_stale_mean())   _calculate_mean();    return _mean;       };
    inline double   rms() {        if (_stale_rms())    _calculate_rms();     return _rms;        };
    inline double   stdev() {      if (_stale_stdev())  _calculate_stdev();   return _stdev;      };
    inline T        median() {     if (_stale_median()) _calculate_median();  return _median;     };
    inline double   snr() {        if (_stale_snr())    _calculate_snr();     return _snr;        };

    inline T*       memPtr() {     return samples;                     };
    inline uint32_t memUsed() {    return (windowSize() * sizeof(T));  };


  private:
    T*       samples    = nullptr;
    T        _min_value = T(0);
    T        _max_value = T(0);
    T        _median    = T(0);
    double   _mean      = 0.0d;
    double   _rms       = 0.0d;
    double   _stdev     = 0.0d;
    double   _snr       = 0.0d;

    int8_t  _reallocate_sample_window(uint32_t);
    int8_t  _zero_samples();
    #if defined(__BUILD_HAS_CBOR)
      void    _serialize_value(cbor::encoder*, uint32_t idx);
      void    _deserialize_value(cbor::encoder*, uint32_t idx);
    #endif

    int8_t  _calculate_minmax();
    int8_t  _calculate_mean();
    int8_t  _calculate_rms();
    int8_t  _calculate_stdev();
    int8_t  _calculate_median();
    int8_t  _calculate_snr();
};



/*
* Filters for linear sequences of vector values.
*/
template <class T> class TimeSeries3 : public TimeSeriesBase {
  public:
    TimeSeries3(T* buf, uint32_t ws);
    TimeSeries3(uint32_t ws) : TimeSeries3(nullptr, tcodeForType(T(0)), ws) {};
    virtual ~TimeSeries3();

    int8_t feedSeries(Vector3<T>*);
    int8_t feedSeries(T x, T y, T z);  // Alternate API for discrete values.
    int8_t feedSeries();
    int8_t init();
    Vector3<T> value();

    void printSeries(StringBuilder*);

    /* Value accessor inlines */
    inline Vector3<T> minValue() {   if (_stale_minmax()) _calculate_minmax();  return _min_value;  };
    inline Vector3<T> maxValue() {   if (_stale_minmax()) _calculate_minmax();  return _max_value;  };
    inline Vector3f64 mean() {       if (_stale_mean())   _calculate_mean();    return _mean;       };
    inline Vector3f64 rms() {        if (_stale_rms())    _calculate_rms();     return _rms;        };
    inline Vector3f64 stdev() {      if (_stale_stdev())  _calculate_stdev();   return _stdev;      };
    inline Vector3<T> median() {     if (_stale_median()) _calculate_median();  return _median;     };
    inline Vector3f64 snr() {        if (_stale_snr())    _calculate_snr();     return _snr;        };

    inline Vector3<T>* memPtr() {    return samples;                              };
    inline uint32_t    memUsed() {   return (windowSize() * sizeof(Vector3<T>));  };


  private:
    Vector3<T>* samples;
    Vector3<T>  _min_value;
    Vector3<T>  _max_value;
    Vector3<T>  _median;
    Vector3f64  _mean;
    Vector3f64  _rms;
    Vector3f64  _stdev;
    Vector3f64  _snr;

    int8_t  _reallocate_sample_window(uint32_t);
    int8_t  _zero_samples();
    #if defined(__BUILD_HAS_CBOR)
      void    _serialize_value(cbor::encoder*, uint32_t idx);
      void    _deserialize_value(cbor::encoder*, uint32_t idx);
    #endif

    int8_t  _calculate_minmax();
    int8_t  _calculate_mean();
    int8_t  _calculate_rms();
    int8_t  _calculate_stdev();
    int8_t  _calculate_median();
    int8_t  _calculate_snr();
};




/*******************************************************************************
* Single-value variant
*******************************************************************************/
/*
* Constructor.
*/
template <class T> TimeSeries<T>::TimeSeries(T* buf, uint32_t ws) :
  TimeSeriesBase(tcodeForType(T(0)), ws, ((nullptr != buf) ? 0:TIMESERIES_FLAG_SELF_ALLOC)),
  samples(buf) {}


/*
* Destructor. Free sample memory.
*/
template <class T> TimeSeries<T>::~TimeSeries() {
  if ((nullptr != samples) & _flags.value(TIMESERIES_FLAG_SELF_ALLOC)) {
    // If we allocated our own memory pool, we need to free it at this point.
    free(samples);
    samples = nullptr;
  }
}


/*
* Mark the series as having been filled and ready to process. Useful for when
*   the series data is populated from the outside via pointer.
*/
template <class T> int8_t TimeSeries<T>::feedSeries() {
  int8_t ret = -1;
  if (initialized()) {
    _flags.set(TIMESERIES_FLAG_WINDOW_FULL);
    _sample_idx = 0;
    _samples_total += _window_size;
    invalidateStats();
    ret = 0;
  }
  return ret;
}

/*
* This must be called ahead of usage to allocate the needed memory.
*/
template <class T> int8_t TimeSeries<T>::init() {
  bool series_initd = false;
  _flags.clear(TIMESERIES_FLAG_FILTER_INITD);
  if (_self_allocated())  {
    uint32_t tmp_window_size = _window_size;
    _window_size = 0;
    series_initd = (0 == _reallocate_sample_window(tmp_window_size));
  }
  else {
    series_initd = ((nullptr != samples) & (_window_size > 0));
  }
  _flags.set(TIMESERIES_FLAG_FILTER_INITD, series_initd);
  return (series_initd ? 0 : -1);
}

/*
* Reallocates the sample memory, freeing the prior allocation if necessary.
* Returns 0 on success, -1 otherwise.
*/
template <class T> int8_t TimeSeries<T>::_reallocate_sample_window(uint32_t win) {
  int8_t ret = -1;
  if (win != _window_size) {
    if (_self_allocated())  {
      _window_size = win;
      _flags.clear(TIMESERIES_FLAG_WINDOW_FULL);
      if (nullptr != samples) {
        free(samples);
        samples = nullptr;
      }
      if (_window_size > 0) {
        samples = (T*) malloc(_window_size * sizeof(T));
        ret = _zero_samples();
        _sample_idx = 0;
      }
      else {   // Program asked for no sample depth.
        ret = 0;
      }
    }
    else {
      // We can't reallocate if we were constructed with a buffer.
      ret--;
    }
  }
  else {
    ret = _zero_samples();
  }
  return ret;
}


/*
* Zero's the content of the sample memory, whatever that means to this type.
* Returns 0 on success, -1 otherwise.
*/
template <class T> int8_t TimeSeries<T>::_zero_samples() {
  int8_t ret = -1;
  _samples_total = 0;
  _value.set(T(0));
  _min_value = T(0);
  _max_value = T(0);
  _median    = T(0);
  _mean      = 0.0d;
  _rms       = 0.0d;
  _stdev     = 0.0d;
  _snr       = 0.0d;

  _flags.clear(TIMESERIES_FLAG_WINDOW_FULL | TIMESERIES_FLAG_MASK_ALL_STATS);
  if (nullptr != samples) {
    if (_window_size > 0) {
      ret = 0;
      for (uint32_t i = 0; i < _window_size; i++) {
        samples[i] = T(0);
      }
    }
  }
  return ret;
}


template <class T> void TimeSeries<T>::printSeries(StringBuilder* output) {
  _print_series_base(output);
  output->concatf("\tMin   = %.8f\n", (double) minValue());
  output->concatf("\tMax   = %.8f\n", (double) maxValue());
  output->concatf("\tRMS   = %.8f\n", (double) rms());
  output->concatf("\tSTDEV = %.8f\n", (double) stdev());
  output->concatf("\tSNR   = %.8f\n", (double) snr());
}


/**
* Add data to the series.
*
* @param val The value to be fed to the series.
* @return -1 if series not initialized, 0 on value acceptance, or 1 one acceptance with new result.
*/
template <class T> int8_t TimeSeries<T>::feedSeries(T val) {
  int8_t ret = -1;
  if (initialized()) {
    if (_window_size > 1) {
      samples[_sample_idx++] = val;
      _samples_total++;
      if (_sample_idx >= _window_size) {
        // NOTE: Will run only on index overflow.
        _flags.set(TIMESERIES_FLAG_WINDOW_FULL);
        _sample_idx = 0;
      }
      ret = (windowFull() ? 1 : 0);
    }
    else {
      _value.set(val);
      _flags.set(TIMESERIES_FLAG_WINDOW_FULL);
      ret = 1;
    }

    if (1 == ret) {
      _value.set(val);
      // Calculating the stats is an expensive process, and most of the
      //   time, there will be no demand for the result. So we mark our
      //   flags to recalculate fresh in the accessor's stack frame.
      invalidateStats();
    }
  }
  return ret;
}


/**
* Returns the most recent result from the series. Marks the series 'not dirty'
*   as a side-effect, so don't call this for internal logic.
*
* @return The new result.
*/
template <class T> T TimeSeries<T>::value() {
  T ret = T(0);
  if (0 <= _value.get_as(&ret)) {
    markClean();
  }
  return ret;
};


/**
* Calulates the min/max over the entire sample window.
* Updates the private cache variable.
*
* @return 0 on success, -1 if the window isn't full.
*/
template <class T> int8_t TimeSeries<T>::_calculate_minmax() {
  int8_t ret = -1;
  if (windowFull()) {
    ret = 0;
    _min_value = samples[0];  // Start with a baseline.
    _max_value = samples[0];  // Start with a baseline.
    for (uint32_t i = 1; i < _window_size; i++) {
      if (samples[i] > _max_value) _max_value = samples[i];
      else if (samples[i] < _min_value) _min_value = samples[i];
    }
    _flags.set(TIMESERIES_FLAG_VALID_MINMAX);
  }
  return ret;
}


/**
* Calulates the statistical mean over the entire sample window.
* Updates the private cache variable.
*
* @return 0 on success, -1 if the window isn't full.
*/
template <class T> int8_t TimeSeries<T>::_calculate_mean() {
  int8_t ret = -1;
  if (windowFull()) {
    ret = 0;
    double summed_samples = 0.0;
    for (uint32_t i = 0; i < _window_size; i++) {
      summed_samples += (double) samples[i];
    }
    _mean = (summed_samples / _window_size);
    _flags.set(TIMESERIES_FLAG_VALID_MEAN);
  }
  return ret;
}


/**
* Calulates the RMS over the entire sample window.
* Updates the private cache variable.
*
* @return 0 on success, -1 if the window isn't full.
*/
template <class T> int8_t TimeSeries<T>::_calculate_rms() {
  int8_t ret = -1;
  if ((_window_size > 1) & windowFull()) {
    ret = 0;
    double squared_samples = 0.0;
    for (uint32_t i = 0; i < _window_size; i++) {
      squared_samples += ((double) samples[i] * (double) samples[i]);
    }
    _rms = sqrt(squared_samples / _window_size);
    _flags.set(TIMESERIES_FLAG_VALID_RMS);
  }
  return ret;
}


/**
* Calulates the standard deviation of the samples.
* Updates the private cache variable.
*
* @return 0 on success, -1 if the window isn't full.
*/
template <class T> int8_t TimeSeries<T>::_calculate_stdev() {
  int8_t ret = -1;
  if ((_window_size > 1) & windowFull()) {
    ret = 0;
    double deviation_sum = 0.0;
    double cached_mean = mean();
    for (uint32_t i = 0; i < _window_size; i++) {
      double tmp = samples[i] - cached_mean;
      deviation_sum += ((double) tmp * (double) tmp);
    }
    _stdev = sqrt(deviation_sum / _window_size);
    _flags.set(TIMESERIES_FLAG_VALID_STDEV);
  }
  return ret;
}


/**
* Calulates the median value of the samples.
* Updates the private cache variable.
*
* @return 0 on success, -1 if the window isn't full.
*/
template <class T> int8_t TimeSeries<T>::_calculate_median() {
  int8_t ret = -1;
  if ((_window_size > 1) & windowFull()) {
    ret = 0;
    T sorted[_window_size];
    for (uint32_t i = 0; i < _window_size; i++) {
      sorted[i] = samples[i];
    }
    // Selection sort.
    uint32_t i  = 0;
    T swap;
    for (uint32_t c = 0; c < (_window_size - 1); c++) {
      i = c;
      for (uint32_t d = c + 1; d < _window_size; d++) {
        if (sorted[i] > sorted[d]) i = d;
      }
      if (i != c) {
        swap = sorted[c];
        sorted[c] = sorted[i];
        sorted[i] = swap;
      }
    }

    if (_window_size & 0x01) {
      // If there are an odd number of samples...
      _median = sorted[(_window_size-1) >> 1];
    }
    else {
      // ...otherwise, we take the mean to the two middle values.
      uint32_t lower = (_window_size-1) >> 1;
      uint32_t upper = lower + 1;
      _median = (sorted[upper] + sorted[lower]) / 2;
    }
    _flags.set(TIMESERIES_FLAG_VALID_MEDIAN);
  }
  return ret;
}


/**
* Calulates the standard deviation of the samples.
* Updates the private cache variable.
*
* @return 0 on success, -1 if the window isn't full.
*/
template <class T> int8_t TimeSeries<T>::_calculate_snr() {
  int8_t ret = -1;
  if ((_window_size > 1) & windowFull()) {
    ret = 0;
    _snr = (mean() / stdev());
    _flags.set(TIMESERIES_FLAG_VALID_SNR);
  }
  return ret;
}



/*******************************************************************************
* Vector3 variant
*******************************************************************************/
/*
* Constructor.
*/
template <class T> TimeSeries3<T>::TimeSeries3(T* buf, uint32_t ws) :
  TimeSeriesBase(tcodeForType(T(0)), ws, ((nullptr != buf) ? 0:TIMESERIES_FLAG_SELF_ALLOC)),
  samples(buf) {}


/*
* Destructor. Free sample memory.
*/
template <class T> TimeSeries3<T>::~TimeSeries3() {
  if ((nullptr != samples) & _self_allocated()) {
    free(samples);
    samples = nullptr;
  }
}

/*
* This must be called ahead of usage to allocate the needed memory.
*/
template <class T> int8_t TimeSeries3<T>::init() {
  bool series_initd = false;
  _flags.clear(TIMESERIES_FLAG_FILTER_INITD);
  if (_self_allocated()) {
    uint32_t tmp_window_size = _window_size;
    _window_size = 0;
    series_initd = (0 == _reallocate_sample_window(tmp_window_size));
  }
  else {
    series_initd = ((nullptr != samples) & (_window_size > 0));
  }
  _flags.set(TIMESERIES_FLAG_FILTER_INITD, series_initd);
  return (series_initd ? 0 : -1);
}


/*
* Mark the series as having been filled and ready to process. Useful for when
*   the series data is populated from the outside via pointer.
*/
template <class T> int8_t TimeSeries3<T>::feedSeries() {
  int8_t ret = -1;
  if (initialized()) {
    _flags.set(TIMESERIES_FLAG_WINDOW_FULL);
    _sample_idx = 0;
    _samples_total += _window_size;
    invalidateStats();
    ret = 0;
  }
  return ret;
}


/*
* Reallocates the sample memory, freeing the prior allocation if necessary.
* Returns 0 on success, -1 otherwise.
*/
template <class T> int8_t TimeSeries3<T>::_reallocate_sample_window(uint32_t win) {
  int8_t ret = -1;
  if (win != _window_size) {
    if (_self_allocated()) {
      _window_size = win;
      _flags.clear(TIMESERIES_FLAG_WINDOW_FULL);
      if (nullptr != samples) {
        free(samples);
        samples = nullptr;
      }
      if (_window_size > 0) {
        samples = (Vector3<T>*) malloc(_window_size * sizeof(Vector3<T>));
        ret = _zero_samples();
        _sample_idx = 0;
      }
      else {   // Program asked for no sample depth.
        ret = 0;
      }
    }
    else {
      // We can't reallocate if we were constructed with a buffer.
      ret--;
    }
  }
  else {
    ret = _zero_samples();
  }
  return ret;
}


/*
* Zero's the content of the sample memory, whatever that means to this type.
* Returns 0 on success, -1 otherwise.
*/
template <class T> int8_t TimeSeries3<T>::_zero_samples() {
  int8_t ret = -1;
  _samples_total = 0;
  Vector3<T> tmp_vect(T(0), T(0), T(0));
  _value.set(&tmp_vect);
  _min_value(T(0), T(0), T(0));
  _max_value(T(0), T(0), T(0));
  _median(T(0), T(0), T(0));
  _mean(0.0d, 0.0d, 0.0d);
  _rms(0.0d, 0.0d, 0.0d);
  _stdev(0.0d, 0.0d, 0.0d);
  _snr(0.0d, 0.0d, 0.0d);
  _flags.clear(TIMESERIES_FLAG_WINDOW_FULL | TIMESERIES_FLAG_MASK_ALL_STATS);
  if (nullptr != samples) {
    if (_window_size > 0) {
      ret = 0;
      for (uint32_t i = 0; i < _window_size; i++) {
        samples[i](T(0), T(0), T(0));
      }
    }
  }
  return ret;
}


template <class T> void TimeSeries3<T>::printSeries(StringBuilder* output) {
  _print_series_base(output);
  if (_stale_minmax()) _calculate_minmax();
  if (_stale_mean())   _calculate_mean();
  if (_stale_rms())    _calculate_rms();
  if (_stale_stdev())  _calculate_stdev();

  output->concatf("\tMin   = (%.4f, %.4f, %.4f)\n", (double) _min_value.x, (double) _min_value.y, (double) _min_value.z);
  output->concatf("\tMax   = (%.4f, %.4f, %.4f)\n", (double) _max_value.x, (double) _max_value.y, (double) _max_value.z);
  output->concatf("\tRMS   = (%.4f, %.4f, %.4f)\n", _rms.x, _rms.y, _rms.z);
  output->concatf("\tSTDEV = (%.4f, %.4f, %.4f)\n", _stdev.x, _stdev.y, _stdev.z);
  //output->concatf("\tSNR             = %.8f\n", snr());
}


/**
* Add data to the series.
*
* @param val The value to be fed to the series.
* @return -1 if series not initialized, 0 on value acceptance, or 1 one acceptance with new result.
*/
template <class T> int8_t TimeSeries3<T>::feedSeries(Vector3<T>* vect) {
  return feedSeries(vect->x, vect->y, vect->z);
}


/**
* Add data to the series.
*
* @param val The value to be fed to the series.
* @return -1 if series not initialized, 0 on value acceptance, or 1 one acceptance with new result.
*/
template <class T> int8_t TimeSeries3<T>::feedSeries(T x, T y, T z) {
  int8_t ret = -1;
  if (initialized()) {
    ret = 0;
    Vector3<T> tmp_vect(x, y, z);
    if (_window_size > 0) {
      samples[_sample_idx++](x, y, z);
      _samples_total++;
      if (_sample_idx >= _window_size) {
        _flags.set(TIMESERIES_FLAG_WINDOW_FULL);
        _sample_idx = 0;
      }
      ret = (windowFull() ? 1 : 0);
    }

    if (1 == ret) {
      _value.set(&tmp_vect);
      // Calculating the stats is an expensive process, and most of the
      //   time, there will be no demand for the result. So we mark our
      //   flags to recalculate fresh in the accessor's stack frame.
      invalidateStats();
    }
  }
  return ret;
}


/**
* Returns the most recent result from the series. Marks the series 'not dirty'
*   as a side-effect, so don't call this for internal logic.
*
* @return The new result.
*/
template <class T> Vector3<T> TimeSeries3<T>::value() {
  Vector3<T> ret(T(0), T(0), T(0));
  if (0 <= _value.get_as(&ret)) {
    markClean();
  }
  return ret;
}


/**
* Calulates the min/max vector (by magnitude) over the entire sample window.
* Updates the private cache variable.
*/
template <class T> int8_t TimeSeries3<T>::_calculate_minmax() {
  int8_t ret = -1;
  if (windowFull()) {
    ret = 0;
    Vector3<T> tmp_min(&samples[0]);   // Start with a baseline.
    Vector3<T> tmp_max(&samples[0]);   // Start with a baseline.
    for (uint32_t i = 1; i < _window_size; i++) {
      if (samples[i].length() > tmp_max.length()) tmp_max.set(&samples[i]);
      else if (samples[i].length() < tmp_min.length()) tmp_min.set(&samples[i]);
    }
    _min_value.set(&tmp_min);
    _max_value.set(&tmp_max);
    _flags.set(TIMESERIES_FLAG_VALID_MINMAX);
  }
  return ret;
}


/**
* Calulates the statistical mean over the entire sample window.
* Updates the private cache variable.
*
* @return 0 on success, -1 on failure
*/
template <class T> int8_t TimeSeries3<T>::_calculate_mean() {
  int8_t ret = -1;
  if (windowFull()) {
    ret = 0;
    Vector3f64 summed_samples;
    for (uint32_t i = 0; i < _window_size; i++) {
      Vector3f64 tmp((double) samples[i].x, (double) samples[i].y, (double) samples[i].z);
      summed_samples += tmp;
    }
    summed_samples /= _window_size;
    _mean(
      summed_samples.x,
      summed_samples.y,
      summed_samples.z
    );
    _flags.set(TIMESERIES_FLAG_VALID_MEAN);
  }
  return ret;
}


/**
* Calulates the RMS over the entire sample window.
* Updates the private cache variable.
*
* @return 0 on success, -1 on failure
*/
template <class T> int8_t TimeSeries3<T>::_calculate_rms() {
  int8_t ret = -1;
  if ((_window_size > 0) & windowFull()) {
    ret = 0;
    Vector3f64 squared_samples;
    for (uint32_t i = 0; i < _window_size; i++) {
      Vector3f64 tmp((double) samples[i].x, (double) samples[i].y, (double) samples[i].z);
      squared_samples(
        squared_samples.x + (tmp.x * tmp.x),
        squared_samples.y + (tmp.y * tmp.y),
        squared_samples.z + (tmp.z * tmp.z)
      );
    }
    squared_samples /= _window_size;
    _rms(
      sqrt(squared_samples.x),
      sqrt(squared_samples.y),
      sqrt(squared_samples.z)
    );
    _flags.set(TIMESERIES_FLAG_VALID_RMS);
  }
  return ret;
}


/**
* Calulates the standard deviation of the samples.
* Updates the private cache variable.
*
* @return 0 on success, -1 on failure
*/
template <class T> int8_t TimeSeries3<T>::_calculate_stdev() {
  int8_t ret = -1;
  if ((_window_size > 1) & windowFull()) {
    ret = 0;
    Vector3f64 deviation_sum;
    if (_stale_mean) _calculate_mean();
    for (uint32_t i = 0; i < _window_size; i++) {
      Vector3f64 temp{(double) samples[i].x, (double) samples[i].y, (double) samples[i].z};
      temp -= _mean;
      deviation_sum.set(
        deviation_sum.x + (temp.x * temp.x),
        deviation_sum.y + (temp.y * temp.y),
        deviation_sum.z + (temp.z * temp.z)
      );
    }
    deviation_sum /= _window_size;
    _stdev(
      sqrt(deviation_sum.x),
      sqrt(deviation_sum.y),
      sqrt(deviation_sum.z)
    );
    _flags.set(TIMESERIES_FLAG_VALID_STDEV);
  }
  return ret;
}


/*
* Calulates the median of the samples.
*/
template <class T> int8_t TimeSeries3<T>::_calculate_median() {
  int8_t ret = -1;
  if ((_window_size > 1) & windowFull()) {
    ret = 0;
    double sorted[3][_window_size];
    for (uint32_t i = 0; i < _window_size; i++) {
      sorted[0][i] = samples[i].x;
      sorted[1][i] = samples[i].y;
      sorted[2][i] = samples[i].z;
    }
    // Selection sort.
    uint32_t i = 0;
    double swap;
    for (uint8_t n = 0; n < 3; n++) {
      for (uint32_t c = 0; c < (_window_size - 1); c++) {
        i = c;
        for (uint32_t d = c + 1; d < _window_size; d++) {
          if (sorted[n][i] > sorted[n][d]) i = d;
        }
        if (i != c) {
          swap = sorted[n][c];
          sorted[n][c] = sorted[n][i];
          sorted[n][i] = swap;
        }
      }
    }

    if (_window_size & 0x01) {
      // If there are an off number of samples...
      uint32_t offset = (_window_size-1) >> 1;
      _median.set(sorted[0][offset], sorted[1][offset], sorted[2][offset]);
    }
    else {
      // ...otherwise, we take the mean to the two middle values.
      uint32_t lower = (_window_size-1) >> 1;
      uint32_t upper = lower + 1;
      _median.set(
        ((sorted[0][upper] + sorted[0][lower]) / 2),
        ((sorted[1][upper] + sorted[1][lower]) / 2),
        ((sorted[2][upper] + sorted[2][lower]) / 2)
      );
    }
    _flags.set(TIMESERIES_FLAG_VALID_MEDIAN);
  }
  return ret;
}


/**
* Calulates the standard deviation of the samples.
* Updates the private cache variable.
* TODO: Is this still valid for vectors?
*
* @return 0 on success, -1 if the window isn't full.
*/
template <class T> int8_t TimeSeries3<T>::_calculate_snr() {
  int8_t ret = -1;
  if ((_window_size > 1) & windowFull()) {
    ret = 0;
    _snr = (mean() / stdev());
    _flags.set(TIMESERIES_FLAG_VALID_SNR);
  }
  return ret;
}


#endif  // __C3P_TIMESERIES_H__
