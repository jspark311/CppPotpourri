/*
File:   Quaternion.cpp
Author: J. Ian Lindsay
Date:   2014.03.10

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

#include "Quaternion.h"
#include "StringBuilder.h"
#include <math.h>

Quaternion::Quaternion() {
  x = 0.0f;
  y = 0.0f;
  z = 0.0f;
  w = 1.0f;
}

Quaternion::Quaternion(float n_x, float n_y, float n_z, float n_w) {
  x = n_x;
  y = n_y;
  z = n_z;
  w = n_w;
}



void Quaternion::set(float n_x, float n_y, float n_z, float n_w) {
  x = n_x;
  y = n_y;
  z = n_z;
  w = n_w;
}


float Quaternion::normalize() {
  float norm = 1.0f / (float) sqrt(w * w + x * x + y * y + z * z);
  w = w * norm;
  x = x * norm;
  y = y * norm;
  z = z * norm;
  return norm;
}


/*
* Assumes that +z is away from Earth.
*/
void Quaternion::setDown(float n_x, float n_y, float n_z) {
  float angle = acos(n_z);

  // Calculate the cross-product against (0, 0, 1)...
  float cp_x = n_y;
  float cp_y = 0 - n_x;
  float cp_z = 0;

  // Normalize the cross-product...
  float len = sqrt(cp_x*cp_x + cp_y*cp_y + cp_z+cp_z);
  if (0 != len) {
    if (len != 1.0f) {
      float inv_len = 1.0f/len;
      cp_x *= inv_len;
      cp_y *= inv_len;
      cp_z *= inv_len;
    }
  }

  float sin_theta = sin(angle / 2.0);
  w = cos(angle / 2.0);
  x = cp_x * sin_theta;
  y = cp_y * sin_theta;
  z = cp_z * sin_theta;
}


/**
* TODO: Is this output order correct?
*/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
void Quaternion::printDebug(StringBuilder* output) {
  output->concatf("(%2.5f, %2.5f, %2.5f, %2.5f) (x,y,z,w)", x, y, z, w);
}
#pragma GCC diagnostic pop
