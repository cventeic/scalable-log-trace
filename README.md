# scalable-log-trace
Extreme throughput, distributed and scalable log and trace library and capture software. 

# Features
- Tested on Linux and VX Works
- Extremely high throughput
- Supports
 - Logging (High formatting, Low frequency)
 - Tracing (Low formatting, High frequency)
 - Packet Capture (No formatting, High Frequency)
- Distributed
  - Log generating components can be on the same or different machines as long as an IP connection exists between the machines and the log receiver.
  - Log receiving is cpu intensive and can (optionally) be performed on an independent machine to avoid impacting component performance.
  - Each component uses it's own IP connection to the log receiver thus buffer overruns or high activity in one component does not impact the performance or log consistency of other components.


# Performance

The following results were obtained by executing:the following on the same i5 laptop running Ubuntu.
  ./log_test_client -t 10000 
  ./log_to_file -j log.json

Key results:    
  data rate:    2 Gbits / sec    
  message rate: 143K trace messages / sec

```
Counts
    microseconds = 42083
      log msgs sent = 828
    trace msgs sent = 6053
      pkt msgs sent = 5026
    total msgs sent = 11907
     msgs sent               = 11907
    header+payload msgs bytes sent         = 10932096
           payload msgs payload bytes sent = 10360560
      log tests = 997
    trace tests = 4035
      pkt tests = 5026
    timeslices = 10000

Message Frequency
      log msgs per second = 19675.403370
    trace msgs per second = 143834.802652
      pkt msgs per second = 119430.648956
    total msgs per second = 282940.854977
    total usec per msg    = 3.534308

Test Frequency
      log tests per timeslice = 0.099700
    trace tests per timeslice = 0.403500
      pkt tests per timeslice = 0.502600
    total tests per timeslice = 1.005800

Data Frequency
    header+payload mbits per second = 2078
           payload mbits per second = 1969

Ratios
        payload to header+payload = 0.947719
         header to header+payload = 0.052281
       log msgs to     total msgs = 0.069539
     trace msgs to     total msgs = 0.508356
       pkt msgs to     total msgs = 0.422105
```

Note:
- Code has not been optimized for performance yet... 
- Better performance is expected if log_to_file is executed on an independent machine.


# Example
```

#include <logging.h>

int test_logging(int i, struct context *ctx) {
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

int test_trace(int i, struct context *ctx) {
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

int test_pkt_capture(int i, struct context *ctx, const char* sample_pkt) {
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
```

# FAQ

## Q) How does a component use logging?

  1. Include <logging.h> in component c files

  2. Link component against liblog

  3. Use macros or functions defined in logger.h

  4. Start "log_to_file -o /tmp/log_capture.json" on the machine where you wish to capture the logs.
    log_to_file receives the log/trace messages from various components and dumps output to log_capture.json

  5. Access and view logs in file "log_capture.json"

## Q) How do I specify Log / Trace Capture Filter Levels

Levels are specified as 8 character strings.

Filters are defined to match characters from left to right.

For example:
  ""   = Pass all Messages
  "L"  = Pass log messages
  "LE" = Pass log error messages
  "T"  = Pass trace messages
  "P"  = Pass captured packets 

Character n+1 should subdivide messages selected by character n
for effective filtering of log message types.

```
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
```

## Q) How do I build and test on Linux.

### Build libpoll
  mkdir  poll/build
  cd     poll/build
  cmake ..
  make

### Build libnanomsg + nanomsg tests
  mkdir  nanomsg/build
  cd     nanomsg/build
  cmake ..
  make

### Build liblog, log_to_file, log_test_client
  mkdir  logger/build
  cd     logger/build

  if you don't have a standalone native version of nanomsg on your system...
  vim ../CMakeLists.txt and change "set(NATIVE_NANOMSG 1)" to "set(NATIVE_NANOMSG 0)"

  cmake ..
  make

### Start log_to_file in one terminal
  ./log_to_file -j log.json

### Start log_test_client in second terminal
  ./log_test_client -t 10000

## Q) How do I test logging?

log_test_client is the test client that co-exists with the logging library

The test client will simulate a number of time slices (specified by -t).

During each time slice log, trace or pkt capture macros will be executed
based on a pre-configured probability of each type of macro being executed
during a time slice.
 
