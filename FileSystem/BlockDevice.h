#ifndef BLOCK_DEVICE_H
#define BLOCK_DEVICE_H

#include "Buf.h"
#include "Utility.h"
#include "Assembly.h"

/* 块设备表devtab定义 */
class Devtab
{
public:
	Devtab();
	~Devtab();
	
public:
	int	d_active;
	int	d_errcnt;
	Buf* b_forw;
	Buf* b_back;
	Buf* d_actf;
	Buf* d_actl;
};

/*
 * 块设备基类，各类块设备都从此基类继承。
 * 派生类override基类中的Open(), Close(), Strategy()函数，
 * 实现对各中块设备不同的操作逻辑。以此替代Unix V6中
 * 原块设备开关表(bdevsw)的功能。
 */
class BlockDevice
{
public:
	static int NSECTOR;		/* ATA磁盘扇区数 */

	BlockDevice();
	BlockDevice(Devtab* tab);
	~BlockDevice();
	/* 
	 * 定义为虚函数，由派生类进行override实现设备
	 * 特定操作。正常情况下，基类中函数不应被调用到。
	 */
	virtual int Open(short dev, int mode);
	virtual int Close(short dev, int mode);
	virtual int Strategy(Buf* bp);
	virtual void Start();
	
public:
	Devtab*	d_tab;		/* 指向块设备表的指针 */
};

#endif
