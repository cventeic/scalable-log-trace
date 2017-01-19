#include <nanomsg/nn.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#if defined _VX_CPU  // vxWorks
#include <sockLib.h>
#endif

#include "util.h"

char *__progname = NULL;

char * get_program_name(){

#ifdef _VX_CPU  // vxWorks

#ifdef _WRS_KERNEL

#include <vxWorks.h>

  if(__progname == NULL){
    TASK_ID task_id = taskIdSelf();

    //printf("\nget_programe_name\n");
    //printf("task_id = %i\n", task_id);
    //printf("task_name = %s\n", taskName(task_id));

    __progname = taskName(task_id);

    //TASK_DESC taskDesc = {0};
    //int rc = taskInfoGet ( task_id, &taskDesc);
    //printf("taskInfoGet rc = %i\n", rc);
    //printf("taskDesc.td_name = %s\n", taskDesc.td_name);
  }

#else
#include <rtpLibCommon.h>
  if(__progname == NULL){
    int rtp_id = getpid();

    RTP_DESC rtp_struct;

    rtpInfoGet(rtp_id, &rtp_struct);

    int len = strnlen_s(rtp_struct.pathName, sizeof(rtp_struct.pathName));

    __progname = malloc(len+1);

    strncpy(__progname, rtp_struct.pathName, len);
  }
#endif  // _WRS_KERNEL

#endif  // _VX_CPU

  // Failsafe
  if(__progname == NULL){ __progname = "PROGNAME_ERROR"; }

  return __progname;
}


uint64_t get_time(){
  //  Use POSIX clock_gettime function to get precise monotonic time.
  struct timespec tv;
  clock_gettime (CLOCK_MONOTONIC, &tv);
  uint64_t usec = (tv.tv_sec * (uint64_t) 1000000 + tv.tv_nsec / 1000);
  return usec;
}

void print_time(char *str){
  uint64_t usec = get_time();
  printf("time usec %li\n", usec);
}

void print_sockaddr_in(struct sockaddr_in sa){
  int len=20;
  char buffer[len];

  inet_ntop(AF_INET, &(sa.sin_addr), buffer, len);
  printf("print_sockaddr_in: address:%s\n",buffer);
}

char * addr_to_str(struct sockaddr_in addr){
    char addr_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), addr_str, sizeof(addr_str));

    char *url;
    asprintf(&url, "tcp://%s:%d", addr_str, ntohs(addr.sin_port));

    return url;
}

void init_sockaddr(struct sockaddr_in *addr){
  /*Inititalize source to zero*/
  memset(addr, 0, sizeof(addr[0]));       //source is an instance of sockaddr_in. Initialization to zero

  /*---- Configure settings of the source address struct, WHERE THE PACKET IS COMING FROM ----*/
  /* Address family = Internet */
  //addr->sin_family = inet_addr("127.0.0.1");
  addr->sin_family = AF_INET;

  /* Set IP address to localhost */
  addr->sin_addr.s_addr = INADDR_ANY;  //INADDR_ANY = 0.0.0.0
  /* Set port number, using htons function to use proper byte order */
  addr->sin_port = htons(0);
  /* Set all bits of the padding field to 0 */
  memset(addr->sin_zero, '\0', sizeof addr->sin_zero); //optional
}

/* Returns an address structure containing 0.0.0.0 and an open port number */
struct sockaddr_in get_open_port(){
  struct sockaddr_in source = {};  //two sockets declared as previously
  int sock = 0;

  /* creating the socket */
  //if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_TCP)) < 0)
  if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    printf("Failed to create socket\n");

  init_sockaddr(&source);

  /*bind socket to the source WHERE THE PACKET IS COMING FROM*/
  if (bind(sock, (struct sockaddr *) &source, sizeof(source)) < 0)
    printf("Failed to bind socket");

  struct sockaddr_in sin = {};
  init_sockaddr(&sin);

  socklen_t len = sizeof(sin);
  if (getsockname(sock, (struct sockaddr *)&sin, &len) == -1)
    perror("getsockname");

  close(sock);

  return sin;
}

struct sockaddr_in bind_socket(int socket){
  // printf("\n%s ENTER\n", __func__);

  int eid = -1;
  struct sockaddr_in bind_addr;

  errno_assert(socket >= 0);

  /* Loop until we successfully establish service on an open port */
  while(eid < 0) {
    bind_addr = get_open_port();
    char *url = addr_to_str(bind_addr);
    eid = nn_bind (socket, url);

    //printf("nn_bind eid = %i, error %s\n", eid, nn_strerror(errno));

    free(url);
  }
  errno_assert (eid >= 0);

  // printf("%s EXIT\n", __func__);
  return bind_addr;
}

void printf_service_description(
  uint64_t usec,
  uint64_t prog_hash,
  struct sockaddr_in addr
    )
{
  char addr_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(addr.sin_addr), addr_str, sizeof(addr));

  printf ("| "
          ", usec: %lX"
          ", prog_hash %lX"
          "\n",
          usec,
          prog_hash
         );

  printf ("| "
          "addr %s"
          ", port %i"
          "\n",
          addr_str,
          ntohs(addr.sin_port)
         );

  printf("\n");
}