After all time slices have been executed performance data will be reported
on stdout.
               

Example:
log_test_client -t 1000
Will simulate 1000 timeslices and report performance data.

Multiple instances of the test client can be executed in parallel to test concurrent performance.
The test client can be started before and during execution of the log_to_file to verify the auto discovery function works in all cases.

./log_test_client -h
Usage: ./log_test_client [-h][-v][-d][-m <bytes>][-t <count>][-s <seconds>][-b
<bytes>][-c <count>]
-h     help
-v     verbose
-d     debug output
-m     packet capture simulated packets of size <bytes> (default 4096)
-t     stop after <count> time slices (default (-1) don't limit)
-s     <seconds> to sleep after each time slice
        0 yield time slice (sleep(0))
        default (-1) don't sleep
-b     stop after <bytes> of log/trace/pkt messages are sent. (default (-1) don't limit)
-c     stop after <count> log/trace/pkt messages are sent. (default (-1) don't limit)


## Q) What header variables are used in the log/trace/pkt capture messages?

### usec - microsecond timestamp
"usec" is sampled at the time the log/trace/pkt macro is executed.

### eid  - executable name id.
"eid" is a hash code uniquely identifying the name of executable generating the log/trace/pkt message.  We don't want to send a full char string identifying the executable in every log message as this would waste space.  Instead we generate a hash code from the full char string one time and send that hash code as "eid" in all subsequent log/trace/pkt messages from the executable. The full executable name char string is sent with the "eid" hash code in an early message so the receiver can map the "eid" hash code back to the actual name of the executable.

### pid  - process id / task id / thread id at the time of execution.
A single executable can produce multiple processes, threads, or tasks either concurrently or sequentially over time as the executable starts and stops. "pid" is used to identify different instances of the executable.

### fptr - function pointer
Used to identify exactly where in code the log/trace/pkt capture macro was called.

Use "addr2line" to obtain function name, source file name and source file line.

For example, if eid maps to "log_test_client" and fptr = 40181f then...
 "addr2line -e log_test_client --functions 40181f" >>  test_func ../log_test_client.c:12

### line - Line in the .h or .c file where the macro was called.

### mask - Identifies the Log/Trace/Pkt Capture message type.
See defines in logger.h

These masks are structured for message filtering.
Each character to the right represents a subtype of the character to the left.

### str  - String generated at runtime from runtime information when the macro is called.
Note: "str" is only for dynamic runtime information.

For the sake of log size and runtime performance. Don't duplicate header info in "str".
Don't duplicate time or location (file, function, or line) in "str"
Don't duplicate "mask" information like LOG, TRACE, DEBUG, ERROR, etc. in "str".

### pkt  - Packet contents
Packets are stored in Base64 encoding and can be converted back to raw binary by reversing the Base64 encoding using standard libraries.

## Q) Why store a log file in ascii / json?

1) Had to start somewhere and didn't have time for complexity in first pass.
2) Json is directly human readable and easy to parse
3) Most of the logging macros to date are string / printf oriented so ascii is already the primary content format.
4) Packet data requires 4 bytes ascii for every 3 bytes of binary... not a huge delta.
5) Encoding the binary parts of the log message in ascii takes some cycles in log_to_file.
   Therefore it's best to run log_to_file on an external machine to avoid slowing down the components generating the log and trace data.
6) Json doesn't prevent using a binary format like msgpack, etc, for disk storage in future if testing shows significant log compression can be achieved.

## Q) How do I make the output of log_to_file proper json?

Add a { at the very beginning and a } at the very end of the file.
The output of log_demangle will be proper json.


## Q) How do you limit the types of log, trace and pkt captures that are stored to file?

Filter masks have been defined in the logging.h header file.
Nanomsg has some built in support for filtering using these types of character masks.
Full implementation can be created in later phased development.


## Q) How do you prevent loss of log / trace / pkt capture messages from a component?

Component should not generate log data at a higher rate than log_to_file can process the data.
Component's tcp buffer for the log data connection should be large enough to cover short term execution mismatches between the component and log_to_file and to allow large transfers when transfers occur.
Component's tcp connection configuration should be optimized for high data rates and large data blocks.

