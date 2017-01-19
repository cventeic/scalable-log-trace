#include <stdio.h>
#include <string.h>

#include "logger.h"
#include "context.h"
#include "discovery.h"
#include "util.h"

#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>
#include <nanomsg/pipeline.h>


#ifdef _VX_CPU  // vxWorks
#include <rtpLib.h>
#include <sockLib.h>
#endif

#include "fnv_hash.h"

#define DISCOVERY_SOCKET_ADDRESS "tcp://127.0.0.1:50002" // todo

struct log_context g_log = {
  .prog_name = "",
  .prog_hash = 0,

  .pub_fd = -1,
  .pub_addr = {0},
  .pub_sendmsg_flags = NN_DONTWAIT,
  //.pub_sendmsg_flags = 0,
  //.pub_send_buf_size  = 0,        // Don't modify (for vxsim) 
  .pub_send_buf_size  = (1<<20) * 20, // Default to 20 MByte
  //.pub_send_buf_size  = (1<<20) * 8, // Default to 8 MByte
  //.pub_send_buf_size  = (1<<20) * 1, // Default to 1 MByte
  .publish_context_ready = 0,

  .discovery_fd = -1,
  .discovery_context_ready = 0,
  .writeable_previous = 0,

  .verbose = 0,

  .msg_sent = 0,
  .msg_bytes_sent = 0,
  .payload_bytes_sent = 0
};


struct log_context * ready_static_info(struct log_context *g_log){
  g_log->prog_name = get_program_name();

  g_log->prog_hash = fnv_64a_str(g_log->prog_name, FNV1A_64_INIT);

  if(g_log->verbose) printf("\n program_name: %s, hash: %lx\n", g_log->prog_name, g_log->prog_hash);

  // g_log->process_id = getpid(); // Linux caches this for 2, 3, ... access

  return g_log;
}

/* Do a bunch of stuff once to establish logging context for this actor.
 * Store the information in global struct log_context data structure.
 */
void ready_publish_context(){
  int rc;

  /* Ready Socket we publish log messages on */

  g_log.pub_fd   = nn_socket (AF_SP, NN_PUB);

  if(g_log.pub_send_buf_size > 0){
    nn_setsockopt_checked(g_log.pub_fd, NN_SOL_SOCKET, NN_SNDBUF,
                          &g_log.pub_send_buf_size,
                          sizeof(g_log.pub_send_buf_size),
                          g_log.verbose);
  }

  g_log.pub_addr = bind_socket (g_log.pub_fd); // Bind to address:port

  ready_static_info(&g_log);

  g_log.publish_context_ready = 1;
}


/* Ready Service Discovery Socket
 */
void ready_discovery_context(){
  int rc;

  g_log.discovery_fd = nn_socket(AF_SP, NN_PUSH);
  rc = nn_connect(g_log.discovery_fd, DISCOVERY_SOCKET_ADDRESS);
  errno_assert(rc>= 0);

  g_log.discovery_context_ready = 1;
}



/* Return pointer to log context for this actor.
 * Make the log context ready prior to returning the pointer (if needed)
 */
struct log_context* get_log_context(){

  if(!g_log.publish_context_ready){
    ready_publish_context();
  }

  if(!g_log.discovery_context_ready){
    ready_discovery_context();
  }

  if(g_log.publish_context_ready){

    /* We send a hash code in log messages to identify the originating program.
     * This sends a message with the full program name.
     * So that the full program name can be matched to the hash code in the log.
     */
    // send_program_info(&g_log);
  }

  // todo: support re-establishment after log_to_file crashes
  if(
      g_log.discovery_context_ready &&
      !g_log.writeable_previous          // This limits the check to once
     ){
    /* Advertise this log capture service on service discovery socket.
     * The advertisement triggers the log receiver to subscribe to log messages from this actor.
     *
     * Send Advertisement when:
     * - pipeline socket transitions from non-writeble to writeable.
     */

    int writeable = is_writeable(g_log.discovery_fd);

    if(
        (!g_log.writeable_previous && writeable)
      )
    {
      send_service_descriptions(g_log.discovery_fd, ntohs(g_log.pub_addr.sin_port), &g_log);
    }

     g_log.writeable_previous = writeable;
  }

  return &g_log;
}

/*
void log_close(){
	
  struct log_context *log = get_log_context();

  nn_shutdown(log->pub_fd, 0);

  printf("log close exit\n");
}
*/

