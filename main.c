#include "sqldb.h"

int main(int argc, char *argv[]) {

  Token *token = tokenize(argv[1]);

  print_token(token);

  print_ast(parse(token), 1);
}
