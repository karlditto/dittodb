#include "sqldb.h"

static Token *new_token(TokenType type, char *loc, size_t len) {
  Token *token = calloc(1, sizeof(Token));
  token->type = type;
  token->loc = loc;
  token->len = len;
  return token;
}

char *get_token_type(const Token *token) {
  char *typestr[] = {"keyword",         "identifier", "string literal",
                     "numeric literal", "EOF",        "operator",
                     "datatype"};
  return typestr[token->type];
}

void print_token(Token *token) {
  Token *cur = token;
  for (; cur->type != EOQ; cur = cur->next) {
    printf("%-20s------> %20s\n", cur->str, get_token_type(cur));
  }
}

static size_t match_keyword(const char *s) {
  int NUM_MATCHES = 1;
  regex_t re;
  regmatch_t match[NUM_MATCHES];
  const char *pattern = "^(create|select|insert|delete|update|from|and|or|is|"
                        "not|null|where|like|table)";

  if (regcomp(&re, pattern, REG_ICASE | REG_EXTENDED)) {
    perror("regex compilation failed");
    exit(EXIT_FAILURE);
  }

  int result = regexec(&re, s, NUM_MATCHES, match, 0);

  if (!result) {
    return match->rm_eo - match->rm_so;
  } else if (result == REG_NOMATCH) {
    return 0;
  } else {
    perror("regex went wrong");
    exit(EXIT_FAILURE);
  }
  regfree(&re);
  return 0;
}

static size_t match_space(const char *s) {
  int NUM_MATCHES = 1;
  regex_t re;
  regmatch_t match[NUM_MATCHES];
  const char *pattern = "^\\s+";

  if (regcomp(&re, pattern, REG_ICASE | REG_EXTENDED)) {
    perror("regex compilation failed");
    exit(EXIT_FAILURE);
  }

  int result = regexec(&re, s, NUM_MATCHES, match, 0);

  if (!result) {
    return match->rm_eo - match->rm_so;
  } else if (result == REG_NOMATCH) {
    return 0;
  } else {
    perror("regex went wrong");
    exit(EXIT_FAILURE);
  }
  regfree(&re);
  return 0;
}

static size_t match_string(const char *s) {
  int NUM_MATCHES = 1;
  regex_t re;
  regmatch_t match[NUM_MATCHES];
  const char *pattern = "^\".*\"";

  if (regcomp(&re, pattern, REG_ICASE | REG_EXTENDED)) {
    perror("regex compilation failed");
    exit(EXIT_FAILURE);
  }

  int result = regexec(&re, s, NUM_MATCHES, match, 0);

  if (!result) {
    return match->rm_eo - match->rm_so;
  } else if (result == REG_NOMATCH) {
    return 0;
  } else {
    perror("regex went wrong");
    exit(EXIT_FAILURE);
  }
  regfree(&re);
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
    return match->rm_eo - match->rm_so;
  } else if (result == REG_NOMATCH) {
    return 0;
  } else {
    perror("regex went wrong");
    exit(EXIT_FAILURE);
  }
  regfree(&re);
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
    return match->rm_eo - match->rm_so;
  } else if (result == REG_NOMATCH) {
    return 0;
  } else {
    perror("regex went wrong");
    exit(EXIT_FAILURE);
  }
  regfree(&re);
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
    return match->rm_eo - match->rm_so;
  } else if (result == REG_NOMATCH) {
    return 0;
  } else {
    perror("regex went wrong");
    exit(EXIT_FAILURE);
  }
  regfree(&re);
  return 0;
}

Token *tokenize(char *s) {
  char *start_loc = s;
  Token root = {0};
  Token *cur = &root;
  size_t len;

  while (*s) {

    // skip whitespace
    len = match_space(s);
    if (len) {
      s += len;
      continue;
    }

    len = match_float(s);
    if (len) {
      cur->next = new_token(NUMERIC, s, len);
      char *buf = calloc(1, len);
      sprintf(buf, "%.*s", (int)len, s);
      cur->next->ival = strtold(buf, NULL);
      cur->next->str = buf;
      cur = cur->next;
      s += len;
      continue;
    }
    len = match_integer(s);
    if (len) {
      cur->next = new_token(NUMERIC, s, len);
      char *buf = calloc(1, len);
      sprintf(buf, "%.*s", (int)len, s);
      cur->next->ival = strtoll(buf, NULL, 10);
      cur->next->str = buf;
      cur = cur->next;
      s += len;
      continue;
    }

    len = match_string(s);
    if (len) {
      cur->next = new_token(STRING, s, len);
      char *buf = calloc(1, len - 2);
      sprintf(buf, "%.*s", (int)len - 2, s + 1);
      cur->next->str = buf;
      cur = cur->next;
      s += len + 1;
      continue;
    }

    len = match_keyword(s);
    if (len) {
      cur->next = new_token(KEYWORD, s, len);
      char *buf = calloc(1, len);
      sprintf(buf, "%.*s", (int)len, s);
      cur->next->str = buf;
      cur = cur->next;
      s += len;
      continue;
    }

    len = match_identifier(s);
    if (len) {
      cur->next = new_token(IDENTIFIER, s, len);
      char *buf = calloc(1, len);
      sprintf(buf, "%.*s", (int)len, s);
      cur->next->str = buf;
      cur = cur->next;
      s += len;
      continue;
    }

    exit(EXIT_FAILURE);
  }

  cur = cur->next = new_token(EOQ, s, 0);
  return root.next;
}
