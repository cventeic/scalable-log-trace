#include "logger.h"

#include <stdio.h>
#include <unistd.h>    /* for getopt */
#include <stdlib.h>    /* for exit, malloc, free */

#include <time.h>


static unsigned long next = 1;

/* RAND_MAX assumed to be 32767 */
double my_rand(double max) {
  next = next * 1103515245 + 12345;
  int i = ((unsigned)(next/65536) % 32768);
  return (i * max / 32768.0);
}

void mysrand(unsigned int seed) {
  next = seed;
}


struct context {
  int log_msg_count;
  int trace_msg_count;
  int pkt_msg_count;
} ctx = {0};

int test_logging(int i, struct context *ctx)
{
  int c = 0;

  // Cycle through the types of log messages
  //
  switch (ctx->log_msg_count % 5){
    case 0: LG_DEBUG("ts %i", i); c++; break;
    case 1: LG_INFO("ts %i", i); c++; break;
    case 2: LG_WARN("ts %i", i); c++; break;
    case 3: LG_ERROR("ts %i", i); c++; break;
    case 4: LG_ASSERT((i % 2 == 0), "mod not 0"); if (i%2!=0) c++; break;
    default: fprintf(stderr, "test_logging error\n"); exit(-1); break; // coding error
  }

  ctx->log_msg_count += c;

  return c;
}

int test_trace(int i, struct context *ctx)
{
  int c = 0;

  int mod = ctx->trace_msg_count % 3;

  switch (mod){
    case 0:
      TRACE_FUNC_ENTER(); c++;
      TRACE_FUNC_EXIT(); c++;
    break;

    case 1: TRACE_BRANCH("special case"); c++; break;
    case 2: TRACE_INFO("i=%i", i); c++; break;

    default: fprintf(stderr, "test_trace error\n"); exit(-1); break; // coding error

  }

  ctx->trace_msg_count += c;

  return c;
}

int test_pkt_capture(int i, struct context *ctx, const char* sample_pkt)
{
  int c = 0;
  // Packet Capture
  // char ptr[64 - (i%16)];
  // int ii;
  // for(ii=i; ii<sizeof(ptr); ii++) ptr[ii] = ii;
  // PKT_CAPTURE("rnd_pkt", ptr, sizeof(ptr)); c++;

  PKT_CAPTURE("rnd_pkt", sample_pkt, 2048); c++;

  ctx->pkt_msg_count += c;

  return c;
}

#include "context.h" // not used by normal clients... Only here for performance data

