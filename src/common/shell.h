/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Sobey.
 *       Filename:  shell.h
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  2015/12/22 11:06:46
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sean. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Sobey
 *
 * =====================================================================================
 */
#ifndef __SHELL_H__
#define __SHELL_H__

#include <string>

/**
 * @brief 返回shell命令的执行结果
 *
 * @param cmd
 *
 * @return 
 */
std::string shell_execute(const char * cmd);
/**
 * @brief
 *
 * @param cmd
 *
 * @return result of shell `cmd` 
 */
int shell_simple(const char *cmd);
#endif

