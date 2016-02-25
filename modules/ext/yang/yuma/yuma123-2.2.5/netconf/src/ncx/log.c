/*
 * Copyright (c) 2008 - 2012, Andy Bierman, All Rights Reserved.
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.    
 */
/*  FILE: log.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
08jan06      abb      begun, borrowed from openssh code

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>

#include "procdefs.h"
#include "log.h"
#include "ncxconst.h"
#include "status.h"
#include "tstamp.h"
#include "xml_util.h"
#include "ncxtypes.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

/* #define LOG_DEBUG_TRACE 1 */

/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION log_open
*
*   Open a logfile for writing
*   DO NOT use this function to send log entries to STDOUT
*   Leave the logfile NULL instead.
*
* INPUTS:
*   fname == full filespec string for logfile
*   append == TRUE if the log should be appended
*          == FALSE if it should be rewriten
*   tstamps == TRUE if the datetime stamp should be generated
*             at log-open and log-close time
*          == FALSE if no open and close timestamps should be generated
*
* RETURNS:
*    status
*********************************************************************/
status_t
    log_open (ncx_instance_t *instance,
              const char *fname,
              boolean append,
              boolean tstamps)
{
    const char *str;
    xmlChar buff[TSTAMP_MIN_SIZE];

#ifdef DEBUG
    if (!fname) {
        return SET_ERROR(instance, ERR_INTERNAL_PTR);
    }
#endif

    if (instance->logfile) {
        return ERR_NCX_DATA_EXISTS;
    }

    if (append) {
        str="a";
    } else {
        str="w";
    }

    instance->logfile = fopen(fname, str);
    if (!instance->logfile) {
        return ERR_FIL_OPEN;
    }

    instance->use_tstamps = tstamps;
    if (tstamps) {
        tstamp_datetime(instance, buff);
        fprintf(instance->logfile, "\n*** log open at %s ***\n", buff);
    }

    return NO_ERR;

}  /* log_open */


/********************************************************************
* FUNCTION log_close
*
*   Close the logfile
*
* RETURNS:
*    none
*********************************************************************/
void
    log_close (ncx_instance_t *instance)
{
    xmlChar buff[TSTAMP_MIN_SIZE];

    if (!instance->logfile) {
        return;
    }

    if (instance->use_tstamps) {
        tstamp_datetime(instance, buff);
        fprintf(instance->logfile, "\n*** log close at %s ***\n", buff);
    }

    fclose(instance->logfile);
    instance->logfile = NULL;

}  /* log_close */


/********************************************************************
* FUNCTION log_audit_open
*
*   Open the audit logfile for writing
*   DO NOT use this function to send log entries to STDOUT
*   Leave the audit_logfile NULL instead.
*
* INPUTS:
*   fname == full filespec string for audit logfile
*   append == TRUE if the log should be appended
*          == FALSE if it should be rewriten
*   tstamps == TRUE if the datetime stamp should be generated
*             at log-open and log-close time
*          == FALSE if no open and close timestamps should be generated
*
* RETURNS:
*    status
*********************************************************************/
status_t
    log_audit_open (ncx_instance_t *instance,
                    const char *fname,
                    boolean append,
                    boolean tstamps)
{
    const char *str;
    xmlChar buff[TSTAMP_MIN_SIZE];

#ifdef DEBUG
    if (fname == NULL) {
        return SET_ERROR(instance, ERR_INTERNAL_PTR);
    }
#endif

    if (instance->auditlogfile != NULL) {
        return ERR_NCX_DATA_EXISTS;
    }

    if (append) {
        str="a";
    } else {
        str="w";
    }

    instance->auditlogfile = fopen(fname, str);
    if (instance->auditlogfile == NULL) {
        return ERR_FIL_OPEN;
    }

    instance->use_audit_tstamps = tstamps;
    if (tstamps) {
        tstamp_datetime(instance, buff);
        fprintf(instance->auditlogfile, "\n*** audit log open at %s ***\n", buff);
    }

    return NO_ERR;

}  /* log_audit_open */


/********************************************************************
* FUNCTION log_audit_close
*
*   Close the audit_logfile
*
* RETURNS:
*    none
*********************************************************************/
void
    log_audit_close (ncx_instance_t *instance)
{
    xmlChar buff[TSTAMP_MIN_SIZE];

    if (instance->auditlogfile == NULL) {
        return;
    }

    if (instance->use_audit_tstamps) {
        tstamp_datetime(instance, buff);
        fprintf(instance->auditlogfile, "\n*** audit log close at %s ***\n", buff);
    }

    fclose(instance->auditlogfile);
    instance->auditlogfile = NULL;

}  /* log_audit_close */


