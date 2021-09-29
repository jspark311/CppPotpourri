/*
File:   TestDataStructures.cpp
Author: J. Ian Lindsay
Date:   2016.09.20

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


This program runs tests against our raw data-handling classes.
*/


/*******************************************************************************
* Globals
*******************************************************************************/

SensorFilter<float> test_filter_0(FilteringStrategy::RAW, 64, 0);
SensorFilter<float> test_filter_1(FilteringStrategy::RAW, 64, 0);
SensorFilter<float> test_filter_2(FilteringStrategy::RAW, 64, 0);
SensorFilter<float> test_filter_3(FilteringStrategy::RAW, 64, 0);

StopWatch stopwatch_0;
StopWatch stopwatch_1;
StopWatch stopwatch_2;
StopWatch stopwatch_3;


/*******************************************************************************
* KVP test routines
*******************************************************************************/

/**
* Test the capability of KeyValuePairs to hold mixed KVP data, test lookup, and
*   to test the mechanics of the pointer-hack on PODs.
* Failure here might result in segfaults. This also needs to be tested against
*   both 32/64-bit builds.
* @return 0 on pass. Non-zero otherwise.
*/
int test_KeyValuePair_KVP() {
  int return_value = -1;
  StringBuilder log("===< KeyValuePairs KVP >====================================\n");

  uint32_t val0  = (uint32_t) randomUInt32();
  uint16_t val1  = (uint16_t) randomUInt32();
  uint8_t  val2  = (uint8_t)  randomUInt32();
  int32_t  val3  = (int32_t)  randomUInt32();
  int16_t  val4  = (int16_t)  randomUInt32();
  int8_t   val5  = (int8_t)   randomUInt32();
  float    val6  = (float)    randomUInt32()/1000000.0f;
  float    val8  = (float)    randomUInt32()/1000000.0f;
  double   val9  = (double)   (double)randomUInt32()/(double)randomUInt32();
  Vector3<float> val7(randomUInt32()/1000000.0f, randomUInt32()/1000000.0f, randomUInt32()/1000000.0f);

  uint32_t ret0 = 0;
  uint16_t ret1 = 0;
  uint8_t  ret2 = 0;
  int32_t  ret3 = 0;
  int16_t  ret4 = 0;
  int8_t   ret5 = 0;
  float    ret6 = 0.0f;
  float    ret8 = 0.0f;
  Vector3<float> ret7(0.0f, 0.0f, 0.0f);
  //float    ret9 = 0.0d;

  log.concat("\t Adding arguments...\n\n");
  KeyValuePair a(val3);
  a.append(val0)->setKey("value0");
  a.append(val1, "value1");
  a.append(val2);  // NOTE: Mixed in with non-KVP.
  a.append(val4, "value4");
  a.append(val5, "value5");
  a.append(val6, "value6");
  a.append(val8, "value8");
  a.append(&val7)->setKey("value7");
  a.append(val9)->setKey("value9");

  a.printDebug(&log);
  log.concat("\n");

  StringBuilder temp_buffer;
  int key_count = a.collectKeys(&temp_buffer);
  log.concatf("\t Breadth-first keyset (%d total keys):   ", key_count);
  for (int i = 0; i < key_count; i++) {
    log.concatf("%s ", temp_buffer.position(i));
  }
  log.concat("\n");

  temp_buffer.clear();
  a.serialize(&temp_buffer, TCode::BINARY);
  log.concatf("\t temp_buffer is %u bytes long.\n", temp_buffer.length());
  temp_buffer.printDebug(&log);

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
                            return_value = 0;
                          }
                          else log.concatf("int8_t failed (%d vs %d)...\n", val5, ret5);
                        }
                        else log.concatf("int16_t failed (%d vs %d)...\n", val4, ret4);
                      }
                      else log.concatf("int32_t failed (%d vs %d)...\n", val3, ret3);
                    }
                    else log.concatf("uint8_t failed (%u vs %u)...\n", val2, ret2);
                  }
                  else log.concatf("uint16_t failed (%u vs %u)...\n", val1, ret1);
                }
                else log.concatf("uint32_t failed (%u vs %u)...\n", val0, ret0);
              }
              else log.concat("Found key (nullptr), which should have been nonexistant...\n");
            }
            else log.concat("Found key 'non-key', which should have been nonexistant...\n");
          }
          else log.concat("Failed to vet key 'value5'...\n");
        }
        else log.concat("Failed to vet key 'value4'...\n");
      }
      else log.concat("Failed to vet key 'value0'...\n");
    }
    else log.concatf("Failed for float (%f vs %f)...\n", (double) val6, (double) ret6);
  }
  else log.concatf("Total KeyValuePairs:  %d\tExpected 10.\n", a.count());

  printf("%s\n\n", (const char*) log.string());
  return return_value;
}


