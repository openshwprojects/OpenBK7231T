
#include <assert.h>
#include "tuya_hal_network.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/inet.h"
#include "tuya_hal_system.h"
#include "tuya_hal_mutex.h"



#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef INOUT
#define INOUT
#endif


#define OPRT_OK                             (0)
#define OPRT_COM_ERROR                      (-1)



#define CANONNAME_MAX 128

typedef struct NETWORK_ERRNO_TRANS {
    int sys_err;
    int priv_err;
}NETWORK_ERRNO_TRANS_S;

const NETWORK_ERRNO_TRANS_S unw_errno_trans[]= {
    {EINTR,UNW_EINTR},
    {EBADF,UNW_EBADF},
    {EAGAIN,UNW_EAGAIN},
    {EFAULT,UNW_EFAULT},
    {EBUSY,UNW_EBUSY},
    {EINVAL,UNW_EINVAL},
    {ENFILE,UNW_ENFILE},
    {EMFILE,UNW_EMFILE},
    {ENOSPC,UNW_ENOSPC},
    {EPIPE,UNW_EPIPE},
    {EWOULDBLOCK,UNW_EWOULDBLOCK},
    {ENOTSOCK,UNW_ENOTSOCK},
    {ENOPROTOOPT,UNW_ENOPROTOOPT},
    {EADDRINUSE,UNW_EADDRINUSE},
    {EADDRNOTAVAIL,UNW_EADDRNOTAVAIL},
    {ENETDOWN,UNW_ENETDOWN},
    {ENETUNREACH,UNW_ENETUNREACH},
    {ENETRESET,UNW_ENETRESET},
    {ECONNRESET,UNW_ECONNRESET},
    {ENOBUFS,UNW_ENOBUFS},
    {EISCONN,UNW_EISCONN},
    {ENOTCONN,UNW_ENOTCONN},
    {ETIMEDOUT,UNW_ETIMEDOUT},
    {ECONNREFUSED,UNW_ECONNREFUSED},
    {EHOSTDOWN,UNW_EHOSTDOWN},
    {EHOSTUNREACH,UNW_EHOSTUNREACH},
    {ENOMEM ,UNW_ENOMEM},
    {EMSGSIZE,UNW_EMSGSIZE}
};

/***********************************************************
*  Function: unw_get_errno
*  Input: none
*  Output: none
*  Return: UNW_ERRNO_T
***********************************************************/
UNW_ERRNO_T tuya_hal_net_get_errno(void)
{
    int i = 0;

    int sys_err = errno;

    for(i = 0; i < (int)sizeof(unw_errno_trans)/sizeof(unw_errno_trans[0]); i++) {
        if(unw_errno_trans[i].sys_err == sys_err) {
            return unw_errno_trans[i].priv_err;
        }
    }

    return -100 - sys_err;
}

UNW_IP_ADDR_T tuya_hal_net_addr(const char *cp)
{
    return inet_addr(cp);
}

#define UNW_TO_SYS_FD_SET(fds)  ((fd_set*)fds)

// 编译期结构体大小检查，如果此处编译错误，请加大 UNW_FD_MAX_COUNT 的值
//typedef static_assert_impl_type[(sizeof(UNW_FD_SET_T)>=sizeof(fd_set))?1:-1]; // 编译期校验

int tuya_hal_net_fd_set(int fd, UNW_FD_SET_T* fds)
{
    assert(fds != NULL);
    FD_SET(fd, UNW_TO_SYS_FD_SET(fds));
    return 0;
}

int tuya_hal_net_fd_clear(int fd, UNW_FD_SET_T* fds)
{
    FD_CLR(fd, UNW_TO_SYS_FD_SET(fds));
    return 0;
}

int tuya_hal_net_fd_isset(int fd, UNW_FD_SET_T* fds)
{
    return FD_ISSET(fd, UNW_TO_SYS_FD_SET(fds));
}

int tuya_hal_net_fd_zero(UNW_FD_SET_T* fds)
{
    FD_ZERO(UNW_TO_SYS_FD_SET(fds));
    return 0;
}

/***********************************************************
*  Function: unw_select
*  Input: maxfdp ms_timeout
*  Output: readfds writefds errorfds
*  Return: as same as the socket return
***********************************************************/
int tuya_hal_net_select(IN const int maxfd,INOUT UNW_FD_SET_T *readfds,INOUT UNW_FD_SET_T *writefds,\
               OUT UNW_FD_SET_T *errorfds,IN const uint32_t ms_timeout)
{
    if(maxfd <= 0) {
        return -3000 + maxfd;
    }

    struct timeval *tmp = NULL;
    struct timeval timeout = {ms_timeout/1000, (ms_timeout%1000)*1000};
    if(0 != ms_timeout) {
        tmp = &timeout;
    }else {
        tmp = NULL;
    }

    return select(maxfd, UNW_TO_SYS_FD_SET(readfds), UNW_TO_SYS_FD_SET(writefds), UNW_TO_SYS_FD_SET(errorfds), tmp);
}

