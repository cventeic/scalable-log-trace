#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>    /* for getopt */

#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>
#include <nanomsg/pipeline.h>

#include "logger.h"
#include "util.h"
#include "list.h"
#include "base64.h"

// todo log message has fixed size buffer

extern int asprintf(char **strp, const char *fmt, ...);

struct Msg_Hdr {
  char     type_lvl[8];
  uint64_t prog_hash;

  uint64_t function_ptr;
  uint64_t process_id;
  uint64_t file_line_number;

  uint64_t usec;
};


/* scatter a single buffer into a set of buffers
 *
 * return a vector to the remaining data in the input_buffer
 */
struct nn_iovec iov_scatter(struct nn_iovec *input_buffer, struct nn_msghdr *set_of_buffers) {
  struct nn_iovec *iov;
  int i;

  struct nn_iovec payload_iov = *input_buffer;

  /*  Copy the message content into the supplied gather array. */

  for (i = 0; i != set_of_buffers->msg_iovlen; ++i) {
    iov = &set_of_buffers->msg_iov [i];

    if (iov->iov_len > payload_iov.iov_len) {
      // edge case
      memcpy (iov->iov_base, payload_iov.iov_base, payload_iov.iov_len);
      payload_iov.iov_base += payload_iov.iov_len;
      payload_iov.iov_len  -= payload_iov.iov_len;
      break;
    }

    memcpy (iov->iov_base, payload_iov.iov_base, iov->iov_len);
    payload_iov.iov_base += iov->iov_len;
    payload_iov.iov_len  -= iov->iov_len;
  }

  return payload_iov;
}

struct nn_iovec get_log_msg_header(struct Msg_Hdr *lm, struct nn_iovec msg_iov){
  struct nn_msghdr hdr = {0};
  struct nn_iovec iov[6];

  int i = 0;
  iov[i].iov_base = &lm->type_lvl;
  iov[i].iov_len  = sizeof(lm->type_lvl);
  i++;

  iov[i].iov_base = &lm->prog_hash;
  iov[i].iov_len  = sizeof(lm->prog_hash);
  i++;

  iov[i].iov_base = &(lm->process_id);
  iov[i].iov_len  = sizeof(lm->process_id);
  i++;

  iov[i].iov_base = &(lm->function_ptr);
  iov[i].iov_len  = sizeof(lm->function_ptr);
  i++;

  iov[i].iov_base = &(lm->file_line_number);
  iov[i].iov_len  = sizeof(lm->file_line_number);
  i++;

  iov[i].iov_base = &lm->usec;
  iov[i].iov_len  = sizeof(lm->usec);
  i++;

  int entries = sizeof(iov)/sizeof(iov[0]);
  errno_assert(entries == i);

  hdr.msg_iov = iov;
  hdr.msg_iovlen = i;

  // Scatter the initial fixed size header parameters into
  // message header structure variables
  //
  struct nn_iovec payload_iov = iov_scatter(&msg_iov, &hdr);

  // Return ptr, length of the msg payload
  return payload_iov;
}

/* Dump the log message to a file in JSON format
 */
void write_log_msg_to_file(FILE *out_file, const struct Msg_Hdr *lm, const struct nn_iovec *payload_iov){

  if(!out_file) return;

  fprintf(out_file, "{");
  fprintf(out_file, "usec: %li", lm->usec);
  fprintf(out_file, ", eid: %lX", lm->prog_hash);
  fprintf(out_file, ", pid: %5li", lm->process_id);
  fprintf(out_file, ", fptr: %8lX", lm->function_ptr);
  fprintf(out_file, ", line: %4li", lm->file_line_number);
  fprintf(out_file, ", mask: %.8s", lm->type_lvl);

  if(lm->type_lvl[0] != 'P'){
    // char string payload

    fprintf(out_file, ", str: \"%s\"", (char *)payload_iov->iov_base);

  } else {
    // binary payload (packet)

    int   base64_len = ((payload_iov->iov_len+6)/3)*4; // 3 bytes into 4 bytes
    char* base64_ptr = malloc(base64_len);

    int rc = base64encode(payload_iov->iov_base, payload_iov->iov_len, base64_ptr, base64_len);
    errno_assert(rc == 0);

    fprintf(out_file, ", pkt: \"%s\"", base64_ptr);

    free(base64_ptr);
  }

  fprintf(out_file, "},\n");
}

int receive_log_msgs(int sock, FILE *out_file){
  struct Msg_Hdr lm ={0};
  struct nn_iovec msg_iov = {0};
  struct nn_iovec payload_iov = {0};
  int msg_count = 0;

  // tight loop here until no more messages available
  // don't go back and do a poll for every message

  while(1){
    // Receive message in a library allocated buffer
    msg_iov.iov_len  = nn_recv(sock, &msg_iov.iov_base, NN_MSG, NN_DONTWAIT);

    if(msg_iov.iov_len == -1 ) break;

    payload_iov = get_log_msg_header(&lm, msg_iov);

    write_log_msg_to_file(out_file, &lm, &payload_iov);

    nn_freemsg (msg_iov.iov_base);
    msg_count += 1;
  }

  return msg_count;
}

