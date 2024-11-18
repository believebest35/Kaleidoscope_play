#pragma once

#include "ast.h"
#include "lexer.h"
#include <cstdio>
#include <map>
#include <memory>
#include <string>
#include <vector>

/// cur_token/getNextToken - Provide a simple token buffer.  cur_token is the
/// current token the parser is looking at.  getNextToken reads another token
/// from the lexer and updates cur_token with its results.
static int cur_token;
static int GetNextToken() { return cur_token = GetToken(); }

/// LogError* - These are little helper functions for error handling.
inline std::unique_ptr<ExprAST> LogError(const char *str) {
  fprintf(stderr, "Error: %s\n", str);
  return nullptr;
}

inline std::unique_ptr<PrototypeAST> LogErrorP(const char *str) {
  LogError(str);
  return nullptr;
}

/// numberexpr ::= number
static std::unique_ptr<ExprAST> ParseNumberExpr() {
  auto result = std::make_unique<NumberExprAST>(NUM_VAL);
  GetNextToken();
  return std::move(result);
}

static std::unique_ptr<ExprAST> ParseExpression();

/// parenexpr ::= '(' expression ')'
static std::unique_ptr<ExprAST> ParseParenExpr() {
  GetNextToken(); // eat (.
  auto V = ParseExpression();
  if (!V) {
    return nullptr;
  }
  if (cur_token != ')') {
    return LogError("expected ')'");
  }
  GetNextToken(); // eat ).
  return V;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
  std::string id_name = IDENTIFIER_STR;

  GetNextToken(); // eat identifier.

  if (cur_token != '(') { // Simple variable ref.
    return std::make_unique<VariableExprAST>(id_name);
  }

  // Call.
  GetNextToken(); // eat (
  std::vector<std::unique_ptr<ExprAST>> args;
  if (cur_token != ')') {
    while (true) {
      if (auto arg = ParseExpression()) {
        args.push_back(std::move(arg));
      } else {
        return nullptr;
      }

      if (cur_token == ')') {
        break;
      }

      if (cur_token != ',') {
        return LogError("Expected ')' or ',' in argument list");
      }
      GetNextToken();
    }
  }

  // eat the ')'
  GetNextToken();
  return std::make_unique<CallExprAST>(id_name, std::move(args));
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
static std::unique_ptr<ExprAST> ParsePrimary() {
  switch (cur_token) {
  case Token::token_identifier:
    return ParseIdentifierExpr();
  case Token::token_number:
    return ParseNumberExpr();
  case '(':
    return ParseParenExpr();
  default:
    return LogError("unknown token when expecting an expression");
  }
}

/// BinopPrecedence - This holds the precedence for each binary operator that is
/// defined.
static std::map<char, int> BinopPrecedence;

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
static int GetTokenPrecedence() {
  if (!isascii(cur_token)) {
    return -1;
  }
  // Make sure it's a declared binop.
  int token_prec = BinopPrecedence[cur_token];
  if (token_prec <= 0) {
    return -1;
  }
  return token_prec;
}

static std::unique_ptr<ExprAST> ParseBinOpRHS(int expr_prec,
                                              std::unique_ptr<ExprAST> LHS);

/// expression
///   ::= primary binoprhs
///
static std::unique_ptr<ExprAST> ParseExpression() {
  auto LHS = ParsePrimary();
  if (!LHS) {
    return nullptr;
  }
  return ParseBinOpRHS(0, std::move(LHS));
}

/// binoprhs
///   ::= ('+' primary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int expr_prec,
                                              std::unique_ptr<ExprAST> LHS) {
  // If this is a binop, find its precedence.
  while (true) {
    int tok_prec = GetTokenPrecedence();

    // If this is a binop that binds at least as tightly as the current binop,
    // consume it, otherwise we are done.
    if (tok_prec < expr_prec) {
      return LHS;
    }

    // Okay, we know this is a binop.
    int binop = cur_token;
    GetNextToken(); // eat binop

    // Parse the primary expression after the binary operator.
    auto RHS = ParsePrimary();
    if (!RHS) {
      return nullptr;
    }

    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    int next_prec = GetTokenPrecedence();
    if (tok_prec < next_prec) {
      RHS = ParseBinOpRHS(tok_prec + 1, std::move(RHS));
      if (!RHS) {
        return nullptr;
      }
    }
    // Merge LHS/RHS.
    LHS =
        std::make_unique<BinaryExprAST>(binop, std::move(LHS), std::move(RHS));
  }
}

/// prototype
///   ::= id '(' id* ')'
static std::unique_ptr<PrototypeAST> ParsePrototype() {
  if (cur_token != Token::token_identifier) {
    return LogErrorP("Expected function name in prototype");
  }

  std::string fn_name = IDENTIFIER_STR;
  GetNextToken();

  if (cur_token != '(') {
    return LogErrorP("Expected '(' in prototype");
  }

  // Read the list of argument names.
  std::vector<std::string> arg_names;
  while (GetNextToken() == Token::token_identifier) {
    arg_names.push_back(IDENTIFIER_STR);
  }
  if (cur_token != ')') {
    return LogErrorP("Expected ')' in prototype");
  }

  // success.
  GetNextToken(); // eat ')'.
  return std::make_unique<PrototypeAST>(fn_name, std::move(arg_names));
}

/// definition ::= 'def' prototype expression
static std::unique_ptr<FunctionAST> ParseDefinition() {
  GetNextToken(); // eat def.
  auto Proto = ParsePrototype();
  if (!Proto) {
    return nullptr;
  }
  if (auto E = ParseExpression()) {
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
  }
  return nullptr;
}

/// external ::= 'extern' prototype
static std::unique_ptr<PrototypeAST> ParseExtern() {
  GetNextToken();  // eat extern.
  return ParsePrototype();
}

/// toplevelexpr ::= expression
static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
  if (auto E = ParseExpression()) {
    // Make an anonymous proto.
    auto proto = std::make_unique<PrototypeAST>("", std::vector<std::string>());
    return std::make_unique<FunctionAST>(std::move(proto), std::move(E));
  }
  return nullptr;
}

// Top-Level parsing
static void HandleDefinition() {
  if (ParseDefinition()) {
    fprintf(stderr, "Parsed a function definition.\n");
  } else {
    // Skip token for error recovery.
    GetNextToken();
  }
}

static void HandleExtern() {
  if (ParseExtern()) {
    fprintf(stderr, "Parsed an extern\n");
  } else {
    // Skip token for error recovery.
    GetNextToken();
  }
}

static void HandleTopLevelExpr() {
  // Evaluate a top-level expression into an anonymous function.
  if (ParseTopLevelExpr()) {
    fprintf(stderr, "Parsed a top-level expression\n");
  } else {
    // Skip token for error recovery.
    GetNextToken();
  }
}

/// top ::= definition | external | expression | ';'
static void MainLoop() {
  while (true) {
    fprintf(stderr, "ready> ");
    switch (cur_token) {
    case Token::token_eof:
      return;
    case ';':
      GetNextToken();
      break;
    case Token::token_def:
      HandleDefinition();
      break;
    case Token::token_extern:
      HandleExtern();
      break;
    default:
      HandleTopLevelExpr();
      break;
    }
  }
}


