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
* NOTE: This function returns 0 to indicate the first sample to arrive.
*
* @param MEM_IDX is the memory index containing the data of interest.
* @returns the computed sample index, or 0 if undefined or window not full.
*/
uint32_t TimeSeriesBase::indexIsWhichSample(const uint32_t MEM_IDX) {
  uint32_t ret = 0;
  if (initialized() & (_samples_total > MEM_IDX)) {
    const uint32_t BACKSET_SAMPLE_IDX = (((0 == _sample_idx) ? _window_size : _sample_idx) - 1);
    const uint32_t IDX_DELTA = ((BACKSET_SAMPLE_IDX >= MEM_IDX) ? BACKSET_SAMPLE_IDX : (_window_size + BACKSET_SAMPLE_IDX)) - MEM_IDX;
    ret = ((_samples_total - IDX_DELTA) - 1);
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



/******************************************************************************
* TimeSeries<T> type specializations
******************************************************************************/
// CBOR handles integer length on its own.
// If someone wants to teach me a less obnoxious way to do this, please branch
//   and PR. I will probably take it.
#if defined(__BUILD_HAS_CBOR)
template <> void TimeSeries<uint64_t>::_serialize_value(cbor::encoder* enc, uint32_t idx) {   enc->write_int(samples[idx]);  }
template <> void TimeSeries<uint64_t>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}
template <> void TimeSeries<uint32_t>::_serialize_value(cbor::encoder* enc, uint32_t idx) {   enc->write_int(samples[idx]);  }
template <> void TimeSeries<uint32_t>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}
template <> void TimeSeries<uint16_t>::_serialize_value(cbor::encoder* enc, uint32_t idx) {   enc->write_int(samples[idx]);  }
template <> void TimeSeries<uint16_t>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}
template <> void TimeSeries<uint8_t>::_serialize_value(cbor::encoder* enc, uint32_t idx) {   enc->write_int(samples[idx]);  }
template <> void TimeSeries<uint8_t>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}

template <> void TimeSeries<int64_t>::_serialize_value(cbor::encoder* enc, uint32_t idx) {   enc->write_int(samples[idx]);  }
template <> void TimeSeries<int64_t>::_deserialize_value(cbor::encoder* enc, uint32_t idx) {}
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




/*******************************************************************************
* C3PTypeConstraint
*******************************************************************************/

template <> int C3PTypeConstraint<TimeSeriesBase*>::serialize(void* _obj, StringBuilder* out, const TCode FORMAT) {
  int ret = -1;
  if (nullptr == _obj) {  return ret;  }
  TimeSeriesBase* obj = (TimeSeriesBase*) _obj;

  switch (FORMAT) {
    case TCode::STR:
      ret = 0;
      break;

    case TCode::BINARY:
      break;

    #if defined(__BUILD_HAS_CBOR)
    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        const uint32_t RANGE_TO_SERIALIZE = obj->windowSize();  // TODO: Calculate from last dirty idx?

        uint8_t map_count = (obj->windowFull() ? 5:3);
        if (obj->_name) {   map_count++;  }
        if (obj->_units) {  map_count++;  }

        encoder.write_tag(C3P_CBOR_VENDOR_CODE | TcodeToInt(TCode::TIMESERIES));
        encoder.write_map(map_count);
        encoder.write_string("tc");    encoder.write_int(TcodeToInt(obj->tcode()));
        encoder.write_string("win");   encoder.write_int(obj->windowSize());
        encoder.write_string("ttl");   encoder.write_int(obj->totalSamples());
        if (obj->_name) {
          encoder.write_string("n");   encoder.write_string(obj->name());
        }
        if (obj->_units) {
          encoder.write_string("u");   encoder.write_string((char*) obj->units());
        }
        if (obj->windowFull()) {
          // If the window is full, the data is worth sending. But first, we
          //   should write the absolute offset of the starting sample index so
          //   that the parser can know where in the series this range belongs.
          // NOTE: Line belong assumes we're sending all of it.
          // TODO: This arrangement will need to mutate soon. It is already
          //   under breaking selective pressure.
          //C3PType* t_helper = getTypeHelper(_TCODE);
          const uint32_t PACKER_ABS_IDX_START = (obj->totalSamples() - RANGE_TO_SERIALIZE);
          encoder.write_string("idx");   encoder.write_int(PACKER_ABS_IDX_START);
          encoder.write_string("dat");
            encoder.write_array(RANGE_TO_SERIALIZE);
            uint32_t real_idx = ((RANGE_TO_SERIALIZE <= obj->_sample_idx) ? obj->_sample_idx : (obj->_window_size + obj->_sample_idx)) - RANGE_TO_SERIALIZE;
            for (uint32_t i = 0; i < RANGE_TO_SERIALIZE; i++) {
              obj->_serialize_value(&encoder, (real_idx + i) % obj->_window_size);
            }
          ret = 0;
        }
      }
      break;
    #endif  // __BUILD_HAS_CBOR

    default:  break;
  }
  return ret;
}


