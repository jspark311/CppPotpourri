/*
File:   M2MLinkRPCHost.cpp
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


M2MLinkRPC_Host::M2MLinkRPC_Host(M2MLink* l, const C3PDefinedRPC* const RPC_DEFS) :
  M2MService("RPC", l), _RPC_LISTING(RPC_DEFS), _rpc_context(), _rpc_running(nullptr) {}


M2MLinkRPC_Host::~M2MLinkRPC_Host() {}


/* Implementation of M2MService. Trades messages with a link. */
int8_t M2MLinkRPC_Host::_handle_msg(uint32_t tag, M2MMsg*) {
  int8_t ret = 0;
  return ret;
}
