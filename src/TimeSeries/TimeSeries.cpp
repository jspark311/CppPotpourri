/*
File:   TimeSeries.cpp
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

#include "TimeSeries.h"
#include "../CppPotpourri.h"
#include "../C3PValue/KeyValuePair.h"


/******************************************************************************
* TimeSeriesBase
******************************************************************************/

/*
* Protected constructor.
* NOTE: The C3PValue pointer will be correct when this constructor is called,
*   and will be assured to be non-null. However, the C3PValue constructor will
*   not yet have been called. So make no access attempts. Just set the pointer.
*/
TimeSeriesBase::TimeSeriesBase(const TCode TC, uint32_t ws, uint16_t flgs) :
  _window_size(ws), _samples_total(0), _sample_idx(0),
  _TCODE(TC), _flags(flgs), _last_trace(0),
  _name(nullptr), _units(nullptr) {}


TimeSeriesBase::~TimeSeriesBase() {
  _set_flags(false, TIMESERIES_FLAG_FILTER_INITD);
  if (nullptr != _name) {
    free(_name);
    _name = nullptr;
  }
  if (nullptr != _units) {
    free(_units);
    _units = nullptr;
  }
}


/*
* Given a sample index in memory, return the sample index (since last purge())
*   that wrote it.
* Example: if you have a sample at index 45, are recording the most-recent 300
*   samples, and there are 2319 samples that have have passed through the class,
*   this function should return 2193.
*
* @param MEM_IDX is the memory index containing the data of interest.
* @returns the computed sample index, or 0 if undefined or window not full.
*/
uint32_t TimeSeriesBase::indexIsWhichSample(const uint32_t MEM_IDX) {
  uint32_t ret = 0;
  if (initialized() & (_samples_total >= MEM_IDX)) {
    ret = (_samples_total - delta_assume_wrap(_sample_idx, MEM_IDX)) - 1;
  }
  return ret;
}


void TimeSeriesBase::printSeries(StringBuilder* output) {
  StringBuilder hdr;
  if (_name) {  hdr.concat(name());  }
  hdr.concatf("%s[%u]", typecodeToStr(_TCODE), windowSize());
  if (_units) {
    hdr.concat(" (");
    SIUnitToStr(_units, &hdr, false);
    hdr.concat(")");
  }
  StringBuilder tmp;
  StringBuilder::styleHeader2(&tmp, (char*) hdr.string());
  hdr.clear();
  tmp.concatf("\tInitialized:   %c\n", initialized() ? 'y':'n');
  tmp.concatf("\tSelf alloc:    %c\n", _self_allocated() ? 'y':'n');
  tmp.concatf("\tDirty:         %c\n", dirty() ? 'y':'n');
  tmp.concatf("\tWindow full:   %c\n", windowFull()  ? 'y':'n');
  tmp.concatf("\tTotal samples: %u\n", _samples_total);
  _print_series(&tmp);
  tmp.concat("\n");
  tmp.string();  // Consolidate heap
  output->concatHandoff(&tmp);
}


int8_t TimeSeriesBase::name(char* n) {
  int8_t ret = -1;
  if (_name) {
    free(_name);
    _name = nullptr;
  }
  if (n) {
    const uint32_t NAME_LEN = strlen(n);
    ret--;
    if (NAME_LEN > 0) {
      _name = StringBuilder::deep_copy(n, NAME_LEN);
      ret = (_name) ? 0 : (ret-1);
    }
  }
  return ret;
}


int8_t TimeSeriesBase::units(SIUnit* u) {
  int8_t ret = -1;
  if (_units) {
    free(_units);
    _units = nullptr;
  }
  if (u) {
    const uint32_t STR_LEN = strlen((char*) u);
    ret--;
    if (STR_LEN > 0) {
      _units = (SIUnit*) StringBuilder::deep_copy((char*) u, STR_LEN);
      ret = (_units) ? 0 : (ret-1);
    }
  }
  return ret;
}


