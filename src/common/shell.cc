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
    if (NULL == fgets(ret, 512, fp))
    {
        printf("fgets null\n");
    }
    pclose(fp);    

    return string(ret);
err:
    return string(ret);
}
