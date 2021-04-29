#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include "Buf.h"
#include "DeviceDriver.h"

class BufferManager
{
public:
	/* static const member */
	static const int NBUF = 15;			/* 缓存控制块、缓冲区的数量 */
	static const int BUFFER_SIZE = 512;	/* 缓冲区大小。 以字节为单位 */

public:
	BufferManager();
	~BufferManager();
	
	void Initialize();					/* 缓存控制块队列的初始化。将缓存控制块中b_addr指向相应缓冲区首地址。*/
	
	Buf* GetBlk(int blkno);				/* 申请一块缓存，用于读写设备dev上的字符块blkno。*/
	void Brelse(Buf* bp);				/* 释放缓存控制块buf */

	Buf* Bread(int blkno);				/* 读一个磁盘块。dev为主、次设备号，blkno为目标磁盘块逻辑块号。 */

	void Bwrite(Buf* bp);				/* 写一个磁盘块 */

	void Bdwrite(Buf* bp);

	void Bclear(Buf* bp);				/* 磁盘块全部清零 */

	void Bflush();						/* 所有延迟写的缓存全部输出到磁盘 */

    void FormatBuffer();				/* 格式化所有Buffer */

private:
	void insertHead(Buf* bp);
	Buf* find(int blkno);
	void detachNode(Buf* bp);
	
private:
	Buf* m_Buf_head;					/* 缓存控制块数组头结点 */
	Buf m_Buf[NBUF];					/* 缓存控制块数组 */
	unsigned char Buffer[NBUF][BUFFER_SIZE];	/* 缓冲区数组 */
	
	DeviceDriver* m_DeviceDriver;		/* 指向设备驱动模块全局对象 */
};

#endif