/**
* These tests are for reference handling and proper type-assignment of internal
*   types.
* @return 0 on pass. Non-zero otherwise.
*/
int test_KeyValuePair_InternalTypes() {
  int return_value = 1;
  StringBuilder log("===< KeyValuePairs Internal Types >=========================\n");
  StringBuilder val0("Some string");
  KeyValuePair a(&val0);
  a.printDebug(&log);

  StringBuilder* ret0 = nullptr;

  if (0 == a.getValueAs(&ret0)) {
    if (&val0 == ret0) {
      return_value = 0;
    }
    else log.concat("StringBuilder pointer retrieved from KeyValuePair is not the same as what went in. Fail...\n");
  }
  else log.concat("Failed to retrieve StringBuilder pointer.\n");

  printf("%s\n\n", (const char*) log.string());
  return return_value;
}


/**
* [test_KeyValuePair_Value_Placement description]
* @return [description]
*/
int test_KeyValuePair_Value_Placement() {
  int return_value = -1;
  StringBuilder log("===< KeyValuePair Value Placement >=========================\n");
  StringBuilder shuttle;  // We will transport the CBOR encoded-bytes through this.

  int32_t  val0  = (int32_t)  randomUInt32();
  int16_t  val1  = (int16_t)  randomUInt32();
  int8_t   val2  = (int8_t)   randomUInt32();
  uint32_t val3  = (uint32_t) randomUInt32();
  uint16_t val4  = (uint16_t) randomUInt32();
  uint8_t  val5  = (uint8_t)  randomUInt32();
  float    val6  = ((uint32_t) randomUInt32()) / ((float) randomUInt32());
  Vector3<float> val7(randomUInt32()/1000000.0f, randomUInt32()/1000000.0f, randomUInt32()/1000000.0f);
  double   val8  = (double)randomUInt32()/(double)randomUInt32();

  KeyValuePair arg0(val0);
  KeyValuePair arg1(val1);
  KeyValuePair arg2(val2);
  KeyValuePair arg3(val3);
  KeyValuePair arg4(val4);
  KeyValuePair arg5(val5);
  KeyValuePair arg6(val6);
  KeyValuePair arg7(&val7);
  KeyValuePair arg8(val8);

  int32_t  ret0 = 0;
  int16_t  ret1 = 0;
  int8_t   ret2 = 0;
  uint32_t ret3 = 0;
  uint16_t ret4 = 0;
  uint8_t  ret5 = 0;
  float    ret6 = 0.0f;
  Vector3<float> ret7(0.0f, 0.0f, 0.0f);
  double   ret8 = 0.0f;

  val0  = (int32_t)  randomUInt32();
  val1  = (int16_t)  randomUInt32();
  val2  = (int8_t)   randomUInt32();
  val3  = (uint32_t) randomUInt32();
  val4  = (uint16_t) randomUInt32();
  val5  = (uint8_t)  randomUInt32();
  val6  = ((uint32_t) randomUInt32()) / ((float) randomUInt32());
  val7(
    ((uint32_t) randomUInt32()) / ((float) randomUInt32()),
    ((uint32_t) randomUInt32()) / ((float) randomUInt32()),
    ((uint32_t) randomUInt32()) / ((float) randomUInt32())
  );
  val8  = ((uint32_t) randomUInt32()) / ((double) randomUInt32());

  arg0.setValue(val0);
  arg1.setValue(val1);
  arg2.setValue(val2);
  arg3.setValue(val3);
  arg4.setValue(val4);
  arg5.setValue(val5);
  arg6.setValue(val6);
  arg7.setValue(&val7);
  arg8.setValue(val8);

  if ((0 == arg0.getValueAs(&ret0)) && (ret0 == val0)) {
    if ((0 == arg1.getValueAs(&ret1)) && (ret1 == val1)) {
      if ((0 == arg2.getValueAs(&ret2)) && (ret2 == val2)) {
        if ((0 == arg3.getValueAs(&ret3)) && (ret3 == val3)) {
          if ((0 == arg4.getValueAs(&ret4)) && (ret4 == val4)) {
            if ((0 == arg5.getValueAs(&ret5)) && (ret5 == val5)) {
              if ((0 == arg6.getValueAs(&ret6)) && (ret6 == val6)) {
                if ((0 == arg8.getValueAs(&ret8)) && (ret8 == val8)) {
                  log.concatf("Value placement tests good for all types.\n");
                  return_value = 0;
                }
                else log.concatf("Failed to vet key 'value8'... %.20f vs %.20f\n", ret8, val8);
              }
              else log.concatf("Failed to vet key 'value6'... %.3f vs %.3f\n", (double) ret6, (double) val6);
            }
            else log.concatf("Failed to vet key 'value5'... %u vs %u\n", ret5, val5);
          }
          else log.concatf("Failed to vet key 'value4'... %u vs %u\n", ret4, val4);
        }
        else log.concatf("Failed to vet key 'value3'... %u vs %u\n", ret3, val3);
      }
      else log.concatf("Failed to vet key 'value2'... %u vs %u\n", ret2, val2);
    }
    else log.concatf("Failed to vet key 'value1'... %u vs %u\n", ret1, val1);
  }
  else log.concatf("Failed to vet key 'value0'... %u vs %u\n", ret0, val0);

  if (return_value) {
    //a.printDebug(&log);
  }
  printf("%s\n\n", (const char*) log.string());
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
  StringBuilder log("===< KeyValuePair Value Translation >=========================\n");

  uint32_t val0  = (uint32_t) randomUInt32();
  uint16_t val1  = (uint16_t) randomUInt32();
  uint8_t  val2  = (uint8_t)  randomUInt32();
  int32_t  val3  = (int32_t)  randomUInt32();
  int16_t  val4  = (int16_t)  randomUInt32();
  int8_t   val5  = (int8_t)   randomUInt32();
  float    val6  = (float)    randomUInt32()/1000000.0f;
  double   val7  = (double)randomUInt32()/(double)randomUInt32();
  Vector3<float> val8(randomUInt32()/1000000.0f, randomUInt32()/1000000.0f, randomUInt32()/1000000.0f);

  KeyValuePair a(val0, "uint32");
  a.append(val1, "uint16");
  a.append(val2, "uint8");
  a.append(val3, "int32");
  a.append(val4, "int16");
  a.append(val5, "int8");
  a.append(val6, "float");
  a.append(val7, "double");
  a.append(&val8, "Vector3<f>");
  a.printDebug(&log);

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
                    log.concatf("Value Translation tests pass.\n");
                    return_value = 0;
                  }
                  else log.concat("Failed to vet Vector3<float> --> Vector3<int32>\n");
                }
                else log.concat("Failed to vet double --> int32_t\n");
              }
              else log.concat("Failed to vet float --> int8\n");
            }
            else log.concat("Failed to vet int8 --> int16\n");
          }
          else log.concat("Failed to vet int16 --> int32\n");
        }
        else log.concat("Failed to vet int32 --> double\n");
      }
      else log.concat("Failed to vet uint8_t --> uint16_t\n");
    }
    else log.concat("Failed to vet uint16_t --> uint32_t\n");
  }
  else log.concat("Failed to vet uint32_t --> double\n");

  // If the safe translations passed, try the ones that should be error-cases.
  if (0 == return_value) {
  }

  printf("\n%s\n\n", (const char*) log.string());
  return return_value;
}