int8_t TimeSeriesBase::serialize(StringBuilder* out, TCode format) {
  int8_t ret = -1;
  switch (format) {
    case TCode::BINARY:
      // TODO
      break;

    case TCode::CBOR:
      #if defined(__BUILD_HAS_CBOR)
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        const uint32_t RANGE_TO_SERIALIZE = windowSize();  // TODO: Calculate from last dirty idx.

        uint8_t map_count = 4;
        if (_name) {           map_count++;   }
        if (initialized()) {   map_count++;   }

        encoder.write_tag(C3P_CBOR_VENDOR_CODE | TcodeToInt(TCode::TIMESERIES));

        encoder.write_map(map_count);
        encoder.write_string("tc");    encoder.write_int(TcodeToInt(_TCODE));
        encoder.write_string("win");   encoder.write_int(windowSize());
        encoder.write_string("ttl");   encoder.write_int(totalSamples());
        if (_name) {
          encoder.write_string("n");   encoder.write_string(_name);
        }
        if (initialized()) {
          encoder.write_string("dat");
            encoder.write_array(RANGE_TO_SERIALIZE);
            uint32_t real_idx = ((RANGE_TO_SERIALIZE <= _sample_idx) ? _sample_idx : (_window_size + _sample_idx)) - RANGE_TO_SERIALIZE;
            for (uint32_t i = 0; i < RANGE_TO_SERIALIZE; i++) {
              _serialize_value(&encoder, (real_idx + i) % _window_size);
            }
            ret = 0;
          }
      }
      #endif
      break;
    default:
      break;
  }
  return ret;
}



/******************************************************************************
* TimeSeries<T> type specializations
******************************************************************************/
// CBOR handles integer length on its own.
// If someone wants to teach me a less obnoxious way to do this, please branch
//   and PR. I will probably take it.
#if defined(__BUILD_HAS_CBOR)
template <> void TimeSeries<uint32_t>::_serialize_value(cbor::encoder* enc, uint32_t idx) {   enc->write_int(samples[idx]);  }
template <> void TimeSeries<uint32_t>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}
template <> void TimeSeries<uint16_t>::_serialize_value(cbor::encoder* enc, uint32_t idx) {   enc->write_int(samples[idx]);  }
template <> void TimeSeries<uint16_t>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}
template <> void TimeSeries<uint8_t>::_serialize_value(cbor::encoder* enc, uint32_t idx) {   enc->write_int(samples[idx]);  }
template <> void TimeSeries<uint8_t>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}

template <> void TimeSeries<int32_t>::_serialize_value(cbor::encoder* enc, uint32_t idx) {   enc->write_int(samples[idx]);  }
template <> void TimeSeries<int32_t>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}
template <> void TimeSeries<int16_t>::_serialize_value(cbor::encoder* enc, uint32_t idx) {   enc->write_int(samples[idx]);  }
template <> void TimeSeries<int16_t>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}
template <> void TimeSeries<int8_t>::_serialize_value(cbor::encoder* enc, uint32_t idx) {   enc->write_int(samples[idx]);  }
template <> void TimeSeries<int8_t>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}

template <> void TimeSeries<float>::_serialize_value(cbor::encoder* enc, uint32_t idx) {  enc->write_float(samples[idx]);  }
template <> void TimeSeries<float>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}
template <> void TimeSeries<double>::_serialize_value(cbor::encoder* enc, uint32_t idx) {  enc->write_double(samples[idx]);  }
template <> void TimeSeries<double>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}


// template <> void TimeSeries3<unsigned int>::_serialize_value(cbor::encoder* enc, uint32_t idx) {   enc->write_int(samples[idx]);  }
// template <> void TimeSeries3<unsigned int>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}

template <> void TimeSeries3<float>::_serialize_value(cbor::encoder* enc, uint32_t idx) {
  enc->write_array(3);
  enc->write_float(samples[idx].x);
  enc->write_float(samples[idx].y);
  enc->write_float(samples[idx].z);
}
template <> void TimeSeries3<float>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}

// template <> void TimeSeries3<double>::_serialize_value(cbor::encoder* enc, uint32_t idx) {  enc->write_double(samples[idx]);  }
// template <> void TimeSeries3<double>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}
#endif
