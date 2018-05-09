/**********************************************************
 * Copyright ,2016, unionsmartit.com
 * FileName:statistic.c
 * Author: Chad   Date:2016-5-24
 * Description: receive messages that send from surveyor,
 *                  collect all of the statistics info(include protocol flow,ip flow and port flow) to update to database.
 * 
 * History:
 *     <author>        <time>        <desc>
 *
 **********************************************************/

#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "common.h"
#include "debug.h"
#include "statistic_sql.h"
#include "statistic_common.h"

#define NEED_TIMER 1
#define MAX_EVENTS 8

struct _statistic_ids {
    int fd; //socket fd
    int t_fd; //timer fd
    int (*statistic_cb)(struct _statisics_msg *); //callback function of socket fd
    int (*timer_cb)(void);//callback function of timer fd
};

static int StatisticPacketFromSurveyor(struct _statisics_msg *count);
static int HanderTimerEvent();
/*Global values*/
static struct _statistic_ids statistic = {
    -1,
    -1,
    StatisticPacketFromSurveyor,
    HanderTimerEvent,
};
static int quit = 0;
static int EpollFd = -1;
static int reset_protocol = 0;/*flag to identify whether the protocol data has updated to db,1 means yes and 0 means no*/
static int reset_port = 0;/*flag to identify whether the port data has updated to db,1 yes and 0 no*/
static int reset_ipv4 = 0;/*flag to identify whether the ipv4 data has updated to db,1 yes and 0 no*/
static int reset_ipv6 = 0;/*flag to identify whether the ipv6 data has updated to db,1 yes and 0 no*/
struct _ProtocolPacketCount protocol_flow[MAX_PROTOCOL_TO_MONITOR];
struct _PortPacketCount port_flow[MAX_RESOURCE_TO_MONITOR];
struct _IPv4AddressPacketCount ipv4_flow[MAX_RESOURCE_TO_MONITOR];
struct _IPv6AddressPacketCount ipv6_flow[MAX_RESOURCE_TO_MONITOR];


/*
Function        : InitMysqlDatabase
Description    : Initialize mysql dababase and connect to it.
Arguments     : server->the host that run mysql server;
                      database->database name;
                      user->user name of database;
                      password->password of database.
Returns         : 0 ->success,1 ->fail.
*/
static int InitMysqlDatabase(char *server,char *database,char *user,char *password)
{
    if(MysqlInit(server,database,user,password))
    {
        log_message(L_ERR,"%s: MysqlInit error\n",__func__);
        return 1;
    }
    
    if(MysqlConnect())
    {
        log_message(L_ERR,"%s: MysqlConnect error\n",__func__);
        return 1;
    }
    return 0;
}

/*
Function        : CombineProtocol
Description    : combine all of the protocol flow.
Arguments     : content->statistic content from surveyor;
                      content_len->length of statistic content.
Returns         : 0 ->success,1 ->fail.
*/
int CombineProtocol(void *content,int content_len)
{
    struct _ProtocolPacketCount *t_protocol = (struct _ProtocolPacketCount *)content;
    int array_len = content_len/sizeof(struct _ProtocolPacketCount);
    /*we copy the structure array directly if received is first message after update to db,
    so we can get the content of the array correctly,even if  the sequency of the array has changed by user(or surveyor),
    we can also handle it without any notify.
    */
    if(reset_protocol == 1){
        /*this is the first message after we update the data to db,so copy it directly.*/
        reset_protocol = 0;
        memcpy(protocol_flow,content,content_len);
    }
    else{
        /*summation*/
        int i;
        for(i = 0; i < array_len && array_len <= MAX_PROTOCOL_TO_MONITOR; i++)
        {
            if(t_protocol[i].count <= 0)
                continue;
            protocol_flow[i].count += t_protocol[i].count;
            protocol_flow[i].Bps += t_protocol[i].Bps;
        }
    }
    
    return 0;
}

