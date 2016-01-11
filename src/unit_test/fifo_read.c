/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  fifo_read.c
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  Sunday, January 10, 2016 01:15:08 CST
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>

#include <errno.h>
#include <stdlib.h>

#define SERVER_MANAGER_FIFO_FILE "/tmp/__server_manager_fifo_file__"

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
int main ( int argc, char *argv[] )
{
    int fd;
    char buf[512];
    int retr = -1;
    int retw = -1;

    fd = open("/tmp/status", O_RDWR|O_NONBLOCK );
    if (fd < 0)
    {
        perror("open fail\n");
    }
//    retw = write(fd, "hello world!\n", 14);
//    while (1)
//    {
        sleep(7);
        retr = read(fd, buf, sizeof(buf));
        printf("retw[%d]retr[%d]read :%s\n", retw, retr, buf);
        close(fd);
//    }
        
    return EXIT_SUCCESS;
}