/***********************************************************
*  Function: unw_get_nonblock
*  Input: fd
*  Output: none
*  Return: <0  fail
*          >0  non block
*          ==0 block
***********************************************************/
int tuya_hal_net_get_nonblock(IN const int fd)
{
    if( fd < 0 ) {
        return -1;
    }

    if((fcntl(fd, F_GETFL, 0) & O_NONBLOCK) != O_NONBLOCK) {
        return 0;
    }

    if(errno == EAGAIN || errno == EWOULDBLOCK) {
        return 1;
    }

    return 0 ;
}

/***********************************************************
*  Function: unw_get_nonblock
*  Input: fd block
*  Output: none
*  Return: UNW_FAIL/UNW_SUCCESS
***********************************************************/
int tuya_hal_net_set_block(IN const int fd,IN const bool block)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }

    int flags = fcntl(fd, F_GETFL, 0);
    if(block) {
        flags &= (~O_NONBLOCK);
    }else {
        flags |= O_NONBLOCK;
    }

    if (fcntl(fd,F_SETFL,flags) < 0) {
        return UNW_FAIL;
    }

    return UNW_SUCCESS;
}

/***********************************************************
*  Function: unw_close
*  Input: fd
*  Output: none
*  Return: as same as the socket return
***********************************************************/
int tuya_hal_net_close(IN const int fd)
{
    return close(fd);
}

/***********************************************************
*  Function: unw_shutdown
*  Input: fd how
*  Output: none
*  Return: as same as the socket return
************************************************************/
int tuya_hal_net_shutdown(IN const int fd,IN const int how)
{
    return shutdown(fd,how);
}

/***********************************************************
*  Function: unw_socket_create
*  Input: type
*  Output: none
*  Return: as same as the socket return
************************************************************/
int tuya_hal_net_socket_create(IN const UNW_PROTOCOL_TYPE type)
{
    int fd = -1;

    if(PROTOCOL_TCP == type) {
        fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    }else {
        fd = socket(AF_INET, SOCK_DGRAM,0);
    }

    return fd;
}

/***********************************************************
*  Function: unw_connect
*  Input: fd addr port
*  Output: none
*  Return: as same as the socket return
************************************************************/
int tuya_hal_net_connect(IN const int fd,IN const UNW_IP_ADDR_T addr,IN const uint16_t port)
{
    struct sockaddr_in sock_addr;
    uint16_t tmp_port = port;
    UNW_IP_ADDR_T tmp_addr = addr;

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(tmp_port);
    sock_addr.sin_addr.s_addr = htonl(tmp_addr);

    return connect(fd, (struct sockaddr*)&sock_addr, sizeof(struct sockaddr_in));
}

/***********************************************************
*  Function: unw_connect_raw
*  Input: fd addr port
*  Output: none
*  Return: as same as the socket return
************************************************************/
int tuya_hal_net_connect_raw(IN const int fd, void *p_socket_addr, IN const int len)
{
    return connect(fd, (struct sockaddr *)p_socket_addr, len);
}

/***********************************************************
*  Function: unw_bind
*  Input: fd
*         addr-> if(addr == 0) then bind src ip by system select
*         port
*  Output: none
*  Return: as same as the socket return
************************************************************/
int tuya_hal_net_bind(IN const int fd,IN const UNW_IP_ADDR_T addr,IN const uint16_t port)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }

    uint16_t tmp_port = port;
    UNW_IP_ADDR_T tmp_addr = addr;

    struct sockaddr_in sock_addr;
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(tmp_port);
    sock_addr.sin_addr.s_addr = htonl(tmp_addr);

    return bind(fd,(struct sockaddr*)&sock_addr, sizeof(struct sockaddr_in));
}

/***********************************************************
*  Function: unw_listen
*  Input: fd
*         backlog
*  Output: none
*  Return: as same as the socket return
************************************************************/
int tuya_hal_net_listen(IN const int fd,IN const int backlog)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }

    return listen(fd,backlog);
}

/***********************************************************
*  Function: unw_accept
*  Input: fd
*  Output: addr port
*  Return: as same as the socket return
************************************************************/
int tuya_hal_net_accept(IN const int fd,OUT UNW_IP_ADDR_T *addr,OUT uint16_t *port)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }

    struct sockaddr_in sock_addr;
    socklen_t len = sizeof(struct sockaddr_in);
    int cfd = accept(fd, (struct sockaddr *)&sock_addr,&len);
    if(cfd < 0) {
        return UNW_FAIL;
    }

    if(addr) {
        *addr = ntohl((sock_addr.sin_addr.s_addr));
    }

    if(port) {
        *port = ntohs((sock_addr.sin_port));
    }

    return cfd;
}

