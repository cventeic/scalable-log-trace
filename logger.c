#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <nanomsg/nn.h>

#include "context.h"

#include "logger.h"
#include "util.h"

int send_log_msg(
    struct log_context *g_log,
    const char *type_lvl,      // LOG_LVL_DEBUG, TRACE_LVL_FUNC, etc.
    uint64_t function_ptr,
    uint64_t file_line_number,
    void *pkt, uint64_t pkt_len
    )
{
  uint64_t usec = get_time();
  uint64_t process_id = getpid(); // Linux caches this for 2, 3, ... access

  struct nn_msghdr hdr;
  struct nn_iovec iov[7];

  int i = 0;
  iov[i].iov_base = type_lvl;
  iov[i].iov_len  = 8;
  i++;

  iov[i].iov_base = &(g_log->prog_hash);
  iov[i].iov_len  = sizeof(g_log->prog_hash);
  i++;

  iov[i].iov_base = &(process_id);
  iov[i].iov_len  = sizeof(process_id);
  i++;

  iov[i].iov_base = &(function_ptr);
  iov[i].iov_len  = sizeof(function_ptr);
  i++;

  iov[i].iov_base = &(file_line_number);
  iov[i].iov_len  = sizeof(file_line_number);
  i++;

  iov[i].iov_base = &usec;
  iov[i].iov_len  = sizeof(usec);
  i++;

  iov[i].iov_base = pkt;
  iov[i].iov_len  = pkt_len;
  i++;

  int entries = sizeof(iov)/sizeof(iov[0]);
  errno_assert(entries == i);

  memset(&hdr, 0, sizeof(hdr));
  hdr.msg_iov = iov;
  hdr.msg_iovlen = i;

  int bytes = nn_sendmsg(g_log->pub_fd, &hdr, g_log->pub_sendmsg_flags);

  g_log->msg_sent += 1;
  g_log->msg_bytes_sent += bytes;
  g_log->payload_bytes_sent += pkt_len;

  if(bytes <= 0) printf("log message not sent, bytes = %i\n", bytes);

  if(0){
    printf("\n%s SENT\n", __func__);
    printf_log_msg( type_lvl, usec, g_log->prog_hash, function_ptr, process_id, file_line_number, pkt, pkt_len);
  }

  return bytes;
}

/*
uint64_t send_program_info(struct log_context *g_log){
  const char *type_lvl  = LOG_LVL_EXEC_NAME;
  uint64_t msg_sub_type = 0x0;

  int bytes = send_log_msg(g_log, type_lvl, msg_sub_type, 0, g_log->prog_name, strlen(g_log->prog_name)+1);

  return bytes;
}
*/

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
 *
 */
// adds message to this process's tcp stream buffer
// tcp library sends tcp stream to trace server in large blocks when netdev is ready
// thus achiving message boundaries, guaranteed delivery and transfer efficiency
//
// trace server can live on CP and write trace to file (over local unix socket)
// trace server can live on AP or Win PC (over pci, ether, usb, etc)
// log messages can be intercepted, inspected and stored by wireshark / tcpdump
//
// messages include usec timestamp
// assumes clocks synchronized (usec level) via ieee1588 or AP/CP sync not important
//
//
void log_printf(const char* type_lvl, int file_line_number, const char *fmt, ...){
  // printf("\n%s ENTER\n", __func__);

  char *s;

  const uint64_t function_ptr = (const uint64_t)__builtin_return_address(0);
  struct log_context *ctx = get_log_context();

  // Render string
  va_list ap;
  va_start (ap, fmt);
  int len = vasprintf(&s, (fmt), ap);
  va_end(ap);

  len += 1; // account for terminating 0 in string

  send_log_msg(ctx, type_lvl, function_ptr, file_line_number, s, len);

  free(s);

  // printf("%s EXIT\n", __func__);

  return;
}

/* send a raw packet to the log and trace system.
 *
 * type_lvl - 8 char string for identfying and filtering messages
 * pkt_id   - 16 char string for identifying and filtering packets
 *            (specify pkt type and capture location for filtering)
 * pkt      - pointer to first byte in packet
 * pkt_len  - number of bytes in packet
 */
// lib0mq adds message to this processes tcp stream buffer
// tcp library sends tcp stream to trace server in large blocks when netdev is ready
// thus achiving message boundaries, guaranteed delivery and transfer efficiency
//
// trace server can live on CP and write trace to file (over local unix socket)
// trace server can live on AP or Win PC (over pci, ether, usb, etc)
// log messages can be intercepted, inspected and stored by wireshark / tcpdump
//
// messages include usec timestamp
// assumes clocks synchronized (usec level) via ieee1588 or AP/CP sync not important
//
//
void log_pkt(const char* type_lvl, const char* pkt_id, void *pkt, int pkt_len){
  // printf("\n%s ENTER\n", __func__);

  const uint64_t function_ptr = (const uint64_t)__builtin_return_address(0);
  struct log_context *ctx = get_log_context();

  // todo: include pkt_id in message
  send_log_msg(ctx, type_lvl, function_ptr, 0, pkt, pkt_len);

  // printf("%s EXIT\n", __func__);
  return;
}