Each component maintains it's own TCP buffer for the connection, within the component, that can fill with message data.
TCP will only send the data when the log_to_file signals log_to_file is ready to receive the data.
The log_to_file must consumer the data faster than the component is adding data to the connection buffer or the buffer will eventually become full.
When the buffer is full the log/trace/pkt macros can't add full messages to the buffer.

Currently the loglib is configured for non-blocking mode.
In that mode loglib will discard any log/trace/pkt messages that can't be added to the TCP buffer.

logLib won't add partial log/trace/pkt messages to the buffer. Either the whole message is added or it's discarded.

Potentially we could support a blocking mode for the log/trace/pkt macros.
Potentially we could add indications of packet loss to the log.
These will be investigated for future releases.


## Q) What is the initialization sequence within components that use liblog?

The first call to a log, trace or pkt capture macro triggers the initialization of the logging system on the component.

Before log messages can be transferred from the component to log_to_file the following must occur...

1) Component establishes a random IP port on which the component will listen to subscription requests from the log_to_file.
2) Component determine the IP addresses the component is listening on (127.0.0.1, 192.168.100.5, etc.) for use in the "Service Advertisement" message.
3) Component establish a connection to a well known "Service Discovery" port exposed by the log_to_file.
4) Component sends a "Service Advertisement" message to log_to_file once connection in step 3 is created.
5) log_to_file establishes a connection to the random IP port created in step one and completes a "Subscribe" transaction with the component.

QQ) How much of this occurs when the first log / trace / pkt macro is called?

Only steps 1 and 2.
Steps 3, 4, 5 are handled in a background thread managed by nanomsg.
In other words you won't see an excessive delay when the first macro is called.

QQ) What if log_to_file hasn't started yet or has crashed when you attempt to establish the connection in step 3?

nanomsg periodically attempts to establish or (re) establish the connection in step 3 in the background.
When nanomsg succeeds in establishing the connection in step 3, then step 4 and step 5 happen to fully connect the component to the log_to_file.

The benefit here is fault tolerance and no dependency on execution order.

QQ) Why go to the trouble of creating two connections with a component.. one connection for identifying the client component instance (service discovery) and the second connection for the actual log message transfer?

1) The second connection is a faster type (one way pub/sub) to make the log data transfer faster.
2) loglib (components) should know where to contact log_to_file. log_to_file should not know when/where/how to contact a changing list of components.
3) In the future we need capability to filter by message types.  The publish / subscribe model in second connection will support message filtering.
4) In the future we want to be able to run the log_to_file on a remote PC.  In this case the service discovery connection can be to fixed local host address but the important data transfer connection is from the external PC to the components generating the log / trace streams. 

## Q) How many cpu instructions must be expended for log, trace and packet captures?

No optimization phase has been completed yet but is planned.
No solid numbers are available yet but testing is planned.

Preliminary testing has identified some important considerations.

- Large Packets require fewer CPU instructions per bit than trace and log.
Each trace/log/pkt message has a header and a payload.
Packet capture messages 1) transfer more bits of payload per bit of header and 2) use memcpy, not sprintf to format the payload.
Gigabit per second rates are realistic for packet capture but not for small trace or complicated log messages.

- Trace calls require fewer CPU instructions per bit than log calls.
Trace macros are designed for speed.
Trace is still Ascii based but complicated printf formatting is not used.
Trace is more like strcat than printf.

- Logging is designed for flexibility.
Logging macros allow complicated printf style formatting.
Research indicates the number of CPU instructions is proportional to the complexity of the formatting string, the type of formatting operations used and the efficiency of the printf code.

## Q) How many cpu instructions are expended by the log_to_file process?
     Can we improve performance by running the log_to_file on an external PC?

Performance data is planned but not yet available.

Preliminary testing and logic indicates the log_to_file impact is significantly more than per component impact of log/trace/pkt capture library execution.

if component requires
  X instructions to form and send log/trace/pkt message

then log_to_file requires
  X instructions to receive and parse log/trace/pkt message in log_to_file
  Y instructions to form message into json for storage
  Z instructions to store message to hard drive.

In addition, the log_to_file process represents a single point of failure or resource collapse in the system because many components are generating log/trace/pkt capture message that a single log_to_file process must service at a high rate.