/*
Function        : CombinePort
Description    : combine all of the port flow.
Arguments     : content->statistic content from surveyor;
                      content_len->length of statistic content.
Returns         : 0 ->success,1 ->fail.
*/
int CombinePort(void *content,int content_len)
{
    struct _PortPacketCount *t_port = (struct _PortPacketCount *)content;
    int array_len = content_len/sizeof(struct _PortPacketCount);
    /*we copy the structure array directly if received is first message after update to db,
    so we can get the content of the array correctly,even if  the sequency of the array has changed by user(or surveyor),
    we can also handle it without any notify.
    */
    if(reset_port == 1){
        reset_port = 0;
        /*this is the first message after we update the data to db,so copy it directly.*/
        memcpy(port_flow,content,content_len);
    }
    else{
        /*summation*/
        int i;
        for(i = 0; i < array_len && array_len <= MAX_RESOURCE_TO_MONITOR; i++)
        {
            if(t_port[i].count <= 0)
                continue;
            port_flow[i].count += t_port[i].count;
            port_flow[i].Bps += t_port[i].Bps;
        }
    }
    
    return 0;
}

/*
Function        : CombineIpv4
Description    : combine all of the ipv4 flow.
Arguments     : content->statistic content from surveyor;
                      content_len->length of statistic content.
Returns         : 0 ->success,1 ->fail.
*/
int CombineIpv4(void *content,int content_len)
{
    struct _IPv4AddressPacketCount *t_ipv4 = (struct _IPv4AddressPacketCount *)content;
    int array_len = content_len/sizeof(struct _IPv4AddressPacketCount);
    /*we copy the structure array directly if received is first message after update to db,
    so we can get the content of the array correctly,even if  the sequency of the array has changed by user(or surveyor),
    we can also handle it without any notify.
    */
    if(reset_ipv4 == 1){
        reset_ipv4 = 0;
        /*this is the first message after we update the data to db,so copy it directly.*/
        memcpy(ipv4_flow,content,content_len);
    }
    else{
        /*summation*/
        int i;
        for(i = 0; i < array_len && array_len <= MAX_RESOURCE_TO_MONITOR; i++)
        {
            if(t_ipv4[i].count <= 0)
                continue;
            ipv4_flow[i].count += t_ipv4[i].count;
            ipv4_flow[i].Bps += t_ipv4[i].Bps;
        }
    }
    
    return 0;
}

int CombineIpv6(void *content,int content_len)
{
    struct _IPv6AddressPacketCount *t_ipv6 = (struct _IPv6AddressPacketCount *)content;
    int array_len = content_len/sizeof(struct _IPv6AddressPacketCount);
    /*we copy the structure array directly if received is first message after update to db,
    so we can get the content of the array correctly,even if  the sequency of the array has changed by user(or surveyor),
    we can also handle it without any notify.
    */
    if(reset_ipv6 == 1){
        reset_ipv6 = 0;
        /*this is the first message after we update the data to db,so copy it directly.*/
        memcpy(ipv6_flow,content,content_len);
    }
    else{
        /*summation*/
        int i;
        for(i = 0; i < array_len && array_len <= MAX_RESOURCE_TO_MONITOR; i++)
        {
            if(t_ipv6[i].count <= 0)
                continue;
            ipv6_flow[i].count += t_ipv6[i].count;
            ipv6_flow[i].Bps += t_ipv6[i].Bps;
        }
    }
    
    return 0;
}

/*
Function        : StatisticProtocolFlow
Description    : Update protocol_stats table,be careful,the packet count we received is the increment count from last count.
Arguments     : content->statistic content from surveyor;
                      content_len->length of statistic content.
Returns         : 0 ->success,1 ->fail.
*/
static int StatisticProtocolFlow(void *content,int content_len)
{
    struct _ProtocolPacketCount *t_protocol = (struct _ProtocolPacketCount *)content;
    uint32_t i,j,ret;
    uint64_t count,total;
    char sql[128] = "";
    
    //log_message(L_DEBUG,"%d:%d:%d\n",content_len,sizeof(struct _ProtocolPacketCount),content_len/sizeof(struct _ProtocolPacketCount));
    for(i = 0; t_protocol[i].is_set && i < MAX_PROTOCOL_TO_MONITOR; i++)
    {
        //if(t_protocol[i].count <= 0)
        //    continue;
        count = 0;
        total = 0;
        snprintf(sql,sizeof(sql),"select count,total from protocol_stats where name = '%s'",t_protocol[i].name);
        ret = MysqlSelectAsULong2(sql,&count,&total);
        if(ret > 0){
            snprintf(sql,sizeof(sql),"update protocol_stats set count = %lu,total = %lu,Bps=%u where name = '%s'",t_protocol[i].count+count,t_protocol[i].Bps+total,t_protocol[i].Bps,t_protocol[i].name);
        }
        else{
            snprintf(sql,sizeof(sql),"insert into protocol_stats (name,count,total,Bps) values ('%s',%lu,%lu,%u);",t_protocol[i].name,t_protocol[i].count,t_protocol[i].Bps+total,t_protocol[i].Bps);
	    }
        #ifdef DEBUG
        if(strcmp(t_protocol[i].name,"ip") == 0){
            log_message(L_DEBUG,"%s:%s protocol %s old pkt count %d,added %d,bps is %d bytes",__func__,(ret>0)?"update":"insert",t_protocol[i].name,count,t_protocol[i].count,t_protocol[i].Bps);
            log_message(L_DEBUG,"%s:sql==>%s",__func__,sql);
        }
        #endif
        MysqlInsert(sql,NULL);
    }
    //done, reset the count
    reset_protocol = 1;
    memset(protocol_flow,0,sizeof(struct _ProtocolPacketCount)*MAX_PROTOCOL_TO_MONITOR);
}

