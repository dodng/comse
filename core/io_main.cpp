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
#include "easy_log.h"
#include "io_config.h"

//////////////base structure///////////////////////////
/////

//struct timeval tv; 
struct event listen_ev;
pthread_t ptocess_thread[MAX_PROCESS_THREAD] = {0};
/*notification mechanism*/
int notify_fd[MAX_PROCESS_THREAD][2] = {0};
int how_many_client = 0;

/////////////////////////////////////////////////////////

struct event update_listen_ev;
//+1 use to handle accept thread 
pthread_t update_ptocess_thread[UPDATE_PROCESS_THREAD + 1] = {0};
/*notification mechanism*/
int update_notify_fd[UPDATE_PROCESS_THREAD][2] = {0};
int update_how_many_client = 0;

struct sig_ev_arg notify_arg[MAX_PROCESS_THREAD] = {0};
struct sig_ev_arg update_notify_arg[UPDATE_PROCESS_THREAD] = {0};

easy_log g_log("./log/comse.log",50000,8);
/////////////////////////////////////////////////////////

int g_recv_buff = (1024 * 4);
int g_send_buff = (1024 * 4);

short g_search_port = 8807;
short g_update_port = 8907;
char g_search_ip[64] = "0.0.0.0";
char g_update_ip[64] = "0.0.0.0";
int g_listen_backlog = -1;

//timeout
struct timeval tv_con_timeout = {0};
struct timeval tv_recv_timeout = {3,0};
struct timeval tv_send_timeout = {0,10000};

//search thread num
int g_search_thread_num = 3;

/////////////////////////////////////////////////////////////

struct interface_data * it_pool_get()
{
	//	printf("it_pool_get\n");
	return new interface_data(g_recv_buff,g_send_buff);
}