/********************************************************************
* FUNCTION log_audit_is_open
*
*   Check if the audit log is open
*
* RETURNS:
*   TRUE if audit log is open; FALSE if not
*********************************************************************/
boolean
    log_audit_is_open (ncx_instance_t *instance)
{
    return (instance->auditlogfile == NULL) ? FALSE : TRUE;

}  /* log_audit_is_open */


/********************************************************************
* FUNCTION log_alt_open
*
*   Open an alternate logfile for writing
*   DO NOT use this function to send log entries to STDOUT
*   Leave the logfile NULL instead.
*
* INPUTS:
*   fname == full filespec string for logfile
*
* RETURNS:
*    status
*********************************************************************/
status_t
    log_alt_open (ncx_instance_t *instance, const char *fname)
{
    const char *str;

#ifdef DEBUG
    if (!fname) {
        return SET_ERROR(instance, ERR_INTERNAL_PTR);
    }
#endif

    if (instance->altlogfile) {
        return ERR_NCX_DATA_EXISTS;
    }

    str="w";

    instance->altlogfile = fopen(fname, str);
    if (!instance->altlogfile) {
        return ERR_FIL_OPEN;
    }

    return NO_ERR;

}  /* log_alt_open */


/********************************************************************
* FUNCTION log_alt_close
*
*   Close the alternate logfile
*
* RETURNS:
*    none
*********************************************************************/
void
    log_alt_close (ncx_instance_t *instance)
{
    if (!instance->altlogfile) {
        return;
    }

    fclose(instance->altlogfile);
    instance->altlogfile = NULL;

}  /* log_alt_close */


/********************************************************************
* FUNCTION log_stdout
*
*   Write lines of text to STDOUT, even if the logfile
*   is open, unless the debug mode is set to NONE
*   to indicate silent batch mode
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for printf
*
*********************************************************************/
void 
    log_stdout (ncx_instance_t *instance, const char *fstr, ...)
{
    va_list args;

    if (log_get_debug_level(instance) == LOG_DEBUG_NONE) {
        return;
    }

    va_start(args, fstr);
    vprintf(fstr, args);
    fflush(stdout);
    va_end(args);

}  /* log_stdout */


/********************************************************************
* FUNCTION log_write
*
*   Generate a log entry, regardless of log level
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for printf
*
*********************************************************************/
void 
    log_write (ncx_instance_t *instance, const char *fstr, ...)
{
    va_list args;

    if (log_get_debug_level(instance) == LOG_DEBUG_NONE) {
        return;
    }

    va_start(args, fstr);

    if (instance->logfile) {
        vfprintf(instance->logfile, fstr, args);
        fflush(instance->logfile);
    } else {
        vprintf(fstr, args);
        fflush(stdout);
    }

    va_end(args);

}  /* log_write */


/********************************************************************
* FUNCTION log_audit_write
*
*   Generate an audit log entry, regardless of log level
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for printf
*
*********************************************************************/
void 
    log_audit_write (ncx_instance_t *instance, const char *fstr, ...)
{
    va_list args;

    va_start(args, fstr);

    if (instance->auditlogfile != NULL) {
        vfprintf(instance->auditlogfile, fstr, args);
        fflush(instance->auditlogfile);
    }

    va_end(args);

}  /* log_audit_write */


/********************************************************************
* FUNCTION log_alt_write
*
*   Write to the alternate log file
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for fprintf
*
*********************************************************************/
void 
    log_alt_write (ncx_instance_t *instance, const char *fstr, ...)
{
    va_list args;

    va_start(args, fstr);

    if (instance->altlogfile) {
        vfprintf(instance->altlogfile, fstr, args);
        fflush(instance->altlogfile);
    }

    va_end(args);

}  /* log_alt_write */


/********************************************************************
* FUNCTION log_alt_indent
* 
* Printf a newline to the alternate logfile,
* then the specified number of space chars
*
* INPUTS:
*    indentcnt == number of indent chars, -1 == skip everything
*
*********************************************************************/
void
    log_alt_indent (ncx_instance_t *instance, int32 indentcnt)
{
    int32  i;

    if (indentcnt >= 0) {
        log_alt_write(instance, "\n");
        for (i=0; i<indentcnt; i++) {
            log_alt_write(instance, " ");
        }
    }

} /* log_alt_indent */

