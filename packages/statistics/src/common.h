/*
 * common.h
 *
 *  Created on: 2016-4-20
 *      Author: lzh
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <iostream>
#include <cstring>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include <exception>
#include <stdexcept>
#include <set>
#include <cstring>
#include <stdint.h>
#include <cstdlib>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <time.h>
#include <sys/wait.h>
#include <syslog.h>
#include <stdarg.h>
#include <signal.h>
/**
 * Globals defined here
 */

#ifndef MAX_RESTART_TIMES
#define MAX_RESTART_TIMES 5
#endif

#ifndef MAX_CMD_ARGS
#define MAX_CMD_ARGS    50
#endif

//heartbeat check interval: 120 seconds
#ifndef HEARTBEAT_CHECK_INTERVAL
#define HEARTBEAT_CHECK_INTERVAL    120
#endif

// process status
typedef enum _status {
    STOPPED = 0, //the process has stopped
    RUNNING = 1, //the process is running
    NOT_STARTED = 2 // the mgmt process has not started the process yet
} ProcessStatus;

// process structure to keep track of related information
typedef struct _Process
{
    pid_t iPid; //child process id
    ProcessStatus cStatus; //process status
    uint16_t iTriedTimes; //how many times we tried to restart the process
    uint32_t iRestartedTimes; //how many times the mgmt process restarted this process
    time_t iLastHeartbeatTime;//last time we got heart beat message from this child
    uint32_t iNubOfHeartbeatsReceived;//number of heartbeat messages received from this child
    std::string cBinaryPath;//the executable binary of this process; e.g: /opt/xeye/snort
    std::string cArguments[MAX_CMD_ARGS]; //arguments to this process take; e.g: "-c /etc/snort.conf -b /opt/log"
    size_t uArgCount; //number of arguments
    time_t iStartTime;//the time we started this process
} Process;

//default configuration of mgmt process
#ifndef DEFAULT_CONFIG_PATH
#define DEFAULT_CONFIG_PATH "/opt/xeye/etc/mgmt.conf"
#endif

//unix domain socket id for web server and other upper layer applications
#ifndef SOCKET_ID_FOR_UPPER_LAYER
#define SOCKET_ID_FOR_UPPER_LAYER "/opt/xeye/web-mgmt-id"
#endif

//unix domain socket id for heartbeat
#ifndef HEARTBEAT_ID
#define SOCKET_HEARTBEAT_ID "/opt/xeye/mgmt-heartbeat-id"
#endif

//snort unix domain socket ID
#ifndef SNORT_SOCKET_ID
#define SNORT_SOCKET_ID "/opt/xeye/snort-domain-id"
#endif

//a child process can be inactive for at most 90 seconds
#ifndef MAX_INACTIVE_TIME_IN_SECONDS
#define MAX_INACTIVE_TIME_IN_SECONDS 90
#endif

#ifndef MAX_RESTART_TRY_TIME
#define MAX_RESTART_TRY_TIME    5
#endif
#endif /* COMMON_H_ */
