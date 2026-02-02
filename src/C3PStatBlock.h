/*
File:   C3PStatBlock.h
Author: J. Ian Lindsay
Date:   2026.01.17


Consolidated statistical measurement template.

This class was intended to be extended by a class that held arbitrarilly-sized
  collections of numeric elements. With a little cooperation from the child
  class, it avoids spending more time than strictly necessary to calculate
  stats. Given the datasets, that might be a substantial amount of time.
*/

#ifndef __C3P_STATBLOCK_H__
#define __C3P_STATBLOCK_H__

#include "CppPotpourri.h"
#include "StringBuilder.h"

#define STATBLOCK_FLAG_VALID_SNR      0x04  // Statistical measurement is valid.
#define STATBLOCK_FLAG_VALID_MINMAX   0x08  // Statistical measurement is valid.
#define STATBLOCK_FLAG_VALID_MEAN     0x10  // Statistical measurement is valid.
#define STATBLOCK_FLAG_VALID_RMS      0x20  // Statistical measurement is valid.
#define STATBLOCK_FLAG_VALID_STDEV    0x40  // Statistical measurement is valid.
#define STATBLOCK_FLAG_VALID_MEDIAN   0x80  // Statistical measurement is valid.

#define STATBLOCK_FLAG_MASK_ALL_STATS ( \
  STATBLOCK_FLAG_VALID_MINMAX | STATBLOCK_FLAG_VALID_MEAN | \
  STATBLOCK_FLAG_VALID_RMS | STATBLOCK_FLAG_VALID_STDEV | \
  STATBLOCK_FLAG_VALID_MEDIAN | STATBLOCK_FLAG_VALID_SNR)



template <class T> class C3PStatBlock {
  public:
    inline void     invalidateStats() {    _set_flags(false, STATBLOCK_FLAG_MASK_ALL_STATS);  };

    /* Value accessor inlines */
    inline T        minValue() {   if (_stale_minmax()) _calculate_minmax();  return _min_value;  };
    inline T        maxValue() {   if (_stale_minmax()) _calculate_minmax();  return _max_value;  };
    inline double   mean() {       if (_stale_mean())   _calculate_mean();    return _mean;       };
    inline double   rms() {        if (_stale_rms())    _calculate_rms();     return _rms;        };
    inline double   stdev() {      if (_stale_stdev())  _calculate_stdev();   return _stdev;      };
    inline T        median() {     if (_stale_median()) _calculate_median();  return _median;     };
    inline double   snr() {        if (_stale_snr())    _calculate_snr();     return _snr;        };


  protected:
    T*       _samples;
    uint32_t _n;
    T        _min_value;
    T        _max_value;
    T        _median;
    double   _mean;
    double   _rms;
    double   _stdev;
    double   _snr;

    C3PStatBlock(T* SAMPLES = nullptr, uint32_t N_VAL = 0) :
      _samples(nullptr), _n(0),
      _min_value(T(0)), _max_value(T(0)),
      _median(T(0)), _mean(0.0d),
      _rms(0.0d), _stdev(0.0d),
      _snr(0.0d), _flags(0) {};


    int8_t  _set_stat_source_data(T* buf, const uint32_t N_VAL);
    int8_t  _calculate_minmax();
    int8_t  _calculate_mean();
    int8_t  _calculate_rms();
    int8_t  _calculate_stdev();
    int8_t  _calculate_median();
    int8_t  _calculate_snr();

    void _print_stats(StringBuilder*);

    /* Semantic breakouts for flags */
    inline bool _stale_minmax() {    return !(_chk_flags(STATBLOCK_FLAG_VALID_MINMAX));  };
    inline bool _stale_mean() {      return !(_chk_flags(STATBLOCK_FLAG_VALID_MEAN));    };
    inline bool _stale_rms() {       return !(_chk_flags(STATBLOCK_FLAG_VALID_RMS));     };
    inline bool _stale_stdev() {     return !(_chk_flags(STATBLOCK_FLAG_VALID_STDEV));   };
    inline bool _stale_median() {    return !(_chk_flags(STATBLOCK_FLAG_VALID_MEDIAN));  };
    inline bool _stale_snr() {       return !(_chk_flags(STATBLOCK_FLAG_VALID_SNR));     };
    inline void _set_flags(bool x, const uint16_t MSK) {  _flags = (x ? (_flags | MSK) : (_flags & ~MSK)); };
    inline bool _chk_flags(const uint16_t MSK) {          return (MSK == (_flags & MSK));                  };


  private:
    uint8_t     _flags;       // Class behavior flags.
};



template <class T> void C3PStatBlock<T>::_print_stats(StringBuilder* output) {
  C3PType* t_helper = getTypeHelper(tcodeForType(T(0)));
  StringBuilder tmp_sb;

  tmp_sb.concatf("\tN      = %u\n", _n);
  if (t_helper) {
    T max_val = maxValue();
    T min_val = minValue();
    T med_val = median();

    tmp_sb.concat("\tMin    = ");
    t_helper->to_string((void*) &min_val, &tmp_sb);
    tmp_sb.concat("\n\tMax    = ");
    t_helper->to_string((void*) &max_val, &tmp_sb);
    tmp_sb.concat("\n\tMedian = ");
    t_helper->to_string((void*) &med_val, &tmp_sb);
  }
  tmp_sb.concatf("\n\tMEAN   = %.8f\n", mean());
  tmp_sb.concatf("\tRMS    = %.8f\n", rms());
  tmp_sb.concatf("\tSTDEV  = %.8f\n", stdev());
  tmp_sb.concatf("\tSNR    = %.8f\n", snr());
  tmp_sb.string();   // Consolidate heap...
  output->concatHandoff(&tmp_sb);
}