/********************************************************************
* FUNCTION vlog_error
*
*   Generate a LOG_DEBUG_ERROR log entry
*
* INPUTS:
*   fstr == format string in printf format
*   valist == any additional arguments for printf
*
*********************************************************************/
void vlog_error (ncx_instance_t *instance, const char *fstr, va_list args )
{
    if (log_get_debug_level(instance) < LOG_DEBUG_ERROR) {
        return;
    }

    if (instance->custom && instance->custom->logging_routine) {
      instance->custom->logging_routine(instance, LOG_DEBUG_ERROR, fstr, args);
    } else if (instance->logfile) {
      vfprintf(instance->logfile, fstr, args);
      fflush(instance->logfile);
    } else {
      vprintf(fstr, args);
      fflush(stdout);
    }
}  /* log_error */

/********************************************************************
* FUNCTION log_error
*
*   Generate a LOG_DEBUG_ERROR log entry
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for printf
*
*********************************************************************/
void log_error (ncx_instance_t *instance, const char *fstr, ...)
{
    va_list args;
    va_start(args, fstr);

    if (instance->custom && instance->custom->logging_routine) {
      instance->custom->logging_routine(instance, LOG_DEBUG_ERROR, fstr, args);
    } else {
      vlog_error(instance,  fstr, args );
    }

    va_end(args);
}  /* log_error */


/********************************************************************
* FUNCTION log_warn
*
*   Generate LOG_DEBUG_WARN log output
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for printf
*
*********************************************************************/
void 
    log_warn (ncx_instance_t *instance, const char *fstr, ...)
{
    va_list args;

    if (log_get_debug_level(instance) < LOG_DEBUG_WARN) {
        return;
    }

    va_start(args, fstr);

    if (instance->custom && instance->custom->logging_routine) {
      instance->custom->logging_routine(instance, LOG_DEBUG_WARN, fstr, args);
    } else if (instance->logfile) {
        vfprintf(instance->logfile, fstr, args);
        fflush(instance->logfile);
    } else {
      vprintf(fstr, args);
      fflush(stdout);
    }

    va_end(args);

}  /* log_warn */


/********************************************************************
* FUNCTION log_info
*
*   Generate a LOG_DEBUG_INFO log entry
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for printf
*
*********************************************************************/
void 
    log_info (ncx_instance_t *instance, const char *fstr, ...)
{
    va_list args;

    if (log_get_debug_level(instance) < LOG_DEBUG_INFO) {
        return;
    }

    va_start(args, fstr);

    if (instance->custom && instance->custom->logging_routine) {
      instance->custom->logging_routine(instance, LOG_DEBUG_INFO, fstr, args);
    } else if (instance->logfile) {
      vfprintf(instance->logfile, fstr, args);
      fflush(instance->logfile);
    } else {
      vprintf(fstr, args);
      fflush(stdout);
    }

    va_end(args);

}  /* log_info */


/********************************************************************
* FUNCTION log_debug
*
*   Generate a LOG_DEBUG_DEBUG log entry
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for printf
*
*********************************************************************/
void 
    log_debug (ncx_instance_t *instance, const char *fstr, ...)
{
    va_list args;

    if (log_get_debug_level(instance) < LOG_DEBUG_DEBUG) {
        return;
    }

    va_start(args, fstr);

    if (instance->custom && instance->custom->logging_routine) {
      instance->custom->logging_routine(instance, LOG_DEBUG_DEBUG, fstr, args);
    } else if (instance->logfile) {
      vfprintf(instance->logfile, fstr, args);
      fflush(instance->logfile);
    } else {
      vprintf(fstr, args);
      fflush(stdout);
    }

    va_end(args);

}  /* log_debug */


/********************************************************************
* FUNCTION log_debug2
*
*   Generate LOG_DEBUG_DEBUG2 log trace output
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for printf
*
*********************************************************************/
void 
    log_debug2 (ncx_instance_t *instance, const char *fstr, ...)
{
    va_list args;

    if (log_get_debug_level(instance) < LOG_DEBUG_DEBUG2) {
        return;
    }

    va_start(args, fstr);

    if (instance->custom && instance->custom->logging_routine) {
      instance->custom->logging_routine(instance, LOG_DEBUG_DEBUG2, fstr, args);
    } else if (instance->logfile) {
      vfprintf(instance->logfile, fstr, args);
      fflush(instance->logfile);
    } else {
      vprintf(fstr, args);
      fflush(stdout);
    }

    va_end(args);

}  /* log_debug2 */


