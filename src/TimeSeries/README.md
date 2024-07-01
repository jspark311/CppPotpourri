# Tools for arrays of sequential numeric data

TODO: The classes discussed also have a "3" variant (EG, `TimeSeries3`). These vector classes should be collapsed into TimeSeries, and handled as would be any other numeric type that is not compound.

### NOTE: Vector conventions within C3P:

Like line-terminators, C3P has a preferred internal convention for right-handed vectors.

## TimeSeries

`TimeSeries` is a glorified ring buffer with statistical and change-notice features. Its intended purpose was to accept and organize samples from hardware sensors. But it can serve as a unit-controlled sample organizer for any data which might be used with the filtering interfaces.

## SensorFilter

`SensorFilter` is (TODO: *should become*) a class that applies filtering to numeric arrays.
