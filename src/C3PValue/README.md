# C3P's internal type system

C3P uses an abstract container for typed data in cases where that data must be
communicated across I/O boundaries. Comm stacks are probably the most obvious
application, but this also includes I/O with self (as would be the case for NVM
or file-stored data).

This subsystem is largely useless to programs that do no such interchange. But
subsystems that do will lean on it heavily. It might be appropriate to view this
capability as a narrowly-implemented kind of reflection for types (without using
proper C++ reflection). Thus, we avoid its costs to build size, as well as
eliminate its implications of exception support.


## Memory model

**NOTE:** This entire subsystem is highly reliant on heap allocation. So attendant care
must be taken to prevent leaks/hard-faults and gross inefficiency.

## API

These are the core objects and ideas for this subsystem. The API is uniform for
all supported types, and can be generated from the sourcecode's autodoc.

### TCode

`TCode` is the enumerated type-code for types supported by the type abstraction.
The values of the enum are specific to C3P, and possibly even builds. With a
proper parse/pack encoding, the constancy of specific values of the `TCode`
enum _should_ remain unimportant for all users of the library.

**TODO:** Layout the values for the enum according to some broader standard, to
  the extent practical.


### C3PType

The bottom of the type stack. This polymorphic class has a single `const` instance
devoted to the handling of each specific `TCode` that a given build supports. The
API for this class doesn't itself do any memory management. It is only the basal
collection of parsers/packers/converters/metadata associated with handling a type.


#### C3PTypeConstraint<T>

This template is the granular implementation of `C3PType`.
Every `C3PType` object must have a concealed template that instructs the compiler
how to specifically implement `C3PType` for a given `TCode`.

Conversion between unequal (but compatible) types is handled by this template, as
are any potential endianness and alignment nuances associated with the type.


#### Type translation matrix

**TODO:** What interconverts into what, and how close to perfectly?


### C3PValue

Sittingg on top of `C3PType` is `C3PValue`, which is tasked with handling the
memory implications of a given value of a given `TCode`. If a concrete value
is to be used, it will come wrapped in an instance of `C3PValue`.

A `C3PValue` object cannot have its `TCode` changed after construction (it is
`const`).


### KeyValuePair

Finally, `KeyValuePair` is a child class of `C3PValue` that adds storage and
handling for a string key.






## Usage to achieve some common arrangements

For simplicity, data structure will be depicted as something like JSON, with the
understanding that pushing the data through a pack/parse cycle will result in
the same content and structure, in the same order.


### Flat

The simplest usage of the type system will simply serialize linked data in the
order that it appears in the structure.

#### Structure

```
{
  0,
  2319,
  "string",
  -61.3
}
```

#### Code

```
C3PValue val_0((uint8_t) 0);
C3PValue val_1((uint16_t) 2319);
C3PValue val_2("string");
C3PValue val_2(-61.3f);

// Or, to heap allocate...
C3PValue values((uint8_t) 0);
values.link((uint16_t) 2319);
values.link("string");
values.link(-61.3f);

```


### Arrays and Maps

By setting the `compound` flag in a `C3PValue`, a node can be designated as an
aggregate of values, such an array. Such values will be serialized depth-first.

#### Structure

```
a: 0,
b: 2319,
c: "string",
d: -61.3
}
```

#### Code

```
KeyValuePair val_0("a", (uint8_t) 0);
KeyValuePair val_1("b", (uint16_t) 2319);
KeyValuePair val_2("c", "string");
KeyValuePair val_2("d", -61.3f);

```


### Maps

#### Structure

```
a: {
  b: 2319,
  c: "string",
  d: -61.3
}
```

#### Code

```
KeyValuePair val_0("a", (uint8_t) 0);
KeyValuePair val_1("b", (uint16_t) 2319);
KeyValuePair val_2("c", "string");
KeyValuePair val_2("d", -61.3f);

C3PValue values((uint8_t) 0);
values.link((uint16_t) 2319);
values.link("string");
values.link(-61.3f);

```


### Named maps and arrays

#### Structure

```
{

}
```