template <> int8_t C3PTypeConstraint<TimeSeriesBase*>::construct(void* _obj, KeyValuePair* kvp) {
  int8_t ret = -1;
  if ((nullptr != _obj) & (nullptr != kvp)) {
    ret--;
    // Always take win, because it may indicate a re-windowing.
    uint32_t win_sz = 0;
    const bool CONTAINED_WIN_KEY = (0 == kvp->valueWithKey("win", &win_sz));

    TimeSeriesBase* obj = *((TimeSeriesBase**) _obj);
    if (nullptr == obj) {
      // TimeSeriesBase requires tc in order to allocate, and having windowSize
      //   up-front is also helpful (but not required). If the key isn't in the
      //   received structure, it will be left as zero.
      TCode tc_val = TCode::NONE;
      if (0 == kvp->valueWithKey("tc", (uint8_t*) &tc_val)) {
        switch (tc_val) {
          case TCode::UINT8:   obj = (TimeSeriesBase*) new TimeSeries<uint8_t>(win_sz);   break;
          case TCode::UINT16:  obj = (TimeSeriesBase*) new TimeSeries<uint16_t>(win_sz);  break;
          case TCode::UINT32:  obj = (TimeSeriesBase*) new TimeSeries<uint32_t>(win_sz);  break;
          case TCode::UINT64:  obj = (TimeSeriesBase*) new TimeSeries<uint64_t>(win_sz);  break;
          case TCode::INT8:    obj = (TimeSeriesBase*) new TimeSeries<int8_t>(win_sz);    break;
          case TCode::INT16:   obj = (TimeSeriesBase*) new TimeSeries<int16_t>(win_sz);   break;
          case TCode::INT32:   obj = (TimeSeriesBase*) new TimeSeries<int32_t>(win_sz);   break;
          case TCode::INT64:   obj = (TimeSeriesBase*) new TimeSeries<int64_t>(win_sz);   break;
          case TCode::FLOAT:   obj = (TimeSeriesBase*) new TimeSeries<float>(win_sz);     break;
          case TCode::DOUBLE:  obj = (TimeSeriesBase*) new TimeSeries<double>(win_sz);    break;
          default:
            break;
        }
      }
      *((TimeSeriesBase**) _obj) = obj; // And assign.
      if (nullptr != obj) {
        if (0 < win_sz) {
          obj->init();  // Initialize if we have everything needed.
        }
      }
    }
    if (nullptr != obj) {
      ret = 0;
      // The specific ordering of key observation matters. If the packer placed
      //   dat ahead of win, we don't want to wipe out the values we just placed
      //   on accident (for instance).
      // First: Do the order invariant things.
      char* name_val   = nullptr;
      char* unit_val   = nullptr;
      kvp->valueWithKey("n", &name_val);
      kvp->valueWithKey("u", &unit_val);
      if (name_val) {  obj->name(name_val);             }
      if (unit_val) {  obj->units((SIUnit*) unit_val);  }

      // Next, look for indications that the window size should be set, and
      //   change it if needed.
      if (CONTAINED_WIN_KEY & (win_sz != obj->windowSize())) {
        obj->windowSize(win_sz);  // This will initialize, as well.
      }

      kvp->valueWithKey("ttl", &(obj->_samples_total));

      uint32_t idx_val = 0;
      C3PValue* dat_val = kvp->valueWithKey("dat");
      const bool CONTAINED_IDX_KEY = (0 == kvp->valueWithKey("idx", &idx_val));
      if (CONTAINED_IDX_KEY & (nullptr != dat_val)) {
        // The dat and idx keys go together, and refer to the (samples)
        //   beginning at the (absolute index), respectively.
        // But because we took ttl already, we need to do a direct mem
        //   manipulation, rather than calling feedSeries().
        const uint32_t SAMPLE_COUNT = strict_min(dat_val->count(), obj->windowSize());
        const uint32_t ADJUSTED_IDX = idx_val + (SAMPLE_COUNT - dat_val->count());
        const uint32_t SIZE_OF_TYPE = sizeOfType(obj->tcode());
        uint32_t count = 0;
        while ((nullptr != dat_val) && (count < SAMPLE_COUNT)) {
          const uint32_t ABS_MEM_IDX  = (count + (obj->_samples_total - ADJUSTED_IDX)) % obj->windowSize();
          // NOTE: The strange casting dance below is to convince the compiler
          //   that we intend to increment the pointer by some byte quantity (as
          //   opposed to the size of some multi-byte type).
          void* raw_ptr = (void*) (((uint8_t*) obj->_mem_raw_ptr()) + (ABS_MEM_IDX * SIZE_OF_TYPE));
          dat_val->get_as(obj->tcode(), raw_ptr);
          dat_val = dat_val->nextValue();
          count++;
        }
      }
    }
  }
  return ret;
}


template <> void C3PTypeConstraint<TimeSeriesBase*>::to_string(void* _obj, StringBuilder* out) {
  C3PTypeConstraint<TimeSeriesBase*>::serialize(_obj, out, TCode::STR);
}
