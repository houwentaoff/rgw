/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  get_usersinfo_from_mds.cpp
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  01/09/2016 02:23:57 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <string>

#include <stdio.h>

#include "cgw/cgw.h"

#include "web_include/MdsSerialJson.h"
#include "FicsJNI/FicsApi.h"

using namespace std;

extern void Fi_RefreshUserInfo();
extern void Fi_InitDll();
extern vector<WebUserInfo> g_vUserInfo;
extern string g_strConfPath;
extern string GetFicsPath();
const string g_strExeDir = GetExeDir();

extern cgw_api_t cgw_ops;

int getpawd(char *user, int sock_fd, ssize_t (*complete)(int, const void *, size_t))
{
    vector<WebUserInfo>::iterator itr;

    for (itr = g_vUserInfo.begin(); itr!=g_vUserInfo.end(); itr++)
    {
        if (itr->strName == user)
        {
            complete(sock_fd, (void *)itr->strPassword.c_str(), itr->strPassword.size());
            break;
        }
        printf("user:[%s]passwd[%s]\n", itr->strName.c_str(), itr->strPassword.c_str()); 
    }
    return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
int main ( int argc, char *argv[] )
{
    vector<WebUserInfo>::iterator itr;
    int fd = -1;
    pthread_t id,com_id; 
    pthread_attr_t attr[3];

    cgw_ops.getpawd = getpawd;

    cout<<"path:"<<GetFicsPath()<<endl;
//    CFiContext::SetRelatvePath("../config/");
//    g_strConfPath = g_strExeDir+ "../config/";
//    STRACE("The global config path[%s]", g_strConfPath.c_str());
    Fi_InitDll();
//    Fi_RefreshUserInfo();
	if (pthread_attr_init(&attr[0]) < 0)
	{
		ut_err("set attr fail\n");
	}
	if (pthread_attr_init(&attr[1]) < 0)
	{
		ut_err("set attr fail\n");
	}
	pthread_attr_setdetachstate(&attr[0], PTHREAD_CREATE_DETACHED);
	pthread_attr_setdetachstate(&attr[1], PTHREAD_CREATE_DETACHED);
    
    pthread_create(&id, &attr[0], (void*)main_loop, NULL);    
    pthread_create(&com_id, &attr[1], (void*)listen_loop, NULL);
    do
    {
        for (itr = g_vUserInfo.begin(); itr!=g_vUserInfo.end(); itr++)
        {
            printf("user:[%s]passwd[%s]\n", itr->strName.c_str(), itr->strPassword.c_str()); 
        }
        sleep(3);
    }while(1);
    return EXIT_SUCCESS;
}
