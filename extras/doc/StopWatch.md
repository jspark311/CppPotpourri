# StopWatch

A class for timing how many microseconds something takes. The same StopWatch can
be used for multiple samples, and will generate the following aggregate data about the timing:

  * `bestTime()`: Best-case runtime
  * `worstTime()`: Worst-case runtime
  * `lastTime()`: The last time recorded
  * `meanTime()`: The arithmetic mean of the runtime
  * `totalTime()`: The total number of microseconds spent for this purpose
  * `executions()`: The number of times that the class measured

The class can be reset by calling `reset()`.

Timing is started by calling `markStart()`, which is as light-weight as possible.

Timing is stoped (and processed) by calling `markStop()`. All the processing for timing data is then refreshed.

Many successive calls to `markStart()` will only discard the previous calls. Only the most recent `markStart()`
is considered, and only once `markStop()` is called.

## Constraints

The class uses uint32 to store all data. Overflow conditions are not tested anywhere in the class, except for
the timer reads. So if `micros()` wraps, the class will still give correct results. But if `totalTime()` wraps,
the class will not report it, and will give incorrect data. If you need to measure an event that takes
more than 4294, or less than 1 microsecond, this class will not work.

The timer doesn't round. So every given sample will be +/- 500ns. This error accumulates linearly in `totalTime()`.
So if you care about the error to that level of detail, use `executions()` to control for it.


## Usage example

    static StopWatch task_timer;

    void some_fxn() {
      task_timer.markStart();
      // Do something that takes more than 1us.
      task_timer.markStop();
    }

    void main() {
      StringBuilder output;
      for (int i = 0; i < MANY_TIMES; i++) {
        some_fxn()
      }
      // Now you can show the results.
      StopWatch::printDebugHeader(&output);  // Print the header for the data.
      task_timer.printDebug("some_fxn()", &output);
      printf("%s", (const char*) output.string());
    }


#### Dependencies

  * This class needs StringBuilder for output. Excision of the print functions will eliminate the dependency.
  * The platform must supply (or alias) an accurate `micros()` function.
