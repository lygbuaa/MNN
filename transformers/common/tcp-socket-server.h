/*************************************************************************
* socket basic methods
************************************************************************/

#ifndef __TCP_SOCKET_SERVER_H__
#define __TCP_SOCKET_SERVER_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <errno.h>
#include "logging_utils.h"

namespace common
{

#define SOCKET_BUF_MAX (1024*1024) //1MB

// #ifdef __FILE_NAME__ /** since gcc-12 */
// #define __HLOG_FILENAME__  __FILE_NAME__
// #else
// #define __HLOG_FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
// #endif
// #define LOGPF(format, ...) fprintf(stderr ,"[%s:%d] " format "\n", __HLOG_FILENAME__, __LINE__, ##__VA_ARGS__)

class TcpSocketServer
{
private:
    int sockfd_ = -1;
    int connfd_ = -1;
    struct sockaddr_in srv_addr_;
    fd_set fdset_;

public:
    TcpSocketServer(const char* ip = "127.0.0.1", const uint16_t port = 10001)
    {
        memset(&srv_addr_, 0, sizeof(srv_addr_));
        srv_addr_.sin_family = AF_INET;
        srv_addr_.sin_port = htons(port);
        srv_addr_.sin_addr.s_addr = inet_addr(ip);
        LOGPF("server endpoint set, %s:%d", inet_ntoa(srv_addr_.sin_addr), ntohs(srv_addr_.sin_port));
    }

    ~TcpSocketServer()
    {
        close_socket();
    }

    void close_socket()
    {
        close_connect();
        if(sockfd_ > 0){
            close(sockfd_);
            sockfd_ = -1;
        }
        LOGPF("socket closed! %s:%d", inet_ntoa(srv_addr_.sin_addr), ntohs(srv_addr_.sin_port));
    }

    void close_connect()
    {
        if(connfd_ > 0){
            close(connfd_);
            LOGPF("connection %d closed.", connfd_);
            connfd_ = -1;
        }
    }

