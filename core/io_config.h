/***************************************************************************
 * 
 * Copyright (c) 2015 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file config.h
 * @date 2015/07/31 10:11:57
 * @brief 
 *  
 **/




#ifndef  __CONFIG_H_
#define  __CONFIG_H_


#define MICRO_SEC 1000000
#define MILL_SEC 1000

#define CON_TIME ( MICRO_SEC / MILL_SEC * 200)
#define RECV_TIME ( MICRO_SEC / MILL_SEC * 200)
#define SEND_TIME ( MICRO_SEC / MILL_SEC * 200)

#define PROCESS_THREAD 3
#define UPDATE_PROCESS_THREAD 1

#define BUFF_SIZE (1024 * 4)
#define SEARCH_PORT 8807
#define UPDATE_PORT 8907
#define LISTEN_PENDING_QUEUE_LENGTH (-1)

enum status_code
{
    status_init = 0,
    status_recv = 1,
    status_recv_done = 2,
    status_send = 3,
    status_send_done = 4,
    status_process = 5,
    status_process_done = 6
};

void RecvData(int fd, short int events, void *arg);
void SendData(int fd, short int events, void *arg);

#define PRINT_ME do { \
printf("file:%s[%d]\t func:%s errno:%d\n", __FILE__,__LINE__,__func__,errno); \
} while(0) 

typedef void (* event_cb_func)(int fd, short int events, void *arg);



#endif  //__CONFIG_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
