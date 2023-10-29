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

#include "CppPotpourri.h"
#include "SensorFilter.h"
#include "C3PValue/KeyValuePair.h"


/******************************************************************************
* Statics
******************************************************************************/

const char* const getFilterStr(FilteringStrategy x) {
  switch (x) {
    case FilteringStrategy::RAW:            return "RAW";
    case FilteringStrategy::MOVING_AVG:     return "MOVING_AVG";
    case FilteringStrategy::MOVING_MED:     return "MOVING_MED";
    case FilteringStrategy::HARMONIC_MEAN:  return "HARMONIC_MEAN";
    case FilteringStrategy::GEOMETRIC_MEAN: return "GEOMETRIC_MEAN";
    case FilteringStrategy::QUANTIZER:      return "QUANTIZER";
  }
  return "UNKNOWN";
}


/******************************************************************************
* SensorFilterBase
******************************************************************************/

SensorFilterBase::~SensorFilterBase() {
  if (_name) {
    free(_name);
    _name = nullptr;
  }
  if (_units) {
    free(_units);
    _units = nullptr;
  }
}


void SensorFilterBase::_print_filter_base(StringBuilder* output) {
  StringBuilder::styleHeader2(output, getFilterStr(strategy()));
  output->concatf("\tInitialized:  %c\n",   initialized() ? 'y':'n');
  output->concatf("\tStatic alloc: %c\n",   _static_alloc ? 'y':'n');
  output->concatf("\tDirty:        %c\n",   _filter_dirty ? 'y':'n');
  output->concatf("\tWindow size:  %u\n",   _window_size);
  output->concatf("\tWindow full:  %c\n",   _window_full  ? 'y':'n');
}


int8_t SensorFilterBase::name(char* n) {
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


int8_t SensorFilterBase::units(SIUnit* u) {
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


int8_t SensorFilterBase::serialize(StringBuilder* out, TCode format) {
  int8_t ret = -1;
  switch (format) {
    case TCode::BINARY:
      // TODO
      break;

    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        const uint32_t RANGE_TO_SERIALIZE = windowSize();  // TODO: Calculate from last dirty idx.

        uint8_t map_count = 4;
        if (_name) {           map_count++;   }
        if (_filter_initd) {   map_count++;   }

        encoder.write_string("SensorFilter");
        encoder.write_map(map_count);
        encoder.write_string("type");
          encoder.write_string("SensorFilter");
        if (_name) {
          encoder.write_string("name");
            encoder.write_string(_name);
        }
        encoder.write_string("win_sz");
          encoder.write_int((unsigned int) windowSize());
        encoder.write_string("total");
          encoder.write_int((unsigned int) totalSamples());
        encoder.write_string("strat");
          encoder.write_string(getFilterStr(strategy()));
        if (_filter_initd) {
          encoder.write_string("dat");
            encoder.write_array(RANGE_TO_SERIALIZE);
            uint32_t real_idx = ((RANGE_TO_SERIALIZE <= _sample_idx) ? _sample_idx : (_window_size + _sample_idx)) - RANGE_TO_SERIALIZE;
            for (uint32_t i = 0; i < RANGE_TO_SERIALIZE; i++) {
              _serialize_value(&encoder, (real_idx + i) % _window_size);
            }
            ret = 0;
          }
      }
      break;
    default:
      break;
  }
  return ret;
}


int8_t SensorFilterBase::deserialize(StringBuilder* raw, TCode format) {
  const uint8_t* SERDAT = raw->string();
  const uint32_t SERLEN = raw->length();
  int8_t ret = -1;
  switch (format) {
    case TCode::CBOR:
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
      break;

    default:
      break;
  }
  return ret;
}


/******************************************************************************
* SensorFilter<T> type specializations
******************************************************************************/
// CBOR handles integer length on its own.
// If someone wants to teach me a less obnoxious way to do this, please branch
//   and PR. I will probably take it.
template <> void SensorFilter<uint32_t>::_serialize_value(cbor::encoder* enc, uint32_t idx) {   enc->write_int(samples[idx]);  }
template <> void SensorFilter<uint32_t>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}
template <> void SensorFilter<uint16_t>::_serialize_value(cbor::encoder* enc, uint32_t idx) {   enc->write_int(samples[idx]);  }
template <> void SensorFilter<uint16_t>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}
template <> void SensorFilter<uint8_t>::_serialize_value(cbor::encoder* enc, uint32_t idx) {   enc->write_int(samples[idx]);  }
template <> void SensorFilter<uint8_t>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}