#if defined(CONFIG_MANUVR_CBOR)
/**
* [test_CBOR_KVP description]
* @return [description]
*/
int test_CBOR_KeyValuePair() {
  int return_value = -1;
  StringBuilder log("===< KVPs CBOR >===================================\n");
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

  KeyValuePair a(val0, "val0");
  a.append(val1)->setKey("val1");
  a.append(val2)->setKey("val2");
  a.append(val3)->setKey("val3");
  a.append(val4)->setKey("val4");
  a.append(val5)->setKey("val5");
  a.append(val6)->setKey("val6");
  a.append(&val7)->setKey("val7");
  a.append(val8, "val8");

  a.printDebug(&log);

  int8_t ret_local = a.serialize(&shuttle, TCode::CBOR);

  if (0 == ret_local) {
    log.concatf("CBOR encoding occupies %d bytes\n\t", shuttle.length());
    shuttle.printDebug(&log);
    log.concat("\n");

    KeyValuePair* r = KeyValuePair::unserialize(shuttle.string(), shuttle.length(), TCode::CBOR);

    if (nullptr != r) {
      log.concat("CBOR decoded:\n");
      r->printDebug(&log);

      log.concat("\n");
      if ((0 == r->valueWithIdx(0, &ret0)) && (ret0 == val0)) {
        if ((0 == r->valueWithIdx(1, &ret1)) && (ret1 == val1)) {
          if ((0 == r->valueWithIdx(2, &ret2)) && (ret2 == val2)) {
            if ((0 == r->valueWithIdx(3, &ret3)) && (ret3 == val3)) {
              if ((0 == r->valueWithIdx(4, &ret4)) && (ret4 == val4)) {
                if ((0 == r->valueWithIdx(5, &ret5)) && (ret5 == val5)) {
                  if ((0 == r->valueWithIdx(6, &ret6)) && (ret6 == val6)) {
                    if ((0 == r->valueWithKey("val8", &ret8)) && (ret8 == val8)) {
                      if (r->count() == a.count()) {
                        return_value = 0;
                      }
                      else log.concatf("Arg counts don't match: %d vs %d\n", r->count(), a.count());
                    }
                    else log.concatf("Failed to vet key 'value8'... %.6f vs %.6f\n", ret8, val8);
                  }
                  else log.concatf("Failed to vet key 'value6'... %.3f vs %.3f\n", (double) ret6, (double) val6);
                }
                else log.concatf("Failed to vet key 'value5'... %u vs %u\n", ret5, val5);
              }
              else log.concatf("Failed to vet key 'value4'... %u vs %u\n", ret4, val4);
            }
            else log.concatf("Failed to vet key 'value3'... %u vs %u\n", ret3, val3);
          }
          else log.concatf("Failed to vet key 'value2'... %u vs %u\n", ret2, val2);
        }
        else log.concatf("Failed to vet key 'value1'... %u vs %u\n", ret1, val1);
      }
      else log.concatf("Failed to vet key 'value0'... %u vs %u\n", ret0, val0);
    }
    else log.concat("Failed to decode KVP chain from CBOR...\n");
  }
  else log.concatf("Failed to encode KVP chain into CBOR: %d\n", ret_local);

  if (return_value) {
    a.printDebug(&log);
  }
  printf("%s\n\n", (const char*) log.string());
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
  StringBuilder log("===< KeyValuePairs CBOR Minefield >=========================\n");
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

  KeyValuePair a(val0);

  a.setKey("val0");
  a.append(val1)->setKey("val1");
  a.append(val2)->setKey("val2");  // NOTE: Mixed in with non-KVP.
  a.append(val3)->setKey("val3");
  a.append(val4)->setKey("val4");
  a.append(val5)->setKey("val5");

  a.printDebug(&log);

  int8_t ret_local = a.serialize(&shuttle, TCode::CBOR);
  if (0 == ret_local) {
    log.concatf("CBOR encoding occupies %d bytes\n\t", shuttle.length());
    shuttle.printDebug(&log);
    log.concat("\n");

    KeyValuePair* r = KeyValuePair::unserialize(shuttle.string(), shuttle.length(), TCode::CBOR);
    if (nullptr != r) {
      log.concat("CBOR decoded:\n");
      r->printDebug(&log);

      log.concat("\n");
      if ((0 == r->valueWithIdx((uint8_t) 0, &ret0)) && (ret0 == val0)) {
        if ((0 == r->valueWithIdx((uint8_t) 1, &ret1)) && (ret1 == val1)) {
          if ((0 == r->valueWithIdx((uint8_t) 2, &ret2)) && (ret2 == val2)) {
            if ((0 == r->valueWithIdx((uint8_t) 3, &ret3)) && (ret3 == val3)) {
              if ((0 == r->valueWithIdx((uint8_t) 4, &ret4)) && (ret4 == val4)) {
                if ((0 == r->valueWithIdx((uint8_t) 5, &ret5)) && (ret5 == val5)) {
                    if (r->count() == a.count()) {
                      return_value = 0;
                    }
                    else log.concatf("Arg counts don't match: %d vs %d\n", r->count(), a.count());
                }
                else log.concatf("Failed to vet key 'value5'... %u vs %u\n", ret5, val5);
              }
              else log.concatf("Failed to vet key 'value4'... %u vs %u\n", ret4, val4);
            }
            else log.concatf("Failed to vet key 'value3'... %u vs %u\n", ret3, val3);
          }
          else log.concatf("Failed to vet key 'value2'... %u vs %u\n", ret2, val2);
        }
        else log.concatf("Failed to vet key 'value1'... %u vs %u\n", ret1, val1);
      }
      else log.concatf("Failed to vet key 'value0'... %u vs %u\n", ret0, val0);
    }
    else log.concat("Failed to decode KeyValuePair chain from CBOR...\n");
  }
  else log.concat("Failed to encode KeyValuePair chain into CBOR...\n");

  if (return_value) {
    a.printDebug(&log);
  }
  printf("%s\n\n", (const char*) log.string());
  return return_value;
}
#endif  // CONFIG_MANUVR_CBOR


