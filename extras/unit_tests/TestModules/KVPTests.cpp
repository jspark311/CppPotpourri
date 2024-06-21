/*
File:   KVPTests.cpp
Author: J. Ian Lindsay
Date:   2021.10.08

Copyright 2021 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


This program runs tests against the KeyValuePair class.
*/


/*******************************************************************************
* KVP test routines
*******************************************************************************/

/**
* This is the test of KVP's ability to accept values for the exportable types
*   that it claims to support.
*
* @return 0 on pass, neagative on failure.
*/
int test_KeyValuePair_Build_Polytyped_KVP() {
  printf("\tKeyValuePair: Build_Polytyped_KVP\n");
  int ret = -1;
  const uint32_t TEST_BUFFER_SIZE    = (16 + (randomUInt32() % 23));

  KeyValuePair a("constchar*", "A const test string");
  int32_t  val0  = (int32_t)  randomUInt32();
  int16_t  val1  = (int16_t)  randomUInt32();
  int8_t   val2  = (int8_t)   randomUInt32();
  uint32_t val3  = (uint32_t) randomUInt32();
  uint16_t val4  = (uint16_t) randomUInt32();
  uint8_t  val5  = (uint8_t)  randomUInt32();
  float    val6  = generate_random_float();
  double   val7  = generate_random_double();
  char*    val8  = (char*) "A non-const test string";
  bool     val9  = flip_coin();
  Vector3<float>    val10(generate_random_float(), generate_random_float(), generate_random_float());
  Vector3<uint32_t> val11(randomUInt32(), randomUInt32(), randomUInt32());
  uint64_t val12 = generate_random_uint64();
  int64_t  val13 = generate_random_int64();

  uint8_t* val20 = (uint8_t*) malloc(TEST_BUFFER_SIZE);
  for (uint32_t i = 0; i < TEST_BUFFER_SIZE; i++) {   *(val20 + i) = (uint8_t) i;   }

  printf("\t\tKVP accepts int32... ");
  if (nullptr != a.append(val0, "int32")) {
    printf("Pass.\n\t\tKVP accepts int16... ");
    if (nullptr != a.append(val1, "int16")) {
      printf("Pass.\n\t\tKVP accepts int8... ");
      if (nullptr != a.append(val2, "int8")) {
        printf("Pass.\n\t\tKVP accepts uint32... ");
        if (nullptr != a.append(val3, "uint32")) {
          printf("Pass.\n\t\tKVP accepts uint16... ");
          if (nullptr != a.append(val4, "uint16")) {
            printf("Pass.\n\t\tKVP accepts uint8... ");
            if (nullptr != a.append(val5, "uint8")) {
              printf("Pass.\n\t\tKVP accepts float... ");
              if (nullptr != a.append(val6, "float")) {
                printf("Pass.\n\t\tKVP accepts double... ");
                if (nullptr != a.append(val7, "double")) {
                  printf("Pass.\n\t\tKVP accepts char*... ");
                  if (nullptr != a.append(val8, "char*")) {
                    printf("Pass.\n\t\tKVP accepts bool... ");
                    if (nullptr != a.append(val9, "bool")) {
                      printf("Pass.\n\t\tKVP accepts Vector3<f>... ");
                      if (nullptr != a.append(&val10, "Vector3<f>")) {
                        printf("Pass.\n\t\tKVP accepts Vector3<u32>... ");
                        if (nullptr != a.append(&val11, "Vector3<u32>")) {
                          printf("Pass.\n\t\tKVP accepts uint64... ");
                          if (nullptr != a.append(val12, "uint64")) {
                            printf("Pass.\n\t\tKVP accepts int64... ");
                            if (nullptr != a.append(val13, "int64")) {
                              printf("Pass.\n\t\tKVP accepts raw binary... ");
                              KeyValuePair* raw_buf_kvp = a.append(val20, TEST_BUFFER_SIZE, "raw_buf");
                              if (nullptr != raw_buf_kvp) {
                                raw_buf_kvp->reapValue(true);
                                printf("Pass.\n\tBuild_Polytyped_KVP passes.\n");
                                ret = 0;
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  if (ret) {
    printf("Fail\n");
  }
  dump_kvp(&a);
  return ret;
}


/**
* These tests are for reference handling and proper type-assignment of internal
*   types.
*
* @return 0 on pass. Non-zero otherwise.
*/
int test_KeyValuePair_InternalTypes() {
  int ret = 1;
  printf("\tKeyValuePair: Internal Types...\n");
  StringBuilder val0("Some string");
  StopWatch     val1(randomUInt32());
  KeyValuePair a("sw_0", &val0);
  KeyValuePair b("sw_1", &val1);

  for (int i = 0; i < 10; i++) {
    val1.markStart();
    sleep_us(randomUInt32() % 400);
    val1.markStop();
  }

  printf("\t\tStringBuilder* can be added as a native type... ");
  if (TCode::STR_BUILDER == a.tcode()) {
    printf("Pass.\n\t\tStringBuilder* can be retrieved correctly by native type... ");
    StringBuilder* ret_sb = nullptr;
    if (0 == a.get_as(&ret_sb)) {
      printf("Pass.\n\t\tThe pointer that went in (%p) is the same one we get back... ", ret_sb);
      if (&val0 == ret_sb) {
        printf("Pass.\n\t\tStopWatch* can be added as a native type... ");
        if (TCode::STOPWATCH == b.tcode()) {
          printf("Pass.\n\t\tStopWatch* can be retrieved correctly by native type... ");
          StopWatch* ret1 = nullptr;
          if (0 == b.get_as(&ret1)) {
            printf("Pass.\n\t\tThe pointer that went in (%p) is the same one we get back... ", ret_sb);
            if (&val1 == ret1) {
              printf("Pass.\n\t\tStopWatch can be serialized... ");
              StringBuilder packed;
              int ser_ret = b.serialize(&packed, TCode::CBOR);
              if (0 == ser_ret) {
                printf("Pass.\n\t\tStopWatch can be deserialized... ");
                C3PValue* deser_val1 = C3PValue::deserialize(&packed, TCode::CBOR);
                if (nullptr != deser_val1) {
                  printf("Pass.\n\t\tDeserialized value is a StopWatch... ");
                  if (0 == deser_val1->get_as(&ret1)) {
                    printf("Pass.\n\t\tDeserialized value is not nullptr %p... ", ret1);
                    if (nullptr != ret1) {
                      printf("Pass.\n\t\tDeserialized StopWatch matches input... ");
                      bool pest_tasses = (ret1->tag() == val1.tag());
                      pest_tasses &= (ret1->bestTime() == val1.bestTime());
                      pest_tasses &= (ret1->lastTime() == val1.lastTime());
                      pest_tasses &= (ret1->worstTime() == val1.worstTime());
                      pest_tasses &= (ret1->meanTime() == val1.meanTime());
                      pest_tasses &= (ret1->totalTime() == val1.totalTime());
                      pest_tasses &= (ret1->executions() == val1.executions());
                      if (pest_tasses) {
                        printf("Pass.\n\tInternal Types tests all pass.\n");
                        ret = 0;
                      }
                    }
                    else {
                      StringBuilder tmp_out;
                      deser_val1->printDebug(&tmp_out);
                      printf("deser_val1:    %s\n", (char*) tmp_out.string());
                    }
                  }
                }
              }
              if (0 != ret) {
                dump_strbldr(&packed);
                StringBuilder tmp_out;
                val1.printDebug("val1", &tmp_out);
                ret1->printDebug("ret1", &tmp_out);
                printf("%s\n", (char*) tmp_out.string());
              }
            }
          }
        }
      }
    }
  }

  if (0 != ret) {
    printf("Fail.\n");
    dump_kvp(&a);
    dump_kvp(&b);
  }
  return ret;
}



/**
* Test the capability of KeyValuePairs to hold mixed KVP data, test lookup, and
*   to test the mechanics of the pointer-hack on PODs.
* Failure here might result in segfaults. This also needs to be tested against
*   both 32/64-bit builds.
*
* @return 0 on pass. Non-zero otherwise.
*/
int test_KeyValuePair_KVP() {
  int return_value = -1;
  printf("===< KeyValuePairs KVP >====================================\n");

  uint32_t val0  = (uint32_t) randomUInt32();
  uint16_t val1  = (uint16_t) randomUInt32();
  uint8_t  val2  = (uint8_t)  randomUInt32();
  int32_t  val3  = (int32_t)  randomUInt32();
  int16_t  val4  = (int16_t)  randomUInt32();
  int8_t   val5  = (int8_t)   randomUInt32();
  float    val6  = (float)    generate_random_float();
  Vector3<float> val7(generate_random_float(), generate_random_float(), generate_random_float());
  float    val8  = generate_random_float();
  double   val9  = generate_random_double();

  uint32_t ret0 = 0;
  uint16_t ret1 = 0;
  uint8_t  ret2 = 0;
  int32_t  ret3 = 0;
  int16_t  ret4 = 0;
  int8_t   ret5 = 0;
  float    ret6 = 0.0f;
  Vector3<float> ret7(0.0f, 0.0f, 0.0f);
  float    ret8 = 0.0f;
  float    ret9 = 0.0d;

  printf("Adding arguments...\n\n");
  KeyValuePair a(val3);
  a.append(val0)->setKey("value0");
  a.append(val1, "value1");
  a.append(val2);
  a.append(val4, "value4");
  a.append(val5, "value5");
  a.append(val6, "value6");
  a.append(val8, "value8");
  a.append(&val7)->setKey("value7");
  a.append(val9)->setKey("value9");

  dump_kvp(&a);

  StringBuilder temp_buffer;
  int key_count = a.collectKeys(&temp_buffer);
  printf("\t Breadth-first keyset (%d total keys):   ", key_count);
  for (int i = 0; i < key_count; i++) {
    printf("%s ", temp_buffer.position(i));
  }
  printf("\n");

  temp_buffer.clear();
  a.serialize(&temp_buffer, TCode::BINARY);
  printf("\t temp_buffer is %u bytes long.\n", temp_buffer.length());
  dump_strbldr(&temp_buffer);

  if (a.count() == 10) {
    if ((0 == a.valueWithKey("value6", &ret6)) && (ret6 == val6)) {
      if ((0 == a.valueWithKey("value0", &ret0)) && (ret0 == val0)) {
        if ((0 == a.valueWithKey("value4", &ret4)) && (ret4 == val4)) {
          if ((0 == a.valueWithKey("value5", &ret5)) && (ret5 == val5)) {
            if (0 != a.valueWithKey("non-key", &ret0)) {   // We shouldn't be able to get a value for a key that doesn't exist...
              if (0 != a.valueWithKey(nullptr, &ret0)) {   // Nor for a NULL key...
                if ((0 == a.valueWithIdx(1, &ret0)) && (ret0 == val0)) {
                  if ((0 == a.valueWithIdx(2, &ret1)) && (ret1 == val1)) {
                    if ((0 == a.valueWithIdx(3, &ret2)) && (ret2 == val2)) {
                      if ((0 == a.valueWithIdx(0, &ret3)) && (ret3 == val3)) {
                        if ((0 == a.valueWithIdx(4, &ret4)) && (ret4 == val4)) {
                          if ((0 == a.valueWithIdx(5, &ret5)) && (ret5 == val5)) {
                            printf("KVP general tests pass.\n");
                            return_value = 0;
                          }
                          else printf("int8_t failed (%d vs %d)...\n", val5, ret5);
                        }
                        else printf("int16_t failed (%d vs %d)...\n", val4, ret4);
                      }
                      else printf("int32_t failed (%d vs %d)...\n", val3, ret3);
                    }
                    else printf("uint8_t failed (%u vs %u)...\n", val2, ret2);
                  }
                  else printf("uint16_t failed (%u vs %u)...\n", val1, ret1);
                }
                else printf("uint32_t failed (%u vs %u)...\n", val0, ret0);
              }
              else printf("Found key (nullptr), which should have been nonexistant...\n");
            }
            else printf("Found key 'non-key', which should have been nonexistant...\n");
          }
          else printf("Failed to vet key 'value5'...\n");
        }
        else printf("Failed to vet key 'value4'...\n");
      }
      else printf("Failed to vet key 'value0'...\n");
    }
    else printf("Failed for float (%f vs %f)...\n", (double) val6, (double) ret6);
  }
  else printf("Total KeyValuePairs:  %d\tExpected 10.\n", a.count());

  return return_value;
}


/**
* [test_KeyValuePair_Value_Placement description]
* @return [description]
*/
int test_KeyValuePair_Value_Placement() {
  int return_value = -1;
  printf("\tKeyValuePair: Value Placement tests...\n");

  int32_t  val0  = 0;
  int16_t  val1  = 0;
  int8_t   val2  = 0;
  uint32_t val3  = 0;
  uint16_t val4  = 0;
  uint8_t  val5  = 0;
  float    val6  = 0.0f;
  Vector3<float> val7(0.0f, 0.0f, 0.0f);
  double   val8  = 0.0d;
  bool     val9  = false;
  bool     val10 = false;
  uint64_t val12 = generate_random_uint64();
  int64_t  val13 = generate_random_int64();
  Vector3<double>   val30(0.0f, 0.0f, 0.0f);
  Vector3<uint8_t>  val31(0, 0, 0);
  Vector3<uint16_t> val32(0, 0, 0);
  Vector3<uint32_t> val33(0, 0, 0);
  Vector3<int8_t>   val34(0, 0, 0);
  Vector3<int16_t>  val35(0, 0, 0);
  Vector3<int32_t>  val36(0, 0, 0);

  KeyValuePair arg0(val0);
  KeyValuePair arg1(val1);
  KeyValuePair arg2(val2);
  KeyValuePair arg3(val3);
  KeyValuePair arg4(val4);
  KeyValuePair arg5(val5);
  KeyValuePair arg6(val6);
  KeyValuePair arg7(&val7);
  KeyValuePair arg8(val8);
  KeyValuePair arg9(val9);
  KeyValuePair arg10(val10);

  KeyValuePair arg12(val12);
  KeyValuePair arg13(val13);
  KeyValuePair arg30(&val30);
  KeyValuePair arg31(&val31);
  KeyValuePair arg32(&val32);
  KeyValuePair arg33(&val33);
  KeyValuePair arg34(&val34);
  KeyValuePair arg35(&val35);
  KeyValuePair arg36(&val36);

  int32_t  ret0 = 0;
  int16_t  ret1 = 0;
  int8_t   ret2 = 0;
  uint32_t ret3 = 0;
  uint16_t ret4 = 0;
  uint8_t  ret5 = 0;
  float    ret6 = 0.0f;
  Vector3<float> ret7(0.0f, 0.0f, 0.0f);
  double   ret8 = 0.0d;
  bool     ret9  = false;
  bool     ret10 = false;
  uint64_t ret12 = 0;
  int64_t  ret13 = 0;
  Vector3<double>   ret30(0.0f, 0.0f, 0.0f);
  Vector3<uint8_t>  ret31(0, 0, 0);
  Vector3<uint16_t> ret32(0, 0, 0);
  Vector3<uint32_t> ret33(0, 0, 0);
  Vector3<int8_t>   ret34(0, 0, 0);
  Vector3<int16_t>  ret35(0, 0, 0);
  Vector3<int32_t>  ret36(0, 0, 0);


  val0  = (int32_t)  randomUInt32();
  val1  = (int16_t)  randomUInt32();
  val2  = (int8_t)   randomUInt32();
  val3  = (uint32_t) randomUInt32();
  val4  = (uint16_t) randomUInt32();
  val5  = (uint8_t)  randomUInt32();
  val6  = generate_random_float();
  Vector3<float> val7_0(generate_random_float(), generate_random_float(), generate_random_float());
  val8  = generate_random_double();
  val9  = !val9;
  val10 = !val10;
  val12 = generate_random_uint64();
  val13 = generate_random_int64();

  Vector3<double>   val30_0(generate_random_double(), generate_random_double(), generate_random_double());
  Vector3<uint8_t>  val31_0((uint8_t) randomUInt32(), (uint8_t) randomUInt32(), (uint8_t) randomUInt32());
  Vector3<uint16_t> val32_0((uint16_t) randomUInt32(), (uint16_t) randomUInt32(), (uint16_t) randomUInt32());
  Vector3<uint32_t> val33_0((uint32_t) randomUInt32(), (uint32_t) randomUInt32(), (uint32_t) randomUInt32());
  Vector3<int8_t>   val34_0((int8_t) randomUInt32(), (int8_t) randomUInt32(), (int8_t) randomUInt32());
  Vector3<int16_t>  val35_0((int16_t) randomUInt32(), (int16_t) randomUInt32(), (int16_t) randomUInt32());
  Vector3<int32_t>  val36_0((int32_t) randomUInt32(), (int32_t) randomUInt32(), (int32_t) randomUInt32());


  printf("\t\tPlacement of TCode::%s... ", typecodeToStr(arg0.tcode()));
  if (0 == arg0.set(val0)) {
    printf("Pass.\n\t\tPlacement of TCode::%s... ", typecodeToStr(arg1.tcode()));
    if (0 == arg1.set(val1)) {
      printf("Pass.\n\t\tPlacement of TCode::%s... ", typecodeToStr(arg2.tcode()));
      if (0 == arg2.set(val2)) {
        printf("Pass.\n\t\tPlacement of TCode::%s... ", typecodeToStr(arg3.tcode()));
        if (0 == arg3.set(val3)) {
          printf("Pass.\n\t\tPlacement of TCode::%s... ", typecodeToStr(arg4.tcode()));
          if (0 == arg4.set(val4)) {
            printf("Pass.\n\t\tPlacement of TCode::%s... ", typecodeToStr(arg5.tcode()));
            if (0 == arg5.set(val5)) {
              printf("Pass.\n\t\tPlacement of TCode::%s... ", typecodeToStr(arg6.tcode()));
              if (0 == arg6.set(val6)) {
                printf("Pass.\n\t\tPlacement of TCode::%s... ", typecodeToStr(arg7.tcode()));
                if (0 == arg7.set(&val7_0)) {
                  printf("Pass.\n\t\tPlacement of TCode::%s... ", typecodeToStr(arg8.tcode()));
                  if (0 == arg8.set(val8)) {
                    printf("Pass.\n\t\tPlacement of TCode::%s... ", typecodeToStr(arg9.tcode()));
                    if (0 == arg9.set(val9)) {
                      printf("Pass.\n\t\tPlacement of TCode::%s... ", typecodeToStr(arg10.tcode()));
                      if (0 == arg10.set(val10)) {
                        printf("Pass.\n\t\tPlacement of TCode::%s... ", typecodeToStr(arg12.tcode()));
                        if (0 == arg12.set(val12)) {
                          printf("Pass.\n\t\tPlacement of TCode::%s... ", typecodeToStr(arg13.tcode()));
                          if (0 == arg13.set(val13)) {
                            printf("Pass.\n\t\tPlacement of TCode::%s... ", typecodeToStr(arg30.tcode()));
                            if (0 == arg30.set(&val30_0)) {
                              printf("Pass.\n\t\tPlacement of TCode::%s... ", typecodeToStr(arg31.tcode()));
                              if (0 == arg31.set(&val31_0)) {
                                printf("Pass.\n\t\tPlacement of TCode::%s... ", typecodeToStr(arg32.tcode()));
                                if (0 == arg32.set(&val32_0)) {
                                  printf("Pass.\n\t\tPlacement of TCode::%s... ", typecodeToStr(arg33.tcode()));
                                  if (0 == arg33.set(&val33_0)) {
                                    printf("Pass.\n\t\tPlacement of TCode::%s... ", typecodeToStr(arg34.tcode()));
                                    if (0 == arg34.set(&val34_0)) {
                                      printf("Pass.\n\t\tPlacement of TCode::%s... ", typecodeToStr(arg35.tcode()));
                                      if (0 == arg35.set(&val35_0)) {
                                        printf("Pass.\n\t\tPlacement of TCode::%s... ", typecodeToStr(arg36.tcode()));
                                        if (0 == arg36.set(&val36_0)) {
                                          return_value = 0;
                                        }
                                      }
                                    }
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  if (0 == return_value) {
    return_value = -1;
    printf("Pass.\n\t\tVerification of TCode::%s... ", typecodeToStr(arg0.tcode()));
    if ((0 == arg0.get_as(&ret0)) && (ret0 == val0)) {
      printf("Pass.\n\t\tVerification of TCode::%s... ", typecodeToStr(arg1.tcode()));
      if ((0 == arg1.get_as(&ret1)) && (ret1 == val1)) {
        printf("Pass.\n\t\tVerification of TCode::%s... ", typecodeToStr(arg2.tcode()));
        if ((0 == arg2.get_as(&ret2)) && (ret2 == val2)) {
          printf("Pass.\n\t\tVerification of TCode::%s... ", typecodeToStr(arg3.tcode()));
          if ((0 == arg3.get_as(&ret3)) && (ret3 == val3)) {
            printf("Pass.\n\t\tVerification of TCode::%s... ", typecodeToStr(arg4.tcode()));
            if ((0 == arg4.get_as(&ret4)) && (ret4 == val4)) {
              printf("Pass.\n\t\tVerification of TCode::%s... ", typecodeToStr(arg5.tcode()));
              if ((0 == arg5.get_as(&ret5)) && (ret5 == val5)) {
                printf("Pass.\n\t\tVerification of TCode::%s... ", typecodeToStr(arg6.tcode()));
                if ((0 == arg6.get_as(&ret6)) && (ret6 == val6)) {
                  printf("Pass.\n\t\tVerification of TCode::%s... ", typecodeToStr(arg7.tcode()));
                  if ((0 == arg7.get_as(&ret7)) && (ret7 == val7_0)) {
                    printf("Pass.\n\t\tVerification of TCode::%s... ", typecodeToStr(arg8.tcode()));
                    if ((0 == arg8.get_as(&ret8)) && (ret8 == val8)) {
                      printf("Pass.\n\t\tVerification of TCode::%s... ", typecodeToStr(arg9.tcode()));
                      if ((0 == arg9.get_as(&ret9)) && (ret9 == val9)) {
                        printf("Pass.\n\t\tVerification of TCode::%s... ", typecodeToStr(arg10.tcode()));
                        if ((0 == arg10.get_as(&ret10)) && (ret10 == val10)) {
                          printf("Pass.\n\t\tVerification of TCode::%s... ", typecodeToStr(arg12.tcode()));
                          if ((0 == arg12.get_as(&ret12)) && (ret12 == val12)) {
                            printf("Pass.\n\t\tVerification of TCode::%s... ", typecodeToStr(arg13.tcode()));
                            if ((0 == arg13.get_as(&ret13)) && (ret13 == val13)) {
                              printf("Pass.\n\t\tVerification of TCode::%s... ", typecodeToStr(arg30.tcode()));
                              if ((0 == arg30.get_as(&ret30)) && (ret30 == val30_0)) {
                                printf("Pass.\n\t\tVerification of TCode::%s... ", typecodeToStr(arg31.tcode()));
                                if ((0 == arg31.get_as(&ret31)) && (ret31 == val31_0)) {
                                  printf("Pass.\n\t\tVerification of TCode::%s... ", typecodeToStr(arg32.tcode()));
                                  if ((0 == arg32.get_as(&ret32)) && (ret32 == val32_0)) {
                                    printf("Pass.\n\t\tVerification of TCode::%s... ", typecodeToStr(arg33.tcode()));
                                    if ((0 == arg33.get_as(&ret33)) && (ret33 == val33_0)) {
                                      printf("Pass.\n\t\tVerification of TCode::%s... ", typecodeToStr(arg34.tcode()));
                                      if ((0 == arg34.get_as(&ret34)) && (ret34 == val34_0)) {
                                        printf("Pass.\n\t\tVerification of TCode::%s... ", typecodeToStr(arg35.tcode()));
                                        if ((0 == arg35.get_as(&ret35)) && (ret35 == val35_0)) {
                                          printf("Pass.\n\t\tVerification of TCode::%s... ", typecodeToStr(arg36.tcode()));
                                          if ((0 == arg36.get_as(&ret36)) && (ret36 == val36_0)) {
                                            printf("Pass.\n\t\tValue placement tests all pass.\n");
                                            return_value = 0;
                                          }
                                        }
                                      }
                                    }
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  if (0 != return_value) {
    printf("Fail.\n");
  }

  return return_value;
}


/**
* The KVP API allows values to be type-degraded transparently. These tests
*   verify that such behavior is correct.
*
* @return [description]
*/
int test_KeyValuePair_Value_Translation() {
  int return_value = -1;
  printf("===< KeyValuePair Value Translation >=========================\n");

  uint32_t val0  = (uint32_t) randomUInt32();
  uint16_t val1  = (uint16_t) randomUInt32();
  uint8_t  val2  = (uint8_t)  randomUInt32();
  int32_t  val3  = (int32_t)  randomUInt32();
  int16_t  val4  = (int16_t)  randomUInt32();
  int8_t   val5  = (int8_t)   randomUInt32();
  float    val6  = (float)    randomUInt32()/1000000.0f;
  double   val7  = (double)randomUInt32()/(double)randomUInt32();
  Vector3<float> val8(randomUInt32()/1000000.0f, randomUInt32()/1000000.0f, randomUInt32()/1000000.0f);

  KeyValuePair a("uint32", val0);
  a.append(val1, "uint16");
  a.append(val2, "uint8");
  a.append(val3, "int32");
  a.append(val4, "int16");
  a.append(val5, "int8");
  a.append(val6, "float");
  a.append(val7, "double");
  a.append(&val8, "Vector3<f>");
  dump_kvp(&a);

  // Experimental values.
  double   ret0 = 0;
  uint32_t ret1 = 0;
  uint16_t ret2 = 0;
  double   ret3 = 0;
  int32_t  ret4 = 0;
  int16_t  ret5 = 0;
  int8_t   ret6 = 0.0f;
  int32_t  ret7 = 0.0f;
  Vector3<int32_t> ret8(0, 0, 0);

  // Control values.
  double   compare0 = (double)   val0;
  uint32_t compare1 = (uint32_t) val1;
  uint16_t compare2 = (uint16_t) val2;
  double   compare3 = (double)   val3;
  int32_t  compare4 = (int32_t)  val4;
  int16_t  compare5 = (int16_t)  val5;
  int8_t   compare6 = (int8_t)   val6;
  int32_t  compare7 = (double)   val7;
  Vector3<int32_t> compare8((int32_t) val8.x, (int32_t) val8.y, (int32_t) val8.z);

  if ((0 == a.valueWithKey("uint32", &ret0)) && (ret0 == compare0)) {
    if ((0 == a.valueWithKey("uint16", &ret1)) && (ret1 == compare1)) {
      if ((0 == a.valueWithKey("uint8", &ret2)) && (ret2 == compare2)) {
        if ((0 == a.valueWithKey("int32", &ret3)) && (ret3 == compare3)) {
          if ((0 == a.valueWithKey("int16", &ret4)) && (ret4 == compare4)) {
            if ((0 == a.valueWithKey("int8", &ret5)) && (ret5 == compare5)) {
              if ((0 == a.valueWithKey("float", &ret6)) && (ret6 == compare6)) {
                if ((0 == a.valueWithKey("double", &ret7)) && (ret7 == compare7)) {
                  if ((0 == a.valueWithKey("Vector3<f>", &ret8)) && (ret8 == compare8)) {
                    printf("Value Translation tests pass.\n");
                    return_value = 0;
                  }
                  else printf("Failed to vet Vector3<float> --> Vector3<int32>\n");
                }
                else printf("Failed to vet double --> int32_t\n");
              }
              else printf("Failed to vet float --> int8\n");
            }
            else printf("Failed to vet int8 --> int16\n");
          }
          else printf("Failed to vet int16 --> int32\n");
        }
        else printf("Failed to vet int32 --> double\n");
      }
      else printf("Failed to vet uint8_t --> uint16_t\n");
    }
    else printf("Failed to vet uint16_t --> uint32_t\n");
  }
  else printf("Failed to vet uint32_t --> double\n");

  // TODO: If the safe translations passed, try the ones that should be error-cases.
  if (0 == return_value) {
  }

  return return_value;
}



/**
* This is the test of key-related edge-cases.
*
* @return [description]
*/
int test_KeyValuePair_Key_Abuse() {
  int return_value = -1;
  printf("\tKeyValuePair: Key Abuse...\n");

  const char* feed_str = "mallocd_key";
  uint32_t m_str_len = strlen(feed_str);

  uint32_t val0 = (uint32_t) randomUInt32();
  uint32_t val1 = (uint32_t) randomUInt32();
  uint32_t val2 = (uint32_t) randomUInt32();
  uint32_t val3 = (uint32_t) randomUInt32();
  uint32_t val4 = (uint32_t) randomUInt32();
  uint32_t val5 = (uint32_t) randomUInt32();
  uint32_t val6 = (uint32_t) randomUInt32();
  uint32_t val7 = (uint32_t) randomUInt32();
  uint32_t val8 = (uint32_t) randomUInt32();
  uint32_t val9 = (uint32_t) randomUInt32();

  // Experimental values.
  uint32_t ret0 = 0;
  uint32_t ret1 = 0;
  uint32_t ret2 = 0;
  uint32_t ret3 = 0;
  uint32_t ret4 = 0;
  uint32_t ret5 = 0;
  uint32_t ret6 = 0;
  uint32_t ret7 = 0;
  uint32_t ret8 = 0;
  uint32_t ret9 = 0;

  const char* key0 = "safe";       // A safe test key.
  const char* key1 = "\t \n\r  ";  // Exotic whitespace is also valid.
  const char* key2 = "duplicate";  // Duplicate keys are allowed, but the second
  const char* key3 = "duplicate";  //   key will only be accessible by index.
  const char* key4 = nullptr;      // This should be the same as not passing a key.
  const char* key5 = "";           // Empty string is a valid key.
  const char* key6 = "test6";
  const char* key7 = nullptr;
  const char* key8 = "test8";
  char* key9 = (char*) malloc(m_str_len);
  memcpy(key9, feed_str, m_str_len);
  *(key9 + m_str_len) = '\0';

  printf("\tCreating abuse keys... ");
  KeyValuePair a(key0, val0);
  if (a.append(val1, key1)) {
    if (a.append(val2, key2)) {
      if (a.append(val3, key3)) {
        if (a.append(val4, key4)) {
          if (a.append(val5, key5)) {
            if (a.append(val6, key6)) {
              if (0 == a.append(val7, key7)->setKey((char*) "test7")) {
                if (a.append(val8, key8)) {
                  if (a.append(val9, key9)) {
                    return_value = 0;
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  if (0 == return_value) {
    return_value = -1;
    printf("Pass.\n\tSafe key lookup succeeds and has a value of %u... ", val0);
    if ((0 == a.valueWithKey(key0, &ret0)) && (ret0 == val0)) {
      printf("Pass.\n\tExotic whitespace lookup succeeds and has a value of %u... ", val1);
      if ((0 == a.valueWithKey(key1, &ret1)) && (ret1 == val1)) {
        printf("Pass.\n\tFirst duplicate lookup succeeds and has a value of %u... ", val2);
        if ((0 == a.valueWithKey(key2, &ret2)) && (ret2 == val2)) {
          printf("Pass.\n\tSecond duplicate lookup succeeds and has a value of %u (not %u)... ", val2, val3);
          if ((0 == a.valueWithKey(key3, &ret3)) && (ret3 == val2)) {
            printf("Pass.\n\tLookup by nullptr fails... ");
            if (0 > a.valueWithKey(key4, &ret4)) {
              printf("Pass.\n\tLookup by empty key succeeds and has a value of %u... ", val5);
              if ((0 == a.valueWithKey(key5, &ret5)) && (ret5 == val5)) {
                printf("Pass.\n\tLookup by safe key following abused keys succeeds and has a value of %u... ", val6);
                if ((0 == a.valueWithKey(key6, &ret6)) && (ret6 == val6)) {
                  printf("Pass.\n\tLookup by displaced key succeeds and has a value of %u... ", val7);
                  if ((0 == a.valueWithKey("test7", &ret7)) && (ret7 == val7)) {
                    printf("Pass.\n\tLookup by safe key following abused keys succeeds and has a value of %u... ", val8);
                    if ((0 == a.valueWithKey(key8, &ret8)) && (ret8 == val8)) {
                      printf("Pass.\n\tLookup by malloc'd key succeeds and has a value of %u... ", val9);
                      if ((0 == a.valueWithKey(key9, &ret9)) && (ret9 == val9)) {
                        return_value = 0;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  if (0 != return_value) {
    printf("Fail.\n");
    dump_kvp(&a);
  }
  else {
    printf("Pass.\nKey Abuse tests all pass.\n");
  }

  return return_value;
}



#if defined(CONFIG_C3P_CBOR)
/**
* [test_CBOR_KVP description]
* @return [description]
*/
int test_CBOR_KeyValuePair() {
  int return_value = -1;
  printf("===< KVPs CBOR >===================================\n");
  StringBuilder shuttle;  // We will transport the CBOR encoded-bytes through this.

  int32_t  val0  = (int32_t)  randomUInt32();
  int16_t  val1  = (int16_t)  randomUInt32();
  int8_t   val2  = (int8_t)   randomUInt32();
  uint32_t val3  = (uint32_t) randomUInt32();
  uint16_t val4  = (uint16_t) randomUInt32();
  uint8_t  val5  = (uint8_t)  randomUInt32();
  float    val6  = ((uint32_t) randomUInt32()) / ((float) randomUInt32());
  Vector3<float> val7(randomUInt32()/1000000.0f, randomUInt32()/1000000.0f, randomUInt32()/1000000.0f);
  double   val8  = ((uint32_t) randomUInt32()) / ((double) randomUInt32());

  int32_t  ret0 = 0;
  int16_t  ret1 = 0;
  int8_t   ret2 = 0;
  uint32_t ret3 = 0;
  uint16_t ret4 = 0;
  uint8_t  ret5 = 0;
  float    ret6 = 0.0f;
  Vector3<float> ret7(0.0f, 0.0f, 0.0f);
  double   ret8 = 0.0f;

  KeyValuePair a("val0", val0);
  a.append(val1)->setKey("val1");
  a.append(val2)->setKey("val2");
  a.append(val3)->setKey("val3");
  a.append(val4)->setKey("val4");
  a.append(val5)->setKey("val5");
  a.append(val6)->setKey("val6");
  a.append(&val7)->setKey("val7");
  a.append(val8, "val8");
  dump_kvp(&a);

  int8_t ret_local = a.serialize(&shuttle, TCode::CBOR);
  KeyValuePair* r = KeyValuePair::unserialize(shuttle.string(), shuttle.length(), TCode::CBOR);

  if (0 == ret_local) {
    printf("CBOR encoding occupies %d bytes\n\t", shuttle.length());
    dump_strbldr(&shuttle);

    if (nullptr != r) {
      printf("CBOR decoded:\n");
      dump_kvp(r);

      if (r->count() == a.count()) {
        if ((0 == r->valueWithIdx(0, &ret0)) && (ret0 == val0)) {
          if ((0 == r->valueWithIdx(1, &ret1)) && (ret1 == val1)) {
            if ((0 == r->valueWithIdx(2, &ret2)) && (ret2 == val2)) {
              if ((0 == r->valueWithIdx(3, &ret3)) && (ret3 == val3)) {
                if ((0 == r->valueWithIdx(4, &ret4)) && (ret4 == val4)) {
                  if ((0 == r->valueWithIdx(5, &ret5)) && (ret5 == val5)) {
                    if ((0 == r->valueWithIdx(6, &ret6)) && (ret6 == val6)) {
                      if ((0 == r->valueWithIdx(7, &ret7)) && (ret7 == val7)) {
                        if ((0 == r->valueWithIdx(8, &ret8)) && (ret8 == val8)) {
                          return_value = 0;
                        }
                        else printf("Failed to vet key 'val8'... %.6f vs %.6f\n", ret8, val8);
                      }
                      else printf("Failed to vet key 'val7'...\n");
                    }
                    else printf("Failed to vet key 'val6'... %.3f vs %.3f\n", (double) ret6, (double) val6);
                  }
                  else printf("Failed to vet key 'val5'... %u vs %u\n", ret5, val5);
                }
                else printf("Failed to vet key 'val4'... %u vs %u\n", ret4, val4);
              }
              else printf("Failed to vet key 'val3'... %u vs %u\n", ret3, val3);
            }
            else printf("Failed to vet key 'val2'... %u vs %u\n", ret2, val2);
          }
          else printf("Failed to vet key 'val1'... %u vs %u\n", ret1, val1);
        }
        else printf("Failed to vet key 'val0'... %u vs %u\n", ret0, val0);
      }
      else printf("Arg counts don't match: %d vs %d\n", r->count(), a.count());
    }
    else printf("Failed to decode KVP chain from CBOR...\n");
  }
  else printf("Failed to encode KVP chain into CBOR: %d\n", ret_local);

  return return_value;
}


/**
* These are values that give the CBOR implementation special flavors of grief.
* Usually, this is due to a boundary condition in the CBOR spec being
*   implemented poorly. All such known cases are implemented here.
*
* [test_CBOR_Problematic_KeyValuePair description]
* @return [description]
*/
int test_CBOR_Problematic_KeyValuePair() {
  int return_value = -1;
  printf("===< KeyValuePairs CBOR Minefield >=========================\n");
  StringBuilder shuttle;  // We will transport the CBOR encoded-bytes through this.

  int32_t  val0  = (int32_t)  -65500;
  int16_t  val1  = (int16_t)  -230;
  int8_t   val2  = (int8_t)   -23;
  uint32_t val3  = (uint32_t) 3643900856;
  uint16_t val4  = (uint16_t) 59041;
  uint8_t  val5  = (uint8_t)  250;

  int32_t  ret0 = 0;
  int16_t  ret1 = 0;
  int8_t   ret2 = 0;
  uint32_t ret3 = 0;
  uint16_t ret4 = 0;
  uint8_t  ret5 = 0;

  KeyValuePair a("val0", val0);
  a.append(val1, "val1");
  a.append(val2, "val2");
  a.append(val3, "val3");
  a.append(val4, "val4");
  a.append(val5, "val5");

  dump_kvp(&a);

  int8_t ret_local = a.serialize(&shuttle, TCode::CBOR);
  if (0 == ret_local) {
    printf("CBOR encoding occupies %d bytes\n\t", shuttle.length());
    dump_strbldr(&shuttle);

    KeyValuePair* r = KeyValuePair::unserialize(shuttle.string(), shuttle.length(), TCode::CBOR);
    if (nullptr != r) {
      printf("CBOR decoded:\n");
      dump_kvp(r);

      if ((0 == r->valueWithIdx((uint8_t) 0, &ret0)) && (ret0 == val0)) {
        if ((0 == r->valueWithIdx((uint8_t) 1, &ret1)) && (ret1 == val1)) {
          if ((0 == r->valueWithIdx((uint8_t) 2, &ret2)) && (ret2 == val2)) {
            if ((0 == r->valueWithIdx((uint8_t) 3, &ret3)) && (ret3 == val3)) {
              if ((0 == r->valueWithIdx((uint8_t) 4, &ret4)) && (ret4 == val4)) {
                if ((0 == r->valueWithIdx((uint8_t) 5, &ret5)) && (ret5 == val5)) {
                    if (r->count() == a.count()) {
                      return_value = 0;
                    }
                    else printf("Arg counts don't match: %d vs %d\n", r->count(), a.count());
                }
                else printf("Failed to vet key 'value5'... %u vs %u\n", ret5, val5);
              }
              else printf("Failed to vet key 'value4'... %u vs %u\n", ret4, val4);
            }
            else printf("Failed to vet key 'value3'... %u vs %u\n", ret3, val3);
          }
          else printf("Failed to vet key 'value2'... %u vs %u\n", ret2, val2);
        }
        else printf("Failed to vet key 'value1'... %u vs %u\n", ret1, val1);
      }
      else printf("Failed to vet key 'value0'... %u vs %u\n", ret0, val0);
    }
    else printf("Failed to decode KeyValuePair chain from CBOR...\n");
  }
  else printf("Failed to encode KeyValuePair chain into CBOR...\n");

  if (return_value) {
    dump_kvp(&a);
  }

  return return_value;
}
#endif  // CONFIG_C3P_CBOR



void print_types_kvp() {
  printf("\tKeyValuePair          %u\t%u\n", sizeof(KeyValuePair),   alignof(KeyValuePair));
}


/**
* This is the root of the KeyValuePair tests.
*
* @return 0 on success. Nonzero otherwise.
*/
int test_KeyValuePair() {
  const char* const MODULE_NAME = "KeyValuePair";
  printf("===< %s >=======================================\n", MODULE_NAME);
  int ret = -1;

  if (0 == test_KeyValuePair_Build_Polytyped_KVP()) {
    if (0 == test_KeyValuePair_Value_Placement()) {
      if (0 == test_KeyValuePair_InternalTypes()) {
        if (0 == test_KeyValuePair_KVP()) {
          if (0 == test_KeyValuePair_Key_Abuse()) {
            if (true) {  //if (0 == test_KeyValuePair_Value_Translation()) {
            #if defined(CONFIG_C3P_CBOR)
              if (0 == test_CBOR_KeyValuePair()) {
                if (0 == test_CBOR_Problematic_KeyValuePair()) {
                  printf("KeyValuePair tests all pass.\n");
                  ret = 0;
                }
              }
              #endif  // CONFIG_C3P_CBOR
            }
          }
        }
      }
    }
  }

  return ret;
}