void printf_log_msg(
  const char type_lvl[8],
  uint64_t usec,
  uint64_t prog_hash,
  uint64_t function_ptr,
  uint64_t process_id,
  uint64_t file_line_number,
  char     *buf,
  int      pkt_len
    )
{

  printf ("| "
          "type_lvl: %.8s"
          ", usec: %li"
          ", prog_hash %lX"
          "\n",
          type_lvl,
          usec,
          prog_hash
         );

  printf ("| "
          "func_ptr %lX"
          ", proc_id %li"
          ", line_num %li"
          "\n",
          function_ptr,
          process_id,
          file_line_number
         );

  printf ("| %s\n",buf);


}

int vasprintf (char **strp, const char *fmt, va_list ap)
{
  va_list args;
  int len;

  va_copy (args, ap);
  len = vsnprintf (NULL, 0, fmt, args);
  va_end (args);

  char *str = malloc (len + 1);
  if (str != NULL)
  {
    int len2;

    va_copy (args, ap);
    len2 = vsprintf (str, fmt, args);
    va_end (args);
  }
  else
  {
    len = -1;
    // str = (void *)(intptr_t)0x41414141; /* poison */
  }
  *strp = str;
  return len;
}

int asprintf (char **strp, const char *fmt, ...)
{
    va_list ap;
    int ret;

    va_start (ap, fmt);
    ret = vasprintf (strp, fmt, ap);
    va_end (ap);
    return ret;
}

/* Return true if socket is writeable.
 *
 * Some services like bus and pub (of pub/sub) are always writeable.
 * Services like pipeline are writeable when a tcp connection exists between source and sink.
 *
 * The nanomsg library will work in background to (re)establish a tcp connection for a connected nn socket.
 * Therefore the writeability of a nn socket can change over time.
 */
int is_writeable(int fd){
  struct nn_pollfd pfd [1];

  pfd [0].fd = fd;
  pfd [0].events = NN_POLLOUT;

  errno = 0;
  int rc = nn_poll (pfd, 1, 0);
  int writeable = (pfd[0].revents & NN_POLLOUT);

  if(0){
    printf("\nnn_poll rc = %i, errno = %s\n", rc, nn_err_strerror(errno));
    printf("pfd [0].revents = %x\n", pfd[0].revents);
    printf("NN_POLLIN = %x, NN_POLOUT = %x\n", NN_POLLIN, NN_POLLOUT);
    printf("writeable = %i\n", writeable);
  }

  return writeable;
}

/* nn_setsockopt but check value is set correctly
 *   and optionally print the before, target and final values
 */
int nn_setsockopt_checked (int s, int level, int option, const void *optval, int optvallen, int verbose){
  int rc = 0;

  if(verbose){
    printf("%s, level %i, option %i, value %i ", __func__, level, option, *(int*)optval);
  }

  int orig_opt_val;
  size_t orig_opt_val_len;

  rc = nn_getsockopt(s, level, option, &orig_opt_val, &orig_opt_val_len);
  if(verbose && (rc != 0)){
    printf("nn_getsockopt rc = %i\n", rc);
  }
  errno_assert(rc == 0);

  rc = nn_setsockopt(s, level, option, optval, optvallen);
  errno_assert(rc == 0);

  int actual_opt_val;
  size_t actual_opt_val_len;

  rc = nn_getsockopt(s, level, option, &actual_opt_val, &actual_opt_val_len);
  if(verbose && (rc != 0)){
    printf("(2) nn_getsockopt rc = %i\n", rc);
  }
  errno_assert(rc == 0);

//  errno_assert(optvallen == actual_opt_val_len);

//  errno_assert(0 == memcmp(optval, actual_opt_val, actual_opt_val_len));

  if(verbose){
    fprintf(stderr, "nn sockopt level %i, option %i ", level, option);

    if(actual_opt_val_len == sizeof(int)){
      int orig_val   = orig_opt_val;
      int target_val = *(int*)optval;
      int actual_val = actual_opt_val;
      fprintf(stderr, "....  orig value %i -> actual value %i  (target val  = %i)\n", orig_val, actual_val, target_val);
    } else {
      fprintf(stderr, " values not integer \n");
    }
  }

  // errno_assert(*(int*)optval == actual_opt_val);

  if (*(int*)optval != actual_opt_val)
  {
    fprintf(stderr, "nn sockopt level %i, option %i ", level, option);
    fprintf(stderr, "....  orig value %i -> actual value %i  (target val  = %i)\n", orig_opt_val, actual_opt_val, *(int*)optval);
    fprintf (stderr, "failure!!!, errno_str: %s, errno: [%d], (%s:%d)\n", nn_err_strerror (errno), (int) errno, __FILE__, __LINE__);
  }

  return rc;
}






