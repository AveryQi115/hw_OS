#include <stdio.h>
#include "console_output.h"
#include "FileSystem.h"
#ifndef device_driver_h
#define device_driver_h


extern SuperBlock* g_spb;
class DiskDriver
{
private:
    static const char *DISK_FILE_NAME; /* 磁盘镜像文件名 */ 
    static const int BLOCK_SIZE = 512; /* 数据块大小为 512 字节 */
    static const short ROOTDEV = (0 << 8) | 0;	/* 磁盘的主、从设备号都为0 */
    int fd; /* 磁盘镜像文件描述符 */ 
public:
    DiskDriver();
    ~DiskDriver();
    void Initialize(); /* 格式化磁盘 */
	void InitializeSuperBlock(); /* 格式化SuperBlock区 */
	
    // void IO(Buf *bp); /* 根据 IO 请求块进行读写 */ 
};

const char* DiskDriver::DISK_FILE_NAME = "disk.img";


#endif