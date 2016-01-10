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

#include "web_include/MdsSerialJson.h"
#include "FicsJNI/FicsApi.h"

using namespace std;

extern void Fi_RefreshUserInfo();
extern void Fi_InitDll();
extern vector<WebUserInfo> g_vUserInfo;
extern string g_strConfPath;
extern string GetFicsPath();
const string g_strExeDir = GetExeDir();

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
int main ( int argc, char *argv[] )
{
    vector<WebUserInfo>::iterator itr;
    cout<<"path:"<<GetFicsPath()<<endl;
//    CFiContext::SetRelatvePath("../config/");
//    g_strConfPath = g_strExeDir+ "../config/";
//    STRACE("The global config path[%s]", g_strConfPath.c_str());
    Fi_InitDll();
//    Fi_RefreshUserInfo();
    do
    {
        for (itr = g_vUserInfo.begin(); itr!=g_vUserInfo.end(); itr++)
        {
            printf("user:[%s]passwd[%s]\n", itr->strName.c_str(), itr->strPassword.c_str()); 
        }
        sleep(1);
    }while(1);
    return EXIT_SUCCESS;
}
