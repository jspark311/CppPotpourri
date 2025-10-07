/*
File:   DataSharing.cpp
Author: J. Ian Lindsay
Date:   2022.08.12

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


Message usage:
--------------------------------------------------------------------------------
LIST:
  Requests or delivers a list of keys and values presently exposed via the link.
  Hosts are expected to provide notices of removal and addition, if so
  configured.

VAL:
  Requests or delivers a value. Values may be automatically sent (change notice)
  or polled, depending on configuration.

CONF:
  Establishes a mutual behavioral agreement on how to handle value updates and
  make best-use of the link.

*/

#include "../M2MLink.h"
#include "../../TimeSeries/SensorFilter.h"
#include "../../Vector3.h"


/*******************************************************************************
* LinkDataHost
*******************************************************************************/

LinkDataHost::LinkDataHost(M2MLink* l) {
}


LinkDataHost::~LinkDataHost() {
}


int8_t LinkDataHost::listData(C3PValue*) {
  int8_t ret = -1;
  return ret;
}

int8_t LinkDataHost::unlistData(C3PValue*) {
  int8_t ret = -1;
  return ret;
}

int8_t LinkDataHost::listData(KeyValuePair*) {
  int8_t ret = -1;
  return ret;
}

int8_t LinkDataHost::unlistData(KeyValuePair*) {
  int8_t ret = -1;
  return ret;
}



/* Implementation of M2MService. Trades messages with a link. */
int8_t LinkDataHost::_handle_msg(uint32_t tag, M2MMsg*);
int8_t LinkDataHost::_poll_for_link(M2MLink*);



/*******************************************************************************
* LinkDataClient
*******************************************************************************/

LinkDataClient::LinkDataClient(M2MLink* l) {
}


LinkDataClient::~LinkDataClient() {
}

/* Implementation of M2MService. Trades messages with a link. */
int8_t LinkDataClient::_handle_msg(uint32_t tag, M2MMsg*);
int8_t LinkDataClient::_poll_for_link(M2MLink*);
