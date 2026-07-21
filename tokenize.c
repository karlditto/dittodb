#include "sqldb.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ucontext.h>
#include <time.h>
#include <unistd.h>

extern Region mem_ast;

void token_free(Token *token) {
  Token *tmp;
  while (token != NULL) {
    tmp = token;
    token = token->next;
    free(tmp->str);
    free(tmp);
  }
}

Token *new_token(TokenType type, char *loc, size_t len) {
  Token *token = region_alloc(&mem_ast, sizeof(Token));
  token->type = type;
  token->loc = loc;
  token->len = len;
  return token;
}

char *get_token_type(const Token *token) {
  char *typestr[TOKENCOUNT] = {0};
  typestr[KEYWORD] = "keyword";
  typestr[IDENTIFIER] = "identifier";
  typestr[STRING] = "string literal";
  typestr[NUMERIC] = "numeric literal";
  typestr[EOQ] = "EOQ";
  typestr[OPERATOR] = "operator";
  typestr[DATATYPE] = "datatype";
  return typestr[token->type];
}

void print_token(Token *token) {
  Token *cur = token;
  for (; cur->type != EOQ; cur = cur->next) {
    printf("%-10s==>  %s\n", cur->str, get_token_type(cur));
  }
}

// TODO: ditch regex
static size_t match_dtype(const char *s) {
  int NUM_MATCHES = 1;
  regex_t re;
  regmatch_t match[NUM_MATCHES];
  const char *pattern = "^(char|boolean|integer|float)\\b";

  if (regcomp(&re, pattern, REG_ICASE | REG_EXTENDED)) {
    perror("regex compilation failed");
    exit(EXIT_FAILURE);
  }

  int result = regexec(&re, s, NUM_MATCHES, match, 0);

  if (!result) {
    regfree(&re);
    return match->rm_eo - match->rm_so;
  } else if (result == REG_NOMATCH) {
    regfree(&re);
    return 0;
  } else {
    regfree(&re);
    perror("regex went wrong");
    exit(EXIT_FAILURE);
  }
  return 0;
}

static size_t match_keyword(const char *s) {
  int NUM_MATCHES = 1;
  regex_t re;
  regmatch_t match[NUM_MATCHES];
  const char *pattern =
      "^(create|select|insert|into|delete|update|from|and|or|is|"
      "not|null|where|like|table|index|order|by|values)\\b";

  if (regcomp(&re, pattern, REG_ICASE | REG_EXTENDED)) {
    perror("regex compilation failed");
    exit(EXIT_FAILURE);
  }

  int result = regexec(&re, s, NUM_MATCHES, match, 0);

  if (!result) {
    regfree(&re);
    return match->rm_eo - match->rm_so;
  } else if (result == REG_NOMATCH) {
    regfree(&re);
    return 0;
  } else {
    regfree(&re);
    perror("regex went wrong");
    exit(EXIT_FAILURE);
  }
  return 0;
}

static size_t match_string(const char *s) {
  int NUM_MATCHES = 1;
  regex_t re;
  regmatch_t match[NUM_MATCHES];
  const char *pattern = "^\"[^\"]*\"";

  if (regcomp(&re, pattern, REG_ICASE | REG_EXTENDED)) {
    perror("regex compilation failed");
    exit(EXIT_FAILURE);
  }

  int result = regexec(&re, s, NUM_MATCHES, match, 0);

  if (!result) {
    regfree(&re);
    return match->rm_eo - match->rm_so;
  } else if (result == REG_NOMATCH) {
    regfree(&re);
    return 0;
  } else {
    regfree(&re);
    perror("regex went wrong");
    exit(EXIT_FAILURE);
  }
  return 0;
}

static size_t match_identifier(const char *s) {
  int NUM_MATCHES = 1;
  regex_t re;
  regmatch_t match[NUM_MATCHES];
  const char *pattern = "^[A-Z]+[A-Z0-9]*";

  if (regcomp(&re, pattern, REG_ICASE | REG_EXTENDED)) {
    perror("regex compilation failed");
    exit(EXIT_FAILURE);
  }

  int result = regexec(&re, s, NUM_MATCHES, match, 0);

  if (!result) {
    regfree(&re);
    return match->rm_eo - match->rm_so;
  } else if (result == REG_NOMATCH) {
    regfree(&re);
    return 0;
  } else {
    regfree(&re);
    perror("regex went wrong");
    exit(EXIT_FAILURE);
  }
  return 0;
}

static size_t match_integer(const char *s) {
  int NUM_MATCHES = 1;
  regex_t re;
  regmatch_t match[NUM_MATCHES];
  const char *pattern = "^[0-9]+";

  if (regcomp(&re, pattern, REG_ICASE | REG_EXTENDED)) {
    perror("regex compilation failed");
    exit(EXIT_FAILURE);
  }

  int result = regexec(&re, s, NUM_MATCHES, match, 0);

  if (!result) {
    regfree(&re);
    return match->rm_eo - match->rm_so;
  } else if (result == REG_NOMATCH) {
    regfree(&re);
    return 0;
  } else {
    regfree(&re);
    perror("regex went wrong");
    exit(EXIT_FAILURE);
  }
  return 0;
}

