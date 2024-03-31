/*
File:   C3PTypePipeTests.cpp
Author: J. Ian Lindsay
Date:   2024.03.28

Copyright 2016 Manuvr, Inc

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

#include "Pipes/BufferAccepter/C3PTypePipe/C3PTypePipe.h"


/*
* This is a test class meant to generate a type rainbow for the sake of testing
*   various pathways in the library.
*/
class TestValuePalette {
  public:
    const uint32_t TEST_BUF_LEN;
    const bool     TEST_VAL_BOOL;
    const uint8_t  TEST_VAL_UINT8;
    const int8_t   TEST_VAL_INT8;
    const uint16_t TEST_VAL_UINT16;
    const int16_t  TEST_VAL_INT16;
    const uint32_t TEST_VAL_UINT32;
    const int32_t  TEST_VAL_INT32;
    const uint64_t TEST_VAL_UINT64;
    const int64_t  TEST_VAL_INT64;
    const float    TEST_VAL_FLOAT;
    const double   TEST_VAL_DOUBLE;
    StringBuilder  TEST_VAL_STRING;   // NOTE: Not a const. Keeping with the lexical pattern.

    C3PValue test_val_bool;
    C3PValue test_val_uint8;
    C3PValue test_val_int8;
    C3PValue test_val_uint16;
    C3PValue test_val_int16;
    C3PValue test_val_uint32;
    C3PValue test_val_int32;
    C3PValue test_val_uint64;
    C3PValue test_val_int64;
    C3PValue test_val_float;
    C3PValue test_val_double;
    //C3PValue test_val_string((char*) TEST_VAL_STRING.string());

    TestValuePalette(const uint32_t TEST_STR_LEN, const uint32_t TEST_STR_LEN_FUZZ) :
      TEST_BUF_LEN(TEST_STR_LEN + (randomUInt32() % TEST_STR_LEN_FUZZ)),
      TEST_VAL_BOOL(flip_coin()),
      TEST_VAL_UINT8((uint8_t)  randomUInt32()),
      TEST_VAL_INT8((int8_t)   randomUInt32()),
      TEST_VAL_UINT16((uint16_t) randomUInt32()),
      TEST_VAL_INT16((int16_t)  randomUInt32()),
      TEST_VAL_UINT32(randomUInt32()),
      TEST_VAL_INT32((int32_t)  randomUInt32()),
      TEST_VAL_UINT64(generate_random_uint64()),
      TEST_VAL_INT64(generate_random_int64()),
      TEST_VAL_FLOAT(generate_random_float()),
      TEST_VAL_DOUBLE(generate_random_double()),
      test_val_bool(TEST_VAL_BOOL),
      test_val_uint8(TEST_VAL_UINT8),
      test_val_int8(TEST_VAL_INT8),
      test_val_uint16(TEST_VAL_UINT16),
      test_val_int16(TEST_VAL_INT16),
      test_val_uint32(TEST_VAL_UINT32),
      test_val_int32(TEST_VAL_INT32),
      test_val_uint64(TEST_VAL_UINT64),
      test_val_int64(TEST_VAL_INT64),
      test_val_float(TEST_VAL_FLOAT),
      test_val_double(TEST_VAL_DOUBLE),
      _parsed_val_bool(!TEST_VAL_BOOL),   // NOTE: Always inverted to prevent false success.
      _parsed_val_uint8(0),
      _parsed_val_int8(0),
      _parsed_val_uint16(0),
      _parsed_val_int16(0),
      _parsed_val_uint32(0),
      _parsed_val_int32(0),
      _parsed_val_uint64(0),
      _parsed_val_int64(0),
      _parsed_val_float(0.0),
      _parsed_val_double(0.0),
      _parsed_val_string(nullptr),
      _tcode_expect(TCode::NONE)
    {
      generate_random_text_buffer(&TEST_VAL_STRING,  (TEST_BUF_LEN-1));
    };

    ~TestValuePalette() {};

    inline TCode expectedTCode() {          return _tcode_expect;  };
    inline void  expectedTCode(TCode x) {   _tcode_expect = x;     };

