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
*   not yet have been called. So make no access attempts. Just set he pointer.
*/
TimeSeriesBase::TimeSeriesBase(const TCode TCODE, uint32_t ws, uint32_t flgs) :
  _value(TCODE), _window_size(ws),
  _samples_total(0), _sample_idx(0), _flags(flgs),
  _name(nullptr), _units(nullptr), _last_trace(0) {}


TimeSeriesBase::~TimeSeriesBase() {
  _flags.clear(TIMESERIES_FLAG_FILTER_INITD | TIMESERIES_FLAG_WINDOW_FULL);
  if (nullptr != _name) {
    free(_name);
    _name = nullptr;
  }
  if (nullptr != _units) {
    free(_units);
    _units = nullptr;
  }
}




void TimeSeriesBase::_print_series_base(StringBuilder* output) {
  StringBuilder::styleHeader2(output, name());
  output->concatf("\tInitialized:  %c\n",   initialized() ? 'y':'n');
  output->concatf("\tSelf alloc:   %c\n",   _self_allocated() ? 'y':'n');
  output->concatf("\tDirty:        %c\n",   dirty() ? 'y':'n');
  output->concatf("\tWindow size:  %u\n",   windowSize());
  output->concatf("\tWindow full:  %c\n",   windowFull()  ? 'y':'n');
  _value.toString(output, true);
  output->concat("\n");
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
      _name = (char*) malloc(NAME_LEN+1);
      ret--;
      if (_name) {
        memcpy(_name, n, NAME_LEN);
        *(_name+NAME_LEN) = 0;
        ret = 0;
      }
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
      _units = (SIUnit*) malloc(STR_LEN+1);
      ret--;
      if (_units) {
        memcpy(_units, u, STR_LEN+1);
        ret = 0;
      }
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

        encoder.write_string("TimeSeries");
        encoder.write_map(map_count);
        encoder.write_string("type");
          encoder.write_string("TimeSeries");
        if (_name) {
          encoder.write_string("name");
            encoder.write_string(_name);
        }
        encoder.write_string("win_sz");
          encoder.write_int(windowSize());
        encoder.write_string("total");
          encoder.write_int(totalSamples());
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


int8_t TimeSeriesBase::deserialize(StringBuilder* raw, TCode format) {
  //const uint8_t* SERDAT = raw->string();
  //const uint32_t SERLEN = raw->length();
  int8_t ret = -1;
  switch (format) {
    case TCode::CBOR:
      #if defined(__BUILD_HAS_CBOR)
      {
        //KeyValuePair* kvp = KeyValuePair::unserialize(raw->string(), raw->length(), TCode::CBOR);
        KeyValuePair* kvp = nullptr;
        CBORArgListener cl(&kvp);
        cbor::input_static input(raw->string(), raw->length());
        cbor::decoder decoder(input, cl);
        //decoder.run();
        if (nullptr != kvp) {
          //StringBuilder txt_output;
          //kvp->printDebug(&txt_output);
          //printf("%s\n", txt_output.string());
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