static size_t match_float(const char *s) {
  int NUM_MATCHES = 1;
  regex_t re;
  regmatch_t match[NUM_MATCHES];
  const char *pattern = "^[0-9]+[.][0-9]+";

  if (regcomp(&re, pattern, REG_ICASE | REG_EXTENDED)) {
    perror("regex compilation failed");
    exit(EXIT_FAILURE);
  }

  int result = regexec(&re, s, NUM_MATCHES, match, 0);

  if (!result) {
    regfree(&re);
    return match->rm_eo - match->rm_so;
  } else if (result == REG_NOMATCH) {
    regfree(&re);
    return 0;
  } else {
    regfree(&re);
    perror("regex went wrong");
    exit(EXIT_FAILURE);
  }
  return 0;
}

static size_t match_operator(const char *s) {
  int NUM_MATCHES = 1;
  regex_t re;
  regmatch_t match[NUM_MATCHES];
  // const char *pattern =
  //     "^(\\+|-|\\*|/|=|<>|<|>|<=|>=|IS\\b|LIKE\\b|NOT\\b|AND\\b|OR\\b)";
  const char *pattern = "^(\\+|-|\\*|/|\\(|\\)|,|=)";

  if (regcomp(&re, pattern, REG_ICASE | REG_EXTENDED)) {
    perror("regex compilation failed");
    exit(EXIT_FAILURE);
  }

  int result = regexec(&re, s, NUM_MATCHES, match, 0);

  if (!result) {
    regfree(&re);
    return match->rm_eo - match->rm_so;
  } else if (result == REG_NOMATCH) {
    regfree(&re);
    return 0;
  } else {
    regfree(&re);
    perror("regex went wrong");
    exit(EXIT_FAILURE);
  }
  return 0;
}

Token *tokenize(char *s) {
  char *start_loc = s;
  Token root = {0};
  Token *cur = &root;
  size_t len;

  while (*s) {

    // skip whitespace
    if (isspace(*s)) {
      s++;
      continue;
    }

    len = match_float(s);
    if (len) {
      cur->next = new_token(NUMERIC, s, len);
      char *buf = region_alloc(&mem_ast, len + 1);
      sprintf(buf, "%.*s", (int)len, s);
      cur->next->fval = strtold(buf, NULL);
      cur->next->str = buf;
      cur = cur->next;
      s += len;
      continue;
    }
    len = match_integer(s);
    if (len) {
      cur->next = new_token(NUMERIC, s, len);
      char *buf = region_alloc(&mem_ast, len + 1);
      sprintf(buf, "%.*s", (int)len, s);
      cur->next->ival = strtoll(buf, NULL, 10);
      cur->next->str = buf;
      cur = cur->next;
      s += len;
      continue;
    }

    len = match_operator(s);
    if (len) {
      cur->next = new_token(OPERATOR, s, len);
      char *buf = region_alloc(&mem_ast, len + 1);
      sprintf(buf, "%.*s", (int)len, s);
      cur->next->str = buf;
      cur = cur->next;
      s += len;
      continue;
    }

    len = match_string(s);
    if (len) {
      cur->next = new_token(STRING, s, len);
      char *buf = region_alloc(&mem_ast, len - 1);
      sprintf(buf, "%.*s", (int)len - 2, s + 1);
      cur->next->str = buf;
      cur = cur->next;
      s += len;
      continue;
    }

    len = match_keyword(s);
    if (len) {
      cur->next = new_token(KEYWORD, s, len);
      char *buf = region_alloc(&mem_ast, len + 1);
      sprintf(buf, "%.*s", (int)len, s);
      cur->next->str = buf;
      cur = cur->next;
      s += len;
      continue;
    }

    len = match_dtype(s);
    if (len) {
      cur->next = new_token(DATATYPE, s, len);
      char *buf = region_alloc(&mem_ast, len + 1);
      sprintf(buf, "%.*s", (int)len, s);
      cur->next->str = buf;
      cur = cur->next;
      s += len;
      continue;
    }

    len = match_identifier(s);
    if (len) {
      cur->next = new_token(IDENTIFIER, s, len);
      char *buf = region_alloc(&mem_ast, len + 1);
      sprintf(buf, "%.*s", (int)len, s);
      cur->next->str = buf;
      cur = cur->next;
      s += len;
      continue;
    }

    fprintf(stderr, "unknown token at: %ld, token:%s\n", s - start_loc, s);
    exit(EXIT_FAILURE);
  }

  cur = cur->next = new_token(EOQ, s, 0);
  return root.next;
}
