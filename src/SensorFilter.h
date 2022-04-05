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

#include "StringBuilder.h"
#include "Vector3.h"


enum class FilteringStrategy : uint8_t {
  RAW            = 0,  // No filtering
  MOVING_AVG     = 1,  // Moving average with a given window size. Arithmetic mean.
  MOVING_MED     = 2,  // Moving median with a given window size.
  HARMONIC_MEAN  = 3,  // Moving harmonic mean with a given window size.
  GEOMETRIC_MEAN = 4,  // Moving geometric mean.
  QUANTIZER      = 5   // A filter that divides inputs up into bins.
};


/*
* Pure-virtual base class that handles the basic flags and meta for filters.
*/
class SensorFilterBase {
  public:
    inline int  purge() {            return _zero_samples();                    };
    inline int  windowSize(uint x) { return _reallocate_sample_window(x);       };
    inline uint windowSize() {       return (_filter_initd ? _window_size : 0); };
    inline uint windowFull() {       return _window_full;                       };
    inline uint lastIndex() {        return _sample_idx;                        };
    inline bool dirty() {            return _filter_dirty;                      };
    inline bool initialized() {      return _filter_initd;                      };
    inline FilteringStrategy strategy() {   return _strat;    };

    /**
    * Marks all appropriate flags for marking the derived data as stale.
    */
    inline void invalidateStats() {
      _stale_minmax = true;
      _stale_mean   = true;
      _stale_rms    = true;
      _stale_stdev  = true;
    };


  protected:
    uint32_t _sample_idx     = 0;
    uint32_t _window_size    = 0;
    FilteringStrategy _strat = FilteringStrategy::RAW;
    bool     _window_full    = false;
    bool     _filter_dirty   = false;
    bool     _filter_initd   = false;
    bool     _stale_minmax   = false;  // Statistical measurement is stale.
    bool     _stale_mean     = false;  // Statistical measurement is stale.
    bool     _stale_rms      = false;  // Statistical measurement is stale.
    bool     _stale_stdev    = false;  // Statistical measurement is stale.

    SensorFilterBase(int ws, FilteringStrategy s) : _window_size(ws), _strat(s) {};
    virtual int8_t  _reallocate_sample_window(uint) =0;
    virtual int8_t  _zero_samples() =0;
};


/*
* Filters for linear sequences of scalar values.
*/
template <typename T> class SensorFilter : public SensorFilterBase {
  public:
    SensorFilter(uint ws, FilteringStrategy s) : SensorFilterBase(ws, s) {};
    ~SensorFilter();

    int8_t feedFilter(T);
    int8_t feedFilter();
    int8_t init();
    T      value();

    int8_t setStrategy(FilteringStrategy);
    void printFilter(StringBuilder*);

    /* Value accessor inlines */
    inline T*       memPtr() {        return samples;         };
    inline T        minValue() {      if (_stale_minmax) _calculate_minmax(); return min_value; };
    inline T        maxValue() {      if (_stale_minmax) _calculate_minmax(); return max_value; };
    inline T        median() {        return _calculate_median();                               };
    inline double   mean() {          return (_stale_mean   ? _calculate_mean()   : _mean);  };
    inline double   rms() {           return (_stale_rms    ? _calculate_rms()    : _rms  ); };
    inline double   stdev() {         return (_stale_stdev  ? _calculate_stdev()  : _stdev); };
    inline double   snr() {           return (mean() / stdev());                             };
    inline uint32_t memUsed() {       return (windowSize() * sizeof(T));  };


  private:
    T*       samples         = nullptr;
    T        last_value      = T(0);
    T        min_value       = T(0);
    T        max_value       = T(0);
    double   _mean           = 0.0;
    double   _rms            = 0.0;
    double   _stdev          = 0.0;

    int8_t  _reallocate_sample_window(uint);
    int8_t  _zero_samples();

    void    _calculate_minmax();
    double  _calculate_mean();
    double  _calculate_rms();
    double  _calculate_stdev();
    T       _calculate_median();
};