/***********************************************************
*  Function: unw_send
*  Input: fd buf nbytes
*  Output: none
*  Return: as same as the socket return
***********************************************************/
int tuya_hal_net_send(IN const int fd, IN const void *buf, IN const uint32_t nbytes)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }

    return send(fd,buf,nbytes,0);
}

/***********************************************************
*  Function: unw_send_to
*  Input: fd buf nbytes addr port
*  Output: none
*  Return: as same as the socket return
***********************************************************/
int tuya_hal_net_send_to(IN const int fd, IN const void *buf, IN const uint32_t nbytes,\
                IN const UNW_IP_ADDR_T addr,IN const uint16_t port)
{
    uint16_t tmp_port = port;
    UNW_IP_ADDR_T tmp_addr = addr;

    struct sockaddr_in sock_addr;
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(tmp_port);
    sock_addr.sin_addr.s_addr = htonl(tmp_addr);

    return sendto(fd,buf,nbytes,0,(struct sockaddr *)&sock_addr,sizeof(sock_addr));
}

/***********************************************************
*  Function: unw_recv
*  Input: sockFd nbytes
*  Output: buf
*  Return: as same as the socket return
***********************************************************/
int tuya_hal_net_recv(IN const int fd, OUT void *buf, IN const uint32_t nbytes)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }
    int flags = fcntl(fd, F_GETFL, 0);
	
	int noblock = flags & O_NONBLOCK;
	//PR_DEBUG(" --------hal recv flags:0x%x, noblock:%d", flags, noblock);
    if(!noblock) {
        fd_set set;
		FD_ZERO(&set);
		FD_SET(fd, &set);
		if(select(fd + 1, &set, NULL, NULL, NULL) < 0){
			return -1;
		}
    }

    return recv(fd,buf,nbytes,0);
}

/***********************************************************
*  Function: unw_recv_nd_size
*  Input: fd buf_size nd_size
*  Output: buf
*  Return: < 0 error
***********************************************************/
int tuya_hal_net_recv_nd_size(IN const int fd, OUT void *buf, \
                     IN const uint32_t buf_size,IN const uint32_t nd_size)
{
    if(NULL == buf || \
       buf_size < nd_size) {
        return -1;
    }

    uint32_t rd_size = 0;
    int ret = 0;

    while(rd_size < nd_size) {
        ret = recv(fd,((uint8_t *)buf+rd_size),nd_size-rd_size,0);
        if(ret <= 0) {
            UNW_ERRNO_T err = tuya_hal_net_get_errno();
            if(UNW_EWOULDBLOCK == err || \
               UNW_EINTR == err || \
               UNW_EAGAIN == err) {
                tuya_hal_system_sleep(10);
                continue;
            }

            break;
        }

        rd_size += ret;
    }

    if(rd_size < nd_size) {
        return -2;
    }

    return rd_size;
}


/***********************************************************
*  Function: unw_recvfrom
*  Input: fd nbytes
*  Output: buf addr port
*  Return: as same as the socket return
***********************************************************/
int tuya_hal_net_recvfrom(IN const int fd,OUT void *buf,IN const uint32_t nbytes,\
                 OUT UNW_IP_ADDR_T *addr,OUT uint16_t *port)
{
    struct sockaddr_in sock_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    int ret = recvfrom(fd,buf,nbytes,0,(struct sockaddr *)&sock_addr,&addr_len);
    if(ret <= 0) {
        return ret;
    }

    if(addr) {
        *addr = ntohl(sock_addr.sin_addr.s_addr);
    }

    if(port) {
        *port = ntohs(sock_addr.sin_port);
    }

    return ret;
}

/***********************************************************
*  Function: unw_set_timeout
*  Input: fd ms_timeout trans_type
*  Output: none
*  Return: UNW_FAIL/UNW_SUCCESS
***********************************************************/
int tuya_hal_net_set_timeout(IN const int fd,IN const int ms_timeout,\
                    IN const UNW_TRANS_TYPE_E type)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }

    struct timeval timeout = {ms_timeout/1000, (ms_timeout%1000)*1000};
    int optname = ((type == TRANS_RECV) ? SO_RCVTIMEO:SO_SNDTIMEO);

    if(0 != setsockopt(fd, SOL_SOCKET, optname, (char *)&timeout, sizeof(timeout))) {
        return UNW_FAIL;
    }

    return UNW_SUCCESS;
}