    int8_t setResultValue(C3PValue* val) {
      int8_t ret = -2;
      switch (_disambiguate_tcode(val)) {
        case TCode::BOOLEAN:   ret = (0 == val->get_as(&_parsed_val_bool))   ? 0 : -1;  break;
        case TCode::UINT8:     ret = (0 == val->get_as(&_parsed_val_uint8))  ? 0 : -1;  break;
        case TCode::INT8:      ret = (0 == val->get_as(&_parsed_val_int8))   ? 0 : -1;  break;
        case TCode::UINT16:    ret = (0 == val->get_as(&_parsed_val_uint16)) ? 0 : -1;  break;
        case TCode::INT16:     ret = (0 == val->get_as(&_parsed_val_int16))  ? 0 : -1;  break;
        case TCode::UINT32:    ret = (0 == val->get_as(&_parsed_val_uint32)) ? 0 : -1;  break;
        case TCode::INT32:     ret = (0 == val->get_as(&_parsed_val_int32))  ? 0 : -1;  break;
        case TCode::UINT64:    ret = (0 == val->get_as(&_parsed_val_uint64)) ? 0 : -1;  break;
        case TCode::INT64:     ret = (0 == val->get_as(&_parsed_val_int64))  ? 0 : -1;  break;
        case TCode::FLOAT:     ret = (0 == val->get_as(&_parsed_val_float))  ? 0 : -1;  break;
        case TCode::DOUBLE:    ret = (0 == val->get_as(&_parsed_val_double)) ? 0 : -1;  break;
        case TCode::STR:       ret = (0 == val->get_as(&_parsed_val_string)) ? 0 : -1;  break;
        default:  break;
      }
      if (0 != ret) {
        printf("TestValuePalette::setResultValue() returns failure (%d).\n", ret);
      }
      return ret;
    };


    bool allValuesMatch() {
      bool ret = true;
      ret &= (TEST_VAL_BOOL    == _parsed_val_bool);
      ret &= (TEST_VAL_UINT8   == _parsed_val_uint8);
      ret &= (TEST_VAL_INT8    == _parsed_val_int8);
      ret &= (TEST_VAL_UINT16  == _parsed_val_uint16);
      ret &= (TEST_VAL_INT16   == _parsed_val_int16);
      ret &= (TEST_VAL_UINT32  == _parsed_val_uint32);
      ret &= (TEST_VAL_INT32   == _parsed_val_int32);
      ret &= (TEST_VAL_UINT64  == _parsed_val_uint64);
      ret &= (TEST_VAL_INT64   == _parsed_val_int64);
      ret &= (TEST_VAL_FLOAT   == _parsed_val_float);
      ret &= (TEST_VAL_DOUBLE  == _parsed_val_double);
      // TODO: String checks.
      return ret;
    };


    void dumpTestValues() {
      printf("TestValuePalette:\n\tReference / Result:\n");
      printf("\t%c / %c\n", (TEST_VAL_BOOL ? 't':'f'), (_parsed_val_bool ? 't':'f'));
      printf("\t%u / %u\n",     TEST_VAL_UINT8,    _parsed_val_uint8);
      printf("\t%d / %d\n",     TEST_VAL_INT8,     _parsed_val_int8);
      printf("\t%u / %u\n",     TEST_VAL_UINT16,   _parsed_val_uint16);
      printf("\t%d / %d\n",     TEST_VAL_INT16,    _parsed_val_int16);
      printf("\t%u / %u\n",     TEST_VAL_UINT32,   _parsed_val_uint32);
      printf("\t%d / %d\n",     TEST_VAL_INT32,    _parsed_val_int32);
      printf("\t%llu / %llu\n", TEST_VAL_UINT64,   _parsed_val_uint64);
      printf("\t%lld / %lld\n", TEST_VAL_INT64,    _parsed_val_int64);
      printf("\t%.3f / %.3f\n", TEST_VAL_FLOAT,    _parsed_val_float);
      printf("\t%.6f / %.6f\n", TEST_VAL_DOUBLE,   _parsed_val_double);
      //printf("\t%s / %s\n", TEST_VAL_STRING.string(), (nullptr == _parsed_val_string) ? "(null)" : _parsed_val_string);
    }


