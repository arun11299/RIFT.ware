#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

#include "logs.h"
#include "cli_str.h"

#define FORMAT_ERR(...) do { ERROR_LOG(__VA_ARGS__); return NULL; } while(0)
#define COPY_ERR(...) do { ERROR_LOG(__VA_ARGS__); return -1; } while(0)

#define REQUIRE_CHAR(src_buf, c, err_action) do { if (src_buf->pos == src_buf->size || src_buf->buf[src_buf->pos] != c) err_action; else src_buf->pos++; } while(0)
#define MK_STR(str) #str
#define COPY_REQUIRE_CHAR(src_buf, c) REQUIRE_CHAR(src_buf, c, COPY_ERR("Expected: " MK_STR(c)));

typedef struct {
  char *buf;    /**< Character buffer */
  int size;     /**< Buffer size */
  int pos;      /**< Position within the buffer */
} buffer_t;

#define BUFFER_PEEK(src) (src->buf[src->pos])
#define BUFFER_PTR(src) (src->buf + src->pos)
#define BUFFER_END(src) (src->pos == src->size)
#define BUFFER_SKIP(src) (src->pos++)
#define BUFFER_SKIP_N(src, n) (src->pos += n)
#define BUFFER_FREE(src) (src->size - src->pos)

#define EX_NAME "EX"
#define BOOL_NAME "BOOL"
#define KEY_NAME "KEY"
#define ARG_NAME "ARG"
#define ALIST_NAME "ALIST"

typedef enum {
  EX_TYPE,
  BOOL_TYPE,
  KEY_TYPE,
  ARG_TYPE,
  ALIST_TYPE
} expansion_t;

buffer_t *mkbuf(char *buf, int size, buffer_t *rv) {
  rv->buf = buf;
  rv->pos = 0;
  rv->size = size;
  return rv;
}

buffer_t *mkbuf_(char *buf, buffer_t *rv) {
  rv->buf = buf;
  rv->pos = 0;
  rv->size = strlen(buf);
  return rv;
}

int buf_strncpy(buffer_t *dst, buffer_t *src, int n) {
  if (dst->pos + n > dst->size) {
    strncpy(dst->buf+dst->pos, src->buf+src->pos, dst->size - dst->pos);
    dst->pos = dst->size;
    src->pos += dst->size - dst->pos;
    return -1;
  }
  strncpy(BUFFER_PTR(dst), BUFFER_PTR(src), n);
  src->pos += n;
  dst->pos += n;
  return 0;
}

/**
 * Copy possibly escaped strings up to first occurence of one of
 * terminating characters. Be sure to include the escape char (usually '\')
 * as the first term character.
 *
 * @param dest
 * @param src
 * @param term special characters; 0th is the escape char, others are
 * the terminators

 * @return index of terminating character found, 0 if copied completely, -1 on error
 */
int copy_escaped(buffer_t *dest, buffer_t *src, const char *term)
{
  char *rv;
  char *inbuf;
  inbuf = src->buf + src->pos;
  while (1) {
    int p;
    rv = strpbrk(inbuf, term);
    if (rv == NULL)
      p = strlen(inbuf);
    else
      p = rv - inbuf;
    if (buf_strncpy(dest, src, p) == -1)
      COPY_ERR("Buffer full!");
    if (rv == NULL || rv[0] != term[0])
      return rv == NULL ? 0 : strchr(term, rv[0]) - term;
    /* found the escape char - is there anything to escape? */
    if (rv[1] == 0) {
      if (buf_strncpy(dest, src, 1))
        COPY_ERR("Buffer full!");
      return 0;
    }
    /* found the escape char - continue */
    src->pos++;
    if (buf_strncpy(dest, src, 1) == -1)
      COPY_ERR("Buffer full!");
    inbuf = rv+2;
  }
}

/**
 * Skip space characters.
 */
void trim(buffer_t *src) {
  while (src->pos < src->size && isspace(src->buf[src->pos]))
    src->pos++;
}

/**
 * Skip a symbol.
 *
 * @return number of characters skipped (0 if the first symbol was not alphabetic)
 */
int skip_symbol(buffer_t *src) {
  int cnt;
  for (cnt = 0; ! BUFFER_END(src) && (isalnum(BUFFER_PEEK(src)) || strchr("-/[].", BUFFER_PEEK(src)) != NULL); cnt++, BUFFER_SKIP(src));
  return cnt;
}

/**
 * Skip a number.
 * @see #skip_symbol
 */
int skip_number(buffer_t *src) {
  int cnt;
  for (cnt = 0; ! BUFFER_END(src) && isdigit(BUFFER_PEEK(src)); cnt++, BUFFER_SKIP(src));
  return cnt;
}

/**
 * Read a value. Value is either a string delimited by single quotes
 * (possibly escaped within the string), or (basically) a symbol.
 *
 * @return -1 if something fails, 0 otherwise
 */