/********************************************************************
* FUNCTION log_debug3
*
*   Generate LOG_DEBUG_DEBUG3 log trace output
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for printf
*
*********************************************************************/
void 
    log_debug3 (ncx_instance_t *instance, const char *fstr, ...)
{
    va_list args;

    if (log_get_debug_level(instance) < LOG_DEBUG_DEBUG3) {
        return;
    }

    va_start(args, fstr);

    if (instance->custom && instance->custom->logging_routine) {
      instance->custom->logging_routine(instance, LOG_DEBUG_DEBUG3, fstr, args);
    } else if (instance->logfile) {
      vfprintf(instance->logfile, fstr, args);
      fflush(instance->logfile);
    } else {
      vprintf(fstr, args);
      fflush(stdout);
    }

    va_end(args);

}  /* log_debug3 */


/********************************************************************
* FUNCTION log_debug4
*
*   Generate LOG_DEBUG_DEBUG4 log trace output
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for printf
*
*********************************************************************/
void 
    log_debug4 (ncx_instance_t *instance, const char *fstr, ...)
{
    va_list args;

    if (log_get_debug_level(instance) < LOG_DEBUG_DEBUG4) {
        return;
    }

    va_start(args, fstr);

    if (instance->custom && instance->custom->logging_routine) {
      instance->custom->logging_routine(instance, LOG_DEBUG_DEBUG4, fstr, args);
    } else if (instance->logfile) {
      vfprintf(instance->logfile, fstr, args);
      fflush(instance->logfile);
    } else {
      vprintf(fstr, args);
      fflush(stdout);
    }

    va_end(args);

}  /* log_debug4 */


/********************************************************************
* FUNCTION log_noop
*
*  Do not generate any log message NO-OP
*  Used to set logfn_t to no-loggging option
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for printf
*
*********************************************************************/
void 
    log_noop (ncx_instance_t *instance, const char *fstr, ...)
{
    (void)instance;
    (void)fstr;
}  /* log_noop */


/********************************************************************
* FUNCTION log_set_debug_level
* 
* Set the global debug filter threshold level
* 
* INPUTS:
*   dlevel == desired debug level
*
*********************************************************************/
void
    log_set_debug_level (ncx_instance_t *instance, log_debug_t dlevel)
{
    if (dlevel > LOG_DEBUG_DEBUG4) {
        dlevel = LOG_DEBUG_DEBUG4;
    }

    if (dlevel != instance->debug_level) {
#ifdef LOG_DEBUG_TRACE
        log_write(instance,
                  "\n\nChanging log-level from '%s' to '%s'\n",
                  log_get_debug_level_string(instance, instance->debug_level),
                  log_get_debug_level_string(instance, dlevel));
#endif
        instance->debug_level = dlevel;
    }

}  /* log_set_debug_level */


/********************************************************************
* FUNCTION log_get_debug_level
* 
* Get the global debug filter threshold level
* 
* RETURNS:
*   the global debug level
*********************************************************************/
log_debug_t
    log_get_debug_level (ncx_instance_t *instance)
{
    return instance->debug_level;

}  /* log_get_debug_level */


