/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  shell.cc
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015/12/22 11:24:37
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */

#include "shell.h"

using namespace std;

string shell_execute(const char * cmd)
{
    char ret[512] = {0};
    FILE *fp = NULL;
    char tmppath[256]={0};
    
    if (NULL == (fp = popen(cmd, "r")))
    {
        printf("popen fail\n");
        goto err;
    }
    if (0 == fread((void *)ret, sizeof(char), 512, fp))
    {
        printf("fread 0\n");
    }
    pclose(fp);    

    return string(ret);
err:
    return string(ret);
}

int shell_simple(const char *cmd)
{
    FILE *fp = NULL;
    int ret  = -1;

    if (NULL == (fp = popen(cmd, "r")))
    {
        printf("popen fail\n");
        ret = -2;
        goto err;
    }
    ret = pclose(fp);

    return ret;
err:
    return ret;
}