/*
* This classs owns no memory. Only reads it. So if the child class ever changes
*   its memory range, this function will need to be called, or crashes will
*   happen.
*/
template <class T> int8_t C3PStatBlock<T>::_set_stat_source_data(T* buf, const uint32_t N_VAL) {
  _samples = buf;
  _n = N_VAL;
  return (((N_VAL > 1) && (nullptr != buf)) ? 0 : -1);
}


/**
* Calulates the min/max over the entire sample window.
* Updates the private cache variable.
*
* @return 0 on success, -1 if the window isn't full.
*/
template <class T> int8_t C3PStatBlock<T>::_calculate_minmax() {
  int8_t ret = -1;
  if (nullptr != _samples) {
    ret = 0;
    _min_value = _samples[0];  // Start with a baseline.
    _max_value = _samples[0];  // Start with a baseline.
    for (uint32_t i = 1; i < _n; i++) {
      if (_samples[i] > _max_value) _max_value = _samples[i];
      else if (_samples[i] < _min_value) _min_value = _samples[i];
    }
    _set_flags(true, STATBLOCK_FLAG_VALID_MINMAX);
  }
  return ret;
}


/**
* Calulates the statistical mean over the entire sample window.
* Updates the private cache variable.
*
* @return 0 on success, -1 if the window isn't full.
*/
template <class T> int8_t C3PStatBlock<T>::_calculate_mean() {
  int8_t ret = -1;
  if (nullptr != _samples) {
    ret = 0;
    double summed_samples = 0.0;
    for (uint32_t i = 0; i < _n; i++) {
      summed_samples += (double) _samples[i];
    }
    _mean = (summed_samples / _n);
    _set_flags(true, STATBLOCK_FLAG_VALID_MEAN);
  }
  return ret;
}


/**
* Calulates the RMS over the entire sample window.
* Updates the private cache variable.
*
* @return 0 on success, -1 if the window isn't full.
*/
template <class T> int8_t C3PStatBlock<T>::_calculate_rms() {
  int8_t ret = -1;
  if (nullptr != _samples) {
    ret = 0;
    double squared_samples = 0.0;
    for (uint32_t i = 0; i < _n; i++) {
      squared_samples += pow((double) _samples[i], 2.0);
    }
    _rms = sqrt(squared_samples / _n);
    _set_flags(true, STATBLOCK_FLAG_VALID_RMS);
  }
  return ret;
}


/**
* Calulates the standard deviation of the samples.
* Updates the private cache variable.
*
* NOTE: Since it is not concerned with extimating stdev against a wider
*   population, this implementation does not use Bessel's correction.
*   https://en.wikipedia.org/wiki/Bessel%27s_correction
*
* @return 0 on success, -1 if the window isn't full.
*/
template <class T> int8_t C3PStatBlock<T>::_calculate_stdev() {
  int8_t ret = -1;
  if (nullptr != _samples) {
    ret = 0;
    double deviation_sum = 0.0;
    double cached_mean = mean();
    for (uint32_t i = 0; i < _n; i++) {
      double tmp = ((double) _samples[i] - cached_mean);
      deviation_sum += pow((double) tmp, 2.0);
    }
    _stdev = sqrt(deviation_sum / _n);
    _set_flags(true, STATBLOCK_FLAG_VALID_STDEV);
  }
  return ret;
}


/**
* Calulates the median value of the samples.
* Updates the private cache variable.
*
* @return 0 on success, -1 if the window isn't full.
*/
template <class T> int8_t C3PStatBlock<T>::_calculate_median() {
  int8_t ret = -1;
  if (nullptr != _samples) {
    ret = 0;
    T sorted[_n];
    for (uint32_t i = 0; i < _n; i++) {
      sorted[i] = _samples[i];
    }
    // Selection sort.
    uint32_t i  = 0;
    T swap;
    for (uint32_t c = 0; c < (_n - 1); c++) {
      i = c;
      for (uint32_t d = c + 1; d < _n; d++) {
        if (sorted[i] > sorted[d]) i = d;
      }
      if (i != c) {
        swap = sorted[c];
        sorted[c] = sorted[i];
        sorted[i] = swap;
      }
    }

    if (_n & 0x01) {
      // If there are an odd number of samples...
      _median = sorted[(_n-1) >> 1];
    }
    else {
      // ...otherwise, we take the mean to the two middle values.
      uint32_t lower = (_n-1) >> 1;
      uint32_t upper = lower + 1;
      _median = (sorted[upper] + sorted[lower]) / 2;
    }
    _set_flags(true, STATBLOCK_FLAG_VALID_MEDIAN);
  }
  return ret;
}


/**
* Calulates the signal-to-noise ratio of the samples.
*
* @return 0 on success, -1 if the window isn't full.
*/
template <class T> int8_t C3PStatBlock<T>::_calculate_snr() {
  int8_t ret = -1;
  if (nullptr != _samples) {
    ret = 0;
    _snr = (pow(mean(), 2.0) / pow(stdev(), 2.0));
    _set_flags(true, STATBLOCK_FLAG_VALID_SNR);
  }
  return ret;
}


#endif  // __C3P_STATBLOCK_H__
