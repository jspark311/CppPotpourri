/*
File:   M2MLinkRPC.cpp
Author: J. Ian Lindsay
Date:   2024.03.16

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
*/

#include "M2MLinkRPC.h"



C3PRPCContext::C3PRPCContext() : _msg(nullptr), _response(nullptr) {
  wipe();
}


C3PRPCContext::~C3PRPCContext() {
  wipe();
}



int8_t C3PRPCContext::init(M2MMsg* m) {
  int8_t ret = 0;
  _msg = m;
  return ret;
}


/*
* Wipes the context for reuse.
*/
void C3PRPCContext::wipe() {
  KeyValuePair* safe = _response;
  if (nullptr != safe) {
    delete safe;
  }
  _response       = nullptr;
  _msg            = nullptr;
  _poll_count     = 0;
  _response_count = 0;
  memset(_cbytes, 0, C3PRPC_CONTEXT_BYTES);
}
