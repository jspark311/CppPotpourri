/*
File:   Vector3Tests.cpp
Author: J. Ian Lindsay
Date:   2023.09.29

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


This program runs tests against the Vector3 template.
*/


/*******************************************************************************
* Vector3 test routines
*******************************************************************************/

/**
 * [vector3_float_test description]
 * @param  log Buffer to receive test log.
 * @return   0 on success. Non-zero on failure.
 */
int vector3_float_test() {
  StringBuilder log;
  int return_value = -1;
  bool print_vectors = true;
  Vector3<float>* print0 = nullptr;
  Vector3<float>* print1 = nullptr;
  Vector3<float>* print2 = nullptr;

  Vector3<float> x_axis(1, 0, 0);
  Vector3<float> y_axis(0, 1, 0);
  Vector3<float> z_axis(0, 0, 1);

  printf("===< Vector3<float> >===================================\n");
  float x0 = randomUInt32()/(float)randomUInt32();
  float y0 = randomUInt32()/(float)randomUInt32();
  float z0 = randomUInt32()/(float)randomUInt32();
  float x1 = randomUInt32()/(float)randomUInt32();
  float y1 = randomUInt32()/(float)randomUInt32();
  float z1 = randomUInt32()/(float)randomUInt32();
  float x2 = randomUInt32()/(float)randomUInt32();
  float y2 = randomUInt32()/(float)randomUInt32();
  float z2 = randomUInt32()/(float)randomUInt32();
  float x3 = randomUInt32()/(float)randomUInt32();
  float y3 = randomUInt32()/(float)randomUInt32();
  float z3 = randomUInt32()/(float)randomUInt32();
  float xR0 = x0 + x1;
  float yR0 = y0 + y1;
  float zR0 = z0 + z1;
  float xR1 = x0 * 5;
  float yR1 = y0 * 5;
  float zR1 = z0 * 5;
  float xR2 = x0 - x1;
  float yR2 = y0 - y1;
  float zR2 = z0 - z1;
  Vector3<float> test;
  Vector3<float> test_vect_0(x0, y0, z0);
  Vector3<float> test_vect_1(x1, y1, z1);
  Vector3<float> test_vect_2(x2, y2, z2);
  Vector3<float> test_vect_4(&test_vect_2);
  Vector3<float> test_vect_3(x3, y3, z3);
  Vector3<float> test_vect_5;
  test_vect_5.set(x0, y0, z0);

  Vector3<float> result_vect_0 = test_vect_0 + test_vect_1;
  Vector3<float> result_vect_1 = test_vect_0 * 5;
  Vector3<float> result_vect_2 = test_vect_0 - test_vect_1;
  Vector3<float> result_vect_3(x3, y3, z3);
  Vector3<float> result_vect_4 = -test_vect_1;
  Vector3<float> result_vect_5;
  result_vect_5.set(&test_vect_0);
  result_vect_5 += test_vect_1;

  float length_r_0 = test_vect_0.length();
  float length_r_1 = test_vect_1.length();
  float length_r_2 = test_vect_1.length_squared();

  print0 = &test_vect_0;
  print1 = &test_vect_1;
  print2 = &result_vect_0;

  if ((result_vect_0.x == xR0) && (result_vect_0.y == yR0) && (result_vect_0.z == zR0)) {
    print2 = &result_vect_5;
    if ((result_vect_5.x == xR0) && (result_vect_5.y == yR0) && (result_vect_5.z == zR0)) {
      print2 = &result_vect_1;
      if ((result_vect_1.x == xR1) && (result_vect_1.y == yR1) && (result_vect_1.z == zR1)) {
        print2 = &result_vect_2;
        if ((result_vect_2.x == xR2) && (result_vect_2.y == yR2) && (result_vect_2.z == zR2)) {
          if (round(1000 * length_r_1) == round(1000 * sqrt(length_r_2))) {
            print0 = &result_vect_0;
            print1 = &result_vect_4;
            print2 = &test_vect_0;
            result_vect_0 += result_vect_4;
            result_vect_0 -= test_vect_0;
            if (0 == round(1000 * result_vect_0.length())) {
              float length_r_3 = test_vect_3.length();
              float scalar_0   = test_vect_3.normalize();
              if (1000 == round(1000 * test_vect_3.length())) {
                if (1000 == round(1000 * scalar_0*length_r_3)) {
                  if (result_vect_3 != test_vect_3) {
                    result_vect_3.normalize();   // Independently normalized vector.
                    if (result_vect_3 == test_vect_3) {
                      print0 = &result_vect_3;
                      print1 = &test_vect_3;
                      float angle_0 = Vector3<float>::angle_normalized(result_vect_3, test_vect_3);
                      if (0.0 == round(100 * angle_0)) {
                        test_vect_2.reflect(x_axis);
                        float angle_1 = Vector3<float>::angle(test_vect_2, x_axis);
                        float angle_2 = Vector3<float>::angle(test_vect_2, test_vect_4);
                        if (round(100 * angle_1*2) == round(100 * angle_2)) {
                          const float RENORM_SCALAR = 6.5f;
                          result_vect_3 *= RENORM_SCALAR;   // Stretch.
                          if ((RENORM_SCALAR*1000) == round(1000 * result_vect_3.length())) {
                            // Normalize to a given length.
                            result_vect_3.normalize(result_vect_3.length());
                            if (100 == round(100 * result_vect_3.length())) {
                              Vector3<float> cross_product = test_vect_0 % test_vect_1;
                              float angle_3 = Vector3<float>::angle(cross_product, test_vect_0);
                              float angle_4 = Vector3<float>::angle(cross_product, test_vect_1);
                              if ((round(100 * (PI/2)) == round(100 * angle_4)) && (round(100 * angle_3) == round(100 * angle_4))) {
                                printf("Vector3 tests pass.\n");
                                print_vectors = false;
                                return_value = 0;
                              }
                              else printf("The cross-product of two vectors was not orthogonal to both. %.3f and %.3f.\n", (double) angle_3, (double) angle_4);
                            }
                            else printf("Failed vector Scaling/renormalizing.\n");
                          }
                          else printf("Scaled vector should be length %.3f, but got %.3f.\n", (double) RENORM_SCALAR, (double) result_vect_3.length());
                        }
                        else printf("The angle between vector0 and its reflection about vector1 should be twice the angle between vector0 nad vector1, but got %.3f and %.3f, respectively.\n", (double) angle_1, (double) angle_2);
                      }
                      else printf("The angle between two equal vectors should be 0.0, but got %.6f.\n", (double) angle_0);
                    }
                    else printf("Failed vector equality test.\n");
                  }
                  else printf("Failed vector inequality test.\n");
                }
                else printf("The scalar value returned by normalize (%.3f) doesn't comport with the original length (%.3f).\n", (double) scalar_0, (double) length_r_3);
              }
              else printf("Normalized vector should be length 1.0, but got %.3f.\n", (double) test_vect_3.length());
            }
            else printf("Failed test of -= operator.\n");
          }
          else printf("Failed len^2.\n");
        }
        else printf("Failed vector subtraction.\n");
      }
      else printf("Failed vector multiplication.\n");
    }
    else printf("Failed test of += operator.\n");
  }
  else printf("Failed vector addition.\n");

  if (print_vectors) {
    printf("\top0:    (%.4f, %.4f, %.4f)\n", (double)(print0->x), (double)(print0->y), (double)(print0->z));
    printf("\top1:    (%.4f, %.4f, %.4f)\n", (double)(print1->x), (double)(print1->y), (double)(print1->z));
    printf("\tresult: (%.4f, %.4f, %.4f)\n", (double)(print2->x), (double)(print2->y), (double)(print2->z));
  }

  printf("%s\n\n", (const char*) log.string());
  return return_value;
}


void print_types_vector3() {
  printf("\tVector3<uint8>           %u\t%u\n", sizeof(Vector3<uint8_t>),  alignof(Vector3<uint8_t>));
  printf("\tVector3<int32>           %u\t%u\n", sizeof(Vector3<int32_t>),  alignof(Vector3<int32_t>));
  printf("\tVector3<float>           %u\t%u\n", sizeof(Vector3<float>),  alignof(Vector3<float>));
  printf("\tVector3<double>          %u\t%u\n", sizeof(Vector3<double>),  alignof(Vector3<double>));
}




/*******************************************************************************
* The main function
*******************************************************************************/

int vector3_test_main() {
  int ret = 1;   // Failure is the default result.
  const char* const MODULE_NAME = "Vector3";
  printf("===< %s >=======================================\n", MODULE_NAME);
  if (0 == vector3_float_test()) {
    ret = 0;
  }

  return ret;
}