void it_pool_put(struct interface_data *p_it)
{
	if (p_it)
		delete p_it;
	//	printf("it_pool_put\n");
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
#if 0
int get_sock_error(int fd)
{
	int error;
	socklen_t len;

	len=sizeof(error);
	getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len);
	return error;

}
#endif
//////////////
void SendData(int fd, short int events, void *arg)
{
	if ( 0 == arg || fd <= 0)
		return;
	struct interface_data *p_it = (struct interface_data *)arg;
	struct event *p_ev = &(p_it->ev);
	struct event_base *base = p_ev->ev_base;
	char *buff = (char *)(p_it->send_buff);
	int buff_len = p_it->send_buff_len;
	int len;
	char log_buff[BUFF_SIZE] = {0};

	if (events == EV_TIMEOUT)
	{
		//		printf("send timeout fd=[%d] error[%d]\n", fd, errno);
		snprintf(log_buff,sizeof(log_buff),"send timeout fd=[%d] error[%d]", fd, errno);
		g_log.write_record(log_buff);
		goto GC;
	} 

	len = send(fd, buff, p_it->now_send_len, 0);
	event_del(p_ev);

	if(len >= 0)
	{
		//		printf("Thread[%x]\tClient[%d]:Send=%s\n",(int)pthread_self(),fd, buff);
		snprintf(log_buff,sizeof(log_buff),"Thread[%x]\tClient[%d]:Send=%s",(int)pthread_self(),fd, buff);
		g_log.write_record(log_buff);
		//send success
		p_it->status = status_send;
		//reset
		p_it->reset();
		//regist recv event
		reg_add_event(p_ev,p_it->sd,EV_READ,RecvData,p_it,base,&tv_recv_timeout);
		//flush stdout
		fflush(stdout);
		//		goto GC;
	}
	else
	{
		//		printf("talk over[fd=%d] error[%d]:%s\n", fd, errno, strerror(errno));
		snprintf(log_buff,sizeof(log_buff),"talk over[fd=%d] error[%d]:%s", fd, errno, strerror(errno));
		g_log.write_record(log_buff);
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
	char *buff = (char *)(p_it->recv_buff) + p_it->now_recv_len;
	int buff_len = p_it->recv_buff_len - p_it->now_recv_len;
	int len;
	char log_buff[BUFF_SIZE] = {0};

	//timeout
	if (events == EV_TIMEOUT)
	{
		close(p_ev->ev_fd);
		event_del(p_ev);
		it_pool_put(p_it);
		//		printf("recv timeout fd=[%d] error[%d]\n", fd, errno);
		snprintf(log_buff,sizeof(log_buff),"recv timeout fd=[%d] error[%d]", fd, errno);
		g_log.write_record(log_buff);
		return;
	} 

	// clear buff
	//	memset(buff,0,buff_len);
	// receive data
	len = recv(fd,buff, buff_len, 0);
	event_del(p_ev);

	if(len > 0)
	{
		//read success
		p_it->status = status_recv;
		//regist send event
		p_it->now_recv_len += len;
		//		int parse_ret = p_it->http.parse_done(buff);
		int parse_ret = p_it->http.parse_done((char *)(p_it->recv_buff));
		snprintf(log_buff,sizeof(log_buff),"Thread[%x]\tClient[%d]:Recv=[%s]:Parse=[%d]",(int)pthread_self(),fd, buff, parse_ret);
		g_log.write_record(log_buff);

		if (parse_ret < 0 )
		{goto GC;}
		else if (parse_ret == 0)	
		{reg_add_event(p_ev,p_it->sd,EV_READ,RecvData,p_it,base,&tv_recv_timeout);}
		else
		{
			p_it->policy.do_one_action(&p_it->http,p_it->send_buff,p_it->send_buff_len,p_it->now_send_len);

			reg_add_event(p_ev,p_it->sd,EV_WRITE,SendData,p_it,base,&tv_send_timeout);}

	}
	else
	{
		if (len == 0)
		{
			snprintf(log_buff,sizeof(log_buff),"[fd=%d] , closed gracefully.", fd);
			g_log.write_record(log_buff);
		}
		else
		{	
			snprintf(log_buff,sizeof(log_buff),"recv[fd=%d] error[%d]:%s", fd, errno, strerror(errno));
			g_log.write_record(log_buff);
		}
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
	char log_buff[128] = {0};
	// accept 
	if((nfd = accept(fd, (struct sockaddr*)&sin, &len)) <= 0)
	{
		if(errno != EAGAIN && errno != EINTR)
		{
		}
		snprintf(log_buff,sizeof(log_buff),"%s[%d]\t%s: accept return:%d, errno:%d", __FILE__,__LINE__,__func__,nfd,errno);
		g_log.write_record(log_buff);
		return;
	}
	send(notify_fd[(++how_many_client) % g_search_thread_num][0],&nfd,sizeof(nfd), 0);
}

void AcceptUpdateConn(int fd, short int events, void *arg)
{
	struct sockaddr_in sin;
	struct event *p_ev = (struct event *)arg;
	/* Reschedule this event */
	event_add(p_ev, NULL);
	socklen_t len = sizeof(struct sockaddr_in);
	int nfd, i;
	char log_buff[128] = {0};
	// accept 
	if((nfd = accept(fd, (struct sockaddr*)&sin, &len)) <= 0)
	{
		if(errno != EAGAIN && errno != EINTR)
		{
		}
		snprintf(log_buff,sizeof(log_buff),"%s[%d]\t%s: accept return:%d, errno:%d", __FILE__,__LINE__,__func__,nfd,errno);
		g_log.write_record(log_buff);
		return;
	}
	send(update_notify_fd[(++update_how_many_client) % UPDATE_PROCESS_THREAD][0],&nfd,sizeof(nfd), 0);
}

void InitListenSocket(struct event_base *base,char *ip, short port) 
{
	struct event * p_listen_ev = &listen_ev;
	if (0 == p_listen_ev || 0 == ip) return ;
	int listenFd = socket(AF_INET, SOCK_STREAM, 0); 
	int ret = 0;
	char log_buff[128] = {0};
	fcntl(listenFd, F_SETFL, O_NONBLOCK); // set non-blocking 
	reg_add_event(p_listen_ev,listenFd,EV_READ,AcceptConn,p_listen_ev,base,NULL);
	// bind & listen 
	sockaddr_in sin; 
	bzero(&sin, sizeof(sin)); 
	sin.sin_family = AF_INET; 
	//	sin.sin_addr.s_addr = INADDR_ANY; 
	sin.sin_addr.s_addr = inet_addr(ip);
	sin.sin_port = htons(port); 
	bind(listenFd, (const sockaddr*)&sin, sizeof(sin)); 
	ret = listen(listenFd, g_listen_backlog); 
	//	printf("server listen fd=%d port=%d retrun:%d errno:%d\n", listenFd,port,ret,errno);
	snprintf(log_buff,sizeof(log_buff),"server listen fd=%d port=%d retrun:%d errno:%d", listenFd,port,ret,errno);
	g_log.write_record(log_buff);
}

void InitUpdateListenSocket(struct event_base *base, char *ip, short port) 
{
	struct event * p_listen_ev = &update_listen_ev;
	if (0 == p_listen_ev || 0 == ip) return ;
	int listenFd = socket(AF_INET, SOCK_STREAM, 0); 
	int ret = 0;
	char log_buff[128] = {0};
	fcntl(listenFd, F_SETFL, O_NONBLOCK); // set non-blocking 
	reg_add_event(p_listen_ev,listenFd,EV_READ,AcceptUpdateConn,p_listen_ev,base,NULL);
	// bind & listen 
	sockaddr_in sin; 
	bzero(&sin, sizeof(sin)); 
	sin.sin_family = AF_INET; 
	sin.sin_addr.s_addr = inet_addr(ip);
	sin.sin_port = htons(port); 
	bind(listenFd, (const sockaddr*)&sin, sizeof(sin)); 
	ret = listen(listenFd, g_listen_backlog); 
	//	printf("server listen update fd=%d port=%d retrun:%d errno:%d\n", listenFd,port,ret,errno);
	snprintf(log_buff,sizeof(log_buff),"server listen update fd=%d port=%d retrun:%d errno:%d", listenFd,port,ret,errno);
	g_log.write_record(log_buff);
}

///////////process thread/////////////////

	static void
evsignal_cb(int fd, short what, void *arg)
{
	struct event *p_ev = (struct event *)arg;
	static char msg[sizeof(int)];
	ssize_t n;
	char log_buff[128] = {0};

	n = recv(fd,msg, sizeof(msg), 0);
	if (n == sizeof(msg))
	{
		//struct event *p_read_event = event_pool_get();
		struct interface_data *p_read_it = it_pool_get();
		int nfd = *(int *)msg;

		int iret = 0;
		if((iret = fcntl(nfd, F_SETFL, O_NONBLOCK)) < 0)
		{   
			snprintf(log_buff,sizeof(log_buff),"%s: fcntl nonblocking failed:%d", __func__, iret);
			g_log.write_record(log_buff);
			//			printf("%s: fcntl nonblocking failed:%d\n", __func__, iret);
			return;
		}   
		// add a read event for receive data
		p_read_it->sd = nfd;
		reg_add_event(&(p_read_it->ev),nfd,EV_READ,RecvData,p_read_it,p_ev->ev_base,&tv_recv_timeout);
		//		printf("process thread read new client fd[%d]\n",nfd);
		snprintf(log_buff,sizeof(log_buff),"process thread read new client fd[%d]",nfd);
		g_log.write_record(log_buff);

	}
	else 
	{
		//		printf("%s[%d]\t%s:  return:%d, errno:%d", __FILE__,__LINE__,__func__,n,errno);
		snprintf(log_buff,sizeof(log_buff),"%s[%d]\t%s:  return:%d, errno:%d", __FILE__,__LINE__,__func__,n,errno);
		g_log.write_record(log_buff);

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

static void *UpdateThreadHandleAccept(void *arg)
{
	struct event_base *base = event_init();
	short update_port = g_update_port;
	char * update_ip = g_update_ip;
	InitUpdateListenSocket(base, update_ip, update_port); 
	event_base_dispatch(base);
	pthread_exit(NULL);

}

////////

int main (int argc, char **argv)
{
	/* Initalize the event library */
	struct event_base *base = event_init();
	short port = g_search_port;
	char *ip = g_search_ip;
	char log_buff[128] = {0};
	//create threads
	for(int i = 0 ; i < g_search_thread_num;i++)
	{
		if (evutil_socketpair(AF_UNIX, SOCK_STREAM, 0,notify_fd[i]) == -1) 
		{
			//perror("socketpair error");
			snprintf(log_buff,sizeof(log_buff),"%s[%d]\t%s:  socketpair error", __FILE__,__LINE__,__func__);
			g_log.write_record(log_buff);
			return -1;
		}

		FD_CLOSEONEXEC(notify_fd[i][0]);
		FD_CLOSEONEXEC(notify_fd[i][1]);
		evutil_make_socket_nonblocking(notify_fd[i][0]);
		notify_arg[i].notify_fd = notify_fd[i][1];
		int err = pthread_create(&ptocess_thread[i],0,ProcessThread,&notify_arg[i]);
		if (err != 0)
		{
			//			printf("can't create ProcessThread thread:%s\n",strerror(err));
			snprintf(log_buff,sizeof(log_buff),"can't create ProcessThread thread:%s",strerror(err));
			g_log.write_record(log_buff);
			return -1;
		}
	}
	//create update threads
	for(int i = 0 ; i < UPDATE_PROCESS_THREAD;i++)
	{
		if (evutil_socketpair(AF_UNIX, SOCK_STREAM, 0,update_notify_fd[i]) == -1) 
		{
			//perror("socketpair error");
			snprintf(log_buff,sizeof(log_buff),"%s[%d]\t%s:  socketpair error", __FILE__,__LINE__,__func__);
			g_log.write_record(log_buff);
			return -1;
		}

		FD_CLOSEONEXEC(update_notify_fd[i][0]);
		FD_CLOSEONEXEC(update_notify_fd[i][1]);
		evutil_make_socket_nonblocking(update_notify_fd[i][0]);
		update_notify_arg[i].notify_fd = update_notify_fd[i][1];
		int err = pthread_create(&update_ptocess_thread[i],0,ProcessThread,&update_notify_arg[i]);
		if (err != 0)
		{
			//	printf("can't create Update ProcessThread thread:%s\n",strerror(err));
			snprintf(log_buff,sizeof(log_buff),"can't create Update ProcessThread thread:%s",strerror(err));
			g_log.write_record(log_buff);
			return -1;
		}
	}
	//create update thread to hanlde accept
	for (int i = UPDATE_PROCESS_THREAD;i < UPDATE_PROCESS_THREAD + 1 ;i++)
	{
		int err = pthread_create(&update_ptocess_thread[i],0,UpdateThreadHandleAccept,0);
		if (err != 0)
		{
			//			printf("can't create UpdateThreadHandleAccept:%s\n",strerror(err));
			snprintf(log_buff,sizeof(log_buff),"can't create UpdateThreadHandleAccept:%s",strerror(err));
			g_log.write_record(log_buff);
			return -1;
		}
	}
	//use signal tell thread com a new client fd	
	InitListenSocket(base, ip, port); 
	event_base_dispatch(base);
	for (int i = 0 ; i < g_search_thread_num;i++)
	{
		pthread_join(ptocess_thread[i],0);
	}
	for (int i = 0 ; i < UPDATE_PROCESS_THREAD + 1;i++)
	{
		pthread_join(update_ptocess_thread[i],0);
	}
	return (0);
}