  private:
    bool     _parsed_val_bool;
    uint8_t  _parsed_val_uint8;
    int8_t   _parsed_val_int8;
    uint16_t _parsed_val_uint16;
    int16_t  _parsed_val_int16;
    uint32_t _parsed_val_uint32;
    int32_t  _parsed_val_int32;
    uint64_t _parsed_val_uint64;
    int64_t  _parsed_val_int64;
    float    _parsed_val_float;
    double   _parsed_val_double;
    char*    _parsed_val_string;
    TCode    _tcode_expect;


    TCode    _disambiguate_tcode(C3PValue* val) {
      TCode ret = _tcode_expect;
      if (TCode::NONE == ret) {
        ret = val->tcode();
        switch (ret) {
          case TCode::UINT8:   case TCode::INT8:
          case TCode::UINT16:  case TCode::INT16:
          case TCode::UINT32:  case TCode::INT32:
          case TCode::UINT64:  case TCode::INT64:
            break;      // Continue along to integer disambiguation.
          default:
            return ret; // Bailout. TCode is unambiguous in any encoding scheme.
        }

        // With no expectations, we need to use the true value of the integer,
        //   and compare against the reference values to decide on its proper
        //   slot.
        // This is not optimal, but largely unavoidable, as many encodings will
        //   destroy the fine-grained type information about the integer type
        //   that produced the encoded value, and will replace it with the
        //   smallest type that fit the encoded data.
        // In a normal program, the parsing side should only care that it is an
        //   integer that it can accept, and should never do ordinal selection
        //   based on the integer type itself (as this code is doing). But this
        //   is exactly the sort of behavior under test.
        uint64_t full_width_int = 0;
        if (0 == val->get_as(&full_width_int)) {
          // Ugly if-else chain is needed. And it is important that it proceed
          //   in order of the least-likely matches first.
          if (TEST_VAL_UINT64 == full_width_int) {  ret = TCode::UINT64;  }
          else if (TEST_VAL_INT64  == (int64_t)  full_width_int) {  ret = TCode::INT64;   }
          else if (TEST_VAL_UINT32 == (uint32_t) full_width_int) {  ret = TCode::UINT32;  }
          else if (TEST_VAL_INT32  == (int32_t)  full_width_int) {  ret = TCode::INT32;   }
          else if (TEST_VAL_UINT16 == (uint16_t) full_width_int) {  ret = TCode::UINT16;  }
          else if (TEST_VAL_INT16  == (int16_t)  full_width_int) {  ret = TCode::INT16;   }
          else if (TEST_VAL_UINT8  == (uint8_t)  full_width_int) {  ret = TCode::UINT8;   }
          else if (TEST_VAL_INT8   == (int8_t)   full_width_int) {  ret = TCode::INT8;    }
          // Fall-back is to trust the decoder's judgement. This will likely
          //   culminate in a downstream failure from an upstream cause.
          else {}
        }
      }

      return ret;
    }
};



/*******************************************************************************
* C3PTypePipe callback and globals
*******************************************************************************/
TestValuePalette* c3ptp_test_values = nullptr;
KeyValuePair*     c3ptp_test_kvp    = nullptr;
int c3ptp_callback_err     = 0;
int c3ptp_callback_err_os  = 0;
int c3ptp_callback_err_kvp = 0;


// Callback for basic testing.
void c3ptype_arrival_callback(C3PValue* val) {
  if (nullptr != val) {
    StringBuilder output;
    val->toString(&output);
    printf("\t\tValue delivered: (%s) %s\n", typecodeToStr(val->tcode()), (char*) output.string());
    if (nullptr != c3ptp_test_values) {
      c3ptp_callback_err -= c3ptp_test_values->setResultValue(val);
    }
    else {
      printf("Value delivery callback took no action.\n");
      c3ptp_callback_err--;
    }
    // It is the responsibility of the callback to handle memory cleanup.
    delete val;
  }
  else {
    printf("Value delivery callback was given a nullptr. Failure.\n");
    c3ptp_callback_err--;
  }
}


// Callback for oversize failure case.
void c3ptype_callback_oversize(C3PValue* val) {
  if (nullptr != val) {
    printf("\t\tValue delivered: (%s) of length (%d)\n", typecodeToStr(val->tcode()), val->length());
    c3ptp_callback_err_os += val->length();
  }
  else {
    printf("Value delivery callback was given a nullptr. Failure.\n");
    c3ptp_callback_err_os = -1;
  }
}