struct Svc_Desc {
  uint64_t timestamp_usec;
  uint64_t prog_hash;
  struct   sockaddr_in addr;
  int      eid;
  int      process_id;
  char     program_name[1024];
  struct list_head mylist;
  // struct list_head mylist_tmp;
};

/* Dump the service descriptor to a file in JSON format
 */
void write_svc_desc_to_file(FILE *out_file, const struct Svc_Desc *sd){
  if(!out_file) return;

  char addr_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(sd->addr.sin_addr), addr_str, sizeof(sd->addr));

  int port = ntohs(sd->addr.sin_port);

  fprintf(out_file, "{");
  fprintf(out_file, "usec: %li", sd->timestamp_usec);
  fprintf(out_file, ", eid: %lX", sd->prog_hash);
  fprintf(out_file, ", pid: %5i", sd->process_id);
  fprintf(out_file, ",  uri: %s:%i", addr_str, port);
  fprintf(out_file, ", prog: %s", sd->program_name);
  fprintf(out_file, "}\n");
}

struct Svc_Desc receive_service_notification(int sock, FILE *out_file){
  struct Svc_Desc sd ={0};

  struct nn_msghdr hdr;
  struct nn_iovec iov[5];

  int i = 0;

  iov[i].iov_base = &sd.prog_hash;
  iov[i].iov_len  = sizeof(sd.prog_hash);
  i++;

  iov[i].iov_base = &sd.timestamp_usec;
  iov[i].iov_len  = sizeof(sd.timestamp_usec);
  i++;

  iov[i].iov_base = &sd.process_id;
  iov[i].iov_len  = sizeof(sd.process_id);
  i++;

  iov[i].iov_base = &sd.addr;
  iov[i].iov_len  = sizeof(sd.addr);
  i++;

  iov[i].iov_base = &sd.program_name;
  iov[i].iov_len  = sizeof(sd.program_name);
  i++;

  int entries = sizeof(iov)/sizeof(iov[0]);
  errno_assert(entries == i);

  memset(&hdr, 0, sizeof(hdr));
  hdr.msg_iov = iov;
  hdr.msg_iovlen = i;

  int nbytes = nn_recvmsg(sock, &hdr, 0);

  errno_assert (nbytes >= 0);

  write_svc_desc_to_file(out_file, &sd);

  return sd;
}

// todo: not sure process_id is valid for kernel tasks???
// todo: need to make sure if client deactivates and reactives we will
// re-connect for both kernel and rtp tasks
int is_srvc_desc_in_list(struct list_head *srv_desc_list, const struct Svc_Desc *sd){
  int matches = 0;
  struct Svc_Desc *ptr = NULL;

  list_for_each_entry(ptr, srv_desc_list, mylist){
    matches = ((ptr->prog_hash == sd->prog_hash) && (ptr->process_id == sd->process_id)) ? (matches+1) : matches;
  }

  return (matches >= 1);
}

//
//

