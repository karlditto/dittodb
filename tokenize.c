#include "sqldb.h"
#include <stdio.h>

static Token *new_token(TokenType type, char *start, char *end) {
  Token *token = calloc(1, sizeof(Token));
  token->type = type;
  token->loc = start;
  token->len = end - start + 1;
  return token;
}

static bool is_keyword(const char *token) {
  const char *KYWD[] = {"CREATE", "SELECT", "INSERT", "DELETE", "FROM",
                        "AND",    "OR",     "IS",     "NOT",    "NULL",
                        "WHERE",  "LIKE",   "TABLE"};
  for (size_t i = 0; i < sizeof(KYWD) / sizeof(char *); i++) {
    if (strcasecmp(token, KYWD[i]) == 0) {
      return true;
    }
  }
  return false;
}

static bool is_operator(const char *token) {
  const char *OP[] = {"+", "-",  "*",  "/",  "=",    "<>", "<",
                      ">", "<=", ">=", "IS", "LIKE", "NOT"};
  for (size_t i = 0; i < sizeof(OP) / sizeof(char *); i++) {
    if (strcasecmp(token, OP[i]) == 0) {
      return true;
    }
  }
  return false;
}

static bool is_datatype(const char *token) {
  const char *DTYPE[] = {"CHAR", "BOOLEAN", "INTEGER", "FLOAT"};
  for (size_t i = 0; i < sizeof(DTYPE) / sizeof(char *); i++) {
    if (strcasecmp(token, DTYPE[i]) == 0) {
      return true;
    }
  }
  return false;
}

char *get_token_name(const Token *token) {
  switch (token->type) {
  case TOKEN_KYWD:
    return "keyword";
    break;
  case TOKEN_IDTF:
    return "identifier";
    break;
  case TOKEN_COMMA:
    return ",";
    break;
  case TOKEN_STR:
    return "string literal";
    break;
  case TOKEN_NUM:
    return "numeric literal";
    break;
  case TOKEN_EOF:
    return "EOF";
    break;
  case TOKEN_OP:
    return "operator";
    break;
  case TOKEN_STAR:
    return "*";
    break;
  case TOKEN_LPAREN:
    return "(";
    break;
  case TOKEN_RPAREN:
    return ")";
    break;
  case TOKEN_DTYPE:
    return "datatype";
    break;
  default:
    fprintf(stderr, "Unkown token in line:%d, file:%s\n", __LINE__, __FILE__);
    exit(1);
  }
  return 0;
}

Token *tokenize(char *s) {
  char *start_loc = s;
  Token root = {0};
  Token *cur = &root;

  while (*s) {

    // skip newline
    if (strncmp(s, "\n", strlen("\n")) == 0) {
      // printf("+ skip newline at: %zu\n", s - start_loc);
      s++;
      continue;
    }

    // skip whitespace
    if (isspace(*s)) {
      // printf("+ skip whilespace at: %zu\n", s - start_loc);
      s++;
      continue;
    }

    // string literal
    if (*s == '"') {
      // find closing quote
      char *ptr = s + 1;
      for (; *ptr != '"'; ptr++) {
        if (*ptr == '\n' || *ptr == '\0') {
          fprintf(stderr, "> error at %zu, quote not closed\n", s - start_loc);
          exit(1);
        }
        if (*ptr == '\\') {
          ptr++;
        }
      }
      // placeholder for literal
      char *buf = calloc(1, ptr - s);
      sprintf(buf, "%.*s", (int)(ptr - s - 1), s + 1);
      cur->next = new_token(TOKEN_STR, s + 1, ptr - 1);
      cur->next->str = buf;
      cur = cur->next;
      s += ptr - s + 1; // forward pointer
      continue;
    }

    // numeric literal
    if (isdigit(*s)) {
      char *ptr = s;
      for (;;) {
        ptr++;
        if (isdigit(*ptr)) {
          continue;
        } else if (*ptr == '.' && isdigit(ptr[1])) {
          ptr++;
        } else {
          break;
        }
      }
      // placeholder for literal
      char *buf = calloc(1, ptr - s + 1);
      sprintf(buf, "%.*s", (int)(ptr - s), s);
      if (strchr(buf, '.') != strrchr(buf, '.')) {
        fprintf(stderr, "> error at %zu, look like a numeric literal but not\n",
                s - start_loc);
        exit(1);
      }
      cur->next = new_token(TOKEN_NUM, s, ptr - 1);
      if (strchr(buf, '.')) {
        cur->next->fval = strtold(buf, NULL);
      } else {
        cur->next->ival = strtoll(buf, NULL, 10);
      }
      cur->next->str = buf;
      cur = cur->next;
      s = ptr;
      continue;
    }

    // keyword or identifier
    // word operators and datatype
    if (isalpha(*s)) {
      char *ptr = s;
      for (; isalnum(*ptr); ptr++) {
        ;
      }
      // placeholder pointer
      char *buf = calloc(1, ptr - s + 1);
      sprintf(buf, "%.*s", (int)(ptr - s), s);

      cur->next = is_keyword(buf) ? new_token(TOKEN_KYWD, s, ptr - 1)
                                  : new_token(TOKEN_IDTF, s, ptr - 1);
      cur->next->str = buf;
      cur->next->type =
          is_operator(cur->next->str) ? TOKEN_OP : cur->next->type;
      cur->next->type =
          is_datatype(cur->next->str) ? TOKEN_DTYPE : cur->next->type;
      cur = cur->next;
      s = ptr;
      continue;
    }

    // comma
    if (*s == ',') {
      cur->next = new_token(TOKEN_COMMA, s, s + 1);
      cur->next->str = ",";
      cur = cur->next;
      s++;
      continue;
    }

    // star
    if (*s == '*') {
      cur->next = new_token(TOKEN_STAR, s, s + 1);
      cur->next->str = "*";
      cur = cur->next;
      s++;
      continue;
    }

    // (
    if (*s == '(') {
      cur->next = new_token(TOKEN_LPAREN, s, s + 1);
      cur->next->str = "(";
      cur = cur->next;
      s++;
      continue;
    }

    // )
    if (*s == ')') {
      cur->next = new_token(TOKEN_RPAREN, s, s + 1);
      cur->next->str = ")";
      cur = cur->next;
      s++;
      continue;
    }

    // operator
    if (ispunct(*s)) {
      char *ptr = s;
      for (; ispunct(*ptr); ptr++) {
        // ignore token
        if (*ptr == '"') {
          break;
        }
        if (*ptr == '*') {
          break;
        }
        if (*ptr == ',') {
          break;
        }
        if (*ptr == '(') {
          break;
        }
        if (*ptr == ')') {
          break;
        }
      }
      // placeholder pointer
      char *buf = calloc(1, ptr - s + 1);
      sprintf(buf, "%.*s", (int)(ptr - s), s);
      cur->next = new_token(TOKEN_OP, s, ptr - 1);
      cur->next->str = buf;
      cur = cur->next;
      s = ptr;
      continue;
    }

    // token invalid
    fprintf(stderr, "> error at %zu, unrecognized token: '%.*s'\n",
            s - start_loc, 1, s);
    exit(1);
  }
  cur = cur->next = new_token(TOKEN_EOF, s, s);
  return root.next;
}
