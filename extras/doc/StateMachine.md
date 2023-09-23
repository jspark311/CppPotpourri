# StateMachine Template

A template for a (very) finite state machine with enum controlled states.

It is envisioned that this class would be extended by a stateful class that
  wants to delegate state logic to this template, and provides protected
  overrides for the functions that amount to a check and a change callback.

Apart from a few read-only accessors, the API to `StateMachine` is almost entirely
protected, and intended to be used by the extending class logic.

### Functions to set future states
  * `_fsm_set_route(int count, ...)` Clobbers any existing future state plan, and replaces it with the one given.
  * `_fsm_append_route(int count, ...)` Appends the given state plan, to the one that might already exist.
  * `_fsm_append_state(T)` Append a state to the state plan.
  * `_fsm_prepend_state(T)` Prepend a state to the state plan (use with great care).

## Constraints

  * `StateMachine` is intended to be asynchronous, and must be polled.
  * The state-planning functions check state codes for validity against the given list, but do
    not error-check the validity of the state traversal order it is instructed to
    take. It just adds valid state codes to the list of future states. Knowledge of state maps,
    behaviors, and relations are the responsibility of the child class.
  * It is the responsibility of either `_fsm_poll()` or `_fsm_set_position(T)` (and thus,
    of the extending class) to observe `_fsm_is_waiting()` (if it is to be observed).

## Usage example

Typical usage is fairly bureaucratic. And it starts with a clean definition of each state. This is
done by a pair of classes named `EnumDef` and `EnumDefList`...

```
enum class StateTest : uint8_t {
  UNINIT =  0,  // The init state is unknown.
  STATE_0,
  STATE_1,
  IDLE,
  STATE_2,
  STATE_3,
  INVALID
};

const EnumDef<StateTest> _ENUM_LIST[] = {
  { StateTest::UNINIT,  "UNINIT"},
  { StateTest::STATE_0, "STATE_0"},
  { StateTest::STATE_1, "STATE_1"},
  { StateTest::IDLE,    "IDLE"},
  { StateTest::STATE_2, "STATE_2"},
  { StateTest::STATE_3, "STATE_3"},
  { StateTest::INVALID, "INVALID", (ENUM_WRAPPER_FLAG_CATCHALL)}
};
const EnumDefList<StateTest> FSM_STATE_LIST(&_ENUM_LIST[0], (sizeof(_ENUM_LIST) / sizeof(_ENUM_LIST[0])));
```

Next, we would define our stateful object...
```
class Example_FSM : public StateMachine<StateTest> {
  public:
    // The example driver will be named, take a list of defined states. The first
    //   state is declared to be UNINIT, and we expect to plan out no more than
    //   15 states in advance.
    Example_FSM() : StateMachine<StateTest>("Example_FSM", &FSM_STATE_LIST, StateTest::UNINIT, 15) {};
    ~Example_FSM() {};

    // The FSM class will need to be polled to check for, and take transitive action.
    // This might be done by many means, but the simplest is to just expose the template's
    //   protected method, and poll() the child object regularly.
    inline int8_t poll() {   return _fsm_poll();     };

    // This is also a common thing to do in an extending class...
    inline bool isIdle() {   return (StateTest::IDLE == currentState());   };

    // Things like hardware drivers often have several sophisticated things they
    //   need to do before being able to present themselves as ready-for-use.
    int example_init() {
      return _fsm_set_route(3, StateTest::STATE_0, StateTest::STATE_1, StateTest::IDLE);
    };

    // After the class has passed through its init stages, high-level asynchronous
    //   requests can be wrapped up neatly...
    int run_business_loop() {
      if (isIdle()) {
        return _fsm_set_route(3, StateTest::STATE_2, StateTest::STATE_3, StateTest::IDLE);
      }
      return -1;
    };


  private:
    /* Mandatory overrides from StateMachine. */
    // These are the required functions to use the state machine. They ask the
    //   the driver implementation if it is ok to leave the current state (_fsm_poll()),
    //   and if the next state was successfully entered (_fsm_set_position(<T>)).
    // These are pure-virtual in the template, so if they are not implemented on the
    //   same enum as those that were passed in as definition, compilation will fail.
    int8_t _fsm_poll();                  // Polling for state exit.
    int8_t _fsm_set_position(StateTest); // Attempt a state entry.
};
```

The real magic of the template is achieved in the `_fsm_poll()` and `_fsm_set_position(T)` functions.
These ought to be where logic is contained for the two questions:

  * Is state exit criteria met for the _current_ state? `_fsm_poll()`
  * Is state entry criteria met for the _given_ (typically next) state? `_fsm_set_position(StateTest new_state)`

```
int8_t Example_FSM::_fsm_poll() {
  int8_t ret = -1;
  bool fsm_advance = false;
  if (_fsm_is_waiting()) {
    return fsm_advance;
  }

  switch (currentState()) {
    // Exit conditions:
    case StateTest::UNINIT:
    case StateTest::STATE_0:
    case StateTest::STATE_1:
    case StateTest::STATE_2:
    case StateTest::STATE_3:
      fsm_advance = true;
      break;

    case StateTest::IDLE:
      fsm_advance = !_fsm_is_stable();
      break;

    default:   // Can't exit from an unknown state.
      ret = -1;
      break;
  }

  // If the current state's exit criteria is met, we advance the FSM.
  if (fsm_advance & (-1 != ret)) {
    ret = (0 == _fsm_advance()) ? 1 : 0;
  }
  return ret;
}


int8_t Example_FSM::_fsm_set_position(StateTest new_state) {
  int8_t ret = -1;
  bool state_entry_success = false;   // Succeed by default.
  switch (new_state) {
    // Entry conditions:
    case StateTest::UNINIT:
    case StateTest::IDLE:
    case StateTest::STATE_0:
    case StateTest::STATE_1:
    case StateTest::STATE_2:
    case StateTest::STATE_3:
      state_entry_success = true;
      break;

    default:
      break;
  }

  if (state_entry_success) {
    printf("State %s ---> %s", _fsm_state_string(currentState()), _fsm_state_string(new_state));
    ret = 0;  // By returning 0, the FSM template will update the states.
  }
  return ret;
}
```


#### Dependencies

  * This class needs `EnumWrapper` for state definition.
  * This class needs `RingBuffer` for state route planning.
  * This class needs `StringBuilder` for output. Excision of the print functions will eliminate the dependency.
  * The platform must supply (or alias) an accurate `millis()` function for rate-limiting features.
