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

static const uint8_t DPAD_ESCAPE_SEQUENCE_U[4] = {27, 91, 65, 0};
static const uint8_t DPAD_ESCAPE_SEQUENCE_D[4] = {27, 91, 66, 0};
static const uint8_t DPAD_ESCAPE_SEQUENCE_R[4] = {27, 91, 67, 0};
static const uint8_t DPAD_ESCAPE_SEQUENCE_L[4] = {27, 91, 68, 0};

/* Common TCode strings. */
const TCode ParsingConsole::tcodes_0[]         = {TCode::NONE};
const TCode ParsingConsole::tcodes_uint_1[]    = {TCode::UINT32, TCode::NONE};
const TCode ParsingConsole::tcodes_uint_2[]    = {TCode::UINT32, TCode::UINT32, TCode::NONE};
const TCode ParsingConsole::tcodes_uint_3[]    = {TCode::UINT32, TCode::UINT32, TCode::UINT32, TCode::NONE};
const TCode ParsingConsole::tcodes_uint_4[]    = {TCode::UINT32, TCode::UINT32, TCode::UINT32, TCode::UINT32, TCode::NONE};
const TCode ParsingConsole::tcodes_str_1[]     = {TCode::STR, TCode::NONE};
const TCode ParsingConsole::tcodes_str_2[]     = {TCode::STR, TCode::STR, TCode::NONE};
const TCode ParsingConsole::tcodes_str_3[]     = {TCode::STR, TCode::STR, TCode::STR, TCode::NONE};
const TCode ParsingConsole::tcodes_str_4[]     = {TCode::STR, TCode::STR, TCode::STR, TCode::STR, TCode::NONE};
const TCode ParsingConsole::tcodes_float_1[]   = {TCode::FLOAT, TCode::NONE};


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
* Constructor
*/
ParsingConsole::ParsingConsole(const uint16_t max_len) : _MAX_LEN(max_len) {
}

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
      if (tmp_str_bldr.length() > 0) {
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
          _log.concat(_prompt_string);   // Write the prompt to the log.
        }
      }
    }
  }
  _relay_to_output_target();
  return ret;
}


int8_t ParsingConsole::_relay_to_output_target() {
  int8_t ret = -1;
  if ((_log.length() > 0) && (nullptr != _output_target)) {
    if (0 == _output_target->provideBuffer(&_log)) {
      _log.clear();
      ret = 0;
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


int8_t ParsingConsole::defineCommand(const char* c, const TCode* f, const char* h, const char* p, const uint8_t r, const consoleCallback ccb) {
  ConsoleCommand* cmd = new ConsoleCommand(c, '\0', f, h, p, r, ccb, true);
  if (nullptr != cmd) {
    _max_cmd_len = strict_max(_max_cmd_len, (uint8_t) strlen(c));
    _cmd_list.insert(cmd);
    return 0;
  }
  return -1;
}


int8_t ParsingConsole::defineCommand(const char* c, const char sc, const TCode* f, const char* h, const char* p, const uint8_t r, const consoleCallback ccb) {
  ConsoleCommand* cmd = new ConsoleCommand(c, sc, f, h, p, r, ccb, true);
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
    if ((tmp_line.count() >= cmd->req_count) && (tmp_line.count() <= cmd->maxArgumentCount())) {
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

  output->concat("---< Help >-------------------------------------------------\n");
  ConsoleCommand* tmpcmd = nullptr;
  for (int i = 0; i < count; i++) {
    tmpcmd = _cmd_list.get(i);
    char* sc_str = (char*) alloca(4);
    memcpy(sc_str, "   ", 3);
    sc_str[3] = '\0';
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
  output->concat("---< History >----------------------------------------------\n");
  int count = _history.size();
  for (int i = 0; i < count; i++) {
    output->concatf("%d:  %s\n", i, (char*) _history.get(i)->string());
  }
}


/*
* Returns true if there is a line-ending in the buffer.
* Tokenizes the line.
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
      //Serial.println("Returning true");
      ret = true;
    }
    else {
      _buffer.clear();
      //Serial.print("Had term, but returning faslse  ");
      //Serial.println(_buffer.length(), DEC);
    }
  }
  //else {    Serial.println("no term");    }
  return ret;
}


/*******************************************************************************
* ConsoleCommand functions
*******************************************************************************/

void ConsoleCommand::printDetailedHelp(StringBuilder* output) {
  output->concatf("---< %s >-------------------------------------------------\n", cmd);
  output->concatf("%s\nUsage: ", help_text);
  for (int i = 0; i < maxArgumentCount(); i++) {
    output->concatf(
      ((i < req_count) ? "%s " : "[%s] "),
      typecodeToStr(*(fmt + i))
    );
  }
  output->concatf("\n%s\n", param_text);
}


int ConsoleCommand::maxArgumentCount() {
  int ret = 0;
  while (TCode::NONE != *(fmt + ret)) {
    ret++;
  }
  return ret;
}
