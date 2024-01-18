#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

#define TOKENS_SIZE 65536

enum {
  TK_NOTYPE = 256,

  /* TODO: Add more token types */
  TK_DEC,
  TK_HEX,
  TK_EQ,
  TK_UNEQ,
  TK_AND,
  TK_OR,
  TK_REG,
  TK_DEREFERENCE,
};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  {" +", TK_NOTYPE},
  {"0x[0-9a-fA-F]+", TK_HEX},
  {"[1-9][0-9]*|0", TK_DEC},
  {"\\$(0|ra|sp|gp|tp|t[0-6]|s[0-9]|a[0-7])", TK_REG},
  {"==", TK_EQ},
  {"!=", TK_UNEQ},
  {"&&", TK_AND},
  {"\\|\\|", TK_OR},
  {"&",'&'},
  {"\\|", '|'},
  {"\\+", '+'},
  {"-", '-'},
  {"\\*", '*'},
  {"/", '/'},
  {"\\(", '('},
  {"\\)", ')'},
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )
static regex_t re[NR_REGEX] = {};

void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        tokens[nr_token].type = rules[i].token_type;
        switch (rules[i].token_type) {
          case TK_REG:
          case TK_HEX:
          case TK_DEC:
            strncpy(tokens[nr_token].str, substr_start, substr_len); 
            tokens[nr_token].str[substr_len] = '\0';
            break;
          default: break;
        }
        if (rules[i].token_type != TK_NOTYPE)
          nr_token++;
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }
  return true;
}

bool check_parentheses(int p, int q, bool *success) {
  int cnt = 0;
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == '(') cnt++;
    else if (tokens[i].type == ')') cnt--;
    if (cnt < 0) break;
  }
  if (cnt != 0) {
    *success = false;
    return false;
  }
  return tokens[p].type == '(' && tokens[q].type == ')';
}

int main_op(int p, int q) {
  int op = p, cnt;
  for (int i = p; i <= q; i++) {
    if(tokens[i].type == '('){
      cnt = 1;
      while (i <= q && cnt) {
        i++;
        if (tokens[i].type == '(')
          cnt++;
        else if (tokens[i].type == ')')
          cnt--;
      }
      i++;
    }
    switch (tokens[i].type) {
      case TK_OR:  
        op = i; 
        break;
      case TK_AND:  
        if (tokens[op].type != TK_OR)
          op = i;
        break;
      case TK_EQ:       
      case TK_UNEQ:  
        if (tokens[op].type != TK_AND && tokens[op].type != TK_OR)
          op = i;
        break;
      case '+':
      case '-':
        if (tokens[op].type != TK_AND
          && tokens[op].type != TK_OR
          && tokens[op].type != TK_EQ
          && tokens[op].type != TK_UNEQ)
          op = i;
        break;
      case '*':
      case '/':
      case '&':
      case '|':
        if (tokens[op].type != TK_AND
          && tokens[op].type != TK_OR
          && tokens[op].type != TK_EQ
          && tokens[op].type != TK_UNEQ
          && tokens[op].type != '+' 
          && tokens[op].type != '-')
          op = i;
        break;
      case TK_DEREFERENCE:   
        if (tokens[op].type != TK_AND
          && tokens[op].type != TK_OR
          && tokens[op].type != TK_EQ
          && tokens[op].type != TK_UNEQ
          && tokens[op].type != '+' 
          && tokens[op].type != '-'
          && tokens[op].type != '*' 
          && tokens[op].type != '/'
          && tokens[op].type != '&' 
          && tokens[op].type != '|')
          op = i;
        break;
      default:
        break;
    }
  }
  return op;
}

uint32_t eval(int p, int q, bool *success) {
  if (p > q) {
    *success = false;
    return 0;
  } else if (p == q) {  // single token
    switch (tokens[p].type) {
      case TK_DEC: return atoi(tokens[p].str);
      case TK_HEX: return strtol(tokens[p].str, NULL, 16);
      case TK_REG: return isa_reg_str2val(tokens[p].str+1, success);
      default: *success = false; return 0; 
    }
  } else if (check_parentheses(p, q, success) == true) {
    return eval(p + 1, q - 1, success);
  } else if (*success == false) {
    return 0;
  } else {
    int op = main_op(p, q);
    if (tokens[op].type == TK_DEREFERENCE) {
      uint32_t addr = eval(op + 1, q, success);
      if (*success == false) return 0;
      return paddr_read(addr, 4);
    }
    uint32_t val1 = eval(p, op - 1, success);
    uint32_t val2 = eval(op + 1, q, success);
    if (*success == false) return 0;
    switch (tokens[op].type) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': 
        if (val2 == 0) {
          *success = false;
          return 0;
        } else {
          return val1 / val2;
        }
      case '&': return val1 & val2;
      case '|': return val1 | val2;
      case TK_EQ: return val1 == val2;
      case TK_UNEQ: return val1 != val2;
      case TK_AND: return val1 && val2;
      case TK_OR:  return val1 || val2;
      default: assert(0);
    }
  }
  return 0;
}

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  for (int i = 0; i < nr_token; i ++) {
    if (tokens[i].type == '*' && (i == 0 
      || tokens[i - 1].type == '('
      || tokens[i - 1].type == '+'
      || tokens[i - 1].type == '-'
      || tokens[i - 1].type == '*'
      || tokens[i - 1].type == '/'
      || tokens[i - 1].type == '&'
      || tokens[i - 1].type == '|'
      || tokens[i - 1].type == TK_EQ
      || tokens[i - 1].type == TK_UNEQ
      || tokens[i - 1].type == TK_AND
      || tokens[i - 1].type == TK_OR
    )) {
      tokens[i].type = TK_DEREFERENCE;
    }
  }

  return eval(0, nr_token - 1, &success);
}
