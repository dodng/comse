#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#if 1
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <event.h>
#include <pthread.h>
#include "evutil.h"
#include "io_config.h"
#include "http.h"
#include "policy_interface.h"
//////////////base structure///////////////////////////
/////

#define PROCESS_THREAD 3
#define UPDATE_PROCESS_THREAD 1
#define FD_CLOSEONEXEC(x) do { \
	if (fcntl(x, F_SETFD, 1) == -1) \
	perror("fcntl(F_SETFD) in FD_CLOSEONEXEC"); \
} while (0)

//struct timeval tv; 
struct event listen_ev;
pthread_t ptocess_thread[PROCESS_THREAD] = {0};
/*notification mechanism*/
int notify_fd[PROCESS_THREAD][2] = {0};
int how_many_client = 0;
//timeout
struct timeval tv_con_timeout = {0};
struct timeval tv_recv_timeout = {0};
struct timeval tv_send_timeout = {0};

/////////////////////////////////////////////////////////
//struct timeval tv; 
struct event update_listen_ev;
pthread_t update_ptocess_thread[UPDATE_PROCESS_THREAD] = {0};
/*notification mechanism*/
int update_notify_fd[UPDATE_PROCESS_THREAD][2] = {0};
int update_how_many_client = 0;

struct sig_ev_arg 
{
	int notify_fd;
	void *ptr;
};

class interface_data
{
	public:
		interface_data()
		{
			recv_buff_len = BUFF_SIZE;
			send_buff_len = BUFF_SIZE;
			now_send_len = 0;
			now_recv_len = 0;

			status = status_init;
//			bs = NULL;
			sd = -1;
		}
		~interface_data(){;}
		//data
		char recv_buff[BUFF_SIZE];
		char send_buff[BUFF_SIZE];
		int recv_buff_len;
		int send_buff_len;
		int now_recv_len;
		int now_send_len;
		struct event ev;
		int status;
		int sd;
		http_entity http;
		policy_entity policy;
};

struct sig_ev_arg notify_arg[PROCESS_THREAD] = {0};
struct sig_ev_arg update_notify_arg[UPDATE_PROCESS_THREAD] = {0};

struct interface_data * it_pool_get()
{
	printf("it_pool_get\n");
	return new interface_data();
}
void it_pool_put(struct interface_data *p_it)
{
	if (p_it)
		delete p_it;
	printf("it_pool_put\n");
}

void inline reg_add_event(struct event *p_eve,int fd,int event_type,event_cb_func p_func,void *p_arg,
		struct event_base *p_base,struct timeval *p_time)
{
	if (0 == p_eve
			|| 0 == p_func
			|| 0 == p_base)
		return;
	event_set(p_eve,fd,event_type,p_func,p_arg);
	event_base_set(p_base,p_eve);
	// add recv timeout
	event_add(p_eve,p_time);
}
//////////////single thread/////////////////
//
int get_sock_error(int fd)
{
	int error;
	socklen_t len;

	len=sizeof(error);
	getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len);
	return error;

}
//////////////
void SendData(int fd, short int events, void *arg)
{
	if ( 0 == arg || fd <= 0)
		return;
	struct interface_data *p_it = (struct interface_data *)arg;
	struct event *p_ev = &(p_it->ev);
	struct event_base *base = p_ev->ev_base;
	char *buff = (char *)&(p_it->send_buff);
	int buff_len = p_it->send_buff_len;
	int len;
	//char buff[512] = {0};

	if (events == EV_TIMEOUT)
	{
		printf("send timeout fd=[%d] error[%d]\n", fd, errno);
		goto GC;
	} 
	
	len = send(fd, buff, p_it->now_send_len, 0);
	event_del(p_ev);

	if(len >= 0)
	{
		printf("Thread[%x]\tClient[%d]:Send=%s\n",(int)pthread_self(),fd, buff);
		//send success
		p_it->status = status_send;
		//regist recv event
		//reg_add_event(p_ev,p_it->sd,EV_READ,RecvData,p_it,base,&tv_recv_timeout);
		//flush stdout
		fflush(stdout);
		goto GC;
	}
	else
	{
		printf("talk over[fd=%d] error[%d]:%s\n", fd, errno, strerror(errno));
		goto GC;
	}
	return;
GC:
	close(p_ev->ev_fd);
	it_pool_put(p_it);
	return;
}

