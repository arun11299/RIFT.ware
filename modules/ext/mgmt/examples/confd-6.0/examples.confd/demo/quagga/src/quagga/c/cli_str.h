#ifndef _CLI_STR_H_
#define _CLI_STR_H_
#include <stdarg.h>

/**
 * Format a cli command. First, do a sprintf according to the va_list
 * arguments, then expand all strings of the form
 * <code>${ex: |varname| [, |def. value|]}</code>,
 * <code>${bool: |varname|, |true value|, |false value|[, |default|]}</code>,
 * <code>${key: |varname|}</code> and <code>${arg [: |varname|]}</code>.
 *
 * @param data_ user callback parameter
 * @param dest destination string
 * @param dest_size size of the destination string
 * @param format
 * @param ap
 *
 * @see expand_value_ex, expand_value_bool, expand_value_key, expand_value_arg
 */
int cli_format(void *data_, char *dest, int dest_size, const char *format, va_list ap);

/* To be defined elsewhere */
/**
 * Read the variable's string value, if exists.
 *
 * @return length of the value, 0 if does not exist, -1 on error
 */
extern int read_variable_ex(void *data_, const char *name, char *value, int size);

/**
 * Read the variable's bool value, if exists.
 *
 * @return 1 if read successfully, 0 if does not exist, -1 on error
 */
extern int read_variable_bool(void *data_, const char *name, int *value);

/**
 * Read the key element's string value (must exist).
 *
 * @return length of the value, -1 on error
 */
extern int read_variable_key(void *data_, const char *name, int index, char *value, int size);

/**
 * Read the argument's value.
 *
 * @return length of the value, -1 on error
 */
extern int read_variable_arg(void *data_, const char *name, char *value, int size);

/**
 * Read access-list value.
 *
 * @return length of the value, -1 on error
 */
extern int read_variable_alist(void *data_, const char *name, char *value, int size);

#endif
