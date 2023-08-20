/*
File:   ParsingConsole.cpp
Author: J. Ian Lindsay
Date:   2020.01.05

Copyright 2020 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "ParsingConsole.h"
#include "CppPotpourri.h"

/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/

static const uint8_t DPAD_ESCAPE_SEQUENCE_U[4] = {27, 91, 65, 0};
static const uint8_t DPAD_ESCAPE_SEQUENCE_D[4] = {27, 91, 66, 0};
static const uint8_t DPAD_ESCAPE_SEQUENCE_R[4] = {27, 91, 67, 0};
static const uint8_t DPAD_ESCAPE_SEQUENCE_L[4] = {27, 91, 68, 0};


const char* const ParsingConsole::errToStr(ConsoleErr err) {
  switch (err) {
    case ConsoleErr::NONE:          return "NONE";
    case ConsoleErr::NO_MEM:        return "Out of memory";
    case ConsoleErr::MISSING_ARG:   return "Missing argument";
    case ConsoleErr::INVALID_ARG:   return "Invalid argument";
    case ConsoleErr::CMD_NOT_FOUND: return "Invalid command";
    case ConsoleErr::RESERVED:      return "Reserved err code";
  }
  return "UNKNOWN";
}


const char* const ParsingConsole::terminatorStr(LineTerm lt) {
  switch (lt) {
    case LineTerm::CR:    return "CR";
    case LineTerm::LF:    return "LF";
    case LineTerm::CRLF:  return "CRLF";
    default:              break;
  }
  return "";
}


const char* const ParsingConsole::_get_terminator(LineTerm lt) {
  switch (lt) {
    case LineTerm::CR:    return "\r";
    case LineTerm::LF:    return "\n";
    case LineTerm::CRLF:  return "\r\n";
    default:              break;
  }
  return "";
}



/*******************************************************************************
* Class boilerplate
*******************************************************************************/
/*
* Destructor
*/
ParsingConsole::~ParsingConsole() {
  _buffer.clear();
  _log.clear();
  clearHistory();
  while (0 < _cmd_list.size()) {
    // Clear out all the command definitions.
    ConsoleCommand* tmp_cmd = _cmd_list.remove(_cmd_list.size() - 1);
    if (tmp_cmd->shouldFree()) {
      delete tmp_cmd;
    }
  }
}


/*
*/
int8_t ParsingConsole::init() {
  if (_MAX_LEN < 8) {
    // Too short to make any sense.
    return -1;
  }
  return 0;
}


/**
* Takes a buffer from outside of this class. Typically a comm port.
* Always takes ownership of the buffer to avoid needless copy and heap-thrash.
*
* @param  buf    A pointer to the buffer.
* @return -1 to reject buffer, 0 to accept without claiming, 1 to accept with claim.
*/
int8_t ParsingConsole::provideBuffer(StringBuilder* incoming) {
  // TODO: This is a proper choke-point for enforcement of inbound line termination.
  // TODO: Implement cursor keys.
  //   U: 0x1b, 0x5b, 0x41   D: 0x1b, 0x5b, 0x42
  //   L: 0x1b, 0x5b, 0x44   R: 0x1b, 0x5b, 0x43
  uint8_t first_byte = *(incoming->string());
  if (localEcho()) {
    _log.concat(incoming);  // We do it this way to copy the buffer. printToLog() will take it.
    //incoming->printDebug(&_log); Uncomment to hex dump received characters.
    if (0x08 == first_byte) {
      uint8_t last_chr_erase[] = {0x20, 0x08};
      _log.concat(last_chr_erase, 2);
    }
    printToLog(nullptr);   // Flush the log out of the console.
  }
  if (0x08 == first_byte) {   // If we see the backspace key...
    int blen = _buffer.length();
    if (0 < blen) {   // ...and we have accumulated input...
      _buffer.cull(0, blen-1);  // Drop the last character of the buffer.
    }
    incoming->cull(1);  // Drop the first character of the input.
  }
  else {
    _buffer.concatHandoff(incoming);  // Take the buffer.
    _process_buffer();  // TODO: Observe return code.
  }
  return 1;
}