// Callback for oversize failure case.
void c3ptype_callback_kvp(C3PValue* val) {
  bool cb_success = false;
  if (nullptr != val) {
    if (TCode::KVP == val->tcode()) {
      KeyValuePair* result_kvp = nullptr;
      if (nullptr != c3ptp_test_kvp) {
        if (0 == val->get_as(&result_kvp)) {
          if (result_kvp->count() == c3ptp_test_kvp->count()) {
            // TODO: Break apart and test each element.
            cb_success = true;
          }
        }
        else {
          printf("\t\tFailed to get KVP from value.\n");
        }
      }
      else {
        printf("\t\tNo reference KVP to compare against.\n");
      }
    }
    else {
      printf("\t\tValue delivered was (%s), which is not KVP.\n", typecodeToStr(val->tcode()));
    }
    // It is the responsibility of the callback to handle memory cleanup.
    delete val;
  }
  else {
    printf("Value delivery callback was given a nullptr. Failure.\n");
  }

  if (!cb_success) {
    c3ptp_callback_err_kvp--;
  }
  else {
    c3ptp_callback_err_kvp++;
  }
}



/*******************************************************************************
* C3PTypePipe test routines
*******************************************************************************/

/*
* This tests basic operation of C3PTypePipe. It relies on foreknowledge of the
*   parsed types that should arrive, and the fact that transfer size always
*   matches the encoded size for a type with no excess.
*/
int c3ptype_pipe_full_buffers() {
  printf("Testing full buffers (single call)...\n");
  //printf("\tPreparing test cases... ");
  int ret = 0;
  C3PTypePipeSink c3ptp_sink(TCode::CBOR, 4096, c3ptype_arrival_callback);
  C3PTypePipeSource c3ptp_src(TCode::CBOR, &c3ptp_sink);
  TestValuePalette test_values(19, 15);
  c3ptp_test_values = &test_values;

  printf("\tPushing types...\n");
  test_values.expectedTCode(TCode::BOOLEAN);
    ret -= c3ptp_src.pushValue(test_values.TEST_VAL_BOOL);
  test_values.expectedTCode(TCode::UINT8);
    ret -= c3ptp_src.pushValue(test_values.TEST_VAL_UINT8);
  test_values.expectedTCode(TCode::INT8);
    ret -= c3ptp_src.pushValue(test_values.TEST_VAL_INT8);
  test_values.expectedTCode(TCode::UINT16);
    ret -= c3ptp_src.pushValue(test_values.TEST_VAL_UINT16);
  test_values.expectedTCode(TCode::INT16);
    ret -= c3ptp_src.pushValue(test_values.TEST_VAL_INT16);
  test_values.expectedTCode(TCode::UINT32);
    ret -= c3ptp_src.pushValue(test_values.TEST_VAL_UINT32);
  test_values.expectedTCode(TCode::INT32);
    ret -= c3ptp_src.pushValue(test_values.TEST_VAL_INT32);
  test_values.expectedTCode(TCode::UINT64);
    ret -= c3ptp_src.pushValue(test_values.TEST_VAL_UINT64);
  test_values.expectedTCode(TCode::INT64);
    ret -= c3ptp_src.pushValue(test_values.TEST_VAL_INT64);
  test_values.expectedTCode(TCode::FLOAT);
    ret -= c3ptp_src.pushValue(test_values.TEST_VAL_FLOAT);
  test_values.expectedTCode(TCode::DOUBLE);
    ret -= c3ptp_src.pushValue(test_values.TEST_VAL_DOUBLE);
  //test_values.expectedTCode(TCode::STR);
  if (0 == ret) {
    printf("\t\tPass.\n\tCallback fxn reported no errors... ");
    ret -= c3ptp_callback_err;
    if (0 == ret) {
      printf("Pass.\n\tTestValuePalette is complete and matching... ");
      ret -= (test_values.allValuesMatch() ? 0 : 1);
      if (0 == ret) {
        printf("Pass.\n");
      }
    }
  }

  if (0 != ret) {
    printf("Fail (%d).\n", ret);
    test_values.dumpTestValues();
  }
  c3ptp_test_values = nullptr;
  c3ptp_callback_err = 0;
  return ret;
}