/*
Function        : StatisticPortFlow
Description    : Update port_stats table,be careful,the packet count we received is the increment count from last count.
Arguments     : content->statistic content from surveyor;
                      content_len->length of statistic content.
Returns         : 0 ->success,1 ->fail.
*/
static int StatisticPortFlow(void *content,int content_len)
{
    struct _PortPacketCount *t_port = (struct _PortPacketCount *)content;
    uint32_t i,j,ret;
    uint64_t count,total;
    char sql[128] = "";
    
    for(i = 0; t_port[i].is_set && i < MAX_RESOURCE_TO_MONITOR; i++)
    {
        //if(t_port[i].count <= 0)
        //    continue;
        count = 0;
        total = 0;
        snprintf(sql,sizeof(sql),"select count,total from port_stats where port = %u",t_port[i].port);
        ret = MysqlSelectAsULong2(sql,&count,&total);
        if(ret > 0){
            snprintf(sql,sizeof(sql),"update port_stats set count = %lu,total=%lu,Bps=%u where port = %u",t_port[i].count+count,t_port[i].Bps+total,t_port[i].Bps,t_port[i].port);
            
            #ifdef DEBUG
            log_message(L_DEBUG,"%s:update port %d old pkt count is %d,added %d,bps is %d bytes",__func__,t_port[i].port,count,t_port[i].count,t_port[i].Bps);
            log_message(L_DEBUG,"%s:sql==>%s",__func__,sql);
            #endif

            MysqlInsert(sql,NULL);
        }
        //else
        //    snprintf(sql,sizeof(sql),"insert into port_stats (port,count,total,Bps) values (%u,%lu,%lu,%u);",t_port[i].port,t_port[i].count,t_port[i].Bps+total,t_port[i].Bps);
        //MysqlInsert(sql,NULL);
    }
    //done,reset the count
    reset_port = 1;
    memset(port_flow,0,sizeof(struct _PortPacketCount)*MAX_RESOURCE_TO_MONITOR);
}

/*
Function        : StatisticIPV4Flow
Description    : Update ipv4_stats table,be careful,the packet count we received is the increment count from last count.
Arguments     : content->statistic content from surveyor;
                      content_len->length of statistic content.
Returns         : 0 ->success,1 ->fail.
*/
static int StatisticIPV4Flow(void *content,int content_len)
{
    struct _IPv4AddressPacketCount *t_ipv4 = (struct _IPv4AddressPacketCount *)content;
    uint32_t i,j,ret;
    uint64_t count,total;
    char sql[128] = "";
    char ip[INET_ADDRSTRLEN] = "";
    
    for(i = 0; t_ipv4[i].is_set && i < MAX_RESOURCE_TO_MONITOR; i++)
    {
        //if(t_ipv4[i].count <= 0)
        //    continue;
        count = 0;
        total = 0;
        inet_ntop(AF_INET,&t_ipv4[i].ipv4_addr,ip,sizeof(ip));
        snprintf(sql,sizeof(sql),"select count,total from ipv4_stats where ip = '%s'",ip);
        ret = MysqlSelectAsULong2(sql,&count,&total);
        if(ret > 0){
            snprintf(sql,sizeof(sql),"update ipv4_stats set count = %lu,total=%lu,Bps=%u where ip = '%s'",t_ipv4[i].count+count,t_ipv4[i].Bps+total,t_ipv4[i].Bps,ip);

            #ifdef DEBUG
            log_message(L_DEBUG,"%s:update ip %s old pkt count is %d,added %d,bps is %d bytes",__func__,ip,count,t_ipv4[i].count,t_ipv4[i].Bps);
            log_message(L_DEBUG,"%s:sql==>%s",__func__,sql);
            #endif

            MysqlInsert(sql,NULL);
        }
        //else
        //    snprintf(sql,sizeof(sql),"insert into ipv4_stats (ip,count,total,Bps) values('%s',%lu,%lu,%u)",ip,t_ipv4[i].count,t_ipv4[i].Bps+total,t_ipv4[i].Bps);
        //MysqlInsert(sql,NULL);
    }
    //done,reset the count
    reset_ipv4 = 1;
    memset(ipv4_flow,0,sizeof(struct _IPv4AddressPacketCount)*MAX_RESOURCE_TO_MONITOR);
}

