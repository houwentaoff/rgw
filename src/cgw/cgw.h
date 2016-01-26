/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  cgw.h
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  Sunday, January 10, 2016 10:39:52 CST
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */
#ifndef __CGW_H__
#define __CGW_H__
//common

typedef enum
{
    CGW_MSG_GET_PASSWORD = 1,
    CGW_MSG_SET_VOLUME,
    CGW_MSG_GET_VOLUME,
    CGW_MSG_DEL_VOLUME,
    CGW_MSG_EXIT,
}cgw_msg_id_t;

string pack(const char *name, const char *value);
string get_val(const char *s, const char *key);

//cgw server use
void *listen_loop(void* arg);
void *main_loop(void* arg);
typedef struct {
    int (*getpawd)(char *user, int sock_fd, ssize_t (*complete)(int fd, const void *buf, size_t count));
    int (*getvol)(char *vol_name, int sock_fd, ssize_t (*complete)(int fd, const void *buf, size_t count));
    int (*delvol)(char *vol_name, int sock_fd, ssize_t (*complete)(int fd, const void *buf, size_t count));
    int (*setvol)(char *vol_name, int sock_fd, ssize_t (*complete)(int fd, const void *buf, size_t count));
}cgw_api_t;
//cgw client use
int post_msg(int msg_id, const char payload[], int payload_size, bool bclose);
int recv_msg(int fd, char *msg,  bool bclose);



#endif

