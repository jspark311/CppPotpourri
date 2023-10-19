# Contract-bound Pipes

This directory is the top-level location of data pipelines in the library that
are bounded by contract to behave in specific ways.

Any specific type may have an interface defined here that obliges classes that
handle the type to meet specific (general) criteria for the sake of making those
classes composable into high-level flows through the program.

## Supported Types
  * `StringBuilder` via the `BufferAccepter` interface.
  * `Vector3<float>` via the `TripleAxisPipe` interface.
  * Numeric values that are controlled for both error and SI units via the `ScalarPipe` interface.

## Unsupported Types, where support would be logical

  * `Image` for pipelined image transforms. Will be very costly of RAM unless the interface contract allows for mutation. The underpinnings of `GfxUI` are heading that direction.
  * `Identity` enrichment and assurance.
