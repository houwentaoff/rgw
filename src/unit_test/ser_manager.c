/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  ser_manager.c
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  Sunday, January 10, 2016 12:13:23 CST
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

#define FIFO_READ_TIMEOUT   5
#define MAX_STRING_SIZE 512
#define SERVER_MANAGER_FIFO_FILE "/tmp/server_manager"
#define SHELL_FIFO_FILE          "/tmp/status"            /*  */
#define RGW_FIFO_FILE            "/tmp/rgw"            /*  */

static int g_main_loop_running = 1;

static int is_fifo (const char* file )
{
    struct stat sbuf;
    int ret;

    ret = stat ( file, &sbuf );
    if ( ret < 0 )
    {
        return 0;
    }

    if ( S_ISFIFO ( sbuf.st_mode ) )
    {
        return 1;
    }

    return 0;
}

static int open_fifo (const char* file, int flag )
{
    int fd = -1;
    int ret;

    ret = is_fifo ( file );
    if ( ret == 0 )
    {
        unlink ( file );
        mkfifo ( file, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH );
    }
    errno = 0;
    fd = open ( file, flag );
    if ( fd == -1 )
    {
        fd = -errno;
    }
    return fd;
}
#define FROM_SHELL           "shell"            /*  */
#define FROM_RGW           "rgw"            /*  */
static void execute_command(char *command)
{
    char *prefix_end=NULL;
    int fd = -1;

    printf("Excuting command is [%s]\n", command);
    prefix_end = strchr(command, ':');
    if (prefix_end==NULL)
        return ;
    *prefix_end = '\0';
    if (strcmp(command, FROM_SHELL) == 0)
    {
        fd = open_fifo(SHELL_FIFO_FILE, O_WRONLY | O_NONBLOCK);
        if (fd < 0)
        {
            printf("open fail\n");
            return;
        }
        write(fd, "this is ok\nthis is ok\n", 22);
        close(fd);
    }
    else if (strcmp(command, FROM_RGW))
    {
       fd = open_fifo(RGW_FIFO_FILE, O_WRONLY | O_NONBLOCK);
    }
    return ;
}

static void start_main_loop()
{
    int  fdr = -1;

    if ( access(SERVER_MANAGER_FIFO_FILE, R_OK) < 0 )
    {
        unlink(SERVER_MANAGER_FIFO_FILE);
        if ( mkfifo(SERVER_MANAGER_FIFO_FILE, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) < 0 )
        {
            printf("mkfifo ["SERVER_MANAGER_FIFO_FILE"] failed!");
            return;
        }
    }
    if ( access(SHELL_FIFO_FILE, R_OK) < 0 )
    {
        unlink(SHELL_FIFO_FILE);
        if ( mkfifo(SHELL_FIFO_FILE, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) < 0 )
        {
            printf("mkfifo ["SHELL_FIFO_FILE"] failed!");
            return;
        }
    }
    if ( access(RGW_FIFO_FILE, R_OK) < 0 )
    {
        unlink(RGW_FIFO_FILE);
        if ( mkfifo(RGW_FIFO_FILE, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) < 0 )
        {
            printf("mkfifo ["RGW_FIFO_FILE"] failed!");
            return;
        }
    }

    do
    {
        fdr = open_fifo(SERVER_MANAGER_FIFO_FILE, O_RDONLY | O_NONBLOCK);
        if ( fdr < 0 )
        {
            printf("open fifio ["SERVER_MANAGER_FIFO_FILE"] as read failed!");
            break;
        }


        while ( g_main_loop_running )
        {
            int ret = 0;
            fd_set fdset;
            struct timeval timeout;

            if (fdr >= 0)
            {
                FD_ZERO(&fdset);
                FD_SET(fdr, &fdset);

                timeout.tv_sec = FIFO_READ_TIMEOUT;
                timeout.tv_usec = 0;
                ret = select(fdr + 1, &fdset, NULL, NULL, &timeout);
                if ( ret <= 0 )
                {
                    // Auto check server status in free time.
//                    update_server_status();
                    continue;
                }

                if ( FD_ISSET(fdr, &fdset) )
                {
                    char command[MAX_STRING_SIZE] = {0};
                    int totalread=0;
                    int retval;
                    char* pCur=command;
                    int leftsize=sizeof(command);
                    memset(command,0,sizeof(command));
                    do
                    {
                        retval = read(fdr,pCur,leftsize);
                        if ( retval >0 )
                        {
                            totalread += retval;
                            pCur += retval;
                            leftsize -= retval;
                        }
                    }while(retval > 0);

                    command[totalread] = '\0';
                    if (totalread > 0)
                    {
                        char send_buf[100]={"status is ok\n"};
                        int retw = -1;
                        execute_command(command);

//                        retw = write(fdr, send_buf, strlen(send_buf));
//                        if (retw <strlen(send_buf)) perror("write error?\n");
//                        printf("write len [%d]\n", retw);

                    }
                    else
                    {
                        /*it not read any chars ,so reopen it ,let it be not read fifo continuously*/
                        close(fdr);
                        fdr = -1;

                        errno = 0;
                        fdr = open_fifo(SERVER_MANAGER_FIFO_FILE, O_RDONLY | O_NONBLOCK);
                        if (fdr < 0)
                        {
                            printf("open fifio ["SERVER_MANAGER_FIFO_FILE"] as read failed %d %m!",errno);
                        }
                    }
                }
            }
            else
            {
                /*we should reopen the fifo first sleep*/
                sleep(FIFO_READ_TIMEOUT);
                errno = 0;
                fdr = open_fifo(SERVER_MANAGER_FIFO_FILE, O_RDONLY | O_NONBLOCK);
                if (fdr < 0)
                {
                    printf("open fifio ["SERVER_MANAGER_FIFO_FILE"] as read failed %d %m!",errno);
                }
                /*update server status */
//                update_server_status();
            }
        }
    } while ( 0 );



    if ( fdr >= 0 )
    {
        close(fdr);
        fdr = -1;
    }
}
static void sigstop(int signo)
{
    printf("signal %d is captured, to exit!", signo);
    g_main_loop_running = 0;
    sleep(1);
}

int main(int argc, const char *argv[])
{
    unsigned int i = 0;
    int ret = 0;

    signal(SIGINT, sigstop);
    signal(SIGQUIT, sigstop);
    signal(SIGTERM, sigstop);
//    signal(SIGCHLD, sigchildstop);


//APP_INFO(">>>>>>>>>>>>>>>>>>>>>>>>>>>> Start Server Manager ...");
#if 0
    ret = init_params(argc, argv);
    if ( ret != 0 )
    {
        print_usage();
        return ret > 0 ? 0 : -1;
    }
#endif
    start_main_loop();
    // note: if stop snmp, when to restart?
    // system("/etc/init.d/s50snmp stop");
    return 0;
}

