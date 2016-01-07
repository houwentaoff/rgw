/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  parse_conf.cpp
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  01/05/2016 10:46:43 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */


#include	<stdlib.h>
#include <stdio.h>
#include <string.h>

int strip_space(char *str)
{
    int len ;
    if (!str)
    {
        return -1;
    }

    len = strlen(str);

    while (str[len-1] == ' ')
    {
        str[len-1] = '\0';
        len--;
    }
    return 0;
}
int parse_conf(const char *path)
{
    FILE *fp = NULL;
    char buf [256]={0};
    char var_name[256]={0};
    char value[256]={0};
    int r = -1;

    if (!path)
    {
        goto err;
    }

    if (NULL == (fp = fopen(path, "r")))
    {
        goto err;
    }

    while ((r = fscanf(fp, "%[^\n]s", buf)) == 1)
    {
        sscanf(buf, "%s", var_name);
        if (var_name[0] == '#')
        {
            if (fscanf(fp, "%*[\n]") < 0)
            {
                printf("skip # fail\n");
                goto err;
            }
            continue;
        }
        if ((r = sscanf(buf, "%[^=]=%s", var_name, value))!=2)
        {
            goto err;
        }
        strip_space(var_name);
        printf("name[%s] val[%s]\n", var_name, value);
        r = fscanf(fp, "%*[\n]");
    }
err:
    return -1;
}
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
int main ( int argc, char *argv[] )
{
    if (argc < 2)
    {
        return -1;
    }
    parse_conf(argv[1]);
    return EXIT_SUCCESS;
}