int test_KeyValuePair() {
  int return_value = test_KeyValuePair_KVP();
  if (0 == return_value) {
    return_value = test_KeyValuePair_InternalTypes();
    if (0 == return_value) {
      return_value = test_KeyValuePair_Value_Placement();
      if (0 == return_value) {
        //return_value = test_KeyValuePair_Value_Translation();
        #if defined(CONFIG_MANUVR_CBOR)
        if (0 == return_value) {
          return_value = test_CBOR_KeyValuePair();
          if (0 == return_value) {
            return_value = test_CBOR_Problematic_KeyValuePair();
          }
        }
        #endif  // CONFIG_MANUVR_CBOR
      }
    }
  }
  return return_value;
}



/*******************************************************************************
* Vector3 test routines
*******************************************************************************/

/**
 * [vector3_float_test description]
 * @param  log Buffer to receive test log.
 * @return   0 on success. Non-zero on failure.
 */
int vector3_float_test(StringBuilder* log) {
  float x = 0.7f;
  float y = 0.8f;
  float z = 0.01f;
  log->concat("===< Vector3<float> >===================================\n");
  Vector3<float> test;
  Vector3<float> test_vect_0(-0.4f, -0.1f, 0.4f);
  Vector3<float> *test1 = &test_vect_0;
  log->concatf("\t (test) (%.4f, %.4f, %.4f)\n", (double)(test.x), (double)(test.y), (double)(test.z));

  test(1.0f, 0.5f, 0.24f);
  log->concatf("\t (test) (%.4f, %.4f, %.4f)\n", (double)(test.x), (double)(test.y), (double)(test.z));

  test(test_vect_0.x, test_vect_0.y, test_vect_0.z);
  log->concatf("\t (test) (%.4f, %.4f, %.4f)\n", (double)(test.x), (double)(test.y), (double)(test.z));

  test(test1->x, test1->y, test1->z);
  log->concatf("\t (test) (%.4f, %.4f, %.4f)\n", (double)(test.x), (double)(test.y), (double)(test.z));

  test(x, y, z);
  log->concatf("\t (test) (%.4f, %.4f, %.4f)\n", (double)(test.x), (double)(test.y), (double)(test.z));
  printf("%s\n\n", (const char*) log->string());
  return 0;
}