/***********************************************************
*  Function: unw_set_bufsize
*  Input: fd ms_timeout trans_type
*  Output: none
*  Return: UNW_FAIL/UNW_SUCCESS
***********************************************************/
int tuya_hal_net_set_bufsize(IN const int fd,IN const int buf_size,\
                    IN const UNW_TRANS_TYPE_E type)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }

    int size = buf_size;
    int optname = ((type == TRANS_RECV) ? SO_RCVBUF:SO_SNDBUF);

    if(0 != setsockopt(fd, SOL_SOCKET, optname, (char *)&size, sizeof(size))) {
        return UNW_FAIL;
    }

    return UNW_SUCCESS;
}

/***********************************************************
*  Function: unw_set_reuse
*  Input: fd
*  Output: none
*  Return: UNW_FAIL/UNW_SUCCESS
***********************************************************/
int tuya_hal_net_set_reuse(IN const int fd)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }

    int flag = 1;
    if(0 != setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(const char*)&flag,sizeof(int))) {
        return UNW_FAIL;
    }

    return UNW_SUCCESS;
}

/***********************************************************
*  Function: unw_disable_nagle
*  Input: fd
*  Output: none
*  Return: UNW_FAIL/UNW_SUCCESS
***********************************************************/
int tuya_hal_net_disable_nagle(IN const int fd)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }

    int flag = 1;
    if(0 != setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,(const char*)&flag,sizeof(int))) {
        return UNW_FAIL;
    }

    return UNW_SUCCESS;
}

/***********************************************************
*  Function: unw_set_boardcast
*  Input: fd
*  Output: none
*  Return: UNW_FAIL/UNW_SUCCESS
***********************************************************/
int tuya_hal_net_set_boardcast(IN const int fd)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }

    int flag = 1;
    if(0 != setsockopt(fd,SOL_SOCKET,SO_BROADCAST,(const char*)&flag,sizeof(int))) {
        return UNW_FAIL;
    }

    return UNW_SUCCESS;
}

int tuya_hal_net_gethostbyname(IN const char *domain,OUT UNW_IP_ADDR_T *addr)
{
    struct hostent* h;
    if ((h = gethostbyname(domain)) == NULL) {
        return UNW_FAIL;
    }

    *addr = ntohl(((struct in_addr *)(h->h_addr_list[0]))->s_addr);
    return UNW_SUCCESS;
}

UNW_IP_ADDR_T tuya_hal_net_str2addr(IN char *ip_str)
{
    if(ip_str == NULL)
    {
        return 0xFFFFFFFF;
    }
    UNW_IP_ADDR_T addr1 = inet_addr(ip_str);
    UNW_IP_ADDR_T addr2 = ntohl(addr1);
    // UNW_IP_ADDR_T addr3 = inet_network(ip_str);

    return addr2;
}

/***********************************************************
*  Function: unw_set_keepalive
*  Input: alive
*  Output: len
*  Return: UNW_FAIL/UNW_SUCCESS
***********************************************************/
int tuya_hal_net_set_keepalive(IN int fd,IN const bool alive,\
                              IN const uint32_t idle,IN const uint32_t intr,\
                              IN const uint32_t cnt)
{
    if( fd < 0 ) {
        return -3000 + fd;
    }

    int ret = 0;
    int keepalive = alive;
    int keepidle = idle;
    int keepinterval = intr;
    int keepcount = cnt;

    ret |= setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive));
    ret |= setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle));
    ret |= setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval));
    ret |= setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount));
    if(0 != ret) {
        return UNW_FAIL;
    }

    return UNW_SUCCESS;
}

/***********************************************************
*  Function: unw_socket_bind
*  Input: alive
*  Output: len
*  Return: UNW_FAIL/UNW_SUCCESS
***********************************************************/
int tuya_hal_net_socket_bind(IN int fd,IN const char *ip)
{
    if(NULL == ip) {
        return -3000;
    }

    struct sockaddr_in addr_client   = {0};
    addr_client.sin_family   = AF_INET;
    addr_client.sin_addr.s_addr      = inet_addr(ip);
    addr_client.sin_port     = 0;    /// 0 表示由系统自动分配端口号

    if (0 != bind(fd,(struct sockaddr*)&addr_client,sizeof(addr_client))) {
        return UNW_FAIL;
    }

    return UNW_SUCCESS;
}


/***********************************************************
*  Function: unw_set_cloexec
*  Input: fd
*  Output: none
*  Return: UNW_FAIL/UNW_SUCCESS
***********************************************************/
int tuya_hal_net_set_cloexec(IN const int fd)
{
    return UNW_SUCCESS;
}


