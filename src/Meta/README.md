# Meta

This directory is the top of a special path in the library. It holds header
logic for such things as build configuration, compiler tweaks, and a few other
concerns which we don't wish to pollute the rest of the code base.

## Contents, purpose, and constraint

Nothing in `Meta` is built. But some things are included. Any CPP files under
`Meta` are either illustrations only, or are to be handled according to where
specifically they are (details below).

Top-level header files in this directory _are_ included near the top of the
inclusion hierarchy. Inclusion of CppPotpourri.h activates this inclusion chain
reliably, but other abstraction layers near the top (or platform packages) may
also do so. Header logic must thus strive for idempotency in its consequences,
or at least make careful notes where this is idempotency is not reliable.

Nothing in this directory should be included by software outside of this
library, although the results of their inclusion can be relied upon to occur if
`CppPotpourri.h` is pulled in.


### `Rationalizer.h`

This header is called to collect build parameters (usually provided via `-DOPTION`)
and sanitize them. This is where build-wide option flags are controlled for the
benefit of their reliability elsewhere in this library (and possibly others).

This is also the file that sets certain platform-level defaults that C3P must
take as assumptions if it is built independently of a notion of a platform that
constrains such assumptions. This includes, but is not limited to...

  * Threading model (if any)
  * Timer resolution
  * Cryptographic support (if any)
  * Default logging disposition.

### `Compilers.h`

This header contains special support for compiler-specific macros, idioms,
section assignments, and so forth.

### `AntiMacro.h`

C3P provides real inline implementations of common functions that are
  traditionally implemented as macros. Those functions, and their rationales,
  are given here.

### `Intrinsics.h`

This header contains special support for architecture-specific functions that
may speed up code tremendously under the right conditions. This file should be
included directly by code that wants to leverage these optimizations.


### Bikeshed

This directory is the appropriate place for things being considered, developed,
or retained for some other historical purpose. Nothing in this directory is here
forever. It should not be included except by informed developers, and mainline
builds should never depend on anything in `Bikeshed`.


### Boneyard

This is where obsolete classes or bad ideas go to die. They will only remain
here as long as there may still be something to learn from their failure or
retirement. Once that end is served, they will be deleted. Don't code against
anything in here. Odds are, it was always garbage.