The log/trace/pkt capture macros use IP protocol to communicate with the log_to_file.
The log_to_file can execute on either the same machine as the components generating log and trace data or on an external PC running Linux.
We can run the log_to_file on an external PC if direct IP communication is possible between the component machine(s) and an external logging PC.

Log/Trace/Pkt capture performance is likely to increase 2x to 5x times when the log_to_file can be offloaded to an external PC.

## Q) What are the benefits of each component having it's own nanomsg TCP connection for log/trace/pkt capture?

With TCP the log/trace/pkt data is queued and buffered within the component that generated the data and is only sent when the consumer is ready to receive the data.

This has some important implications.
1) Sending buffered log/trace/packet data is handled fairly and optimally as part of process scheduling and time slicing.
The log/trace/pkt data is sent when the originating process runs and is scheduled at the priority of the originating process.
The log/trace/pkt data does not get processed in or take the priority of some single global OS process.

2) TCP is optimized to reduce the instructions per byte transferred.
Other IPC mechanisms are optimized to reduce message latency and jitter.
Log/Trace/Pkt messages are timestamped at the source and are not impacted by delivery latency and jitter.
Reducing instructions per byte transferred is most important consideration for log/trace/pkt capture data.

Log/Trace/Pkt data is queued up within the TCP Buffer and the data is sent efficiently in large blocks when the consumer is ready to receive the data.
As a result the instructions per data byte is lower than systems that are optimized for low latency and jitter at lower data rates.

3) Failure is localized to the failing component.

Log/Trace/Pkt capture data is only lost when the data exceeds the size of the components TCP buffer.
The data exceeds the TCP buffer size only when the Log/Trace/Pkt data is added to the component's buffer faster than the consumer can consume data from the component's buffer.
Other components do not loose Log/Trace/Pkt data if their data rate is below the rate of consumption such that the components TCP buffer does not over flow.

In other words, if component A enables too much logging component A will loose log/trace/pkt capture data but component B probably will not loose data.

## Q) What is the roll of the send_buffer?

Send Buffer holds log messages within task until log consumer is ready to
receive the queued messages.

The log production rate must be less than the log consumption rate over the long
term or log messages will be discarded or the originating task must block.

The send buffer buffers data during short term differences between the
production and consumption rate.

For example log messages can be buffered in the send buffer while the log
consumer is busy processing a large burst of log messages from another task.

A larger send buffer also results in more efficient transfer because larger
blocks of data can be transferred in one transfer.
## Building for VX Works OS

### Q) What components must VIP (Vx Works Image) include for logging support?

INCLUDE_POSIX_CLOCKS
INCLUDE_POSIX_PTHREAD_SCHEDULER  (For pthreadLib for nanomsg)

INCLUDE_POSIX_PIPES (for pipe2 api for nanomsg)

INCLUDE_DOSFS       (dos file system support)
INCLUDE_DOSFS_CACHE
INCLUDE_XBD

INCLUDE_PASSFS (for Passthrough File System for testing on Simulator)

## Q) What is the purpose of the log_lib_vx, log_to_file_vx, log_test_client_vx build targets?

VxWorks doesn't have full implementations of some posix functions nanomsg can use.
As a result nanomsg must be compiled with different options for vxWorks and must use the libpoll library for an implementation of the poll function.
The libpoll library can be found here: http://software.clapper.org/poll

It's possible the performance of nanomsg for vxWorks is different from the performance of nanomsg using all the normal Linux capabilities so we create versions of the library and executables with both nanomsg libraries.

The linux executables use the native linux form of nanomsg as found in the standard library and include paths in linux.

The vx works executables, with the underscore vx use vxWorks form of nanomsg.

## Q) What configuration changes were needed to make a VX works kernel module rather than rtp executable?

- Remove #include <inttypes.h>
- Remove -ansi from C flags to allow c++ style comments in c files

# ToDo

-  The logic that detects when peer is available and sends service advertisement is questionable.
-- It should not call poll / select every time a message is sent
-- It should resend advertisement when peer exits and re-enters

- log_to_file executable depends on list.h which depends on cntr_of.h which are extracted from the Linux kernel source code for expediency.
  An appropriate list macro should be found to address licensing issues before log_to_file is used publicly.