/*******************************************************************************
* PriorityQueue test routines
*******************************************************************************/
// Tests for:
//   int insert(T);
//   T get(void);
//   T get(int position);
//   bool contains(T);
//   bool hasNext(void);
//   int clear(void);
int test_PriorityQueue0(StringBuilder* log) {
  int return_value = -1;
  PriorityQueue<uint32_t*> queue0;
  uint32_t vals[16] = { randomUInt32(), randomUInt32(), randomUInt32(), randomUInt32(),
                        randomUInt32(), randomUInt32(), randomUInt32(), randomUInt32(),
                        randomUInt32(), randomUInt32(), randomUInt32(), randomUInt32(),
                        randomUInt32(), randomUInt32(), randomUInt32(), randomUInt32() };
  int q_size = queue0.size();
  if (0 == q_size) {
    if (!queue0.contains(&vals[5])) {  // Futile search for non-existant value.
      // Populate the queue...
      for (int i = 0; i < (int) sizeof(vals); i++) {
        int q_pos = queue0.insert(&vals[i]);
        if (i != q_pos) {
          log->concatf("Returned index from queue insertion didn't match the natural order. %d verus %d.\n", i, q_pos);
          return -1;
        }
      }
      q_size = queue0.size();
      if (sizeof(vals) == q_size) {
        if (queue0.hasNext()) {
          bool contains_all_elements = true;
          bool contains_all_elements_in_order = true;
          for (int i = 0; i < q_size; i++) {
            contains_all_elements &= queue0.contains(&vals[i]);
            contains_all_elements_in_order &= (*queue0.get(i) == vals[i]);
          }
          if (contains_all_elements & contains_all_elements_in_order) {
            if (*queue0.get(0) == vals[0]) {
              int q_clear_val = queue0.clear();
              if (q_size == q_clear_val) {
                if (0 == queue0.size()) {
                  if (!queue0.hasNext()) {
                    return_value = 0;
                  }
                  else log->concat("hasNext() reports true, when it ought to report false.\n");
                }
                else log->concat("The queue's size ought to be zero, but it isn't.\n");
              }
              else log->concatf("clear() ought to have cleared %d value. But it reports %d.\n", q_size, q_clear_val);
            }
            else log->concat("The queue's first element return didn't match the first element.\n");
          }
          else log->concat("Queue didn't contain all elements in their natural order.\n");
        }
        else log->concat("hasNext() reports false, when it ought to report true.\n");
      }
      else log->concatf("Queue didn't take all elements. Expected %u, but got %d.\n", sizeof(vals), q_size);
    }
    else log->concat("Queue claims to have a value it does not.\n");
  }
  else log->concat("Empty queue reports a non-zero size.\n");
  return return_value;
}


