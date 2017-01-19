
/*
 * logger.h
 *
 * Logging is error reporting.
 *   Logging needs to be flexible.  
 *
 * Tracing is following your program flow and data.
 *   Tracing generates huge amounts of data in very short time.  
 *   Tracing has three main goals: Correctness, performance and performance.
 */


#ifndef _SCALEABLE_LOG_TRACE_H_
#define _SCALEABLE_LOG_TRACE_H_

/* Log / Trace / Capture Filter Levels
 *
 * Levels are specified as 8 character strings.
 * 
 * Filters are defined to match characters from left to right.
 *
 * For example:
 *   ""   = Pass all Messages
 *   "L"  = Pass log messages
 *   "LE" = Pass log error messages
 *   "T"  = Pass trace messages
 *   "P"  = Pass captured packets 
 *
 * Character n+1 should subdivide messages selected by character n
 * for effective filtering of log message types.
 */

#define TRACE_LVL_GENERIC     "TG      "
#define TRACE_LVL_INFO        "TI      "
#define TRACE_LVL_FUNC_ENTER  "TF+     "
#define TRACE_LVL_FUNC_EXIT   "TF-     "
#define TRACE_LVL_BRANCH      "TB      "
#define TRACE_LVL_EXCEP       "TE      "
#define TRACE_LVL_TIMER_START "TT+     "
#define TRACE_LVL_TIMER_STOP  "TT-     "

#define LOG_LVL_DEBUG         "LD      "
#define LOG_LVL_DEBUG_ASSERT  "LDA     "
#define LOG_LVL_INFO          "LI      "
#define LOG_LVL_WARN          "LW      "
#define LOG_LVL_ERROR         "LE      "

#define PKT_LVL_GENERIC       "PG      "

#define LOG_LVL_EXEC_NAME     "EN      "


/* printf and send a log or trace mesage to the log and trace system.
 *
 * type_lvl         - 8 char string for identfying and filtering messages
 * file_line_number - line number in source file
 * fmt, ...         - printf compliant format and parameters specification
 *
 * notes: 
 * - Execution time is directly related to complexity of formatting.
 *   Prefer simple formatting to decrease execution time.
 * - Don't use formatting for single strings. (Replace: fmt="%s" with
 *   fmt="string")
 */
void log_printf(const char* type_lvl, int file_line_number, const char *fmt, ...);

/* send a raw packet to the log and trace system.
 *
 * type_lvl - 8 char string for identfying and filtering messages
 * pkt_id   - 16 char string for identifying and filtering packets
 *            (specify pkt type and capture location for filtering)
 * pkt      - pointer to first byte in packet
 * pkt_len  - number of bytes in packet
 */ 
void log_pkt(const char* type_lvl, const char* pkt_id, void *pkt, int pkt_len);


/******* Logging ************/

#define LG_DEBUG(fmt, ...) \
    log_printf(LOG_LVL_DEBUG, __LINE__, fmt, ##__VA_ARGS__);

#define LG_INFO(fmt, ...) \
    log_printf(LOG_LVL_INFO, __LINE__, fmt, ##__VA_ARGS__);

#define LG_WARN(fmt, ...) \
    log_printf(LOG_LVL_WARN, __LINE__, fmt, ##__VA_ARGS__);

#define LG_ERROR(fmt, ...) \
    log_printf(LOG_LVL_ERROR, __LINE__, fmt, ##__VA_ARGS__);

#define LG_ASSERT(a_condition, a_message_Ptr) \
    if(!(a_condition)){ log_printf(LOG_LVL_DEBUG_ASSERT, __LINE__, "%s %s", #a_condition, a_message_Ptr);}


/******* Tracing ************/

#define TRACE_FUNC_ENTER() \
    log_printf(TRACE_LVL_FUNC_ENTER, __LINE__, __FUNCTION__);

#define TRACE_FUNC_EXIT() \
    log_printf(TRACE_LVL_FUNC_EXIT, __LINE__, __FUNCTION__);

#define TRACE_BRANCH(a_msg)\
    log_printf(TRACE_LVL_BRANCH, __LINE__, a_msg);

#define TRACE_INFO(fmt, ...)\
    log_printf(TRACE_LVL_INFO, __LINE__, fmt, ##__VA_ARGS__);


/******* Packet Capture ************/

#define PKT_CAPTURE(pkt_id, pkt_ptr, pkt_len)\
    log_pkt(PKT_LVL_GENERIC, pkt_id, (void*) pkt_ptr, pkt_len);
    
#endif /* _SCALEABLE_LOG_TRACE_H_ */
