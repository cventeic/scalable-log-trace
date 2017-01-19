#include <netinet/in.h>

// global socket to communicate with trace server
struct log_context {
  char    *prog_name;
  uint64_t prog_hash;

  int pub_fd;
  struct sockaddr_in pub_addr;
  int pub_send_buf_size;
  int publish_context_ready;
  int pub_sendmsg_flags;

  int discovery_fd;
  int discovery_context_ready;
  int writeable_previous;

  int msg_sent;
  int msg_bytes_sent;
  int payload_bytes_sent;

  int verbose;
};

struct log_context * get_log_context();