// Tests for:
//   int insertIfAbsent(T);
//   bool remove(T);
//   bool remove(int position);
//   int getPosition(T);
int test_PriorityQueue1(StringBuilder* log) {
  int return_value = -1;
  PriorityQueue<uint32_t*> queue0;
  uint32_t vals[16] = { 234, 734, 733, 7456, 819, 943, 223, 936,
                        134, 634, 633, 6456, 719, 843, 123, 836 };
  int vals_accepted = 0;
  int vals_rejected = 0;
  for (int n = 0; n < 2; n++) {
    for (unsigned int i = 0; i < sizeof(vals); i++) {
      if (-1 != queue0.insertIfAbsent(&vals[i])) {
        vals_accepted++;
      }
      else {
        vals_rejected++;
      }
    }
  }
  int q_size = queue0.size();
  if (vals_accepted == q_size) {
    if (vals_rejected == sizeof(vals)) {
      // Try some removal...
      if (!queue0.remove((int) sizeof(vals))) {  // This ought to fail.
        if (!queue0.remove((int) -1)) {   // This is not a PHP array. Negative indicies are disallowed.
          if (vals_accepted == q_size) {  // Is the size unchanged?
            if (queue0.remove((int) vals_accepted - 1)) {  // Remove the last element.
              if (queue0.remove((int) 1)) {                // Remove element at position 1.
                if (queue0.remove(&vals[4])) {             // Remove the value 819.
                  if (234 == *queue0.get()) {              // Does not change queue.
                    if (234 == *queue0.dequeue()) {        // Removes the first element.
                      q_size = queue0.size();
                      if (q_size == (vals_accepted - 4)) { // Four removals have happened.
                        if (2 == queue0.getPosition(&vals[5])) {
                          if (-1 == queue0.getPosition(&vals[4])) {
                            return_value = 0;
                          }
                          else log->concat("A previously removed element was found.\n");
                        }
                        else log->concat("Known element is not at the position it is expected to be.\n");
                      }
                      else log->concat("The queue is not the expected size following removals.\n");
                    }
                    else log->concat("dequeue(): First element is wrong.\n");
                  }
                  else log->concat("get(): First element is wrong.\n");
                }
                else log->concat("Queue remove() returned failure when it ought not to have (named value).\n");
              }
              else log->concat("Queue remove() returned failure when it ought not to have (intermediary index).\n");
            }
            else log->concat("Queue remove() returned failure when it ought not to have (last index).\n");
          }
          else log->concat("Queue operation that ought not to have changed the size have done so anyhow.\n");
        }
        else log->concat("Queue remove() returned success when it ought not to have (negative index).\n");
      }
      else log->concat("Queue remove() returned success when it ought not to have (out-of-bounds index).\n");
    }
    else log->concatf("vals_rejected=%d, but should have been %d.\n", vals_rejected, sizeof(vals));
  }
  else log->concatf("Queue acceptance mismatch. q_size=%d   vals_accepted=%d   vals_rejected=%d\n", q_size, vals_accepted, vals_rejected);
  return return_value;
}