/*
Function        : StatisticIPV6Flow
Description    : Update ipv6_stats table,be careful,the packet count we received is the increment count from last count.
Arguments     : content->statistic content from surveyor;
                      content_len->length of statistic content.
Returns         : 0 ->success,1 ->fail.
*/
static int StatisticIPV6Flow(void *content,int content_len)
{
    struct _IPv6AddressPacketCount *t_ipv6 = (struct _IPv6AddressPacketCount *)content;
    uint32_t i,j,ret;
    uint64_t count,total;
    char sql[128] = "";
    char ip[INET6_ADDRSTRLEN] = "";
    
    for(i = 0; t_ipv6[i].is_set && i < MAX_RESOURCE_TO_MONITOR; i++)
    {
        //if(t_ipv6[i].count <= 0)
        //    continue;
        count = 0;
        total = 0;
        inet_ntop(AF_INET6,&t_ipv6[i].ipv6_addr,ip,sizeof(ip));
        snprintf(sql,sizeof(sql),"select count,total from ipv6_stats where ip = '%s'",ip);
        ret = MysqlSelectAsULong2(sql,&count,&total);
        if(ret > 0){
            snprintf(sql,sizeof(sql),"update ipv6_stats set count = %lu,total=%lu,Bps=%u where ip = '%s'",t_ipv6[i].count+count,t_ipv6[i].Bps+total,t_ipv6[i].Bps,ip);
            MysqlInsert(sql,NULL);
        }
        //else
        //    snprintf(sql,sizeof(sql),"insert into ipv6_stats (ip,count,total,Bps) values('%s',%lu,%lu,%u)",ip,t_ipv6[i].count,t_ipv6[i].Bps+total,t_ipv6[i].Bps);
        //MysqlInsert(sql,NULL);
    }
    //done,reset the count
    reset_ipv6 = 1;
    memset(ipv6_flow,0,sizeof(struct _IPv6AddressPacketCount)*MAX_RESOURCE_TO_MONITOR);
}

/*
Function        : HanderTimerEvent
Description    : Callback function of the timer,handle timeout event.
Arguments     : NULL.
Returns         : 0 ->success,others ->fail.
*/
static int HanderTimerEvent()
{
    //log_message(L_DEBUG,"%s\n",__func__);
    static int times = 0;
    if(++times >= 60){
        log_message(L_INFO,"%s=============update data to database\n",__func__);
        times = 0;
    }
    
    //update protocol data to db
    StatisticProtocolFlow(protocol_flow,0);
    
    //upadte port data to db
    StatisticPortFlow(port_flow,0);
    
    //upadte ipv4 data to db
    StatisticIPV4Flow(ipv4_flow,0);
    
    //update ipv6 data to db,doesn't support at this time
    //StatisticIPV6Flow(ipv6_flow,0);
    return  0;
}

/*
Function        : StatisticPacketFromSurveyor
Description    : Callback function of the socket,got a message from surveyor,handle epoll event.
Arguments     : msg->pointer to statistic_msg.
Returns         : 0 ->success,others ->fail.
*/
static int StatisticPacketFromSurveyor(struct _statisics_msg *msg)
{
    static int times = 0;
    if(++times >= 720){ //3*4*60;every 60 seconds log the message to syslog;(3 messsages * 4 surveyor )
        log_message(L_INFO,"%s**************msg len is %d,msg type is %d\n",__func__,msg->hdr.msg_len,msg->hdr.msg_type);
        if(times > 721)
            times = 0;
    }
    switch(msg->hdr.msg_type){
        case PROTOCOL_MSG:
            //StatisticProtocolFlow(msg->content,msg->hdr.msg_len);
            CombineProtocol(msg->content,msg->hdr.msg_len);
            break;
        case PORT_MSG:
            //StatisticPortFlow(msg->content,msg->hdr.msg_len);
            CombinePort(msg->content,msg->hdr.msg_len);
            break;
        case IPV4_MSG:
            //StatisticIPV4Flow(msg->content,msg->hdr.msg_len);
            CombineIpv4(msg->content,msg->hdr.msg_len);
            break;
        case IPV6_MSG:
            //StatisticIPV6Flow(msg->content,msg->hdr.msg_len);
            CombineIpv6(msg->content,msg->hdr.msg_len);
            break;
        default:
            break;
    }
}

