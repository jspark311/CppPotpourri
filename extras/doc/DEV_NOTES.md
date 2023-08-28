# Dev notes

This is a journal associated with the development and maintenance of this library. It is being kept public in the repo to help track thoughts associated with changes to the code, where such commentary would be a distraction (or even burden) if it were in the source files themselves. This document is not for release notes, is not required reading to understand the library, and is mostly for the use of the author(s) and anyone who wants to understand how it came to be the way that it is without digging through commit.

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
---------------

I am moving CryptoBurrito into C3P. Keeping it as a separate repository is too
  much of a headache for basically zero benefit, considering it is API-dependent
  on C3P, and all of the hardware and platform abstraction has since been
  handled in a more general manner.

The high-level abstraction is still nascent, and shouldn't be used yet. Even
  though the last version of mbedTLS it was tested against is ancient, at least
  it compiles. I'm committing it now and bumping the MINOR version of C3P.

Going forward, hardware cryptographic drivers should reside in either
  ManuvrDrivers, or ManuvrPlatform, as appropriate.
                                         `---J. Ian Lindsay 2022.05.08 00:51:49`

---------------

Circumstances have caused me to devote some time to improve unit testing. That
  happened. Bugs were found and fixed. StringBuilder has come under efficiency
  scrutiny recently. And its unit-tests are indeed languishing near 60%
  coverage. But before I strive to improve that figure, I decided it would be
  better to clean out some of the bugs and inefficiencies for which I already
  have outstanding TODOs. That effort passed through a short-lived TODO items in
  StringBuilder, which I will replicate here, so that they do not continue to
  clutter the source file with historical notes.


TODO: Retrospective on the fragment structure...
  The reap member costs more memory (by way of alignmnet and padding) than it
  was saving in non-replication of const char* const. Under almost all usage
  patterns, and certainly the most common of them.
TODID.

TODO: Following the removal of zero-copy-on-const, there is no longer any reason
  to handle StrLL allocation as two separate steps. Much simplicity will be
  gained by doing so. If zero-copy-on-const is to make a return, it will be in
  the context of a pluggable memory model, and won't belong here anyway.
TODID: This is implemented, and passing unit tests, as-written.

TODO: Direct-castablity to a string ended up being a non-value. Re-order to
  support merged allocation.
TODID.

TODO: Merge the memory allocations for StrLLs, as well as their content. The
  following functions are hotspots for absurdities. Pay close attention to them
  before making a choice:
  1) concatHandoff(uint8_t*, int)
     Might be easy and safe to assume a split reap if we keep the pointer member
     and it isn't at the correct offset. A fragment created with merged
     allocation will always have a value for str that is a constant offset from
     its own.
TODID: This is indeed what I did.

  2) `_null_term_check()`
     This function does a regional reallocation to accommodate a null-terminator
     for safety's sake. This extra byte is _not_ accounted for in the string's
     reported length. It is strictly a safety measure that the API otherwise
     ignores. This (and a few other things that are sketchy) could be solved by
     always allocating one extra byte and assuring that it is always null.
WASDID: This was _already how things worked_. But it was scattered throughout the
  class, and probably had bugs somewhere.

  3) Can `_null_term_check()` be removed entirely?
TODID: Yup. Dropped.

TODO: Style. Replace "unsigned char" with "uint8".
TODID.

TODO: Style. Remove "this->" and use the same convention as elsewhere in the library for
  concealed members.
TODO: Style. Homogenize orders of test-for-equality to not risk accidental assignment.


                                         `---J. Ian Lindsay 2023.0827 11:03:16`
