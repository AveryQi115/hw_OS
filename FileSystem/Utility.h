#ifndef UITILITY_H
#define UITILITY_H

/*
 *@comment 定义一些工具常量
 * 由于使用了编译选项-fno-builtin，
 * 编译器不提供这些常量的定义。
 */
#define NULL	0

/*
 *@comment 一些经常被使用到的工具函数
 *
 *
 */
class Utility
{
public:
	static void MemCopy(unsigned long src, unsigned long des, unsigned int count);
	
	static int CaluPageNeed(unsigned int memoryneed, unsigned int pagesize);

	static void StringCopy(char* src, char* dst);

	static int StringLength(char* pString);
	
	/* @comment
	 * 用于从物理地址src copy 到物理地址des 1个byte
	 */
	/* 提取参数dev中的主设备号major，高8比特 */
	static short GetMajor(const short dev);
	/* 提取参数dev中的次设备号minor，低8比特 */
	static short GetMinor(const short dev);
	/* 设置参数dev中的主设备号部分，高8比特 */
	static short SetMajor(short dev, const short value);
	/* 设置参数dev中的次设备号部分，低8比特 */
	static short SetMinor(short dev, const short vlaue);
	/* 输出错误信息，然后死循环 */
	static void Panic(char* str);

	/* 以src为源地址，dst为目的地址，复制count个双字 */
	static void DWordCopy(int* src, int* dst, int count);

	static int Min(int a, int b);

	static int Max(int a, int b);

	/* 用于在读、写文件时，高速缓存与用户指定目标内存区域之间数据传送 */
	static void IOMove(unsigned char* from, unsigned char* to, int count);
};

#endif