/*
Function        : InitStatisticPacketModule
Description    : Initialize this module,creat a socket and add it to epoll.
Arguments     : NULL.
Returns         : 0 ->success,others ->fail.
*/
static int InitStatisticPacketModule()
{
    
    struct sockaddr_un cServerSock;

    statistic.fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if ( statistic.fd < 0 )
    {
        log_message(L_ERR,"%s: socket error\n",__func__);
        return 1;
    }

    unlink(STATISTICS_PATH);
    cServerSock.sun_family = AF_UNIX;
    strcpy(cServerSock.sun_path, STATISTICS_PATH);

    if ( bind(statistic.fd, (struct sockaddr*)&cServerSock, sizeof(cServerSock)) < 0)
    {
        log_message(L_ERR,"%s: bind error:%s\n",__func__,strerror(errno));
        close(statistic.fd);
        return 2;
    }
    
    if (listen(statistic.fd, 128) < 0 ){
        log_message(L_ERR,"%s: listen error:%s\n",__func__,strerror(errno));
        close(statistic.fd);
        return 3;
    }

    struct epoll_event cEvent;
    cEvent.data.fd = statistic.fd;
    cEvent.events = EPOLLIN;
    
    if (epoll_ctl(EpollFd, EPOLL_CTL_ADD, statistic.fd, &cEvent) < 0)
    {
        log_message(L_ERR,"%s: epoll_ctl error\n",__func__);
        close(statistic.fd);
        return 3;
    }
    
    return 0;
}

/*
Function        : InitStatisticPacketModuleTimer
Description    : Initialize a timer for this module,creat a timer and add it to epoll.
Arguments     : NULL.
Returns         : 0 ->success,others ->fail.
*/
static int InitStatisticPacketModuleTimer()
{
    struct epoll_event cEvent;
    struct itimerspec cSpec;

    statistic.t_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if ( statistic.t_fd < 0 )
    {
        log_message(L_ERR,"%s: socket error\n",__func__);
        return 1;
    }

    //set timer
    cSpec.it_value.tv_sec = 1;
    cSpec.it_value.tv_nsec = 0;
    cSpec.it_interval.tv_sec = 1;
    cSpec.it_interval.tv_nsec = 0;
    
    if ( timerfd_settime(statistic.t_fd, 0, &cSpec, NULL) < 0)
    {
        log_message(L_ERR,"%s: timerfd_settime error:%s\n",__func__,strerror(errno));
        close(statistic.t_fd);
        return 2;
    }

    //add timer id to epoll
    cEvent.data.fd = statistic.t_fd;
    cEvent.events = EPOLLIN;
    
    if (epoll_ctl(EpollFd, EPOLL_CTL_ADD, statistic.t_fd, &cEvent) < 0)
    {
        log_message(L_ERR,"%s: epoll_ctl error\n",__func__);
        close(statistic.t_fd);
        return 3;
    }
    
    return 0;
}

int acceptConn(int fd)
{
    int cli = -1;
    struct sockaddr_un sock;
    bzero(&sock, sizeof(sock));
    socklen_t len = sizeof(sock);

    cli = accept(fd, (struct sockaddr*)&sock, &len);
    if (cli < 0)
    {
        log_message(L_NOTICE,"acceptConn error: %s\n",strerror(errno));
    }

    return cli;
}

/*
Function        : InitLoop
Description    : Initialize epoll.
Arguments     : NULL.
Returns         : 0 ->success,others ->fail.
*/
static int InitLoop()
{
    if (EpollFd >= 0)
        return 0;

    EpollFd = epoll_create(32);
    if (EpollFd < 0)
        return -1;

    fcntl(EpollFd, F_SETFD, fcntl(EpollFd, F_GETFD) | FD_CLOEXEC);
    
    return 0;
}

