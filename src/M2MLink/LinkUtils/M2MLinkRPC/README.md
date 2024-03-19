# M2MLinkRPC

## Usage notes:

RPCs, although similar in function to a string-based console are not quite the
  same use-case.

#### ParsingConsole

`ParsingConsole` is a buffet of human-friendly (very expensive) string-based
  handlers. Those handlers are implemented by the classes themselves, and thus
  have snowflake semantics.
String-based consoles are intended to be used as a "maintenance hatch" during
  design and development. They are highly-granular in scope, and are bothersome
  to use if deployed to the field.


#### M2MLinkRPC_Host

`M2MLinkRPC_Host`, by contrast, is defined as a monolithic interface at the
  top of program scope, with a strict type-controlled grammer. It is intended to
  create deployable device APIs.
Exposed procedure calls and their semantics are entirely determined by the
  top-level program implementing the host. There is no analog to
  `ConsoleCommand`, as there would be for a human-friendly console.



## Request message structure

The initiation of an RPC starts with the client sending a message similar to this:

    { "p": "procedure's case-sensitive name",
      "a": {
        "first_arg_key":  3.14,
        "second_arg_key": "some string data",
        ...
      }
    }

Key | Mnemonic    | Required | Type   | Value meaning
----|-------------|----------|--------|---------------
`p` | "procedure" | Yes      | String | The name of the remote function to call.
`a` | "arguments" | No       | *      | Free-form argument data to the named RP.


### The `p` key
This key is required for all valid RPC requests. It need not be printable, but
  must not contain the zero byte in any position other than the final byte. It
  is case-sensitive.

### The `a` key
If `a` is provided, it must conform to the semantics defined by the host (see
  the example for listing RPCs).


## Response message structure

Upon receiving the request, the host will compare `p` against all available
  exposed functions and begin execution if one is found with compatible
  semantics. It will eventually respond with one or more messages similar to
  this:

    { "t": 1.
      "e": -1,
      "r": {
        "some_response_key": "some response data"
        ...
      }
    }

Key | Mnemonic   | Required | Type  | Value meaning
----|------------|----------|-------|---------------
`t` | "terminal" | Yes      | INT8  | Indicates the return code of the RPC to the client.
`e` | "error"    | No       | INT32 | An error specific to the procedure, and the call.
`r` | "response" | No       | *     | The type=controlled result of the RPC's execution.


### The `t` key
This will always be provided, and indicates the return code of the function to
  the client, or a specific error condition according to the following table:

`t` | Meaning
----|-------------
1   | RPC accepted and completed with success.
0   | RPC accepted and running. Expect future messages.
-1  | Completed with failure. See documentation for the `e` key.
-2  | RP named by `p` not found (or missing `p` key).
-3  | Invalid/insufficient parameters for the RP named by `p`.
-4  | A previous RPC function is already running.


### The `e` key
If `e` is provided and non-zero, it will be an error code specific to that RPC.
  Consult its documentation for interpretation.
Generally, APIs will not return `e` if they succeed with a response (although
  they are free to do so). Some APIs might return something like this to
  unambiguously indicate success without the bulk of an `r` key.

    { "t": 1.
      "e": 0 }


### The `r` key
The results of an RPC (either completed, or in-progress) are communicated back
  to the client via `r`. This might be data of any type and complexity.
  Presumably, the caller will know what to do with it (see implementation strategy notes).
Handling of memory issues surrounding `r` is entirely up to the specific RPC.
  Some RPs only run more than once for the same of conforming to the MTU
  requirements of the link, and will return immediately if given the bandwidth
  to allow it. For instance, these are two possible responses from the same RPC
  under different conditions....


#### Large RAM allocation, and large link MTU:

    { "t": 1.
      "r": {
        "title": "Data from time range",
        "row0": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19],
        "row1": [20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39],
        "row2": [40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59]
      }
    }


#### Small RAM allocation, or small link MTU:

    { "t": 0.
      "r": {
        "title": "Data from time range",
        "row0": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19]
      }
    }

    { "t": 0.
      "r": {
        "row1": [20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39]
      }
    }

    { "t": 1.
      "r": {
        "row2": [40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59]
      }
    }



## Boilerplate-provided RPs

These RPs are always provided by `M2MLinkRPC_Host`, and may be automatically invoked by `M2MLinkRPC_Client`:

### `rpc_defs`

This procedure is called automatically on link establishment to relay a list of
  hosted RPC definitions from host to client. This simplifies host handler design
  by allowing dynamic coding of type sanitizers, and detecting "semantic shear"
  before the client invokes an RPC.


#### Example request:

    { "p": "rpc_defs" }

#### Example response:
    { "t": 1.
      "r": {
        "ex_rpc_name": {
          "ex_arg_key0": INT32,
          "ex_arg_key1": UINT8,
          "ex_arg_key2": STRING
        },
        "rpc_without_args": {}
      }
    }
