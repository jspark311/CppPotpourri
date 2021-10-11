/*
File:   ManuvrLinkTests.cpp
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


This program runs tests against the M2M communication class.
*/

/*******************************************************************************
* Basic ManuvrLink functionality
*******************************************************************************/

void callback_vlad(uint32_t tag, XenoMessage* msg) {
  printf("callback_vlad(0x%x): \n", tag, msg->uniqueId());
}


void callback_carl(uint32_t tag, XenoMessage* msg) {
  printf("callback_carl(0x%x): \n", tag, msg->uniqueId());
}


/*
* Setup two Link objects, and connect them together.
* Note that this test is entirely synthetic. The pathway looks like this...
*   callback_vlad <---> vlad <---> carl <---> callback_carl
* In a real-world application, it would have a transport in the middle...
*   callback_vlad <---> vlad <---> UART <---> UART <---> carl <---> callback_carl
* ...or something similar.
*/
int link_tests_build_and_connect(ManuvrLink* vlad, ManuvrLink* carl) {
  StringBuilder log("===< ManuvrLink Build and connect >====================================\n");
  int ret = -1;
  if ((nullptr != vlad) & (nullptr != carl)) {
    // Connect Vlad's output to Carl's input, and Carl's output to Vlad's input.
    vlad->setOutputTarget(carl);
    carl->setOutputTarget(vlad);
    // Now connect each of them to their respective application callbacks.
    vlad->setCallback(callback_vlad);
    carl->setCallback(callback_carl);
  }
  else log.concat("Failed to allocate two ManuvrLinks.\n");
  printf("%s\n\n", (const char*) log.string());
  return ret;
}


int link_tests_simple_messages(ManuvrLink* vlad, ManuvrLink* carl) {
  StringBuilder log("===< ManuvrLink Simple messages >====================================\n");
  int ret = -1;
  printf("%s\n\n", (const char*) log.string());
  return ret;
}


int link_tests_complex_messages(ManuvrLink* vlad, ManuvrLink* carl) {
  StringBuilder log("===< ManuvrLink complex messages >====================================\n");
  int ret = -1;
  printf("%s\n\n", (const char*) log.string());
  return ret;
}


int link_tests_exotic_encodings(ManuvrLink* vlad, ManuvrLink* carl) {
  StringBuilder log("===< ManuvrLink exotic encodings >====================================\n");
  int ret = -1;
  printf("%s\n\n", (const char*) log.string());
  return ret;
}


int link_tests_message_flood(ManuvrLink* vlad, ManuvrLink* carl) {
  StringBuilder log("===< ManuvrLink message flood >====================================\n");
  int ret = -1;
  printf("%s\n\n", (const char*) log.string());
  return ret;
}


int link_tests_interrupted_transport(ManuvrLink* vlad, ManuvrLink* carl) {
  StringBuilder log("===< ManuvrLink interrupted transport >====================================\n");
  int ret = -1;
  printf("%s\n\n", (const char*) log.string());
  return ret;
}


int link_tests_corrupted_transport(ManuvrLink* vlad, ManuvrLink* carl) {
  StringBuilder log("===< ManuvrLink corrupted transport >====================================\n");
  int ret = -1;
  printf("%s\n\n", (const char*) log.string());
  return ret;
}



/**
* This is the root of the ManuvrLink tests.
*
* @return 0 on success. Nonzero otherwise.
*/
int manuvrlink_main() {
  ManuvrLinkOpts opts_vlad(40, 1024);
  ManuvrLinkOpts opts_carl(40, 1024);
  ManuvrLink* vlad = new ManuvrLink(&opts_vlad);  // One half of the link.
  ManuvrLink* carl = new ManuvrLink(&opts_carl);  // One half of the link.
  int ret = link_tests_build_and_connect(vlad, carl);
  if (0 == ret) {
    ret = link_tests_simple_messages(vlad, carl);
    if (0 == ret) {
      ret = 0;
    }
    else printTestFailure("link_tests_simple_messages");
  }
  else printTestFailure("link_tests_build_and_connect");

  if (0 == ret) {
    printf("**********************************\n");
    printf("*  ManuvrLink tests all pass     *\n");
    printf("**********************************\n");
  }
  return ret;
}