void RecvData(int fd, short int events, void *arg)
{
	if ( 0 == arg || fd <= 0)
		return;
	struct interface_data *p_it = (struct interface_data *)arg;
	struct event *p_ev = &(p_it->ev);
	struct event_base *base = p_ev->ev_base;
	char *buff = (char *)&(p_it->recv_buff) + p_it->now_recv_len;
	int buff_len = p_it->recv_buff_len - p_it->now_recv_len;
	int len;
	//timeout
	if (events == EV_TIMEOUT)
	{
		close(p_ev->ev_fd);
		event_del(p_ev);
		it_pool_put(p_it);
		printf("recv timeout fd=[%d] error[%d]\n", fd, errno);
		return;
	} 

	// clear buff
	memset(buff,0,buff_len);
	// receive data
	len = recv(fd,buff, buff_len, 0);
	event_del(p_ev);

	if(len > 0)
	{
		printf("Thread[%x]\tClient[%d]:Recv=%s\n",(int)pthread_self(),fd, buff);
		//read success
		p_it->status = status_recv;
		//regist send event
		p_it->now_recv_len += len;
		p_it->http.parse_header(buff);
		p_it->http.parse_body(buff);
		printf("%s",(p_it->http.print_all()).c_str());
		int parse_ret = p_it->http.parse_done();
		printf("http parse_done ret:[%d]\n",parse_ret);

		if (parse_ret < 0 )
		{goto GC;}
		else if (parse_ret == 0)	
		{reg_add_event(p_ev,p_it->sd,EV_READ,RecvData,p_it,base,&tv_recv_timeout);}
		else
		{
		p_it->policy.set_http(&p_it->http);
		p_it->policy.parse_in_json();
		p_it->policy.get_out_json();
		p_it->policy.cook_senddata(p_it->send_buff,p_it->send_buff_len,p_it->now_send_len);

		printf("%s",(p_it->policy.print_all()).c_str());
		
		reg_add_event(p_ev,p_it->sd,EV_WRITE,SendData,p_it,base,&tv_send_timeout);}

	}
	else
	{
		if (len == 0)
			printf("[fd=%d] , closed gracefully.\n", fd);
		else
			printf("recv[fd=%d] error[%d]:%s\n", fd, errno, strerror(errno));
		goto GC;
	}
	return;

GC:
	close(p_ev->ev_fd);
	it_pool_put(p_it);
	return;
}
// accept new connections from clients 
void AcceptConn(int fd, short int events, void *arg)
{
	struct sockaddr_in sin;
	struct event *p_ev = (struct event *)arg;
	/* Reschedule this event */
	event_add(p_ev, NULL);
	socklen_t len = sizeof(struct sockaddr_in);
	int nfd, i;
	// accept 
	if((nfd = accept(fd, (struct sockaddr*)&sin, &len)) <= 0)
	{
		if(errno != EAGAIN && errno != EINTR)
		{
		}
		printf("%s[%d]\t%s: accept return:%d, errno:%d\n", __FILE__,__LINE__,__func__,nfd,errno);
		return;
	}
	send(notify_fd[(++how_many_client) % PROCESS_THREAD][0],&nfd,sizeof(nfd), 0);
}

void AcceptUpdateConn(int fd, short int events, void *arg)
{
	struct sockaddr_in sin;
	struct event *p_ev = (struct event *)arg;
	/* Reschedule this event */
	event_add(p_ev, NULL);
	socklen_t len = sizeof(struct sockaddr_in);
	int nfd, i;
	// accept 
	if((nfd = accept(fd, (struct sockaddr*)&sin, &len)) <= 0)
	{
		if(errno != EAGAIN && errno != EINTR)
		{
		}
		printf("%s[%d]\t%s: accept return:%d, errno:%d\n", __FILE__,__LINE__,__func__,nfd,errno);
		return;
	}
	send(update_notify_fd[(++update_how_many_client) % UPDATE_PROCESS_THREAD][0],&nfd,sizeof(nfd), 0);
}

void InitListenSocket(struct event_base *base, short port) 
{
	struct event * p_listen_ev = &listen_ev;
	if (0 == p_listen_ev) return ;
	int listenFd = socket(AF_INET, SOCK_STREAM, 0); 
	int ret = 0;
	fcntl(listenFd, F_SETFL, O_NONBLOCK); // set non-blocking 
	reg_add_event(p_listen_ev,listenFd,EV_READ,AcceptConn,p_listen_ev,base,NULL);
	// bind & listen 
	sockaddr_in sin; 
	bzero(&sin, sizeof(sin)); 
	sin.sin_family = AF_INET; 
	sin.sin_addr.s_addr = INADDR_ANY; 
	sin.sin_port = htons(port); 
	bind(listenFd, (const sockaddr*)&sin, sizeof(sin)); 
	ret = listen(listenFd, LISTEN_PENDING_QUEUE_LENGTH); 
	printf("server listen fd=%d port=%d retrun:%d errno:%d\n", listenFd,port,ret,errno);
}