int test_PriorityQueue(void) {
  StringBuilder log("===< PriorityQueue >====================================\n");
  int return_value = -1;
  if (0 == test_PriorityQueue0(&log)) {
    if (0 == test_PriorityQueue1(&log)) {
      return_value = 0;
    }
  }

  //T dequeue(void);              // Removes the first element of the list. Return the node's data on success, or nullptr on empty queue.
  //T recycle(void);              // Reycle this element. Return the node's data on success, or nullptr on empty queue.

  //int insert(T, int priority);  // Returns the ID of the data, or -1 on failure. Makes only a reference to the payload.
  //int insertIfAbsent(T, int);   // Same as above, but also specifies the priority if successful.
  //int getPriority(T);           // Returns the priority in the queue for the given element.
  //int getPriority(int position);           // Returns the priority in the queue for the given element.
  //bool incrementPriority(T);    // Finds the given T and increments its priority by one.
  //bool decrementPriority(T);    // Finds the given T and decrements its priority by one.
  printf("%s\n\n", (const char*) log.string());
  return return_value;
}


int test_RingBuffer() {
  int return_value = -1;
  StringBuilder log("===< RingBuffer >=======================================\n");
  const int TEST_SIZE = 18;
  RingBuffer<uint32_t> a(TEST_SIZE);
  if (a.allocated()) {
    log.concatf("RingBuffer under test is using %u bytes of heap to hold %u elements.\n", a.heap_use(), a.capacity());
    if (0 == a.count()) {
      unsigned int test_num = TEST_SIZE/3;
      uint32_t val;
      log.concat("\tInserting:");
      for (unsigned int i = 0; i < test_num; i++) {
        val = randomUInt32();
        if (a.insert(val)) {
          log.concat("\nFailed to insert.\n");
          printf("%s\n\n", (const char*) log.string());
          return -1;
        }
        log.concatf(" (%u: %08x)", a.count(), val);
      }
      if (test_num == a.count()) {
        log.concat("\n\tGetting:  ");
        for (unsigned int i = 0; i < test_num/2; i++) {
          unsigned int count = a.count();
          val = a.get();
          log.concatf(" (%u: %08x)", count, val);
        }
        unsigned int n = TEST_SIZE - a.count();
        log.concatf("\n\tRingBuffer should have space for %u more elements... ", n);
        for (unsigned int i = 0; i < n; i++) {
          if (a.insert(randomUInt32())) {
            log.concatf("Falsified. Count is %u\n", a.count());
            printf("%s\n\n", (const char*) log.string());
            return -1;
          }
        }
        if (a.count() == TEST_SIZE) {
          log.concatf("Verified. Count is %u\n", a.count());
          log.concat("\tOverflowing... ");
          if (a.insert(randomUInt32())) {
            log.concatf("Is handled correctly. Count is %u\n", a.count());
            log.concat("\tDraining... ");
            for (unsigned int i = 0; i < TEST_SIZE; i++) {
              val = a.get();
            }
            if (0 == a.count()) {
              log.concat("done.\n\tTrying to drive count negative... ");
              if (0 == a.get()) {
                if (0 == a.count()) {
                  log.concat("pass.\n");
                  return_value = 0;
                }
                else log.concatf("Count should still be 0 but is %u\n", a.count());
              }
              else log.concatf("Get on an empty buffer should return 0.\n");
            }
            else log.concatf("Count should have been 0 but is %u\n", a.count());
          }
          else log.concatf("Sadly worked. Count is %u\n", a.count());
        }
        else log.concatf("Count mismatch. Got %u but was expecting %u.\n", a.count(), TEST_SIZE);
      }
      else log.concatf("Fairly certain we inserted %u elements, but the count says %u.\n", test_num, a.count());
    }
    else log.concatf("Newly created RingBuffers ought to be empty. This one reports %u.\n", a.count());
  }
  else log.concat("\nFailed to allocate.\n");

  printf("%s\n\n", (const char*) log.string());
  return return_value;
}


