#ifndef _LOGS_H_
#define _LOGS_H_

#define ERRSTR stderr
#define ERROR_LOG(format, ...) fprintf(ERRSTR, "ERROR %s:%d: " format "\n", __FILE__, __LINE__ , ##__VA_ARGS__)

#ifdef DO_DEBUG_LOG
#define DEBUGSTR stderr
/* if you are using stdout, it is advisable to fflush after each write... */
#define DEBUG_LOG(format, ...) fprintf(DEBUGSTR, "%s:%d: " format "\n", __FILE__, __LINE__ , ##__VA_ARGS__)
#define DUMP_UPDATE( func, data) dumpUpdate( __FILE__, __LINE__, func, data->kp->len, data->kp, data->op,  NULL, NULL)
#else
#define DEBUG_LOG(format, ...)
#define DUMP_UPDATE( func, data)
#endif

#endif
