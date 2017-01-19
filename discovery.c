#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ifaddrs.h>

#include <nanomsg/nn.h>

#include "context.h"
#include "util.h"

int send_service_description(int sock_fd, struct log_context *g_log,
                             struct sockaddr_in service_address,
                             int process_id, char* program_name){
  // printf("\n%s ENTER\n", __func__);

  int bytes;
  uint64_t usec = get_time();

  struct nn_msghdr hdr;
  struct nn_iovec iov[5];

  int i = 0;

  iov[i].iov_base = &(g_log->prog_hash);
  iov[i].iov_len  = sizeof(g_log->prog_hash);
  i++;

  iov[i].iov_base = &usec;
  iov[i].iov_len  = sizeof(usec);
  i++;

  iov[i].iov_base = &process_id;
  iov[i].iov_len  = sizeof(process_id);
  i++;

  iov[i].iov_base = &service_address;
  iov[i].iov_len  = sizeof(service_address);
  i++;

  char name[1024] = {0};
  strncpy(name, program_name, sizeof(name));

  iov[i].iov_base = &name;
  iov[i].iov_len  = sizeof(name);
  i++;

  int entries = sizeof(iov)/sizeof(iov[0]);
  errno_assert(entries == i);

  memset(&hdr, 0, sizeof(hdr));
  hdr.msg_iov = iov;
  hdr.msg_iovlen = i;

  bytes = nn_sendmsg(sock_fd, &hdr, NN_DONTWAIT);
  // errno_assert (bytes >= 0);

  if(bytes <= 0) printf("discovery message not sent\n");

  printf("\nSENT service descriptor\n");
  printf_service_description(usec, g_log->prog_hash, service_address);

  // printf("%s EXIT\n", __func__);
  return bytes;
}

void send_service_descriptions(int sock_fd, int port, struct log_context *g_log){
  // printf("\n%s ENTER\n", __func__);
  struct ifaddrs *ifaddr, *ifa;
  int family, n;
  char * program_name;
  program_name = (char*) get_program_name();

  uint64_t process_id = getpid(); // Linux caches this for 2, 3, ... access

  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    exit(EXIT_FAILURE);
  }

  /* Walk through linked list, maintaining head pointer so we
   *               can free list later */

  for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
    if (ifa->ifa_addr == NULL) continue;

    family = ifa->ifa_addr->sa_family;

    if(0){
      /* Display interface name and family */
      printf("%-8s %s (%d)\n",
             ifa->ifa_name,
             (family == AF_PACKET) ? "AF_PACKET" :
             (family == AF_INET) ? "AF_INET" :
             (family == AF_INET6) ? "AF_INET6" : "???",
             family);
    }

    if (family == AF_INET) {

      // int size = (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);

      struct sockaddr_in addr = {0};
      memcpy(&addr, ifa->ifa_addr, sizeof(struct sockaddr_in));

      addr.sin_port = htons(port); // important: use the port number our log subscribe / publish service is bound to


      send_service_description(sock_fd, g_log, addr, process_id, program_name);
    }
  }
  // printf("%s EXIT\n", __func__);
}


