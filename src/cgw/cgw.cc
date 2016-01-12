/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  cgw.cc
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  Sunday, January 10, 2016 06:12:03 CST
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <string.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/un.h>

#include "cgw.h"

#define CMD_SERVICE  "/tmp/cmd_server"
#define MSG_QUEUE_SIZE        10
#define CGW_MSG_SIZE  128            /*  */


typedef struct cgw_msg_s
{
    cgw_msg_id_t msg_id;
    char param[128];
    int sock_fd;
}cgw_msg_t;


cgw_api_t cgw_ops;// = 
//{
//    .getpawd    = getpawd;
//};

int com_id_exit_flag = 0;
int id_exit_flag = 0;
cgw_msg_t msg[MSG_QUEUE_SIZE];
int msg_update;

struct sockaddr_un clt_addr, srv_addr;

pthread_mutex_t    update_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t    msg_mutex = PTHREAD_MUTEX_INITIALIZER;

int listen_socket()
{
    int listen_fd;

    unlink(CMD_SERVICE);
    if((listen_fd = socket(PF_UNIX, SOCK_STREAM, 0))<0) {
        perror("listen socket uninit\n");
        return -1;
    }

    srv_addr.sun_family = AF_UNIX;
    strncpy(srv_addr.sun_path,CMD_SERVICE,sizeof(srv_addr.sun_path)-1);

    if((bind(listen_fd,(struct sockaddr *)&srv_addr,sizeof(srv_addr)))<0) {
        perror("cannot bind srv socket\n");
        return -1;
    }

    if(listen(listen_fd, 1)<0) {
        perror("cannot listen");
        close(listen_fd);
        unlink(CMD_SERVICE);
        return -1;
    }

    return listen_fd;
}
int recv_msg(int fd, char *msg, bool bclose)
{
    int num;
    socklen_t len;
    len = sizeof(clt_addr);
    num = read(fd, msg, CGW_MSG_SIZE+sizeof(int));
    if (bclose)
        close(fd);
    return num;
}
int receive_msg(int fd, char *msg, int *cli_fd, bool bclose)
{
    int com_fd, num;
    socklen_t len;

    len = sizeof(clt_addr);
    com_fd = accept(fd,(struct sockaddr*)&clt_addr,&len);

    if(com_fd < 0){
        perror("cannot accept client connect request");
        close(com_fd);
        return -1;
    }
    *cli_fd = com_fd; 
    num = read(com_fd, msg, CGW_MSG_SIZE+sizeof(int));
    
    if (bclose)
        close(com_fd);

    return num;
}

void *listen_loop(void* arg)
{
    int fd, num, com_fd;
    char buffer[CGW_MSG_SIZE+sizeof(int)];

    fd = listen_socket();
    if (fd < 0) {
        printf("Incorrect socket fd!\n");
//        listen_loop_error_exit = 1;
//        sem_post(&img_sem);
        return NULL;
    }
//    listen_loop_error_exit = 0;
//    sem_post(&img_sem);
    while (!com_id_exit_flag) {
        if(msg_update >MSG_QUEUE_SIZE-1)
        {
            usleep(2000);
            continue;
        }
        num = receive_msg(fd, buffer, &com_fd, false);
        if (num > 0) {
            pthread_mutex_lock(&msg_mutex);
            msg_update++;
            msg[msg_update-1].msg_id = (cgw_msg_id_t)*(int *)&buffer[0];
            memcpy(msg[msg_update-1].param, &buffer[sizeof(int)], num-sizeof(int));
            msg[msg_update-1].sock_fd = com_fd;
        //    printf("accept %d\n", msg_update);
            pthread_mutex_unlock(&msg_mutex);
        }
    }
    close(fd);
    return NULL;
}
int cgw_msg_handler(cgw_msg_t *p_msg, cgw_api_t *p_cgw_api)
{
    switch(p_msg->msg_id)
    {
        case CGW_MSG_GET_PASSWORD:
            p_cgw_api->getpawd(p_msg->param, p_msg->sock_fd, write);
            break;
        default:
            printf("error msg\n");
            break;
    }
    return 0;
}
void *main_loop(void* arg)
{
    int count;
    while(!id_exit_flag) 
    {
        if(pthread_mutex_trylock (&msg_mutex) ==0)//unlocked
        {
            count = 0;
            while(msg_update >0) {
                cgw_msg_handler(&msg[count], &cgw_ops);
            //    printf("process msg_update %d ,count %d\n", msg_update, count);
                msg_update --;
                count++;
            }
            pthread_mutex_unlock (&msg_mutex);
        }
        
    }
    return NULL;
}
int post_msg(int msg_id, const char payload[], int payload_size, bool bclose)
{
    int connect_fd, ret;
    char buffer[sizeof(int) + CGW_MSG_SIZE];

    if(payload_size>(CGW_MSG_SIZE-1)||payload_size<0)
        return -1;

    connect_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if(connect_fd<0) {
        perror("comm socket uninit");
        return -1;
    }

    srv_addr.sun_family = AF_UNIX;
    strcpy((char*)&srv_addr.sun_path, CMD_SERVICE);
    ret = connect(connect_fd,(struct sockaddr*)&srv_addr,sizeof(srv_addr));

    if(ret == -1){
        perror("cannot connect to the server");
        return -1;
    }
    *(int *)&buffer[0] = msg_id;
    memcpy(&buffer[sizeof(int)], payload, payload_size);
    write(connect_fd, buffer, payload_size+sizeof(int));
    
    if (bclose)
        close(connect_fd);
    return connect_fd;
}


