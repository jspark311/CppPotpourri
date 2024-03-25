/*
File:   M2MService.cpp
Author: J. Ian Lindsay
Date:   2024.02.26

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

#include "M2MLink.h"

#if defined(CONFIG_C3P_M2M_SUPPORT)


/*******************************************************************************
*
*******************************************************************************/

/**
* This will delete everything in the outbound queue.
*/
M2MService::~M2MService() {
  while (_outbound.count() > 0) {
    delete _outbound.get();
  }
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void M2MService::printM2MService(StringBuilder* output) {
  output->concatf("M2MService (0x%08x)\n", SVC_TAG);
}

#endif   // CONFIG_C3P_M2M_SUPPORT