    /* allow socket bind to port without TIME_WAIT, after socket close */
    int set_reuseaddr(int reuse = 1)
    {
        socklen_t optlen = sizeof(reuse);
        setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &reuse, optlen);
        getsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &reuse, &optlen);
        LOGPF("set socket reuse addr option: %d", reuse);
        return reuse;
    }

    /* set socket linger time, close cleanly by default */
    int set_linger(int l_onoff = 0, int l_linger = 5)
    {
        struct linger lg;
        lg.l_onoff = l_onoff;
        lg.l_linger = l_linger;
        socklen_t optlen = sizeof(lg);
        setsockopt(sockfd_, SOL_SOCKET, SO_LINGER, &lg, optlen);
        memset(&lg, 0, sizeof(lg));
        getsockopt(sockfd_, SOL_SOCKET, SO_LINGER, &lg, &optlen);
        LOGPF("set socket linger option, l_onoff = %d, l_linger = %d", lg.l_onoff, lg.l_linger);
        return lg.l_onoff;
    }

    void set_heartbeat()
    {
        int optval = 1; //enable heartbeat, default is 0
        int tcp_keepalive_time = 300; //heartbeat idle time 300 secs, default is 7200s
        int tcp_keepalive_probes = 3; //interact 3 heartbeat frames, default is 9
        int tcp_keepalive_intvl = 15; //heartbeat frame interval 15 secs, default is 75s

        socklen_t optlen = sizeof(optval);
        setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen);
        getsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen);
        LOGPF("set socket option SO_KEEPALIVE: %d", optval);

        optlen = sizeof(tcp_keepalive_time);
        setsockopt(sockfd_, SOL_TCP, TCP_KEEPIDLE, &tcp_keepalive_time, optlen);
        getsockopt(sockfd_, SOL_TCP, TCP_KEEPIDLE, &tcp_keepalive_time, &optlen);
        LOGPF("set TCP option TCP_KEEPIDLE: %d", tcp_keepalive_time);
        optlen = sizeof(tcp_keepalive_probes);
        setsockopt(sockfd_, SOL_TCP, TCP_KEEPCNT, &tcp_keepalive_probes, optlen);
        getsockopt(sockfd_, SOL_TCP, TCP_KEEPCNT, &tcp_keepalive_probes, &optlen);
        LOGPF("set TCP option TCP_KEEPCNT: %d", tcp_keepalive_probes);
        optlen = sizeof(tcp_keepalive_intvl);
        setsockopt(sockfd_, SOL_TCP, TCP_KEEPINTVL, &tcp_keepalive_intvl, optlen);
        getsockopt(sockfd_, SOL_TCP, TCP_KEEPINTVL, &tcp_keepalive_intvl, &optlen);
        LOGPF("set TCP option TCP_KEEPINTVL: %d", tcp_keepalive_intvl);
    }

    bool init_server()
    {
        sockfd_ = socket(AF_INET, SOCK_STREAM, 0); //IPv4, TCP
        if(sockfd_ < 0)
        {
            LOGPF("open socket failed! errno: %s", strerror(errno));
        }
        else
        {
            LOGPF("open socket success! sockfd_: %d", sockfd_);
        }
        set_reuseaddr();

        if(bind(sockfd_, (struct sockaddr*)&srv_addr_, sizeof(srv_addr_)) == -1)
        {
            LOGPF("bind socket failed! errno: %s", strerror(errno));
            return false;
        }
        /* only 1 connection */
        if(listen(sockfd_, 1) < 0)
        {
            LOGPF("listen socket error! errno: %s", strerror(errno));
            return false;
        }

        set_heartbeat();
        return true;
    }

    int waiting_client()
    {
        struct sockaddr_in clt_addr;
        memset(&clt_addr, 0, sizeof(clt_addr));
        socklen_t length = sizeof(clt_addr);
        connfd_ = accept(sockfd_, (struct sockaddr*)&clt_addr, &length);
        if(connfd_ < 0)
        {
            LOGPF("connect client error! errno: %s", strerror(errno));
        }
        else
        {
            LOGPF("accept connection %d from ip %s:%d", connfd_, inet_ntoa(clt_addr.sin_addr), ntohs(clt_addr.sin_port));
        }
        return connfd_;
    }

    bool init_client()
    {
        sockfd_ = socket(AF_INET, SOCK_STREAM, 0); //IPv4, TCP
        if(sockfd_ < 0)
        {
            LOGPF("open socket failed! errno: %s", strerror(errno));
        }
        else
        {
            LOGPF("open socket success! sockfd_: %d", sockfd_);
        }

        if (connect(sockfd_, (struct sockaddr*)&srv_addr_, sizeof(srv_addr_)) < 0)
        {
            LOGPF("connect to server error! errno: %s", strerror(errno));
            return false;
        }
        else
        {
            connfd_ = sockfd_; //connfd_ and sockfd_ are same for client
            LOGPF("server connected, %s:%d", inet_ntoa(srv_addr_.sin_addr), ntohs(srv_addr_.sin_port));      
        }
        return true;
    }

    /* send successively, at most 1MB per bout */
    ssize_t s_send(const char* buf, size_t total)
    {
        size_t sent = 0;
        while (sent < total)
        {
            size_t remain = (total - sent) > SOCKET_BUF_MAX ? SOCKET_BUF_MAX : (total - sent);
            ssize_t ret = send(connfd_, buf, remain, 0);
            if (ret < 0)
            {
                LOGPF("send data error! errno: %s", strerror(errno));
                return ret;
            }
            sent += ret;
            buf += ret;
            LOGPF("send ret (%ld), bytes already sent (%ld).", ret, sent);
        }

        return sent;
    }

    /* read total bytes from socket, blocking by default */
    ssize_t s_recv(char *buf, size_t total, struct timeval *timeout = NULL)
    {
        FD_ZERO(&fdset_);
        FD_SET(connfd_, &fdset_);

        size_t recved = 0;
        while(recved < total){
            size_t remain = (total - recved) > SOCKET_BUF_MAX ? SOCKET_BUF_MAX : (total - recved);
            ssize_t ret = select(connfd_ + 1, &fdset_, NULL, NULL, timeout);
            if (ret == 0)
            {
                LOGPF("socket select timeout: %s", strerror(errno));
                return ret;
            } 
            else if(ret < 0) 
            {
                LOGPF("socket select error! errno: %s", strerror(errno));
                return ret;
            }

            if(FD_ISSET(connfd_, &fdset_))
            {
                ret = recv(connfd_, buf, remain, 0);
                if(ret == 0) 
                {
                    FD_CLR(connfd_, &fdset_);
                    LOGPF("socket closed by peer: %s!", strerror(errno));
                    return -11;
                }
                else if (ret < 0)
                {
                    LOGPF("socket recv error! errno: %s", strerror(errno));
                    return ret;
                }
            }

            recved += ret;
            buf += ret;
            LOGPF("recv ret (%ld), bytes already recved (%ld)", ret, recved);
        }

        return recved;
    }

    /* read line from socket, return if encounter tail, default tail is \0 */
    ssize_t s_readline(std::string& str, char tail = '\0', struct timeval *timeout = NULL)
    {
        char buf;
        do{
            buf = 0;
            ssize_t len = s_recv(&buf, 1, timeout);
            if(len == 1)
            {
                if(buf == tail)
                {
                    break; //discard tail
                }
                else
                {
                    str += buf;
                }
            }
            else
            {
                LOGPF("socket readline abnormal: %s", strerror(errno));
                return len; //if socket closed by peer, len = 0
            }
        }while(true);

        return str.size();
    }
};
}
#endif //__TCP_SOCKET_SERVER_H__