/*
* Filters for linear sequences of vector values.
*/
template <typename T> class SensorFilter3 : public SensorFilterBase {
  public:
    SensorFilter3(uint ws, FilteringStrategy s) : SensorFilterBase(ws, s) {};
    virtual ~SensorFilter3();

    int8_t feedFilter(Vector3<T>*);
    int8_t feedFilter(T x, T y, T z);  // Alternate API for discrete values.
    int8_t feedFilter();
    int8_t init();
    Vector3<T>* value();

    /* Value accessor inlines */
    inline Vector3<T>* memPtr() {     return samples;         };
    inline Vector3<T>* minValue() {   if (_stale_minmax) _calculate_minmax(); return &min_value; };
    inline Vector3<T>* maxValue() {   if (_stale_minmax) _calculate_minmax(); return &max_value; };
    inline Vector3f64* mean() {       if (_stale_mean)   _calculate_mean();   return &_mean;     };
    inline Vector3f64* rms() {        if (_stale_rms)    _calculate_rms();    return &_rms;      };  
    inline Vector3f64* stdev() {      if (_stale_stdev)  _calculate_stdev();  return &_stdev;    };
    //inline Vector3f64* snr() {        return (mean() / stdev());                               };
    inline uint32_t    memUsed() {    return (windowSize() * sizeof(Vector3<T>));  };

    int8_t setStrategy(FilteringStrategy);
    void printFilter(StringBuilder*);


  private:
    Vector3<T>* samples      = nullptr;
    Vector3<T>  last_value;
    Vector3<T>  min_value;
    Vector3<T>  max_value;
    Vector3f64  _mean;
    Vector3f64  _rms;
    Vector3f64  _stdev;
    //Vector3<double>  _snr;

    int8_t  _reallocate_sample_window(uint);
    int8_t  _zero_samples();
    int8_t  _calculate_minmax();
    int8_t  _calculate_mean();
    int8_t  _calculate_rms();
    int8_t  _calculate_stdev();
    int8_t  _calculate_median();
};




/*******************************************************************************
* Statics and externs
*******************************************************************************/
extern const char* const getFilterStr(FilteringStrategy);
static const char* const FILTER_HEADER_STRING = "\t%s filter\n\t-----------------------------\n";


/*******************************************************************************
* Linear filter class
*******************************************************************************/

/*
* Base destructor. Free sample memory.
*/
template <typename T> SensorFilter<T>::~SensorFilter() {
  if (nullptr != samples) {
    free(samples);
    samples = nullptr;
  }
}


/*
* Mark the filter as having been filled and ready to process. Useful for when
*   the filter data is populated from the outside via pointer.
*/
template <typename T> int8_t SensorFilter<T>::feedFilter() {
  int8_t ret = -1;
  if (initialized()) {
    _window_full = true;
    _sample_idx = 0;
    invalidateStats();
    ret = 0;
  }
  return ret;
}

/*
* This must be called ahead of usage to allocate the needed memory.
*/
template <typename T> int8_t SensorFilter<T>::init() {
  uint32_t tmp_window_size = _window_size;
  _window_size = 0;
  int8_t ret = _reallocate_sample_window(tmp_window_size);
  _filter_initd = (0 == ret);
  return ret;
}

/*
* Reallocates the sample memory, freeing the prior allocation if necessary.
* Returns 0 on success, -1 otherwise.
*/
template <typename T> int8_t SensorFilter<T>::_reallocate_sample_window(uint win) {
  int8_t ret = -1;
  if (win != _window_size) {
    _window_size = win;
    _window_full = false;
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
    ret = _zero_samples();
  }
  return ret;
}