/*
* Real-world applications of C3PTypePipe should never expect to have their
*   transfer boundaries neatly-ordered to match the values encoded therin. So
*   here is tested the ability of the class to respect type boundaries as they
*   are found within the stream, and without mutating the memory layout unless a
*   fully-defined value can be resolved from the buffer.
*/
int c3ptype_pipe_partial_buffers() {
  printf("Testing partial buffers (many calls)...\n");
  int ret = 0;
  const uint32_t CAPTURE_MAX_LEN = 4096;
  StringBuilderSink sb_sink(CAPTURE_MAX_LEN);
  C3PTypePipeSink c3ptp_sink(TCode::CBOR, CAPTURE_MAX_LEN, c3ptype_arrival_callback);
  C3PTypePipeSource c3ptp_src(TCode::CBOR, &sb_sink);   // Delivery to a StringBuilder for manipulation.
  TestValuePalette test_values(19, 15);
  c3ptp_test_values = &test_values;

  printf("\tPushing types... ");
  test_values.expectedTCode(TCode::NONE);
    ret -= c3ptp_src.pushValue(test_values.TEST_VAL_BOOL);
    ret -= c3ptp_src.pushValue(test_values.TEST_VAL_UINT8);
    ret -= c3ptp_src.pushValue(test_values.TEST_VAL_INT8);
    ret -= c3ptp_src.pushValue(test_values.TEST_VAL_UINT16);
    ret -= c3ptp_src.pushValue(test_values.TEST_VAL_INT16);
    ret -= c3ptp_src.pushValue(test_values.TEST_VAL_UINT32);
    ret -= c3ptp_src.pushValue(test_values.TEST_VAL_INT32);
    ret -= c3ptp_src.pushValue(test_values.TEST_VAL_UINT64);
    ret -= c3ptp_src.pushValue(test_values.TEST_VAL_INT64);
    ret -= c3ptp_src.pushValue(test_values.TEST_VAL_FLOAT);
    ret -= c3ptp_src.pushValue(test_values.TEST_VAL_DOUBLE);
  const uint32_t TOTAL_SER_LEN = sb_sink.length();
  ret -= (0 < TOTAL_SER_LEN) ? 0 : 1;
  if (0 == ret) {
    // NOTE: The value of CHUNK_LEN should assure that both of the following
    //   conditions transpire during this test:
    //   1. A call to pushBuffer() that yields no parse, nor memory mutation.
    //   2. A call to pushBuffer() that yields multiple value parses.
    // Accordingly, we _want_ an odd value to maximize failure odds, and we want
    //   it at least large enough that condition (2) can be met with several
    //   single-byte values that are clustered together in the input string.
    const uint32_t CHUNK_LEN = ((TOTAL_SER_LEN % 4) + 4) | 1;
    printf("Pass. Resulting string was %d bytes long.\n\tChunking buffer along %d byte boundaries... ", TOTAL_SER_LEN, CHUNK_LEN);
    if ((0 < CHUNK_LEN) && (0 < sb_sink.chunk(CHUNK_LEN))) {
      printf("Pass.\n\tFeeding chunk-wise into buffer pipeline (forced inference at sink)...\n");
      StringBuilder step_buf;
      while ((sb_sink.count() > 0) & (0 == ret)) {
        // Transfer a chunk into the "stream", and try to "send" it.
        step_buf.concatHandoffLimit(&sb_sink, CHUNK_LEN);
        c3ptp_sink.pushBuffer(&step_buf);
      }
      if (0 == ret) {
        printf("\t\tPass.\n\tCallback fxn reported no errors... ");
        ret -= c3ptp_callback_err;
        if (0 == ret) {
          printf("Pass.\n\tTestValuePalette is complete and matching... ");
          ret -= (test_values.allValuesMatch() ? 0 : 1);
          if (0 == ret) {
            printf("Pass.\n\tIntermediate sink was fully consumed... ");
            ret -= sb_sink.length();
            if (0 == ret) {
              printf("Pass.\n");
            }
          }
        }
      }
    }
  }

  if (0 != ret) {
    printf("Fail (%d).\n", ret);
    test_values.dumpTestValues();
  }
  c3ptp_test_values = nullptr;
  c3ptp_callback_err = 0;
  return ret;
}


