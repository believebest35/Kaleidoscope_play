#include "parser.h"

int main() {
  // Install standard binary operators.
  // 1 is lowest precedence.
  BinopPrecedence['<'] = 10;
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 30;
  BinopPrecedence['*'] = 40;

  fprintf(stderr, "ready> ");
  GetNextToken();

  MainLoop();
  return 0;
}