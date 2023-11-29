# KeyValuePair

`KeyValuePair` is a data type that is meant to act as a descriptive, general carrier and serialization layer for data in the program.

### Lessons learned from ManuvrOS:

This class can easily become obnoxious and brittle. ManuvrOS tried to use this
idea as a richly-typed data carrier for general internal use. Although this
strategy functioned, it was prone to memory faults and ownership confusion.

The use of this class should be restricted to working as an intermediary
between the serialized and the in-memory forms of class data. Unless/until
the API can be made to work without the problems it grew the last time.

One of the problems it grew was that it came to be doing too much. `C3PValue`
now handles the type abstraction, and `KeyValuePair` should remain concerned
with grouping C3PValues into a shape for aggregate handling.


## Usage example


## Constraints

`KeyValuePair` has members to facilitate the representation of data structured like this...

```
    "key1": {
      "key2": {
        "innerkey3": 5,
        "innerkey4": -88,
        "innerkey5": [1, 2, 3, 4, 5, 6],    // Array of uniform data
        [0.0, 1.2, 2.2, 3, true, "some string"]  // Array of heterogeneous types
      },
    }
```

This capability is implemented as a single-linked list, with key names optionally copied to a heap-allocated string. It will have overhead characteristics as you would expect.

Data referenced by `KeyValuePair` that is `<= sizeof(uintptr_t)` will be stored as value directly, rather than as a pointer to the data. Data bigger than this threshold will be stored by reference, and will be subject to the choices made by `reapKey(bool)`, and `reapValue(bool)`. See [Destructor behavior](#Destructor_behavior).

## Costs

Inclusion of `KeyValuePair` carries implications of general support for many (possibly unused) binary pathways that will make large swaths of the resulting binary opaque to `--gc-sections`. If it gets out-of-hand, pre-processor case-offs might be used in the future to limit binary bloat.


## Destructor behavior

The `KeyValuePair` destructor *will* subsequently call the destructor for the `KeyValuePair` referenced by its `_next` member, if applicable. This ensures the release of all memory associated with a list of `KeyValuePair` being handled by its head.