// TODO: For minimum confusion, we need a bi-directional analog of BufferAccepter.
int32_t ParsingConsole::bufferAvailable() {
  int32_t  ret = -1;
  if (nullptr != _output_target) {
    ret = _output_target->bufferAvailable();
  }
  return ret;
}


/**
*
* @return 1 on command execution,
*   0  on command not found,
*   -1 on no action
*   -2 on input overflow
*/
int8_t ParsingConsole::_process_buffer() {
  int8_t ret = -1;
  if (_buffer.length() > _MAX_LEN) {
    _buffer.clear();
    ret = -2;
  }
  else if (_line_ending_rxd()) {
    while (0 < _buffer.count()) {
      StringBuilder tmp_str_bldr(_buffer.position_trimmed(0));
      _buffer.drop_position(0);
      if (!tmp_str_bldr.isEmpty()) {
        ret = (0 != ret) ? ret : 0;
        _history_idx = _max_history;
        if (0 == _exec_line(&tmp_str_bldr)) {
          ret++;   // We successfuly proc'd a command.
          _append_to_history(&tmp_str_bldr);
        }
        else {   // NOTE: Do not make this an else-if. Dangerous...
          if (historyFail()) {
            _append_to_history(&tmp_str_bldr);
          }
        }

        if (forceReturn() && true) {   // TODO: How to deal with this?
          // TODO: force a return string.
        }
        if (emitPrompt() && (nullptr != _prompt_string)) {
          if (hasColor()) {
            // TODO: Change to prompt color.
          }
          // Write the prompt to the log.
          _log.concatf("\n%s", _prompt_string);
        }
      }
    }
  }
  _relay_to_output_target();
  return ret;
}


int8_t ParsingConsole::_relay_to_output_target() {
  int8_t ret = -1;
  if ((!_log.isEmpty()) && (nullptr != _output_target)) {
    if (LineTerm::LF != _tx_terminator) {
      _log.replace("\n", _get_terminator(_tx_terminator));
    }
    _log.string();
    switch (_output_target->provideBuffer(&_log)) {
      case 0:   _log.clear();  // Be sure to discard the log if the downstream BufferAcceptor didn't entirely claim it.
      case 1:   ret = 0;
      default:  break;
    }
  }
  return ret;
}


/*
* Allow the application to print to the console unsolicited.
*/
void ParsingConsole::printToLog(StringBuilder* l) {
  if (nullptr != l) {
    _log.concatHandoff(l);
  }
  _relay_to_output_target();
}


void ParsingConsole::printPrompt() {
  if (nullptr != _prompt_string) {
    _log.concat(_prompt_string);
    _relay_to_output_target();
  }
};


/*
* Allow the application to retreive the log. Ideally, this would be done after
*   each discrete command to minimize peak memory usage to hold the log.
*/
void ParsingConsole::fetchLog(StringBuilder* l) {
  if (nullptr != l) {
    l->concatHandoff(&_log);
  }
}


int8_t ParsingConsole::defineCommand(const char* c, const char* h, const char* p, const uint8_t r, const consoleCallback ccb) {
  ConsoleCommand* cmd = new ConsoleCommand(c, '\0', h, p, r, ccb, true);
  if (nullptr != cmd) {
    _max_cmd_len = strict_max(_max_cmd_len, (uint8_t) strlen(c));
    _cmd_list.insert(cmd);
    return 0;
  }
  return -1;
}


int8_t ParsingConsole::defineCommand(const char* c, const char sc, const char* h, const char* p, const uint8_t r, const consoleCallback ccb) {
  ConsoleCommand* cmd = new ConsoleCommand(c, sc, h, p, r, ccb, true);
  if (nullptr != cmd) {
    _max_cmd_len = strict_max(_max_cmd_len, (uint8_t) strlen(c));
    _cmd_list.insert(cmd);
    return 0;
  }
  return -1;
}


int8_t ParsingConsole::defineCommand(const ConsoleCommand* cmd) {
  if (nullptr != cmd) {
    _max_cmd_len = strict_max(_max_cmd_len, (uint8_t) strlen(cmd->cmd));
    _cmd_list.insert((ConsoleCommand*) cmd);
    return 0;
  }
  return -1;
}

