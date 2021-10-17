/**
File:   Logger.h
Author: J. Ian Lindsay
Date:   2021.10.16

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


Personal note:
  I've taught many people over the years. When guy who is software-enabled asks
  me how to get started in hardware, I tell him to pick up a cheap Arduino and
  implement a push button from scratch. And it sounds pointless and easy, until
  you try to do it for the first time. Because buttons are surprisingly
  difficult. Contact bounce, state-tracking, long-press or not, distinguishing
  events as spurious or intentionally repeated, etc...

  When a someone who is hardware-enabled asks me how to get started in software,
  I might start telling them to write a reusable system logger from scratch.
  Because it sounds pointless and easy, until you try to do it for the fifth
  time. At which point, the smartest of us should have given up trying. So
  before I start in on my 6th attempt, I'm going to lay out my general values
  and concerns before I begin work.

  The dangers here are complexity and weight. In that order. The logger has to
  be present and operable under all conditions where log might be generated,
  and it can't rely on any other data structures in CppPotpourri which might
  generate logs.

  The logging system in ManuvrOS had a _very_ basic API (too basic), but far too
  much implementation complexity (logging was the province of the Kernel). This
  last choice caused a tremendous maintenance burden in the Kernel class, as it
  was required to be included in every class that might potentially generate
  logs. Don't do that sort of thing again. If anything, this class might should
  be a singleton of its own, apart from even platform. That said...

  Logging is fundamentally a platform choice, since platform support is
  ultimately required to print a character to a screen, file, socket, whatever.
  So this class should remain an interface (at minimum), or a pure-virtual (in
  the heaviest case), with the final implementation being given in
  ManuvrPlatform, with the rest of the platform-specific implementations of
  AbstractPlatform, I2CAdapter, et al. At that point, the platform can make
  choices about which modes of output/caching/policy will be available to the
  program.
                                           ---J. Ian Lindsay 2021.10.16 20:59:15
*/
