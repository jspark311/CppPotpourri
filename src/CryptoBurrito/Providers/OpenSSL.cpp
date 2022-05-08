/*
File:   OpenSSL.c
Author: J. Ian Lindsay
Date:   2016.10.01

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


Implements cryptography via OpenSSL.

OpenSSL support will use the library on the host building the binary. If the
  host cannot supply a static version of OpenSSL, this will fail to build.
*/

#if defined(WITH_OPENSSL)
#endif   // WITH_OPENSSL