template <> void SensorFilter<int32_t>::_serialize_value(cbor::encoder* enc, uint32_t idx) {   enc->write_int(samples[idx]);  }
template <> void SensorFilter<int32_t>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}
template <> void SensorFilter<int16_t>::_serialize_value(cbor::encoder* enc, uint32_t idx) {   enc->write_int(samples[idx]);  }
template <> void SensorFilter<int16_t>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}
template <> void SensorFilter<int8_t>::_serialize_value(cbor::encoder* enc, uint32_t idx) {   enc->write_int(samples[idx]);  }
template <> void SensorFilter<int8_t>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}

template <> void SensorFilter<float>::_serialize_value(cbor::encoder* enc, uint32_t idx) {  enc->write_float(samples[idx]);  }
template <> void SensorFilter<float>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}
template <> void SensorFilter<double>::_serialize_value(cbor::encoder* enc, uint32_t idx) {  enc->write_double(samples[idx]);  }
template <> void SensorFilter<double>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}


// template <> void SensorFilter3<unsigned int>::_serialize_value(cbor::encoder* enc, uint32_t idx) {   enc->write_int(samples[idx]);  }
// template <> void SensorFilter3<unsigned int>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}

// template <> void SensorFilter3<int>::_serialize_value(cbor::encoder* enc, uint32_t idx) {    enc->write_int(samples[idx]);    }
// template <> void SensorFilter3<int>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}

// template <> void SensorFilter3<float>::_serialize_value(cbor::encoder* enc, uint32_t idx) {  enc->write_float(samples[idx]);  }
// template <> void SensorFilter3<float>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}

// template <> void SensorFilter3<double>::_serialize_value(cbor::encoder* enc, uint32_t idx) {  enc->write_double(samples[idx]);  }
// template <> void SensorFilter3<double>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}



template <> TCode SensorFilter<int8_t>::_value_tcode() {     return TCode::INT8;    };
template <> TCode SensorFilter<int16_t>::_value_tcode() {    return TCode::INT16;   };
template <> TCode SensorFilter<int32_t>::_value_tcode() {    return TCode::INT32;   };
template <> TCode SensorFilter<int64_t>::_value_tcode() {    return TCode::INT64;   };
template <> TCode SensorFilter<uint8_t>::_value_tcode() {    return TCode::UINT8;   };
template <> TCode SensorFilter<uint16_t>::_value_tcode() {   return TCode::UINT16;  };
template <> TCode SensorFilter<uint32_t>::_value_tcode() {   return TCode::UINT32;  };
template <> TCode SensorFilter<uint64_t>::_value_tcode() {   return TCode::UINT64;  };
template <> TCode SensorFilter<float>::_value_tcode() {      return TCode::FLOAT;   };
template <> TCode SensorFilter<double>::_value_tcode() {     return TCode::DOUBLE;  };

template <> TCode SensorFilter3<int8_t>::_value_tcode() {    return TCode::VECT_3_INT8;    };
template <> TCode SensorFilter3<int16_t>::_value_tcode() {   return TCode::VECT_3_INT16;   };
template <> TCode SensorFilter3<int32_t>::_value_tcode() {   return TCode::VECT_3_INT32;   };
template <> TCode SensorFilter3<uint8_t>::_value_tcode() {   return TCode::VECT_3_UINT8;   };
template <> TCode SensorFilter3<uint16_t>::_value_tcode() {  return TCode::VECT_3_UINT16;  };
template <> TCode SensorFilter3<uint32_t>::_value_tcode() {  return TCode::VECT_3_UINT32;  };
template <> TCode SensorFilter3<float>::_value_tcode() {     return TCode::VECT_3_FLOAT;   };
template <> TCode SensorFilter3<double>::_value_tcode() {    return TCode::VECT_3_DOUBLE;  };