/********************************************************************
* FUNCTION log_get_debug_level_enum
* 
* Get the corresponding debug enum for the specified string
* 
* INPUTS:
*   str == string value to convert
*
* RETURNS:
*   the corresponding enum for the specified debug level
*********************************************************************/
log_debug_t
    log_get_debug_level_enum (ncx_instance_t *instance, const char *str)
{
#ifdef DEBUG
    if (!str) {
        SET_ERROR(instance, ERR_INTERNAL_PTR);
        return LOG_DEBUG_NONE;
    }
#endif

    if (!xml_strcmp(instance, (const xmlChar *)str, LOG_DEBUG_STR_OFF)) {
        return LOG_DEBUG_OFF;
    } else if (!xml_strcmp(instance, (const xmlChar *)str, LOG_DEBUG_STR_ERROR)) {
        return LOG_DEBUG_ERROR;
    } else if (!xml_strcmp(instance, (const xmlChar *)str, LOG_DEBUG_STR_WARN)) {
        return LOG_DEBUG_WARN;
    } else if (!xml_strcmp(instance, (const xmlChar *)str, LOG_DEBUG_STR_INFO)) {
        return LOG_DEBUG_INFO;
    } else if (!xml_strcmp(instance, (const xmlChar *)str, LOG_DEBUG_STR_DEBUG)) {
        return LOG_DEBUG_DEBUG;
    } else if (!xml_strcmp(instance, (const xmlChar *)str, LOG_DEBUG_STR_DEBUG2)) {
        return LOG_DEBUG_DEBUG2;
    } else if (!xml_strcmp(instance, (const xmlChar *)str, LOG_DEBUG_STR_DEBUG3)) {
        return LOG_DEBUG_DEBUG3;
    } else if (!xml_strcmp(instance, (const xmlChar *)str, LOG_DEBUG_STR_DEBUG4)) {
        return LOG_DEBUG_DEBUG4;
    } else {
        return LOG_DEBUG_NONE;
    }

}  /* log_get_debug_level_enum */


/********************************************************************
* FUNCTION log_get_debug_level_string
* 
* Get the corresponding string for the debug enum
* 
* INPUTS:
*   level ==  the enum for the specified debug level
*
* RETURNS:
*   the string value for this enum

*********************************************************************/
const xmlChar *
    log_get_debug_level_string (ncx_instance_t *instance, log_debug_t level)
{
    switch (level) {
    case LOG_DEBUG_NONE:
    case LOG_DEBUG_OFF:
        return LOG_DEBUG_STR_OFF;
    case LOG_DEBUG_ERROR:
        return LOG_DEBUG_STR_ERROR;
    case LOG_DEBUG_WARN:
        return LOG_DEBUG_STR_WARN;
    case LOG_DEBUG_INFO:
        return LOG_DEBUG_STR_INFO;
    case LOG_DEBUG_DEBUG:
        return LOG_DEBUG_STR_DEBUG;
    case LOG_DEBUG_DEBUG2:
        return LOG_DEBUG_STR_DEBUG2;
    case LOG_DEBUG_DEBUG3:
        return LOG_DEBUG_STR_DEBUG3;
    case LOG_DEBUG_DEBUG4:
        return LOG_DEBUG_STR_DEBUG4;
    default:
        SET_ERROR(instance, ERR_INTERNAL_VAL);
        return LOG_DEBUG_STR_OFF;
    }
    /*NOTREACHED*/

}  /* log_get_debug_level_string */


/********************************************************************
* FUNCTION log_is_open
* 
* Check if the logfile is active
* 
* RETURNS:
*   TRUE if logfile open, FALSE otherwise
*********************************************************************/
boolean
    log_is_open (ncx_instance_t *instance)
{
    return (instance->logfile) ? TRUE : FALSE;

}  /* log_is_open */


/********************************************************************
* FUNCTION log_indent
* 
* Printf a newline, then the specified number of chars
*
* INPUTS:
*    indentcnt == number of indent chars, -1 == skip everything
*
*********************************************************************/
void
    log_indent (ncx_instance_t *instance, int32 indentcnt)
{
    int32  i;

    if (indentcnt >= 0) {
        log_write(instance, "\n");
        for (i=0; i<indentcnt; i++) {
            log_write(instance, " ");
        }
    }

} /* log_indent */


/********************************************************************
* FUNCTION log_stdout_indent
* 
* Printf a newline to stdout, then the specified number of chars
*
* INPUTS:
*    indentcnt == number of indent chars, -1 == skip everything
*
*********************************************************************/
void
    log_stdout_indent (ncx_instance_t *instance, int32 indentcnt)
{
    int32  i;

    if (indentcnt >= 0) {
        log_stdout(instance, "\n");
        for (i=0; i<indentcnt; i++) {
            log_stdout(instance, " ");
        }
    }

} /* log_stdout_indent */


/********************************************************************
* FUNCTION log_get_logfile
* 
* Get the open logfile for direct output
* Needed by libtecla to write command line history
*
* RETURNS:
*   pointer to open FILE if any
*   NULL if no open logfile
*********************************************************************/
FILE *
    log_get_logfile (ncx_instance_t *instance)
{
    return instance->logfile;

}  /* log_get_logfile */


/* END file log.c */
