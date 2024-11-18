#pragma once

#include <cctype>
#include <cstdio>
#include <string>
#include <vector>

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
enum Token {
  token_eof = -1,
  // commands
  token_def = -2,
  token_extern = -3,
  // primary
  token_identifier = -4,
  token_number = -5
};

// Simply use a global variable here, it is not a good practice though.
static std::string IDENTIFIER_STR; // Filled in if token is an identifier
static double NUM_VAL;             // Filled in if token is a number

inline bool IsSpace(int ch) { return ch == ' ' || ch == '\t'; }

static int GetToken() {
  static int last_char = ' ';
  while (isspace(last_char)) {
    last_char = getchar();
  }

  if (isalpha(last_char)) {
    IDENTIFIER_STR = last_char;
    while (isalnum(last_char = getchar())) {
      IDENTIFIER_STR += last_char;
    }
    if (IDENTIFIER_STR == "def") {
      return Token::token_def;
    }
    if (IDENTIFIER_STR == "extern") {
      return Token::token_extern;
    }
    return Token::token_identifier;
  }

  if (isdigit(last_char) || last_char == '.') {
    std::string num_str{""};
    num_str += last_char;
    last_char = getchar();
    while (isdigit(last_char) || last_char == '.') {
      num_str += last_char;
      last_char = getchar();
    }
    NUM_VAL = std::stod(num_str);
    return Token::token_number;
  }

  if (last_char == '#') {
    last_char = getchar();
    while (last_char != EOF && last_char != '\n' && last_char != '\r') {
      last_char = getchar();
    }
    if (last_char != EOF) {
      return GetToken();
    }
  }

  if (last_char == EOF) {
    return Token::token_eof;
  }

  int this_char = last_char;

  last_char = getchar();
  return this_char;
}
