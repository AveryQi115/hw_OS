#ifndef UITILITY_H
#define UITILITY_H

#include <bits/stdc++.h>
using namespace std;

/*
 * @comment 定义一些工具常量
 * 由于使用了编译选项-fno-builtin，
 * 编译器不提供这些常量的定义。
 */
#define NULL	0

/*
 *@comment 一些经常被使用到的工具函数
 *
 *
 */
class Utility{
public:
	static void memcpy(void *dest, const void *src, size_t n);
	static void memset(void *s, int ch, size_t n);
	static int memcmp(const void *buf1, const void *buf2, unsigned int count);
	static int min(int a, int b);
	static time_t time(time_t* t);
};

#endif
