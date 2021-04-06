#include <stdio.h>
#ifndef device_driver_h
#define device_driver_h
class SuperBlock
{
	/* Functions */
public:
	/* Constructors */
	SuperBlock();
	/* Destructors */
	~SuperBlock();
	
	/* Members */
public:
	int		s_isize;		/* 外存Inode区占用的盘块数 */
	int		s_fsize;		/* 盘块总数 */
	
	int		s_nfree;		/* 直接管理的空闲盘块数量 */
	int		s_free[100];	/* 直接管理的空闲盘块索引表 */
	
	int		s_ninode;		/* 直接管理的空闲外存Inode数量 */
	int		s_inode[100];	/* 直接管理的空闲外存Inode索引表 */
	
	int		s_flock;		/* 封锁空闲盘块索引表标志 */
	int		s_ilock;		/* 封锁空闲Inode表标志 */
	
	int		s_fmod;			/* 内存中super block副本被修改标志，意味着需要更新外存对应的Super Block */
	int		s_ronly;		/* 本文件系统只能读出 */
	int		s_time;			/* 最近一次更新时间 */
	int		padding[47];	/* 填充使SuperBlock块大小等于1024字节，占据2个扇区 */
};

extern SuperBlock g_spb;


class DiskDriver
{
private:
    static const char *DISK_FILE_NAME; /* 磁盘镜像文件名 */ 
    static const int BLOCK_SIZE = 512; /* 数据块大小为 512 字节 */
    FILE *fp; /* 磁盘镜像文件指针 */ 
public:
    DiskDriver();
    ~DiskDriver();
    void Initialize(); /* 初始化磁盘镜像 */
    // void IO(Buf *bp); /* 根据 IO 请求块进行读写 */ 
};

const char* DiskDriver::DISK_FILE_NAME = "disk.img";


#endif