int main(int argc, char *argv[])
{
  int rc;

  struct {
    int srv_adv_sock;
    int sub_sock;

    FILE *out_file;
    char out_file_name[1024];

    int listening_port;
    int sub_recv_buf_size;
 
    int verbose;
    int debug;
  } ctx = {0, 0, NULL, {0},
    .listening_port = 50002,
    .sub_recv_buf_size  = (1<<20) * 20, // Default to 20 MByte
    //.sub_recv_buf_size  = (1<<20) * 1, // Default to 1 MByte
    //.sub_recv_buf_size    = 0,             // Don't confuse the vxsim
    .verbose = 0,
    .debug = 0
  };

  int opt, i;

  while ((opt = getopt(argc, argv, "vndshj:p:")) != -1) {
    switch (opt) {

      case 'v':
        ctx.verbose = 1;
        break;

      case 'd':
        ctx.debug = 1;
        break;

      case 'n':
        ctx.out_file = fopen("/dev/null", "w+");
        break;

      case 's':
        ctx.out_file = stdout;
        break;

      case 'j':
        strcpy(ctx.out_file_name, optarg);
        ctx.out_file = fopen(optarg, "w+");
      break;

      case 'p':
        ctx.listening_port = atoi(optarg);
        break;

      case 'h':
      default: /* '?' */
        fprintf(stderr, "Usage: %s [-h][-s][-d][-n][-j <file>][-p <port>], [-r <bytes>]\n"
                "-h     help\n"
                "-v     verbose \n"
                "-d     debug \n"
                "-j     output json to <file>\n"
                "-s     output json to stdout\n"
                "-r     receive buffer size in bytes (default %i bytes, 0 = use system defaults)\n"
                "-n     output json to /dev/null\n"
                "-p     listening port <port> (default %i)\n"
                "\n"
                "Note: Log/Trace/Pkt Capture messages will be recieved but not parsed or stored unless -j, -s or -n is specified\n",
                argv[0],
                ctx.sub_recv_buf_size,
                ctx.listening_port
                );
        exit(EXIT_FAILURE);
    }
  }

  fprintf(stderr,
    "out_file_name: %s\n"
    "listening_port: %i\n"
    "sub_recv_buf_size: %i\n"
    "verbose: %i\n"
    "debug: %i\n",
    ctx.out_file_name,
    ctx.listening_port,
    ctx.sub_recv_buf_size,
    ctx.verbose,
    ctx.debug
    );

  LIST_HEAD(srv_desc_list);

  // Create and bind socket to receive service advertisements
  // sent by components that can produce log/trace/pkt capture messages
  ctx.srv_adv_sock = nn_socket(AF_SP, NN_PULL);
  errno_assert(ctx.srv_adv_sock >= 0);

  char *listening_address;
  asprintf(&listening_address, "tcp://0.0.0.0:%i", ctx.listening_port);
  rc = nn_bind(ctx.srv_adv_sock, listening_address);
  errno_assert (rc >= 0);

  // Create socket we can use to subscribe to log/trace/pkt capture messages
  // from external components capable of producing those messages
  ctx.sub_sock = nn_socket (AF_SP, NN_SUB);
  errno_assert (ctx.sub_sock >= 0);

  if(ctx.sub_recv_buf_size > 0){
    rc = nn_setsockopt_checked(ctx.sub_sock, NN_SOL_SOCKET, NN_RCVBUF,
                               &ctx.sub_recv_buf_size,
                               sizeof(ctx.sub_recv_buf_size),
                               ctx.verbose);

    errno_assert (rc >= 0);
  }

  rc = nn_setsockopt (ctx.sub_sock, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
  errno_assert (rc >= 0);

  fprintf(stderr, "connected, enter message processing loop\n");

  struct nn_pollfd pfd [2];
  pfd [0].fd = ctx.sub_sock;
  pfd [0].events = NN_POLLIN;
  pfd [1].fd = ctx.srv_adv_sock;
  pfd [1].events = NN_POLLIN;

  int received_msg_count = 0;

  int seconds_between_stats = ctx.verbose ? 10 : 10;

  while(1){
    if(ctx.verbose){
      fprintf(stderr, "\n\n********* Enter Poll, %i messages so far, ", received_msg_count);
    }

    rc = nn_poll (pfd, sizeof(pfd)/sizeof(pfd[0]), seconds_between_stats * 1000);

    if(ctx.verbose && ctx.debug){
      for(i=0; i <(sizeof(pfd)/sizeof(pfd[0])); i++){
        fprintf(stderr, "pfd[%d].revents = %x; ", i, pfd[i].revents);
      }
    }

    if (rc == 0) {
      if(ctx.verbose){
        fprintf (stderr, "Timeout!");
      }
    } else if (rc == -1) {
      fprintf (stderr, "nn_poll Error! %s", nn_strerror(errno));
    } else {

      if (pfd [0].revents & NN_POLLIN) {
        if(ctx.verbose && (received_msg_count == 0)) fprintf(stderr, "**** Received first message\n");
        received_msg_count += receive_log_msgs(pfd[0].fd, ctx.out_file);
      }

      if (pfd [1].revents & NN_POLLIN) {
        struct Svc_Desc sd = receive_service_notification(pfd[1].fd, ctx.out_file);

        // subscribe to log stream from the actor
        //   identified in the recieved service descriptor
        //
        // don't duplicate subscribe if we are already subscribed
        //
        int already_in_list = is_srvc_desc_in_list(&srv_desc_list, &sd);

        if(!already_in_list){
          char *url = addr_to_str(sd.addr);
          int eid = nn_connect (ctx.sub_sock, url);

          if(eid >= 0){
            fprintf(stderr, "***** Pub/Sub Data Stream connected to log client/provider @ %s, eid = %i\n", url, eid);
            // subscription successfull

            // add program hash to list of connected log clients
            sd.eid = eid;
            INIT_LIST_HEAD(& sd.mylist);

            struct Svc_Desc *ptr_sd = malloc(sizeof(sd));
            *ptr_sd = sd;

            list_add(&(ptr_sd->mylist), &srv_desc_list);
          }
          free(url);
        }
      }

      if(ctx.verbose){
        // fprintf(stderr, "\n");
        // todo: how often???
        // fflush(NULL);
      }
    }
  }

  free(listening_address);

  nn_close (ctx.sub_sock);
}