/*
Function        : LoopRun
Description    : Main loop of this module,wait for intersting events occured and handle it.
Arguments     : NULL.
Returns         : 0 ->success,others ->fail.
*/
static void LoopRun()
{
    struct sockaddr_un cClientSock;
    struct _statisics_msg msg;
    int32_t recv_len = 0;
    socklen_t len = sizeof(cClientSock);
    int32_t num = 0;
    
    struct epoll_event cEvs[MAX_EVENTS];

    while(1){
        if(quit)
        {
            break;
        }
        //wait for events to wake up
        num = epoll_wait(EpollFd, cEvs, MAX_EVENTS, -1);
        if ( num < 0 )
        {
            if ( EINTR == errno )
                continue;
            else
                return;
        }

        //check events
        for (int i = 0; i < num; i++) {
            if (cEvs[i].data.fd == statistic.fd )
            {
                if (cEvs[i].events & EPOLLIN)
                {
                    //receive the message header first to get the data length
                    memset(&msg,0,sizeof(msg));
                    int connfd = acceptConn(statistic.fd);
                    if(connfd < 0)
                        continue;
                    recv_len = 0;
                    while((recv_len = read(connfd,&(msg.hdr),sizeof(msg.hdr))) > 0){
                        if(recv_len == sizeof(msg.hdr))
                        {
                            //log_message(L_DEBUG,"recv %d bytes\n",msg.hdr.msg_len);
                            msg.content= calloc(1,msg.hdr.msg_len+1);
                            if(msg.content)
                            {
                                //receive message content
                                recv_len = read(connfd,msg.content,msg.hdr.msg_len);
                                if(recv_len > 0)
                                {
                                    statistic.statistic_cb(&msg);
                                }
                                else
                                {
                                    log_message(LOG_WARNING,"%s:content len is %d,but read nothing,read return %d\n",__func__,msg.hdr.msg_len,recv_len);
                                }
                                free(msg.content);
                            }
                            else
                            {
                                log_message(L_ERR,"%s:out of memory\n",__func__);
                            }
                        }
                        else{
                            log_message(LOG_WARNING,"%s:hdr len not match\n",__func__);
                        }
                    }
                    close(connfd);
                }
            }
            else if(cEvs[i].data.fd == statistic.t_fd)
            {
                //timer timeout call the timer's callback function
                uint64_t n,len;
                len = read(statistic.t_fd,&n,sizeof(uint64_t));
                if(len == sizeof(uint64_t))
                    statistic.timer_cb();
            }
        }
    }
}

void handle_signal(int signo)
{
    quit = 1;
}

void InitSignal()
{
    signal(SIGHUP,handle_signal);
    signal(SIGINT,handle_signal);
    signal(SIGTERM,handle_signal);
    //signal(SIGCHLD,handle_sigchld);
}

int main(int argc,char *argv[])
{
    //check parameters first
    #ifdef CHANGE_DATABASE
    if(argc != 5)
    {
        log_message(L_ERR,"parameter error\n");
        return 1;
    }
    #endif
    InitSignal();
    //initialize epoll
    if(InitLoop())
    {
        log_message(L_ERR,"InitLoop error\n");
        return 1;
    }
    //create socket and add it to epoll to receive the event from ids kernel
    if(InitStatisticPacketModule())
    {
        log_message(L_ERR,"InitStatisticPacketModule error\n");
        return 1;
    }
    
    //if we need a timer to do something, initialize a timer and add the timer_fd to epoll
    #if NEED_TIMER
    if(InitStatisticPacketModuleTimer())
    {
        log_message(L_ERR,"InitStatisticPacketModuleTimer error\n");
        return 1;
    }
    #endif
    //initialize database
    #ifdef CHANGE_DATABASE
    if(InitMysqlDatabase(argv[1],argv[2],argv[3],argv[4]))
    #else
    if(InitMysqlDatabase("localhost", "surveyor", "root", "13246"))
    #endif
    {
        log_message(L_ERR,"InitMysqlDatabase error\n");
        return 1;
    }
    
    log_message(L_INFO,"statistic is running...\n");
    
    //create a thread ,send hello to manager every 30 seconds

    //loop
    LoopRun();
    
    log_message(L_NOTICE,"statistic exit now...\n");
    
    close(statistic.fd);
    #if NEED_TIMER
    close(statistic.t_fd);
    #endif
    
    //clean
    MysqlClose();
    CleanMysqlData();
    return 0;
}

