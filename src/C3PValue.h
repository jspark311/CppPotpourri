/*
File:   C3PValue.h
Author: J. Ian Lindsay
Date:   2023.07.29

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


An abstract typeless data container class. This is used to support type
  abstraction of our internal types, and cuts down on templating elsewhere. It
  is also used as an intermediary for parsers and packers.
*/

#include <inttypes.h>
#include <stddef.h>
#include "EnumeratedTypeCodes.h"

class StringBuilder;


// TODO: This needs to eat all of the type polymorphism in KeyValuePair.
class C3PValue {
  public:
    const TCode TCODE;

    C3PValue(const TCode TC) : TCODE(TC) {};
    ~C3PValue() {};

    int8_t serialize(StringBuilder*, TCode);
    int8_t deserialize(StringBuilder*, TCode);
    void   toString(StringBuilder*);


  private:
    uint8_t       _flags      = 0;
    uint32_t      _len        = 0;
    intptr_t      _target_mem = 0;     // Type-punned memory.
};
