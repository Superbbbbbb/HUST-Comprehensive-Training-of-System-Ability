#include "klib.h"
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
#define STR_LEN 256
#define IS_D(c) c >= '0' && c <= '9'

int printf(const char *fmt, ...) {
  char out[STR_LEN] = {0};
  va_list ap;
  va_start(ap, fmt);
  int len = vsprintf(out, fmt, ap);
  va_end(ap);
  for (int i = 0; out[i]; i++) {
    _putc(out[i]);
  }
  return len;
}

int number(char *out, int len, unsigned int num, int neg, int width) {
  char str[STR_LEN] = {0};
  int i = 0;
  if(num == 0)
    str[i++] = '0';
  while (num) {
    str[i++] = num % 10 + '0';
    num /= 10;
  }
  int l=i;
  if (width > l){
    while(width != l){
      str[i++] = '0';
      width--;
    }
  }
  if (neg) {
    str[i++] = '-';
  }
  while (--i >= 0) {
    out[len++] = str[i];
  }
  return len;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  int len = 0;
  const char *p = fmt;
  int arg_num, neg=0, width=0;
  char *arg_s;
  while (*p) {
    if(*p != '%')
      out[len++] = *(p++);
    else {
      p++;
      if (*p == '0'){
        width = *(++p)-'0';
        p++;
      }
      switch (*p) {
      case 'd':
        arg_num = va_arg(ap, int);
        if (arg_num < 0) {
          neg = 1;
          arg_num = -arg_num;
        }
        len = number(out, len, arg_num, neg, width);
        break;
      case 's' :
        arg_s = (char*) va_arg(ap, char*);
        while (*arg_s) {
          out[len++] = *(arg_s++);
        }
        break;
      default:
        break;
      }
      p++;
    }
  }
  out[len] = '\0';
  return len;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int length = vsprintf(out, fmt, ap);
  va_end(ap);
  return length;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  return 0;
}

#endif

