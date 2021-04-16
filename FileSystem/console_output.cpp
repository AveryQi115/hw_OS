#include "console_output.h"
#include <stdio.h>

Diagnose::Diagnose()
{
	//全部都是static成员变量，所以没有什么需要在构造函数中初始化的。
}

Diagnose::~Diagnose()
{
	//this is an empty dtor
}

/*
	目前没有别的功能，就是输出msg加一个/n换行
*/
void Diagnose::Write(const char* msg)
{
	printf("%s\n",msg);
}
