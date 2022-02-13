# Dev notes

This is a journal associated with the development and maintenance of this library. It is being kept public in the repo to help track thoughts associated with changes to the code, where such commentary would be a distraction (or even burden) in the commentary of the source files themselves. This document is not for release notes, is not required reading to understand the library, and is mostly for the use of the author(s) and anyone who wants to understand how it came to be the way that it is without digging through commit.

---------------

Personal note:
  I've taught many people over the years. When a guy who is software-enabled asks
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

  The API to the Logger ought to support log severity, source tags, and
  build-time definitions for which levels of logging should be included in the
  binary. As before, we adopt the SYSLOG severity conventions. Because those
  work really well.
                                         `---J. Ian Lindsay 2021.10.16 20:59:15`
---------------

The Logger abstraction strategy is running on ESP-IDF (platform wrapper case).
  The design values above were taken as canon. It looks like it will work for
  all cases. Time will tell. Considering this task complete.
                                         `---J. Ian Lindsay 2022.02.13 02:58:37`
