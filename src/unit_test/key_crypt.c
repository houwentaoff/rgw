/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  key_crypt.c
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  01/07/2016 10:28:38 PM
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
#define _XOPEN_SOURCE
#include <unistd.h>
#include <shadow.h>
#include <errno.h>
#include <string.h>

#include <crypt.h>

void help(void)
{
    printf("用户密码验证程序\n第一个参数为用户名!\n");
    exit(-1);
}

void error_quit(char *msg)
{
    perror(msg);
    exit(-2);
}

void get_salt(char *salt,char *passwd)
{
    int i,j;

    //取出salt,i记录密码字符下标,j记录$出现次数
    for(i=0,j=0;passwd[i] && j != 3;++i)
    {
        if(passwd[i] == '$')
            ++j;
    }
    if (j == 0)
        strcpy(salt, passwd);
    else
    strncpy(salt,passwd,i-1);
}

int main(int argc,char **argv)
{
    struct spwd *sp;
    const char *passwd;
    char salt[512]={0};
    char const * dst = NULL;// 

    if(argc != 3)
        help();

    //输入用户名密码
    passwd = argv[2];//getpass("请输入密码:");
    printf("passwd[%s]\n", passwd);
    //得到用户名以及密码,命令行参数的第一个参数为用户名
    if((sp=getspnam(argv[1])) == NULL)
        error_quit("获取用户名和密码");
    //得到salt,用得到的密码作参数
    printf("sp_pwdp[%s]\n", sp->sp_pwdp);
//    get_salt(salt,sp->sp_pwdp);

    //进行密码验证
    dst = crypt(passwd, "sa");
    printf("dst[%s]\n", dst);
    if(0 == strcmp(sp->sp_pwdp, dst))
        printf("验证通过!\n");
    else
        printf("验证失败!\n");

    return 0;
}

