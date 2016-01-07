/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  setuid.c
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  01/06/2016 11:48:58 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <fstream>

using namespace std;

void test_read_file(const char *name)
{
    ifstream f(name);
    if(f.fail())
        cout<<"=[ERROR]: read failed."<<endl;
    else
        cout<<"=[OK]: read successful."<<endl;
}
//打印uid和euid.
inline void p_states(void)
{
    int uid,euid;
    cout<<"-----Current states--------------------------"<<endl;
    cout<<"real uid\t"<<getuid()<<endl;
    cout<<"effective uid\t"<<geteuid()<<endl;
    cout<<"---------------------------------------------"<<endl;
}
//调用setuid.
inline void run_setuid_fun(int uid)
{
    if(setuid(uid) == -1)
         cout<<"=[ERROR]: setuid("<<uid<<") error"<<endl;
    p_states();
}
//调用seteuid.
inline void run_seteuid_fun(int uid)
{
    if(seteuid(uid) == -1)
         cout<<"=[ERROR]: seteuid("<<uid<<") error"<<endl;
    p_states();
}

int main()
{
    int t_re=0;
    const char * file = "root_only.txt";

    cout<<endl<<"TEST 1: "<<endl;
    p_states();
    //[1]此时 real uid= login user id
    //     effective uid = root
    //     saved uid = root
    test_read_file(file);

    cout<<endl<<"TEST 2: seteuid(getuid())"<<endl;
    run_seteuid_fun(getuid());
    //[2]此时 real uid= login user id
    //     effective uid = login user id
    //     saved uid = root
    test_read_file(file);

    cout<<endl<<"TEST 3: seteuid(0)"<<endl;
    run_seteuid_fun(0);
    //[3]此时 real uid= login user id
    //     effective uid = root
    //     saved uid = root
    test_read_file(file);

    cout<<endl<<"TEST 4: setuid(0)"<<endl;
    run_setuid_fun(0);
    //[4]此时 real uid= root
    //     effective uid = root
    //     saved uid = root
    test_read_file(file);

    cout<<endl<<"TEST 5: setuid(503)"<<endl;
    run_setuid_fun(503);
    //[5]此时 real uid= login user id
    //     effective uid = login user id
    //     saved uid = login user id
    test_read_file(file);

    cout<<endl<<"TEST 6: setuid(0)"<<endl;
    //[6]此时 real uid= login user id
    //     effective uid = login user id
    //     saved uid = login user id
    // 运行setuid(0)是将返回错误值-1.
    run_setuid_fun(0);
    test_read_file(file);
    return 0;
}