/*
* This allows all commands to be defined in a single call from a (possibly) flash-resident array
*   of ConsoleCommand objects.
* NOTE: This is only providing API features at the moment. It is not saving any RAM.
*/
int8_t ParsingConsole::defineCommands(const ConsoleCommand* cmds, const int cmd_count) {
  if (nullptr != cmds) {
    for (int i = 0; i < cmd_count; i++) {
      _cmd_list.insert((ConsoleCommand*) cmds + i);
      _max_cmd_len = strict_max(_max_cmd_len, (uint8_t) strlen(((ConsoleCommand*) cmds + i)->cmd));
    }
    return 0;
  }
  return -1;
}


/*
* Given a buffer, parse and execute the command it indicates.
* This is the point where the command callback is invoked.
* Returns...
*   -1 on error
*    0 on success
*/
int8_t ParsingConsole::_exec_line(StringBuilder* line) {
  int8_t ret = -1;
  StringBuilder tmp_line((char*) line->string());
  tmp_line.split(" ");
  char* cmd_str = tmp_line.position_trimmed(0);
  ConsoleCommand* cmd = _cmd_def_lookup(cmd_str);
  if (nullptr != cmd) {
    tmp_line.drop_position(0);  // Drop the command, leaving the arguments.
    ret--;
    if ((tmp_line.count() >= cmd->req_count)) {
      // If we have enough arguments to be plausibly valid....
      if (0 != cmd->ccb(&_log, &tmp_line)) {
        if (printHelpOnFail()) {
          cmd->printDetailedHelp(&_log);
          printToLog(nullptr);   // Flush the log out of the console.
        }
      }
      ret = 0;
    }
    else if (nullptr != errCB) {
      // Call the error callback with a report of the user's sins.
      errCB(&_log, ConsoleErr::MISSING_ARG, cmd, &tmp_line);
    }
    else {
      // Report to the log.
      _log.concatf("Command '%s' requires %d arguments. Only %d provided.\n", cmd->cmd, cmd->req_count, tmp_line.count());
      cmd->printDetailedHelp(&_log);
    }
  }
  else if (nullptr != errCB) {
    // If we have an error callback pointer, ping it with CMD_NOT_FOUND.
    errCB(&_log, ConsoleErr::CMD_NOT_FOUND, nullptr, &tmp_line);
  }
  else {
    _log.concatf("Command '%s' not supported.\n", cmd_str);
  }
  return ret;
}


/*
* Lookup a command definition by its command string (case insensitive).
* If the command is not found that way, try again by a case-sensitive shortcut.
* Returns NULL if nothing was found.
*/
ConsoleCommand* ParsingConsole::_cmd_def_lookup(char* str) {
  ConsoleCommand* ret = nullptr;
  if (nullptr != str) {
    int i = 0;
    int cmd_def_count = _cmd_list.size();
    while ((nullptr == ret) && (i < cmd_def_count)) {
      ConsoleCommand* tmp = _cmd_list.get(i);
      if (0 == StringBuilder::strcasecmp(tmp->cmd, str)) {
        ret = tmp;
      }
      i++;
    }
    if ((nullptr == ret) && (1 == strlen(str))) {
      // If we failed on the whole string, and the input was only one character
      //   long, look for shortcuts...
      i = 0;
      while ((nullptr == ret) && (i < cmd_def_count)) {
        ConsoleCommand* tmp = _cmd_list.get(i);
        if (tmp->shortcut == *(str)) {
          ret = tmp;
        }
        i++;
      }
    }
  }
  return ret;
}


/*
* Appends the given line to the history.
*/
void ParsingConsole::_append_to_history(StringBuilder* line) {
  StringBuilder* heap_ref = new StringBuilder();
  heap_ref->concatHandoff(line);
  _history.insert(heap_ref);
  _cull_history();
}


/*
* Sets the history depth and cleans out anything that might be
*   over the new limit.
*/
void ParsingConsole::maxHistoryDepth(uint8_t new_max) {
  _max_history = new_max;
  _cull_history();
}


/*
* If we have more history than the maximum allows, drop the oldest history until
*   we are back within bounds...
*/
void ParsingConsole::_cull_history() {
  while (_max_history < _history.size()) {
    delete _history.remove(0);
  }
}