/**
* UUID battery.
* @return 0 on pass. Non-zero otherwise.
*/
int test_UUID() {
  StringBuilder log("===< UUID >=============================================\n");
  StringBuilder temp;
  UUID test0;
  UUID test1;

  // Do UUID's initialize to zero?
  for (int i = 0; i < 16; i++) {
    if (0 != *((uint8_t*) &test0.id[i])) {
      log.concat("UUID should be initialized to zeros. It was not. Failing...\n");
      printf("%s\n\n", (const char*) log.string());
      return -1;
    }
  }

  // Does the comparison function work?
  if (uuid_compare(&test0, &test1)) {
    log.concat("UUID function considers these distinct. Failing...\n");
    temp.concat((uint8_t*) &test0.id, sizeof(test0));
    temp.printDebug(&log);
    temp.clear();
    temp.concat((uint8_t*) &test1.id, sizeof(test1));
    temp.printDebug(&log);
    printf("%s\n\n", (const char*) log.string());
    return -1;
  }
  uuid_gen(&test0);
  // Does the comparison function work?
  if (0 == uuid_compare(&test0, &test1)) {
    log.concat("UUID function considers these the same. Failing...\n");
    temp.concat((uint8_t*) &test0.id, sizeof(test0));
    temp.printDebug(&log);
    temp.clear();
    temp.concat((uint8_t*) &test1.id, sizeof(test1));
    temp.printDebug(&log);
    printf("%s\n\n", (const char*) log.string());
    return -1;
  }

  // Generate a whole mess of UUID and ensure that they are different.
  for (int i = 0; i < 10; i++) {
    temp.concat((uint8_t*) &test0.id, sizeof(test0));
    log.concat("temp0 bytes:  ");
    temp.printDebug(&log);
    temp.clear();

    if (0 == uuid_compare(&test0, &test1)) {
      log.concat("UUID generator gave us a repeat UUID. Fail...\n");
      printf("%s\n\n", (const char*) log.string());
      return -1;
    }
    uuid_copy(&test0, &test1);
    if (0 != uuid_compare(&test0, &test1)) {
      log.concat("UUID copy appears to have failed...\n");
      temp.concat((uint8_t*) &test0.id, sizeof(test0));
      temp.printDebug(&log);
      temp.clear();
      temp.concat((uint8_t*) &test1.id, sizeof(test1));
      temp.printDebug(&log);
      printf("%s\n\n", (const char*) log.string());
      return -1;
    }
    uuid_gen(&test0);
  }

  char str_buffer[40] = "";
  uuid_to_str(&test0, str_buffer, 40);
  log.concatf("test0 string: %s\n", str_buffer);
  log.concat("uuid_to_sb(test0): ");
  uuid_to_sb(&test0, &log);
  log.concat("\n");

  uuid_from_str(str_buffer, &test1);
  log.concat("temp1 bytes:  ");
  temp.concat((uint8_t*) &test1.id, sizeof(test1));
  temp.printDebug(&log);

  // TODO: This is the end of the happy-path. Now we should abuse the program
  // by feeding it garbage and ensure that its behavior is defined.

  printf("%s\n\n", (const char*) log.string());
  return 0;
}

/****************************************************************************************************
* The main function.                                                                                *
****************************************************************************************************/
int data_structure_main() {
  int ret = 1;   // Failure is the default result.
  StringBuilder out;
  printTypeSizes(&out);

  if (0 == test_PriorityQueue()) {
    if (0 == vector3_float_test(&out)) {
      if (0 == test_KeyValuePair()) {
        if (0 == test_UUID()) {
          if (0 == test_RingBuffer()) {
            printf("**********************************\n");
            printf("*  DataStructure tests all pass  *\n");
            printf("**********************************\n");
            ret = 0;
          }
          else printTestFailure("RingBuffer");
        }
        else printTestFailure("UUID");
      }
      else printTestFailure("KeyValuePair");
    }
    else printTestFailure("Vector3");
  }
  else printTestFailure("PriorityQueue");

  return ret;
}