int read_value(buffer_t *dst, buffer_t *src) {
  if (BUFFER_PEEK(src) == '\'') {
    /* value is enclosed in apostrophes */
    BUFFER_SKIP(src);
    if (copy_escaped(dst, src, "\\'") != 1)
      COPY_ERR("Expected quotes-delimited value");
    BUFFER_SKIP(src);
  } else {
    /* value delimited by space */
    if (copy_escaped(dst, src, "\\ ,}") < 1)
      COPY_ERR("Expected value");
  }
  return 0;
}

/**
 * Expand variable from the database, use default (if defined) if it
 * does not exist. The expansion uses pattern '%s', if none is
 * supplied.  Syntax: ${ex: |varname| [, |pattern| [, |default|]}
 */
int expand_value_ex(void *data_, buffer_t *dst, buffer_t *src, char *varname) {
  int deflen;
  char value[BUFSIZ];
  char pattern[BUFSIZ];
  int var_len;
  deflen = 0;
  if ((var_len = read_variable_ex(data_, varname, value, BUFSIZ)) < 0)
    COPY_ERR("read failed: varname %s, buffer pos.: %i, read: %i", varname, dst->pos, var_len);
  if (BUFFER_PEEK(src) == ',') {
    /* pattern is present */
    buffer_t pattern_buf;
    BUFFER_SKIP(src);
    trim(src);
    if (read_value(mkbuf(pattern, BUFSIZ, &pattern_buf), src) == -1)
      return -1;
    trim(src);
    pattern[pattern_buf.pos] = 0;
  } else {
    strcpy(pattern, "%s");
  }
  if (var_len > 0) {
    int out_len;
    if ((out_len = snprintf(BUFFER_PTR(dst), BUFFER_FREE(dst), pattern, value)) >= BUFFER_FREE(dst))
      COPY_ERR("Buffer full - could not expand: ${ex: %s, %s}, value = %s!", varname, pattern, value);
    BUFFER_SKIP_N(dst, out_len);
  }
  if (BUFFER_PEEK(src) == ',') {
    buffer_t valbuf;
    /* default value is present */
    BUFFER_SKIP(src);
    trim(src);
    if (read_value(mkbuf(value, BUFSIZ, &valbuf), src) == -1)
      return -1;
    deflen = valbuf.pos;
  }
  value[deflen] = 0;
  if (var_len == 0) {
    /* means, the variable was not read */
    if (deflen <= BUFFER_FREE(dst)) {
      strncpy(BUFFER_PTR(dst), value, deflen);
      BUFFER_SKIP_N(dst, deflen);
    } else
      COPY_ERR("Buffer full!");
  }
  return 0;
}

/**
 * Expand boolean variable from the database, substitute values
 * according to its database value, use default if it does not exist.
 * Syntax: ${bool: |varname|, |true_value|, |false_value| [, |default|]}
 */
int expand_value_bool(void *data_, buffer_t *dst, buffer_t *src, const char *varname) {
  char tmp[BUFSIZ];
  buffer_t valbuf;
  int var_value, exists;
  COPY_REQUIRE_CHAR(src, ',');
  trim(src);

  if ((exists = read_variable_bool(data_, varname, &var_value)) == -1)
    COPY_ERR("Could not read bool variable %s: %i", varname, exists);

  /* true value */
  if (read_value(mkbuf(tmp, BUFSIZ, &valbuf), src) == -1)
    return -1;
  tmp[valbuf.pos] = 0;
  if (exists && var_value) {
    if (valbuf.pos >= BUFFER_FREE(dst))
      COPY_ERR("Buffer full - could not expand bool value: %s, value = %s", varname, tmp);
    strcpy(BUFFER_PTR(dst), tmp);
    dst->pos += valbuf.pos;
  }
  trim(src);
  COPY_REQUIRE_CHAR(src, ',');
  trim(src);

  /* false value */
  if (read_value(mkbuf(tmp, BUFSIZ, &valbuf), src) == -1)
    return -1;
  tmp[valbuf.pos] = 0;
  if (exists && ! var_value) {
    if (valbuf.pos >= BUFFER_FREE(dst))
      COPY_ERR("Buffer full - could not expand bool value: %s, value = %s", varname, tmp);
    strcpy(BUFFER_PTR(dst), tmp);
    dst->pos += valbuf.pos;
  }

  trim(src);
  valbuf.pos = 0;
  if (BUFFER_PEEK(src) == ',') {
    /* there is a default value */
    BUFFER_SKIP(src);
    trim(src);
    if (read_value(mkbuf(tmp, BUFSIZ, &valbuf), src) == -1)
      return -1;
    if (! exists) {
      if (valbuf.pos >= BUFFER_FREE(dst))
        COPY_ERR("Buffer full - could not expand bool value: %s, value = %s", varname, tmp);
      strcpy(BUFFER_PTR(dst), tmp);
      dst->pos += valbuf.pos;
    }
  }

  return 0;
}

/**
 * Substitute an argument. If no argument found, use variable as given
 * by the (optional) name. Syntax: ${arg[: varname]}
 */
int expand_value_arg(void *data_, buffer_t *dst, char *varname) {
  int var_len;
  if ((var_len = read_variable_arg(data_, varname, BUFFER_PTR(dst), BUFFER_FREE(dst))) < 0)
    COPY_ERR("read failed - buffer pos. %i", dst->pos);
  BUFFER_SKIP_N(dst, var_len);
  return 0;
}

