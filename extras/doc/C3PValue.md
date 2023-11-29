# C3PValue

`C3PValue` is a container class for type-abstracted data, and is primarily used as an intermediary for parsers and packers.

## Usage example

`C3PValue` is not a template, and has inlines for supported types. To create them. it is sufficient to...

```
C3PValue value_0((double)  PI);   
C3PValue value_1((float)   PI);
C3PValue value_2((uint8_t) PI);
C3PValue value_3((int32_t) PI);
C3PValue value_4("About 3");
C3PValue value_5((char*) "About 3 (deep-copied string)");

// If it is necessary to create the container before the specific value is known...
C3PValue value_6(TCode::UINT16);

// Higher-level types are also supported via reference...
Vector3f   v3_float(1.0f, 0.8f, 0.7f);
Vector3u32 v3_u32(0, 0, 0);
Vector3i32 v3_i32(0, 0, 0);
Vector3u16 v3_u16(0, 0, 0);
Vector3i16 v3_i16(0, 0, 0);
IdentityUUID ident_uuid("SOME_ID", (char*) "18b31628-9c5e-41bb-a637-6370d126b39b");

C3PValue value_7(&v3_float);
C3PValue value_8(&v3_u32);
C3PValue value_9(&v3_i32);
C3PValue value_10(&v3_u16);
C3PValue value_11(&v3_i16);

C3PValue value_12((Identity*) &ident_uuid);

// Pointer-length compounds are also handled...
uint8_t demo_bin_field[17] = {0, };
C3PValue value_13(demo_bin_field, sizeof(demo_bin_field));

// The container class supports strict value coercion (so as long as the values
//   are type-convertible), and types are noted by their pointers.
double   fetched_value_0 = 0.0d;
float    fetched_value_1 = 0.0f;
uint8_t  fetched_value_2 = 0;
int32_t  fetched_value_3 = 0;
value_0.get_as(&fetched_value_0);
value_1.get_as(&fetched_value_1);
value_2.get_as(&fetched_value_2);
value_3.get_as(&fetched_value_3);

// Values are settable by container...
value_6.set(&value_2);


// Detection of value change...
```


## Constraints

Data referenced by `C3PValue` that is `<= sizeof(uintptr_t)` will be stored as value directly, rather than as a pointer to the data. Data bigger than this threshold will be stored by reference, and will be subject to the choices made by `reapValue(bool)`. See [Destructor behavior](#Destructor_behavior).


## Costs

Inclusion of `C3PValue` carries implications of general support for many (possibly unused) binary pathways that will make large swaths of the resulting binary opaque to `--gc-sections`. If it gets out-of-hand, pre-processor case-offs might be used in the future to limit binary bloat.


## Destructor behavior

The `C3PValue` destructor *will not* free memory referenced by pointers, unless the instance being destroyed
was instructed to do so earlier in its life-cycle.