/*
* Clears the history.
*/
void ParsingConsole::clearHistory() {
  while (0 < _history.size()) {
    delete _history.remove(0);
  }
}


/*
* Print all defined command definitions.
*/
void ParsingConsole::printHelp(StringBuilder* output) {
  int count = _cmd_list.size();
  StringBuilder fmt("%-");
  fmt.concatf("%ds %%s   %%s\n", _max_cmd_len+2);
  StringBuilder::styleHeader2(output, "Help");
  ConsoleCommand* tmpcmd = nullptr;
  for (int i = 0; i < count; i++) {
    tmpcmd = _cmd_list.get(i);
    char sc_str[4] = {0, };
    memcpy(sc_str, "   ", 3);
    if ('\0' != tmpcmd->shortcut) {
      sc_str[0] = '(';
      sc_str[1] = tmpcmd->shortcut;
      sc_str[2] = ')';
    }
    output->concatf((const char*) fmt.string(), tmpcmd->cmd, sc_str, tmpcmd->help_text);
  }
}

/*
* Print the detailed help text for only a specific command.
*/
void ParsingConsole::printHelp(StringBuilder* output, char* specific_cmd) {
  ConsoleCommand* tmpcmd = _cmd_def_lookup(specific_cmd);
  if (nullptr != tmpcmd) {
    tmpcmd->printDetailedHelp(output);
  }
  else {
    output->concatf("Command '%s' not supported.\n", specific_cmd);
  }
}


/*
* Print the running history of commands.
*/
void ParsingConsole::printHistory(StringBuilder* output) {
  StringBuilder::styleHeader2(output, "History");
  int count = _history.size();
  for (int i = 0; i < count; i++) {
    output->concatf("\t%d:  %s\n", i, (char*) _history.get(i)->string());
  }
}


/**
* Checks for the configured terminator in the input stream, and tokenizes the
*   line if found.
*
* @return true if there is a line-ending in the buffer.
*/
bool ParsingConsole::_line_ending_rxd() {
  const char* const L_TERM = _get_terminator(_rx_terminator);
  const int L_TERM_LEN = strlen(L_TERM);
  bool ret = false;
  bool buf_has_lterm = false;
  switch (L_TERM_LEN) {
    case 2:
      buf_has_lterm = _buffer.contains(L_TERM);
      break;
    case 1:
      buf_has_lterm = _buffer.contains(L_TERM[0]);
      break;
    case 0:
    default:
      break;
  }

  if (buf_has_lterm) {
    if (0 < _buffer.split(L_TERM)) {
      ret = true;
    }
    else {
      _buffer.clear();
    }
  }
  return ret;
}


/*******************************************************************************
* ConsoleCommand functions
*******************************************************************************/

void ConsoleCommand::printDetailedHelp(StringBuilder* output) {
  StringBuilder tmp("Help: ");
  tmp.concat(cmd);
  StringBuilder::styleHeader2(output, (const char*) tmp.string());
  output->concatf("%s\nUsage: ", help_text);
  // for (int i = 0; i < maxArgumentCount(); i++) {
  //   output->concatf(
  //     ((i < req_count) ? "%s " : "[%s] "),
  //     typecodeToStr(*(fmt + i))
  //   );
  // }
  output->concatf("\n%s\n", param_text);
}


/*******************************************************************************
* Console callback
* These are built-in handlers for using this instance via a console.
*******************************************************************************/

int8_t ParsingConsole::console_handler_help(StringBuilder* text_return, StringBuilder* args) {
  if (0 < args->count()) {
    printHelp(text_return, args->position_trimmed(0));
  }
  else printHelp(text_return);
  return 0;
}