int main(int argc, char *argv[])
{
  struct {
    double ts_logging_prob;
    double ts_trace_prob;
    double ts_packet_capture_prob;
    int verbose;
    int debug;
    int cap_msg_bytes;
    int cap_msg_count;
    int cap_time_slices;
    int post_time_slice_sleep;
    int service_discovery_port;
    int sample_pkt_size;
  } config = {
    .ts_logging_prob= 0.1,
    .ts_trace_prob= 0.4,
    .ts_packet_capture_prob= 0.5,
    .verbose= 0,
    .debug= 0,
    .cap_msg_bytes= -1,
    .cap_msg_count= -1,
    .cap_time_slices= 1000,
    .post_time_slice_sleep= -1,
    .service_discovery_port=50002,
    .sample_pkt_size= 4096
  };

  struct timespec tv;
  int opt;

#if !defined(_WRS_KERNEL) // VxWorks DKM don't support argc, argv

  while ((opt = getopt(argc, argv, "hvdp:c:b:s:t:m:")) != -1) {
    switch (opt) {

      case 'v':
        config.verbose = 1;
        break;

      case 'd':
        config.debug = 1;
        break;

      case 'm':
        config.sample_pkt_size = atoi(optarg);
        break;

      case 'p':
        fprintf(stderr, "todo: port specifcation not implemented yet\n");
        config.service_discovery_port = atoi(optarg);
        break;

      case 'c':
        config.cap_msg_count = atoi(optarg);
        break;

      case 'b':
        config.cap_msg_bytes = atoi(optarg);
        break;

      case 't':
        config.cap_time_slices = atoi(optarg);
        break;

      case 's':
        config.post_time_slice_sleep = atoi(optarg);
        break;

      case 'h':
      default: /* '?' */
        fprintf(stderr, "Usage: %s [-h][-v][-d][-m <bytes>][-t <count>][-s <seconds>][-b <bytes>][-c <count>]\n"
                "-h     help\n"
                "-v     verbose \n"
                "-d     debug output\n"
                "-m     packet capture simulated packets of size <bytes> (default 4096)\n"
           
                "-t     stop after <count> time slices (default 1000, -1 = don't limit)\n"
                "-s     <seconds> to sleep after each time slice\n"
                "        0 yield time slice (sleep(0))\n"
                "        default (-1) don't sleep\n"

                "-b     stop after <bytes> of log/trace/pkt messages are sent. (default (-1) don't limit)\n"
                "-c     stop after <count> log/trace/pkt messages are sent. (default (-1) don't limit)\n",
                argv[0]);
        exit(-1);
    }
  }
  // "-p     advertise log capablities of client to <port> (default 50002)\n",
#endif // !defined(_WRS_KERNEL)

  fprintf(stderr,
          "Configuration:\n"
          "ts_logging_prob: %f\n"
          "ts_trace_prob: %f\n"
          "ts_packet_capture_prob: %f\n"
          "verbose: %i\n"
          "debug: %i\n"
          "cap_msg_bytes: %i\n"
          "cap_msg_count: %i\n"
          "cap_time_slices: %i\n"
          "post_time_slice_sleep: %i\n"
          "service_discovery_port: %i\n"
          "sample_pkt_size: %i\n",
          config.ts_logging_prob,
          config.ts_trace_prob,
          config.ts_packet_capture_prob,
          config.verbose,
          config.debug,
          config.cap_msg_bytes,
          config.cap_msg_count,
          config.cap_time_slices,
          config.post_time_slice_sleep,
          config.service_discovery_port,
          config.sample_pkt_size
            );

  char *sample_pkt = malloc(config.sample_pkt_size);
  int ii;
  for(ii=0; ii<config.sample_pkt_size; ii++) sample_pkt[ii] = ii % 0xff;

  // Note: This kicks off the connections to log receiver
  struct log_context *log_context = get_log_context();

  sleep(2); // Let connections get established

  clock_gettime (CLOCK_MONOTONIC, &tv);
  uint64_t usec_start = (tv.tv_sec * (uint64_t) 1000000 + tv.tv_nsec / 1000);

  int time_slice_count = 0;
  int msg_count = 0;
  double r;

  int tst_logging, tst_trace, tst_pkt;
  int msg_byte_limit, msg_count_limit, time_slice_limit;

  struct {
    int logging;
    int trace;
    int pkt;
    int total_tests;
    int time_slices;
  } tst_count = {0};

  if(config.verbose) fprintf(stderr, "\n%s ENTER loop\n", __func__);

  while(1){
    time_slice_count++;

    if(config.debug) fprintf(stderr, "Timeslice %i .....\n", time_slice_count);

    tst_logging = (my_rand(1.0) <= config.ts_logging_prob);
    tst_trace   = (my_rand(1.0) <= config.ts_trace_prob);
    tst_pkt     = (my_rand(1.0) <= config.ts_packet_capture_prob);

    tst_count.time_slices += 1;
    if(tst_logging) tst_count.logging += 1;
    if(tst_trace)   tst_count.trace   += 1;
    if(tst_pkt)     tst_count.pkt     += 1;


    if(tst_logging) msg_count += test_logging(time_slice_count, &ctx);
    if(tst_trace)   msg_count += test_trace(time_slice_count, &ctx);
    if(tst_pkt)     msg_count += test_pkt_capture(time_slice_count, &ctx, sample_pkt);


    if(config.debug){
      if(tst_logging) fprintf(stderr, "test_logging\n");
      if(tst_trace)   fprintf(stderr, "test_trace\n");
      if(tst_pkt)     fprintf(stderr, "test_pkt\n");
    }

    msg_byte_limit = ((config.cap_msg_bytes > 0) && (log_context->msg_bytes_sent >= config.cap_msg_bytes));
    msg_count_limit = ((config.cap_msg_count > 0) && (msg_count >= config.cap_msg_count));
    time_slice_limit = ((config.cap_time_slices > 0) && (time_slice_count >= config.cap_time_slices));

    if(config.verbose){
      if(msg_byte_limit)   fprintf(stderr, "reached msg_byte_limit\n");
      if(msg_count_limit)  fprintf(stderr, "reached msg_count_limit\n");
      if(time_slice_limit) fprintf(stderr, "reached time_slice_limit\n");
    }

    if(msg_byte_limit) break;
    if(msg_count_limit) break;
    if(time_slice_limit) break;

    if(config.post_time_slice_sleep >= 0) sleep(config.post_time_slice_sleep);
  }

  clock_gettime (CLOCK_MONOTONIC, &tv);
  uint64_t usec_end = (tv.tv_sec * (uint64_t) 1000000 + tv.tv_nsec / 1000);
  uint64_t usec     = usec_end - usec_start;
  usec = (usec > 0) ? usec : -1;  // prevent divide by 0 error
  double    sec     = usec / 1000000.0;


  if(config.verbose) fprintf(stderr, "\n%s EXIT loop\n", __func__);

  fprintf(stderr, "\nCounts\n");
  fprintf(stderr, "    %s = %li\n", "microseconds",       usec);

  int total_msg_count = ctx.log_msg_count + ctx.trace_msg_count + ctx.pkt_msg_count;
  fprintf(stderr, "    %s msgs sent = %d\n", "  log",   ctx.log_msg_count);
  fprintf(stderr, "    %s msgs sent = %d\n", "trace", ctx.trace_msg_count);
  fprintf(stderr, "    %s msgs sent = %d\n", "  pkt",   ctx.pkt_msg_count);
  fprintf(stderr, "    %s msgs sent = %d\n", "total", total_msg_count);

  fprintf(stderr, "    %s msgs sent               = %d\n", "",   log_context->msg_sent);
  fprintf(stderr, "    %s msgs bytes sent         = %d\n", "header+payload",   log_context->msg_bytes_sent);
  fprintf(stderr, "    %s msgs payload bytes sent = %d\n", "       payload",   log_context->payload_bytes_sent);

  fprintf(stderr, "    %s tests = %d\n", "  log", tst_count.logging);
  fprintf(stderr, "    %s tests = %d\n", "trace", tst_count.trace);
  fprintf(stderr, "    %s tests = %d\n", "  pkt", tst_count.pkt);
  fprintf(stderr, "    %s = %d\n", "timeslices", tst_count.time_slices);

  fprintf(stderr, "\nMessage Frequency\n");
  fprintf(stderr, "    %s msgs per second = %f\n", "  log",   ctx.log_msg_count / sec);
  fprintf(stderr, "    %s msgs per second = %f\n", "trace", ctx.trace_msg_count / sec);
  fprintf(stderr, "    %s msgs per second = %f\n", "  pkt",   ctx.pkt_msg_count / sec);
  fprintf(stderr, "    %s msgs per second = %f\n", "total",   (ctx.log_msg_count + ctx.trace_msg_count + ctx.pkt_msg_count)/sec);
  fprintf(stderr, "    %s usec per msg    = %f\n", "total",   1000000.0 * sec/(ctx.log_msg_count + ctx.trace_msg_count + ctx.pkt_msg_count));

  tst_count.total_tests = tst_count.logging + tst_count.trace + tst_count.pkt;
  fprintf(stderr, "\nTest Frequency\n");
  fprintf(stderr, "    %s tests per timeslice = %f\n", "  log", (1.0 * tst_count.logging) / tst_count.time_slices);
  fprintf(stderr, "    %s tests per timeslice = %f\n", "trace", (1.0 * tst_count.trace) / tst_count.time_slices);
  fprintf(stderr, "    %s tests per timeslice = %f\n", "  pkt", (1.0 * tst_count.pkt) / tst_count.time_slices);
  fprintf(stderr, "    %s tests per timeslice = %f\n", "total", (1.0 * tst_count.total_tests) / tst_count.time_slices);

  fprintf(stderr, "\nData Frequency\n");
  fprintf(stderr, "    %s mbits per second = %li\n", "header+payload", log_context->msg_bytes_sent * 8 / usec);
  fprintf(stderr, "    %s mbits per second = %li\n", "       payload", log_context->payload_bytes_sent * 8 / usec);

  double payload_percent_of_msg= (log_context->payload_bytes_sent * 1.0)/log_context->msg_bytes_sent;
  double header_percent_of_msg = ((log_context->msg_bytes_sent - log_context->payload_bytes_sent) * 1.0)/log_context->msg_bytes_sent;
  fprintf(stderr, "\nRatios\n");
  fprintf(stderr, "    %s to %s = %f\n", "    payload", "header+payload", payload_percent_of_msg);
  fprintf(stderr, "    %s to %s = %f\n", "     header", "header+payload", header_percent_of_msg);
  fprintf(stderr, "    %s to %s = %f\n", "   log msgs", "    total msgs", 1.0 * ctx.log_msg_count / total_msg_count);
  fprintf(stderr, "    %s to %s = %f\n", " trace msgs", "    total msgs", 1.0 * ctx.trace_msg_count / total_msg_count);
  fprintf(stderr, "    %s to %s = %f\n", "   pkt msgs", "    total msgs", 1.0 * ctx.pkt_msg_count / total_msg_count);

  free(sample_pkt);
}