/*
* Other features rely specifically on C3PTypePipe's treatment of KVPs. And their
*   parsing involves complex stack-frames and memory implications. So KVP is
*   given its own block of tests.
*/
int c3ptype_pipe_kvp_simple() {
  printf("Testing flat KVPs...\n");
  int ret = 0;
  C3PTypePipeSink c3ptp_sink(TCode::CBOR, 4096, c3ptype_callback_kvp);
  C3PTypePipeSource c3ptp_src(TCode::CBOR, &c3ptp_sink);
  TestValuePalette test_values(61, 17);

  printf("\tGenerating test KVP... ");

  KeyValuePair a("key0", "A const test string");
    ret -= (nullptr != a.append(test_values.TEST_VAL_BOOL,   "key1"))  ? 0 : 1;
    ret -= (nullptr != a.append(test_values.TEST_VAL_UINT8,  "key2"))  ? 0 : 1;
    ret -= (nullptr != a.append(test_values.TEST_VAL_INT8,   "key3"))  ? 0 : 1;
    ret -= (nullptr != a.append(test_values.TEST_VAL_UINT16, "key4"))  ? 0 : 1;
    ret -= (nullptr != a.append(test_values.TEST_VAL_INT16,  "key5"))  ? 0 : 1;
    ret -= (nullptr != a.append(test_values.TEST_VAL_UINT32, "key6"))  ? 0 : 1;
    ret -= (nullptr != a.append(test_values.TEST_VAL_INT32,  "key7"))  ? 0 : 1;
    ret -= (nullptr != a.append(test_values.TEST_VAL_UINT64, "key8"))  ? 0 : 1;
    ret -= (nullptr != a.append(test_values.TEST_VAL_INT64,  "key9"))  ? 0 : 1;
    ret -= (nullptr != a.append(test_values.TEST_VAL_FLOAT,  "key10")) ? 0 : 1;
    ret -= (nullptr != a.append(test_values.TEST_VAL_DOUBLE, "key11")) ? 0 : 1;
  if (0 == ret) {
    printf("Pass.\n\tPushing KVP... ");
    c3ptp_test_kvp = &a;
    ret -= c3ptp_src.pushValue(&a);
    if (0 == ret) {
      printf("Pass.\n\tCallback fxn reported no errors... ");
      ret -= (1 == c3ptp_callback_err_kvp) ? 0 : 1;
      if (0 == ret) {
        printf("Pass.\n");
      }
    }
  }

  if (0 != ret) {
    printf("Fail (%d).\n", ret);
  }
  c3ptp_test_kvp = nullptr;
  c3ptp_callback_err_kvp = 0;
  return ret;
}


int c3ptype_pipe_kvp_recursive() {
  printf("Testing nested KVPs...\n");
  int ret = 0;
  C3PTypePipeSink c3ptp_sink(TCode::CBOR, 4096, c3ptype_callback_kvp);
  C3PTypePipeSource c3ptp_src(TCode::CBOR, &c3ptp_sink);
  TestValuePalette test_values(61, 17);

  printf("\tGenerating test KVP... ");

  KeyValuePair a("a0", "A const test string");
  KeyValuePair b("b0", "B const test string");
    ret -= (nullptr != a.append(test_values.TEST_VAL_BOOL,   "a1"))  ? 0 : 1;
    ret -= (nullptr != a.append(test_values.TEST_VAL_UINT8,  "a2"))  ? 0 : 1;
    ret -= (nullptr != a.append(test_values.TEST_VAL_INT8,   "a3"))  ? 0 : 1;
    ret -= (nullptr != a.append(test_values.TEST_VAL_UINT16, "a4"))  ? 0 : 1;
    ret -= (nullptr != a.append(test_values.TEST_VAL_INT16,  "a5"))  ? 0 : 1;
    ret -= (nullptr != b.append(test_values.TEST_VAL_UINT32, "b6"))  ? 0 : 1;
    ret -= (nullptr != b.append(test_values.TEST_VAL_INT32,  "b7"))  ? 0 : 1;
    ret -= (nullptr != b.append(test_values.TEST_VAL_UINT64, "b8"))  ? 0 : 1;
    ret -= (nullptr != b.append(test_values.TEST_VAL_INT64,  "b9"))  ? 0 : 1;
    ret -= (nullptr != b.append(test_values.TEST_VAL_FLOAT,  "b10")) ? 0 : 1;
    ret -= (nullptr != b.append(test_values.TEST_VAL_DOUBLE, "b11")) ? 0 : 1;
  KeyValuePair c("a_branch", &a);
  ret -= (nullptr != c.append(&b,  "b_branch"))  ? 0 : 1;
  if (0 == ret) {
    printf("Pass.\n\tPushing KVP... ");
    c3ptp_test_kvp = &c;
    ret -= c3ptp_src.pushValue(&c);
    if (0 == ret) {
      printf("Pass.\n\tCallback fxn reported no errors... ");
      ret -= (1 == c3ptp_callback_err_kvp) ? 0 : 1;
      if (0 == ret) {
        printf("Pass.\n");
      }
    }
  }

  if (0 != ret) {
    printf("Fail (%d).\n", ret);
  }
  c3ptp_test_kvp = nullptr;
  c3ptp_callback_err_kvp = 0;
  return ret;
}


