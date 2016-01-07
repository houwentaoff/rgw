/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  setuid2.cpp
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  01/07/2016 12:14:44 AM
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <iostream>

using namespace std;
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
int main ( int argc, char *argv[] )
{
    int fd = -1;
    char buf[256]={"hello world!"};
    int len;
    
    cout<<"uid:"<<getuid()<<endl;
    cout<<"euid:"<<geteuid()<<endl;
    cout<<"gid:"<<getgid()<<endl;
    
    cout<<"set gid uid euid"<<endl;
    if (setuid(502) == -1)
    {
        perror("set uid fail\n");
        goto err;
    }
#if 0   
    if (seteuid(0xFFFFFFFF) == -1)
    {
        perror("set euid fail\n");
        goto err;
    }
    if (setgid(502) == -1)
    {
        perror("set gid fail\n");
        goto err;
    }
#endif

    cout<<"uid:"<<getuid()<<endl;
    cout<<"euid:"<<geteuid()<<endl;
    cout<<"gid:"<<getgid()<<endl;
    if ((fd = open("/fisamba/s3_test/abcde.txt", O_CREAT|O_TRUNC|O_RDWR, 0777)) < 0)
    {
        perror("open fail\n");
        goto err;
    }
    len = strlen(buf)+1;
    if (len != write(fd, (void *)buf, len))
    {
        perror("write loss?\n");
    }
    close(fd);

    do
    {
        sleep(1);
    }while(1);

    return EXIT_SUCCESS;
err:
    return -1;
}
