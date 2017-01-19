#include <stdarg.h>
#include <stdint.h>

enum {
	false	= 0,
	true	= 1
};

typedef _Bool			bool;

extern int asprintf(char **strp, const char *fmt, ...);

int is_writeable(int fd);

char * get_program_name();

uint64_t get_time();
void print_time(char *str);
void print_sockaddr_in(struct sockaddr_in sa);

char * addr_to_str(struct sockaddr_in addr);
void init_sockaddr(struct sockaddr_in *addr);

struct sockaddr_in get_open_port();
struct sockaddr_in ready_listening_socket(int socket);

struct sockaddr_in bind_socket(int socket);

void printf_log_msg(
  const char type_lvl[8],
  uint64_t usec,
  uint64_t prog_hash,
  uint64_t function_ptr,
  uint64_t process_id,
  uint64_t file_line_number,
  char     *buf,
  int      pkt_len
    );

void printf_service_description(
  uint64_t usec,
  uint64_t prog_hash,
  struct sockaddr_in addr
    );


#if defined __GNUC__ || defined __llvm__
#define lg_fast(x) __builtin_expect ((x), 1)
#define lg_slow(x) __builtin_expect ((x), 0)
#else
#define lg_fast(x) (x)
#define lg_slow(x) (x)
#endif


// #include <nanomsg/nn.h>
extern const char *nn_err_strerror (int errnum);
extern void nn_err_abort (void);

/*  Check the condition. If false prints out the errno. */
#define errno_assert(x) \
    do {\
        if (lg_slow (!(x))) {\
            fprintf (stderr, "errno_assert failure, errno_str: %s, errno: [%d], (%s:%d)\n", nn_err_strerror (errno),\
                (int) errno, __FILE__, __LINE__);\
            fflush (stderr);\
            nn_err_abort ();\
        }\
    } while (0)

extern int asprintf(char **strp, const char *fmt, ...);
extern int vasprintf(char **strp, const char *fmt, va_list ap);

extern int nn_setsockopt_checked (int s, int level, int option, const void *optval, int optvallen, int verbose);