/*
* This behavior is intended to provide the parser with a means of preventing
*  parsing of types that it knows it doesn't have the memory to comfortably
*  hold.
*/
int c3ptype_pipe_oversize() {
  printf("Testing oversized value handling...\n");
  int ret = 0;
  const uint32_t MAX_SINK_LEN = (32 + (randomUInt32() % 51));
  C3PTypePipeSink c3ptp_sink(TCode::CBOR, MAX_SINK_LEN, c3ptype_callback_oversize);
  C3PTypePipeSource c3ptp_src(TCode::CBOR, &c3ptp_sink);
  StringBuilder  TEST_VAL_PASSABLE;
  StringBuilder  TEST_VAL_TOO_BIG;
  generate_random_text_buffer(&TEST_VAL_PASSABLE, (MAX_SINK_LEN >> 1));
  generate_random_text_buffer(&TEST_VAL_TOO_BIG,  (MAX_SINK_LEN+1));
  C3PValue test_val_passable((char*) TEST_VAL_PASSABLE.string());
  C3PValue test_val_too_big((char*) TEST_VAL_TOO_BIG.string());

  // The expected result is for the first value to parse out with a failure to
  //   fully claim, and the second attempt to fail. Unless the source gives up,
  //   the stream will deadlock.
  printf("\tPushing an in-bounds length (%u) into an unyielding decoder with MAX_LEN of (%u)...\n", test_val_passable.length(), MAX_SINK_LEN);
  ret -= (0 == c3ptp_src.pushValue(&test_val_passable)) ? 0 : 1;
  if (0 == ret) {
    printf("\t\tPass.\n\tChecking that the callback reports reception of the same length... ");
    ret -= (test_val_passable.length() == c3ptp_callback_err_os) ? 0 : 1;
    if (0 == ret) {
      printf("Pass.\n\tPushing an out-of-bounds length (%u) into an unyielding decoder with MAX_LEN of (%u)... ", test_val_too_big.length(), MAX_SINK_LEN);
      // NOTE: Be careful with conditionals in here... Rule #16:
      //   If you fail in epic proportions, it may just become a winning failure.
      ret -= (-3 == c3ptp_src.pushValue(&test_val_too_big)) ? 0:1;
      ret -= (test_val_passable.length() == c3ptp_callback_err_os) ? 0 : 1;
      if (0 == ret) {
        printf("Pass.\n");
      }
    }
  }


  if (0 != ret) {
    printf("Fail (%d).\n", ret);
  }
  c3ptp_callback_err_os = 0;
  return ret;
}


int c3ptype_pipe_garbage_flood() {
  printf("Testing garbage handling...\n");
  int ret = -1;
  C3PTypePipeSink c3ptp_sink(TCode::CBOR, 4096, c3ptype_arrival_callback);
  C3PTypePipeSource c3ptp_src(TCode::CBOR, &c3ptp_sink);
  return ret;
}



