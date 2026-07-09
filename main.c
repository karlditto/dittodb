#include "sqldb.h"

int main(int argc, char *argv[]) {

  Token *token = tokenize(argv[1]);

  print_token(token);

  Node *ast = parse(token);
  print_ast(ast, 1);
}