/**
 * Substitute alist value. Syntax: ${alist: varname}
 */
int expand_value_alist(void *data_, buffer_t *dst, char *varname) {
  int var_len;
  if ((var_len = read_variable_alist(data_, varname, BUFFER_PTR(dst), BUFFER_FREE(dst))) < 0)
    COPY_ERR("read failed - buffer pos. %i", dst->pos);
  BUFFER_SKIP_N(dst, var_len);
  return 0;
}

/**
 * Substitute a key element's value. Syntax: ${key: |key_name|}
 */
int expand_value_key(void *data_, buffer_t *dst, buffer_t *src, const char *name) {
  int var_len;
  int index = 0;
  trim(src);
  if (BUFFER_PEEK(src) == '(') {
    char *iptr;
    BUFFER_SKIP(src);
    trim(src);
    index = strtol(BUFFER_PTR(src), &iptr, 10);
    if (iptr - BUFFER_PTR(src) == 0)
      COPY_ERR("read failed - expecting a number (pos. %i)", src->pos);
    BUFFER_SKIP_N(src, iptr - BUFFER_PTR(src));
    trim(src);
    COPY_REQUIRE_CHAR(src, ')');
  }
  if ((var_len = read_variable_key(data_, name, index, BUFFER_PTR(dst), BUFFER_FREE(dst))) < 0)
    COPY_ERR("Key read failed - name %s, buffer pos. %i", name, dst->pos);
  BUFFER_SKIP_N(dst, var_len);

  return 0;
}

/**
 * Expand variable substitution at given place in the input buffer.
 *
 * @return 0 if successful, -1 otherwise (buffer overflow, bad substitution string, ...)
 */
int expand_value(void *data_, buffer_t *dst, buffer_t *src) {
  char *cptr;
  char tmp[BUFSIZ];
  int symlen;
  expansion_t expansion_type;
  COPY_REQUIRE_CHAR(src, '$');
  COPY_REQUIRE_CHAR(src, '{');
  trim(src);

  /* First, find the type of expansion */
  cptr = BUFFER_PTR(src);
  if ((symlen = skip_symbol(src)) == 0)
    COPY_ERR("Expected expansion type identification");
  if (strncasecmp(cptr, EX_NAME, symlen) == 0)
    expansion_type = EX_TYPE;
  else if (strncasecmp(cptr, BOOL_NAME, symlen) == 0)
    expansion_type = BOOL_TYPE;
  else if (strncasecmp(cptr, KEY_NAME, symlen) == 0)
    expansion_type = KEY_TYPE;
  else if (strncasecmp(cptr, ARG_NAME, symlen) == 0)
    expansion_type = ARG_TYPE;
  else if (strncasecmp(cptr, ALIST_NAME, symlen) == 0)
    expansion_type = ALIST_TYPE;
  else
    COPY_ERR("Expected one of %s, %s, %s, %s, %s", EX_NAME, BOOL_NAME, KEY_NAME, ARG_NAME, ALIST_NAME);

  trim(src);

  symlen = 0;
  tmp[0] = 0;
  if (expansion_type != ARG_TYPE || BUFFER_PEEK(src) == ':') {
    COPY_REQUIRE_CHAR(src, ':');
    trim(src);

    /* now find the variable name */
    cptr = BUFFER_PTR(src);
    if ((symlen = skip_symbol(src)) == 0)
      COPY_ERR("Expected variable name");
    strncpy(tmp, src->buf + src->pos - symlen, symlen);
    tmp[symlen] = 0;
    DEBUG_LOG("Expanding variable %s", tmp);
    trim(src);
  }

  /* perform the expansion */
  switch (expansion_type) {
  case EX_TYPE:
    if (expand_value_ex(data_, dst, src, tmp) == -1)
      return -1;
    break;
  case BOOL_TYPE:
    if (expand_value_bool(data_, dst, src, tmp) == -1)
      return -1;
    break;
  case ARG_TYPE:
    if (expand_value_arg(data_, dst, tmp) == -1)
      return -1;
    break;
  case KEY_TYPE:
    if (expand_value_key(data_, dst, src, tmp) == -1)
      return -1;
    break;
  case ALIST_TYPE:
    if (expand_value_alist(data_, dst, tmp) == -1)
      return -1;
    break;
  }
  trim(src);
  COPY_REQUIRE_CHAR(src, '}');
  return 0;
}

int cli_format(void *data_, char *dest, int dest_size, const char *format, va_list ap) {
  char tmp[BUFSIZ];
  int v;
  buffer_t in, out;
  vsprintf(tmp, format, ap);
  mkbuf(dest, dest_size, &out);
  mkbuf_(tmp, &in);
  do {
    switch (v = copy_escaped(&out, &in, "\\$")) {
    case 1:
      if (expand_value(data_, &out, &in) != 0) {
        out.buf[out.pos] = 0;
        return -1;
      }
      break;
    case -1:
      out.buf[out.pos] = 0;
      return -1;
    }
  } while (v != 0);
  if (out.pos < out.size)
    out.buf[out.pos] = 0;
  return out.pos;
}
