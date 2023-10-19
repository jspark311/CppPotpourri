# BufferAccepter

`BufferAccepter` is an interface pattern used throughout the library for composition of buffer
sources, sinks, and transforms.

## Conventional usage examples

### A simple class that forwards data

```
```

### A simple class that forwards data, and counts instances of the letter 'H'.

```
```

### Same as above, but now it does the comparison faster by mutating the memory

```
```

## Constraints and contract

`BufferAccepter` is implemented by inheritance, and so has vtable entries for
each of the implemented functions. The interface exposes public methods, and so
care must be taken to not create diamonds, or other similar absurdities.

### Basis of buffers, and inherited contractual obligations
`BufferAccepter::pushBuffer()` uses `StringBuilder` as its buffer abstraction.
Implementing classes that wish to leverage that fact may do so, if it doesn't
run afoul of the contract otherwise.

### Parameters and return code conformance

`BufferAccepter` operation is unidirectional, and is synchronous source-push
with bi-directional back-pressure control implemented via return code.

`pushBuffer()` may make its own decision about how much of a buffer to take (if
any). But if it takes nothing, it should return `-1`. If _all_ of the buffer is
taken, `pushBuffer()` should return `1`. If only part of a pushed buffer was taken,
`pushBuffer()` should return `0`.

If a caller wants to implement special behaviors (all-or-nothing, chunked or
zero-copy transfers, etc), it may call `BufferAccepter::bufferAvailable()` to get
a current free-space check from the `BufferAccepter` it is about to to push into.
If the `BufferAccepter` cannot meet the demands of the caller, the caller may
decline to call `pushBuffer()` at all, preferring to wait until such time as
more space vacates.

If a caller notices that `pushBuffer()` returned `0`, it may optionally check the
remaining length of the buffer it did not fully push. But it will have the
assurance that whatever remains is what entered most recently, and apart from
possibly being truncated at the beginning, its content should remain the same.


### Mutation of memory layout and ownership of heap-pointers

Implementations of `BufferAccepter` may give their own contractual assurances
regarding preservation of structured buffers. But any such assurances would be
extra-contractual with respect to `BufferAccepter`, which itself has no notion
of structure within `StringBuilder`. Absent other measures, `pushBuffer()` should
be expected to freely re-order the memory layout of the buffer to meet its own
contracts.

`BufferAccepter::pushBuffer(StringBuilder*)` is obliged to take as much of the
offered buffer as it can without overfilling its own (or its downstream if it
does no local buffering of its own), or violating its other contracts.
Under no-pressure cases, that means a seamless ownership transfer from the
source `StringBuilder` created in a caller's stack frame, all the way down into
whatever buffer sink lies at the end of the chain, with all heap memory being
free'd before the return from the original call to `pushBuffer(src)` (which
would return `1`).

A call to `pushBuffer(src)` should be assumed to invalidate all pointers
previously returned by `src`. This includes collapsed strings returned by
`src->string()`, as well as structured fragment pointers returned by
`src->position_as_int(int)`, and similar. So the caller must take care not to
violate the `StringBuilder` contract on accident.

Unless it has taken measures to artificially restrict the length of a buffer
passed into `pushBuffer()`, the calling function should _expect_ to lose all
ownership and reference to all memory it may have allocated ahead of the call.
But it should also be prepared to cope with back-pressure by noting the return
code from `pushBuffer()`, and act according to its own local constraints. If
`pushBuffer()` does not take all of the buffer it was given, what remains is
ipso facto the callers ownership and responsibility. Following a call to
`pushBuffer(src)`, both the caller and the `BufferAccepter` will own all memory
presently exposed to them (via `src` in the case of the caller, and for the
`BufferAccepter`, whatever it decided to take).


### Non-mutation of buffer content

Beyond truncating the beginning of the buffer to signify a partial take,
`pushBuffer()` is _not_ allowed to alter the content of the pushed buffer
without having first claimed it.

`pushBuffer()` is allowed to read the entire buffer it is pushed, but is _not_
allowed to claim length starting from anything but the beginning of the buffer
pushed. That is, it may not take from the middle unless it took everything
beginning at index 0. And it may not take from the end of a pushed buffer unless
it takes the entire buffer. Not all of the buffer must be _used_, but the caller
must be discharged of the responsibility of tracking each byte in the same order
as those bytes naturally occur in a pushed buffer.


### Synchronicity, and mutability of buffer pointers

`BufferAccepter::pushBuffer(StringBuilder* src)` is allowed to mutate the layout
of `src`, but only within its own stack frame. That is, `pushBuffer(src)` may
not store a reference to `src` for use after `pushBuffer()` has returned. If
an implementation of `BufferAccepter` conducts some sort of asynchronous action
on buffers that have been pushed to it, that part of the buffer _must_ be claimed
prior to `pushBuffer()` returning.
The nature of `src` is assumed to be an ephemeral stack object, and no assumptions
are made about its heap layout or content outside of `pushBuffer()`.


## Common mistakes in usage

These are the natural mistakes to make with `BufferAccepter` that represent
genuine misuse, that cannot be tested for at build-time, and cannot be covered
by in-situ safety checks.

### Asynchronous use of buffer reference
