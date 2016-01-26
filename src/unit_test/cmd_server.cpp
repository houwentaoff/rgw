/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  cmd_server.cpp
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

using namespace std;

#include "cgw/cgw.h"

#include "web_include/MdsSerialJson.h"
#include "FicsJNI/FicsApi.h"


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
    bool exist = false;

    for (itr = g_vUserInfo.begin(); itr!=g_vUserInfo.end(); itr++)
    {
        if (itr->strName == user)
        {
            complete(sock_fd, (void *)itr->strPassword.c_str(), itr->strPassword.size()+1);
            printf("[client] user:[%s]passwd[%s]\n", itr->strName.c_str(), itr->strPassword.c_str()); 
            exist = true;
            break;
        }
    }
    if (!exist)
    {
        complete(sock_fd, (void *)"", 1);
    }
    return 0;
}
int getvol(char *vol_name, int sock_fd, ssize_t (*complete)(int, const void *, size_t))
{
    vector<WebVolInfo> vVolumeInfo;
    vector<WebVolInfo> ::iterator it;
    vector<WebUserInfo> vUser;
    vector<WebUserInfo> ::iterator itr;
    string sendBuf="";
    bool exist = false;

    Fi_GetVolumeInfo(vVolumeInfo);
    for (it = vVolumeInfo.begin(); it != vVolumeInfo.end(); it++)
    {
        if (it->strName == vol_name)
        {
            for (itr = it->vUser.begin(); itr!=vUser.end(); itr++)
            {
                exist = true;
                sendBuf += pack("owner", itr->strName.c_str());
                break;
            }
        }
        if (exist)
        {
            break;
        }
    }
    if (!exist)
    {
        complete(sock_fd, (void *)"", 1);
    }
    else
    {
        printf("[client] vol:[%s] params[%s]\n", vol_name, sendBuf.c_str()); 
        complete(sock_fd, sendBuf.c_str(), sendBuf.size()+1);
    }
    return 0;
}

int setvol(char *params,int sock_fd, ssize_t (*complete)(int, const void *, size_t))
{
    bool ret;
	string strInfo;
	WebVolInfo volumeInfo;
    byte  type = 0;//add 
    WebUserInfo userInfo;
    volumeInfo.strOldName ="";
    volumeInfo.llUsedCap = 0;
    volumeInfo.warnType = 0;
    volumeInfo.llWarnLevel = 80;

    volumeInfo.llTotalCap = 10240;
    volumeInfo.strName = get_val(params, "vol_name");
    //userInfo.uUserId 
    userInfo.uLimit =1000;
    userInfo.llTotalCap = 100;
    userInfo.llUsedCap = 0;
    userInfo.warnType  = 0;
    userInfo.llWarnLevel = 80;
    userInfo.strName = get_val(params, "owner");
    //vOwnGroup.push_back("") = "";
    volumeInfo.vUser.push_back(userInfo);
    ret = Fi_ModifyVolume(type, volumeInfo, strInfo);
    if (!ret)
    {
        complete(sock_fd, strInfo.c_str(), strInfo.size()+1);
        printf("[client] modify vol fail\n"
               "\t\t\t=============\t\t\n"
               "%s"
               "\t\t\t=============\t\t\n"
               "info[%s]\n", params, strInfo.c_str());
    }
    else
    {
        complete(sock_fd, "", 1);
        printf("[client] modify vol success\n"
               "\t\t\t=============\t\t\n"
               "%s"
               "\t\t\t=============\t\t\n"
               "info[%s]\n", params, strInfo.c_str());
     }
    return 0;
}
int delvol(char *params,int sock_fd, ssize_t (*complete)(int, const void *, size_t))
{
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
    cgw_ops.getvol  = getvol;
    cgw_ops.setvol  = setvol;
    cgw_ops.delvol  = delvol;
    
    cout<<"path:"<<GetFicsPath()<<endl;
//    CFiContext::SetRelatvePath("../config/");
//    g_strConfPath = g_strExeDir+ "../config/";
//    STRACE("The global config path[%s]", g_strConfPath.c_str());
    Fi_InitDll();
//    Fi_RefreshUserInfo();
	if (pthread_attr_init(&attr[0]) < 0)
	{
		printf("set attr fail\n");
	}
	if (pthread_attr_init(&attr[1]) < 0)
	{
		printf("set attr fail\n");
	}
	pthread_attr_setdetachstate(&attr[0], PTHREAD_CREATE_DETACHED);
	pthread_attr_setdetachstate(&attr[1], PTHREAD_CREATE_DETACHED);
    
    pthread_create(&id, &attr[0], main_loop, NULL);    
    pthread_create(&com_id, &attr[1], listen_loop, NULL);
    do
    {
        for (itr = g_vUserInfo.begin(); itr!=g_vUserInfo.end(); itr++)
        {
            printf("[main] user:[%s]passwd[%s]\n", itr->strName.c_str(), itr->strPassword.c_str()); 
        }
        sleep(3);
    }while(1);
    return EXIT_SUCCESS;
}