/*******************************************************************************
* C3PTypePipe test plan
*******************************************************************************/
#define CHKLST_C3PTP_TEST_FULL_BUFFER     0x00000001  // Full-buffer, framed out.
#define CHKLST_C3PTP_TEST_SPLIT_BUFFER    0x00000002  // Partial buffers, split across calls.
#define CHKLST_C3PTP_TEST_KVP_SIMPLE      0x00000004  //
#define CHKLST_C3PTP_TEST_KVP_RECURSIVE   0x00000008  //
#define CHKLST_C3PTP_TEST_OVERSIZE        0x00000010  // Too-large value.
#define CHKLST_C3PTP_TEST_GARBAGE_FLOOD   0x00000020  // Piping random bytes into the sink.

#define CHKLST_C3PTP_TESTS_ALL ( \
  CHKLST_C3PTP_TEST_FULL_BUFFER | CHKLST_C3PTP_TEST_SPLIT_BUFFER | \
  CHKLST_C3PTP_TEST_KVP_SIMPLE | CHKLST_C3PTP_TEST_KVP_RECURSIVE | \
  CHKLST_C3PTP_TEST_OVERSIZE)

const StepSequenceList TOP_LEVEL_C3PTP_TEST_LIST[] = {
  { .FLAG         = CHKLST_C3PTP_TEST_FULL_BUFFER,
    .LABEL        = "Full buffers",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return ((nullptr == c3ptp_test_values) ? 1: 0);  },
    .POLL_FXN     = []() { return ((0 == c3ptype_pipe_full_buffers()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_C3PTP_TEST_SPLIT_BUFFER,
    .LABEL        = "Split buffers",
    .DEP_MASK     = (CHKLST_C3PTP_TEST_FULL_BUFFER),
    .DISPATCH_FXN = []() { return ((nullptr == c3ptp_test_values) ? 1: 0);  },
    .POLL_FXN     = []() { return ((0 == c3ptype_pipe_partial_buffers()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_C3PTP_TEST_KVP_SIMPLE,
    .LABEL        = "Flat KVPs",
    .DEP_MASK     = (CHKLST_C3PTP_TEST_SPLIT_BUFFER),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3ptype_pipe_kvp_simple()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_C3PTP_TEST_KVP_RECURSIVE,
    .LABEL        = "Recursive KVPs",
    .DEP_MASK     = (CHKLST_C3PTP_TEST_KVP_SIMPLE),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3ptype_pipe_kvp_recursive()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_C3PTP_TEST_OVERSIZE,
    .LABEL        = "Oversized value",
    .DEP_MASK     = (CHKLST_C3PTP_TEST_FULL_BUFFER),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3ptype_pipe_oversize()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_C3PTP_TEST_GARBAGE_FLOOD,
    .LABEL        = "Garbage flood",
    .DEP_MASK     = (CHKLST_C3PTP_TEST_SPLIT_BUFFER),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3ptype_pipe_garbage_flood()) ? 1:-1);  }
  },
};

AsyncSequencer c3ptp_test_plan(TOP_LEVEL_C3PTP_TEST_LIST, (sizeof(TOP_LEVEL_C3PTP_TEST_LIST) / sizeof(TOP_LEVEL_C3PTP_TEST_LIST[0])));



/*******************************************************************************
* The main function.
*******************************************************************************/

void print_types_c3ptypepipe() {
  printf("\tC3PTypePipeSource     %u\t%u\n", sizeof(C3PTypePipeSource),  alignof(C3PTypePipeSource));
  printf("\tC3PTypePipeSink       %u\t%u\n", sizeof(C3PTypePipeSink),  alignof(C3PTypePipeSink));
}


int c3ptype_pipe_tests() {
  const char* const MODULE_NAME = "C3PTypePipe";
  printf("===< %s >=======================================\n", MODULE_NAME);

  c3ptp_test_plan.requestSteps(CHKLST_C3PTP_TESTS_ALL);
  while (!c3ptp_test_plan.request_completed() && (0 == c3ptp_test_plan.failed_steps(false))) {
    c3ptp_test_plan.poll();
  }
  int ret = (c3ptp_test_plan.request_fulfilled() ? 0 : 1);

  StringBuilder report_output;
  c3ptp_test_plan.printDebug(&report_output, "C3PTypePipe test report");
  printf("%s\n", (char*) report_output.string());
  return ret;
}