/**
* @page console-handlers
* @section parsing-console-tools ParsingConsole tools
*
* This is an optional console handler for the configuration of the console itself.
*
* @subsection cmd-actions Actions
*
* Action         | Description | Additional arguments
* ------------   | ------------| --------------------
* `echo`         | Set local echo on or off | [value]
* `history`      | Control command history retention and logging | [clear, depth, logerrors]
* `help-on-fail` | Console prints command help on failure. | [value]
* `prompt`       | Console autoprompt enaabled or disable.| [value]
* `force`        | Console force-return enable or disable. | [value]
* `rxterm`       | Set console RX terminator | [ZEROBYTE, CR, LF, CRLF]
* `txterm`       | Set console TX treminator  | [ZEROBYTE, CR, LF, CRLF]
*
* @subsection arguments Arguments
* Argument    | Purpose   | Required
* ----------- | --------- | --------
* value       | Enable or Disable the function (0 = off, 1 = on)| yes
* terminator  | Set the value of the line termination character | no, Will be enabled if not provided
*/
int8_t ParsingConsole::console_handler_conf(StringBuilder* text_return, StringBuilder* args) {
  // TODO: Unimplemented breakouts:
  //inline void setPromptString(const char* str);
  //inline bool hasColor();
  //inline void hasColor(bool x);
  int ret = 0;
  char* cmd    = args->position_trimmed(0);
  int   arg1   = args->position_as_int(1);
  bool  print_term_enum = false;
  if (0 == StringBuilder::strcasecmp(cmd, "echo")) {
    if (1 < args->count()) {
      localEcho(0 != arg1);
    }
    text_return->concatf("Console RX echo %sabled.\n", localEcho()?"en":"dis");
  }
  else if (0 == StringBuilder::strcasecmp(cmd, "history")) {
    if (1 < args->count()) {
      emitPrompt(0 != arg1);
      char* subcmd = args->position_trimmed(1);
      if (0 == StringBuilder::strcasecmp(subcmd, "clear")) {
        clearHistory();
        text_return->concat("History cleared.\n");
      }
      else if (0 == StringBuilder::strcasecmp(subcmd, "depth")) {
        if (2 < args->count()) {
          arg1 = args->position_as_int(2);
          maxHistoryDepth(arg1);
        }
        text_return->concatf("History depth: %u\n", maxHistoryDepth());
      }
      else if (0 == StringBuilder::strcasecmp(subcmd, "logerrors")) {
        if (2 < args->count()) {
          arg1 = args->position_as_int(2);
          historyFail(0 != arg1);
        }
        text_return->concatf("History %scludes failed commands.\n", historyFail()?"in":"ex");
      }
      else text_return->concat("Valid options are [clear|depth|logerrors]\n");
    }
    else printHistory(text_return);
  }
  else if (0 == StringBuilder::strcasecmp(cmd, "help-on-fail")) {
    if (1 < args->count()) {
      printHelpOnFail(0 != arg1);
    }
    text_return->concatf("Console prints command help on failure: %s.\n", printHelpOnFail()?"yes":"no");
  }
  else if (0 == StringBuilder::strcasecmp(cmd, "prompt")) {
    if (1 < args->count()) {
      emitPrompt(0 != arg1);
    }
    text_return->concatf("Console autoprompt %sabled.\n", emitPrompt()?"en":"dis");
  }
  else if (0 == StringBuilder::strcasecmp(cmd, "force")) {
    if (1 < args->count()) {
      forceReturn(0 != arg1);
    }
    text_return->concatf("Console force-return %sabled.\n", forceReturn()?"en":"dis");
  }
  else if (0 == StringBuilder::strcasecmp(cmd, "rxterm")) {
    if (1 < args->count()) {
      switch (arg1) {
        case 0:  case 1:  case 2:  case 3:
          setRXTerminator((LineTerm) arg1);
          break;
        default:
          print_term_enum = true;
          break;
      }
    }
    text_return->concatf("Console RX terminator: %s\n", ParsingConsole::terminatorStr(getRXTerminator()));
  }
  else if (0 == StringBuilder::strcasecmp(cmd, "txterm")) {
    if (1 < args->count()) {
      switch (arg1) {
        case 0:  case 1:  case 2:  case 3:
          setTXTerminator((LineTerm) arg1);
          break;
        default:
          print_term_enum = true;
          break;
      }
    }
    text_return->concatf("Console TX terminator: %s\n", ParsingConsole::terminatorStr(getTXTerminator()));
  }
  else {
    ret = -1;
  }

  if (print_term_enum) {
    text_return->concat("Terminator options:\n");
    text_return->concat("\t0: ZEROBYTE\n");
    text_return->concat("\t1: CR\n");
    text_return->concat("\t2: LF\n");
    text_return->concat("\t3: CRLF\n");
  }
  return ret;
}