/*
* Zero's the content of the sample memory, whatever that means to this type.
* Returns 0 on success, -1 otherwise.
*/
template <typename T> int8_t SensorFilter<T>::_zero_samples() {
  int8_t ret = -1;
  last_value = T(0);
  min_value  = T(0);
  max_value  = T(0);
  _mean      = 0.0;
  _rms       = 0.0;
  _stdev     = 0.0;
  invalidateStats();
  _window_full = false;
  if (nullptr != samples) {
    if (_window_size > 0) {
      ret = 0;
      for (uint i = 0; i < _window_size; i++) {
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
  output->concatf(FILTER_HEADER_STRING, getFilterStr(strategy()));
  output->concatf("\tInitialized:  %c\n",   initialized() ? 'y':'n');
  output->concatf("\tDirty:        %c\n",   _filter_dirty ? 'y':'n');
  output->concatf("\tWindow full:  %c\n",   _window_full  ? 'y':'n');
  output->concatf("\tMin             = %.8f\n", minValue());
  output->concatf("\tMax             = %.8f\n", maxValue());
  output->concatf("\tSample window   = %u\n",   _window_size);
  switch (_strat) {
    case FilteringStrategy::RAW:
      output->concatf("\tValue           = %.8f\n", last_value);
      break;
    case FilteringStrategy::MOVING_AVG:
      output->concatf("\tRunning average = %.8f\n", last_value);
      break;
    case FilteringStrategy::MOVING_MED:
      output->concatf("\tRunning median  = %.8f\n", last_value);
      break;
    case FilteringStrategy::HARMONIC_MEAN:
      output->concatf("\tHarmonic mean   = %.8f\n", last_value);
      break;
    case FilteringStrategy::GEOMETRIC_MEAN:
      output->concatf("\tGeometric mean  = %.8f\n", last_value);
      break;
    case FilteringStrategy::QUANTIZER:
      output->concatf("\tQuantized value = %.8f\n", last_value);
      break;
  }
  output->concatf("\tRMS             = %.8f\n", rms());
  output->concatf("\tSTDEV           = %.8f\n", stdev());
  output->concatf("\tSNR             = %.8f\n", snr());
}


/**
* Add data to the filter.
*
* @param val The value to be fed to the filter.
* @return -1 if filter not initialized, 0 on value acceptance, or 1 one acceptance with new result.
*/
template <typename T> int8_t SensorFilter<T>::feedFilter(T val) {
  int8_t ret = -1;
  if (initialized()) {
    if (_window_size > 1) {
      samples[_sample_idx++] = val;
      if (_sample_idx >= _window_size) {
        // NOTE: Will run only on index overflow.
        _window_full = true;
        _sample_idx = 0;
      }
      ret = (_window_full ? 1 : 0);

      if (_window_size) {
        switch (_strat) {
          case FilteringStrategy::HARMONIC_MEAN:   // TODO: This.
          case FilteringStrategy::GEOMETRIC_MEAN:  // TODO: This.
          case FilteringStrategy::QUANTIZER:       // TODO: This.
          case FilteringStrategy::RAW:
            // We're being used as a glorified FIFO with top-peek.
            last_value = val;
            break;
          case FilteringStrategy::MOVING_AVG:
            // Calculate the moving average...
            last_value = ((last_value * (_window_size-1)) + val) / _window_size;
            break;
          case FilteringStrategy::MOVING_MED:
            // Calculate the moving median...
            last_value = _calculate_median();
            break;
        }
      }
    }
    else {   // This is a null filter with extra steps.
      last_value = val;
      ret = 1;
    }

    if (1 == ret) {
      _filter_dirty = true;
      // Calculating the stats is an expensive process, and most of the
      //   time, there will be no demand for the result. So we mark our
      //   flags to recalculate fresh in the accessor's stack frame.
      invalidateStats();
    }
  }
  return ret;
}


/**
* Returns the most recent result from the filter. Marks the filter 'not dirty'
*   as a side-effect, so don't call this for internal logic.
*
* @return The new result.
*/
template <typename T> T SensorFilter<T>::value() {
  _filter_dirty = false;
  return last_value;
};


/**
* Calulates the min/max over the entire sample window.
* Updates the private cache variable.
*/
template <typename T> void SensorFilter<T>::_calculate_minmax() {
  if (_filter_initd && _window_full) {
    min_value = samples[0];   // Start with a baseline.
    max_value = samples[0];   // Start with a baseline.
    for (uint i = 1; i < _window_size; i++) {
      if (samples[i] > max_value) max_value = samples[i];
      else if (samples[i] < min_value) min_value = samples[i];
    }
    _stale_minmax = false;
  }
}


/**
* Calulates the statistical mean over the entire sample window.
* Updates the private cache variable.
*
* @return the mean of the contents of the filter.
*/
template <typename T> double SensorFilter<T>::_calculate_mean() {
  if (_filter_initd && _window_full) {
    double summed_samples = 0.0;
    for (uint i = 0; i < _window_size; i++) {
      summed_samples += (double) samples[i];
    }
    _mean = (summed_samples / _window_size);
    _stale_mean  = false;
  }
  return _mean;
}


/**
* Calulates the RMS over the entire sample window.
* Updates the private cache variable.
*
* @return the root mean square of the contents of the filter.
*/
template <typename T> double SensorFilter<T>::_calculate_rms() {
  if ((_window_size > 1) && _filter_initd && _window_full) {
    double squared_samples = 0.0;
    for (uint i = 0; i < _window_size; i++) {
      squared_samples += ((double) samples[i] * (double) samples[i]);
    }
    _rms = sqrt(squared_samples / _window_size);
    _stale_rms = false;
  }
  return _rms;
}


/**
* Calulates the standard deviation of the samples.
* Updates the private cache variable.
*
* @return the standard deviation of the contents of the filter.
*/
template <typename T> double SensorFilter<T>::_calculate_stdev() {
  if ((_window_size > 1) && _filter_initd && _window_full) {
    double deviation_sum = 0.0;
    double cached_mean = mean();
    for (uint i = 0; i < _window_size; i++) {
      double tmp = samples[i] - cached_mean;
      deviation_sum += ((double) tmp * (double) tmp);
    }
    _stdev = sqrt(deviation_sum / _window_size);
    _stale_stdev = false;
  }
  return _stdev;
}


/**
* Calulates the median value of the samples.
*
* @return the median of the contents of the filter.
*/
template <typename T> T SensorFilter<T>::_calculate_median() {
  T sorted[_window_size];
  T ret = T(0);
  for (uint i = 0; i < _window_size; i++) {
    sorted[i] = samples[i];
  }
  // Selection sort.
  uint i  = 0;
  T swap;
  for (uint c = 0; c < (_window_size - 1); c++) {
    i = c;
    for (uint d = c + 1; d < _window_size; d++) {
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
    ret = sorted[(_window_size-1) >> 1];
  }
  else {
    // ...otherwise, we take the mean to the two middle values.
    uint lower = (_window_size-1) >> 1;
    uint upper = lower + 1;
    ret = (sorted[upper] + sorted[lower]) / 2;
  }
  return ret;
}


/*******************************************************************************
* Linear Vector3 filter class
*******************************************************************************/

/*
* Base destructor. Free sample memory.
*/
template <typename T> SensorFilter3<T>::~SensorFilter3() {
  if (nullptr != samples) {
    free(samples);
    samples = nullptr;
  }
}

/*
* This must be called ahead of usage to allocate the needed memory.
*/
template <typename T> int8_t SensorFilter3<T>::init() {
  uint32_t tmp_window_size = _window_size;
  _window_size = 0;
  int8_t ret = _reallocate_sample_window(tmp_window_size);
  _filter_initd = (0 == ret);
  return ret;
}


/*
* Mark the filter as having been filled and ready to process. Useful for when
*   the filter data is populated from the outside via pointer.
*/
template <typename T> int8_t SensorFilter3<T>::feedFilter() {
  int8_t ret = -1;
  if (initialized()) {
    _window_full = true;
    _sample_idx = 0;
    invalidateStats();
    ret = 0;
  }
  return ret;
}



/*
* Reallocates the sample memory, freeing the prior allocation if necessary.
* Returns 0 on success, -1 otherwise.
*/
template <typename T> int8_t SensorFilter3<T>::_reallocate_sample_window(uint win) {
  int8_t ret = -1;
  if (win != _window_size) {
    _window_size = win;
    _window_full = false;
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
    ret = _zero_samples();
  }
  return ret;
}

/*
* Zero's the content of the sample memory, whatever that means to this type.
* Returns 0 on success, -1 otherwise.
*/
template <typename T> int8_t SensorFilter3<T>::_zero_samples() {
  int8_t ret = -1;
  last_value(T(0), T(0), T(0));
  _mean(T(0), T(0), T(0));
  _rms(T(0), T(0), T(0));
  _stdev(T(0), T(0), T(0));
  if (nullptr != samples) {
    if (_window_size > 0) {
      ret = 0;
      for (uint i = 0; i < _window_size; i++) {
        samples[i](T(0), T(0), T(0));
      }
    }
  }
  return ret;
}


template <typename T> int8_t SensorFilter3<T>::setStrategy(FilteringStrategy s) {
  int8_t ret = -1;
  if (_strat != s) {
    _strat = s;
    last_value(T(0), T(0), T(0));
    ret = 0;
  }
  return ret;
}


template <typename T> void SensorFilter3<T>::printFilter(StringBuilder* output) {
  output->concatf(FILTER_HEADER_STRING, getFilterStr(strategy()));
  output->concatf("\tInitialized:  %c\n",   initialized() ? 'y':'n');
  output->concatf("\tDirty:        %c\n",   _filter_dirty ? 'y':'n');
  output->concatf("\tWindow full:  %c\n",   _window_full  ? 'y':'n');

  if (_stale_minmax) _calculate_minmax();
  if (_stale_mean)   _calculate_mean();
  if (_stale_rms)    _calculate_rms();
  if (_stale_stdev)  _calculate_stdev();

  output->concatf("\tMin             = (%.4f, %.4f, %.4f)\n", min_value.x, min_value.y, min_value.z);
  output->concatf("\tMax             = (%.4f, %.4f, %.4f)\n", max_value.x, max_value.y, max_value.z);
  output->concatf("\tSample window   = %u\n",   _window_size);
  const char* lv_label;
  switch (_strat) {
    case FilteringStrategy::MOVING_AVG:
      lv_label = "Arithmetic mean";
      break;
    case FilteringStrategy::MOVING_MED:
      lv_label = "Median";
      break;
    case FilteringStrategy::HARMONIC_MEAN:
      lv_label = "Harmonic mean";
      break;
    case FilteringStrategy::GEOMETRIC_MEAN:
      lv_label = "Geometric mean";
      break;
    case FilteringStrategy::QUANTIZER:
      lv_label = "Quantized value";
      break;
    default:
      lv_label = "Value";
      break;
  }
  output->concatf("\t%15s = (%.4f, %.4f, %.4f)\n", lv_label, (double) last_value.x, (double) last_value.y, (double) last_value.z);
  output->concatf("\tRMS             = (%.4f, %.4f, %.4f)\n", _rms.x, _rms.y, _rms.z);
  output->concatf("\tSTDEV           = (%.4f, %.4f, %.4f)\n", _stdev.x, _stdev.y, _stdev.z);
  //output->concatf("\tSNR             = %.8f\n", snr());
}


/**
* Add data to the filter.
*
* @param val The value to be fed to the filter.
* @return -1 if filter not initialized, 0 on value acceptance, or 1 one acceptance with new result.
*/
template <typename T> int8_t SensorFilter3<T>::feedFilter(Vector3<T>* vect) {
  return feedFilter(vect->x, vect->y, vect->z);
}


/**
* Add data to the filter.
*
* @param val The value to be fed to the filter.
* @return -1 if filter not initialized, 0 on value acceptance, or 1 one acceptance with new result.
*/
template <typename T> int8_t SensorFilter3<T>::feedFilter(T x, T y, T z) {
  int8_t ret = -1;
  if (initialized()) {
    ret = 0;
    if (_window_size > 1) {
      samples[_sample_idx++](x, y, z);
      if (_sample_idx >= _window_size) {
        _window_full = true;
        _sample_idx = 0;
      }
      ret = (_window_full ? 1 : 0);
      switch (_strat) {
        case FilteringStrategy::HARMONIC_MEAN:   // TODO: This.
        case FilteringStrategy::GEOMETRIC_MEAN:  // TODO: This.
        case FilteringStrategy::QUANTIZER:       // TODO: This.

        case FilteringStrategy::RAW:
          last_value(x, y, z);
          break;
        case FilteringStrategy::MOVING_AVG:   // Calculate the moving average...
          {
            Vector3f64 input_vect((double) x, (double) y, (double) z);
            Vector3f64 temp_avg((double) last_value.x, (double) last_value.y, (double) last_value.z);
            temp_avg *= (_window_size-1);
            temp_avg += input_vect;
            temp_avg /= (double) _window_size;
            last_value((T) temp_avg.x, (T) temp_avg.y, (T) temp_avg.z);
          }
          break;
        case FilteringStrategy::MOVING_MED:   // Calculate the moving median...
          // Calculate the moving median...
          if (_window_full) {
            _calculate_median();
          }
          break;
      }
    }
    else {   // This is a null filter with extra steps.
      last_value(x, y, z);
      ret = 1;
    }

    if (1 == ret) {
      _filter_dirty = true;
      // Calculating the stats is an expensive process, and most of the
      //   time, there will be no demand for the result. So we mark our
      //   flags to recalculate fresh in the accessor's stack frame.
      invalidateStats();
    }
  }
  return ret;
}


/**
* Returns the most recent result from the filter. Marks the filter 'not dirty'
*   as a side-effect, so don't call this for internal logic.
*
* @return The new result.
*/
template <typename T> Vector3<T>* SensorFilter3<T>::value() {
  _filter_dirty = false;
  return &last_value;
}


/**
* Calulates the min/max vector (by magnitude) over the entire sample window.
* Updates the private cache variable.
*/
template <typename T> int8_t SensorFilter3<T>::_calculate_minmax() {
  int8_t ret = -1;
  if (_filter_initd && _window_full) {
    Vector3<T> tmp_min(&samples[0]);   // Start with a baseline.
    Vector3<T> tmp_max(&samples[0]);   // Start with a baseline.
    for (uint i = 1; i < _window_size; i++) {
      if (samples[i].length() > tmp_max.length()) tmp_max.set(&samples[i]);
      else if (samples[i].length() < tmp_min.length()) tmp_min.set(&samples[i]);
    }
    min_value.set(&tmp_min);
    max_value.set(&tmp_max);
    _stale_minmax = false;
    ret = 0;
  }
  return ret;
}


/**
* Calulates the statistical mean over the entire sample window.
* Updates the private cache variable.
*
* @return 0 on success, -1 on failure
*/
template <typename T> int8_t SensorFilter3<T>::_calculate_mean() {
  int8_t ret = -1;
  if (_filter_initd && _window_full) {
    Vector3f64 summed_samples;
    for (uint i = 0; i < _window_size; i++) {
      Vector3f64 tmp((double) samples[i].x, (double) samples[i].y, (double) samples[i].z);
      summed_samples += tmp;
    }
    summed_samples /= _window_size;
    _mean(
      summed_samples.x,
      summed_samples.y,
      summed_samples.z
    );
    _stale_mean  = false;
    ret = 0;
  }
  return ret;
}


/**
* Calulates the RMS over the entire sample window.
* Updates the private cache variable.
*
* @return 0 on success, -1 on failure
*/
template <typename T> int8_t SensorFilter3<T>::_calculate_rms() {
  int8_t ret = -1;
  if (windowSize() > 0) {
    Vector3f64 squared_samples;
    for (uint i = 0; i < _window_size; i++) {
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
    _stale_rms = false;
    ret = 0;
  }
  return ret;
}


/**
* Calulates the standard deviation of the samples.
* Updates the private cache variable.
*
* @return 0 on success, -1 on failure
*/
template <typename T> int8_t SensorFilter3<T>::_calculate_stdev() {
  int8_t ret = -1;
  if ((_window_size > 1) && (nullptr != samples)) {
    Vector3f64 deviation_sum;
    if (_stale_mean) _calculate_mean();
    for (uint i = 0; i < _window_size; i++) {
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
    _stale_stdev = false;
    ret = 0;
  }
  return ret;
}


/*
* Calulates the median of the samples.
*/
template <typename T> int8_t SensorFilter3<T>::_calculate_median() {
  double sorted[3][_window_size];
  for (uint i = 0; i < _window_size; i++) {
    sorted[0][i] = samples[i].x;
    sorted[1][i] = samples[i].y;
    sorted[2][i] = samples[i].z;
  }
  // Selection sort.
  uint i = 0;
  double swap;
  for (uint8_t n = 0; n < 3; n++) {
    for (uint c = 0; c < (_window_size - 1); c++) {
      i = c;
      for (uint d = c + 1; d < _window_size; d++) {
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
    uint offset = (_window_size-1) >> 1;
    last_value.set(sorted[0][offset], sorted[1][offset], sorted[2][offset]);
  }
  else {
    // ...otherwise, we take the mean to the two middle values.
    uint lower = (_window_size-1) >> 1;
    uint upper = lower + 1;
    last_value.set(
      ((sorted[0][upper] + sorted[0][lower]) / 2),
      ((sorted[1][upper] + sorted[1][lower]) / 2),
      ((sorted[2][upper] + sorted[2][lower]) / 2)
    );
  }
  return 0;
}


#endif  // __SENSOR_FILTER_H__