void InitUpdateListenSocket(struct event_base *base, short port) 
{
	struct event * p_listen_ev = &update_listen_ev;
	if (0 == p_listen_ev) return ;
	int listenFd = socket(AF_INET, SOCK_STREAM, 0); 
	int ret = 0;
	fcntl(listenFd, F_SETFL, O_NONBLOCK); // set non-blocking 
	reg_add_event(p_listen_ev,listenFd,EV_READ,AcceptUpdateConn,p_listen_ev,base,NULL);
	// bind & listen 
	sockaddr_in sin; 
	bzero(&sin, sizeof(sin)); 
	sin.sin_family = AF_INET; 
	sin.sin_addr.s_addr = INADDR_ANY; 
	sin.sin_port = htons(port); 
	bind(listenFd, (const sockaddr*)&sin, sizeof(sin)); 
	ret = listen(listenFd, LISTEN_PENDING_QUEUE_LENGTH); 
	printf("server listen update fd=%d port=%d retrun:%d errno:%d\n", listenFd,port,ret,errno);
}

///////////process thread/////////////////

	static void
evsignal_cb(int fd, short what, void *arg)
{
	struct event *p_ev = (struct event *)arg;
	static char msg[sizeof(int)];
	ssize_t n;

	n = recv(fd,msg, sizeof(msg), 0);
	if (n == sizeof(msg))
	{
		//struct event *p_read_event = event_pool_get();
		struct interface_data *p_read_it = it_pool_get();
		int nfd = *(int *)msg;

		int iret = 0;
		if((iret = fcntl(nfd, F_SETFL, O_NONBLOCK)) < 0)
		{   
			printf("%s: fcntl nonblocking failed:%d\n", __func__, iret);
			return;
		}   
		// add a read event for receive data
		p_read_it->sd = nfd;
		reg_add_event(&(p_read_it->ev),nfd,EV_READ,RecvData,p_read_it,p_ev->ev_base,&tv_recv_timeout);
		printf("process thread read new client fd[%d]\n",nfd);

	}
	else 
	{
		printf("%s[%d]\t%s:  return:%d, errno:%d", __FILE__,__LINE__,__func__,n,errno);

	}

}

static void *ProcessThread(void *arg)
{
	if (0 == arg) pthread_exit(NULL);
	struct sig_ev_arg *p_ev_arg = (struct sig_ev_arg *)arg;
	struct event notify_event;
	struct event_base *base = event_init();
	reg_add_event(&notify_event,p_ev_arg->notify_fd,EV_READ | EV_PERSIST, evsignal_cb, &notify_event,base,NULL);
	event_base_dispatch(base);
	pthread_exit(NULL);
}


////////

int main (int argc, char **argv)
{
	int socket;
	/* Initalize the event library */
	struct event_base *base = event_init();
	short port = SEARCH_PORT;
	short update_port = UPDATE_PORT;
	//init timeout
	tv_recv_timeout.tv_sec = 3;
	tv_recv_timeout.tv_usec = 0;
	tv_send_timeout.tv_sec = 0;
	tv_send_timeout.tv_usec = 10000;
	tv_con_timeout.tv_sec = 0;
	tv_con_timeout.tv_usec = 100000;
	//create threads
	for(int i = 0 ; i < PROCESS_THREAD;i++)
	{
		if (evutil_socketpair(AF_UNIX, SOCK_STREAM, 0,notify_fd[i]) == -1) 
		{perror("socketpair error");
			return -1;
		}

		FD_CLOSEONEXEC(notify_fd[i][0]);
		FD_CLOSEONEXEC(notify_fd[i][1]);
		evutil_make_socket_nonblocking(notify_fd[i][0]);
		notify_arg[i].notify_fd = notify_fd[i][1];
		int err = pthread_create(&ptocess_thread[i],0,ProcessThread,&notify_arg[i]);
		if (err != 0)
		{
			printf("can't create ProcessThread thread:%s\n",strerror(err));
			return -1;
		}
	}
	//create update threads
	for(int i = 0 ; i < UPDATE_PROCESS_THREAD;i++)
	{
		if (evutil_socketpair(AF_UNIX, SOCK_STREAM, 0,update_notify_fd[i]) == -1) 
		{perror("socketpair error");
			return -1;
		}

		FD_CLOSEONEXEC(update_notify_fd[i][0]);
		FD_CLOSEONEXEC(update_notify_fd[i][1]);
		evutil_make_socket_nonblocking(update_notify_fd[i][0]);
		update_notify_arg[i].notify_fd = update_notify_fd[i][1];
		int err = pthread_create(&update_ptocess_thread[i],0,ProcessThread,&update_notify_arg[i]);
		if (err != 0)
		{
			printf("can't create Update ProcessThread thread:%s\n",strerror(err));
			return -1;
		}
	}
	//use signal tell thread com a new client fd	
	InitListenSocket(base, port); 
	InitUpdateListenSocket(base, update_port); 
	event_base_dispatch(base);
	for (int i = 0 ; i < PROCESS_THREAD;i++)
	{
		pthread_join(ptocess_thread[i],0);
	}
	return (0);
}

