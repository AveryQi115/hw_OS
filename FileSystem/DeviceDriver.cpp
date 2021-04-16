#include "DeviceDriver.h"
#include <sys/types.h>
#include <sys/mman.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <Buf.h>

//磁盘的内存映射
void* g_disk;

DiskDriver::DiskDriver(){
    fd = open(DISK_FILE_NAME,O_RDWR,0);
    if (fd==-1){
        Diagnose::Write("cannot open disk image file");
        return;
    }
    ftruncate(fd,BLOCK_NUM*BLOCK_SIZE);
}

DiskDriver::~DiskDriver(){
    close(fd);
}

// 格式化磁盘
// 只适用于第一次加载
void DiskDriver::Initialize(){
    g_disk = mmap(NULL,BLOCK_NUM*BLOCK_SIZE, PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    // superblock区映射
    g_spb = (SuperBlock*)((char*)g_disk + FileSystem::SUPER_BLOCK_SECTOR_NUMBER*BLOCK_SIZE);
    InitializeSuperBlock();
}

void DiskDriver::InitializeSuperBlock(){
    g_spb->s_isize = INODE_NUM;
    g_spb->s_fsize = BLOCK_NUM;

    // 初始化空闲inode表
    // 需分配时由FileSystem::IAlloc自己搜索
    g_spb->s_ninode = 0;
    for(int i=0;i<100;i++)
        g_spb->s_inode[i] = 0;

    // 初始化空闲盘块表
    // SuperBlock区200，201号盘块不为空闲盘块
    g_spb->s_nfree = 100;

	//写空闲盘块索引表
	int blkno = FileSystem::DATA_ZONE_END_SECTOR;
    int lastblk = 0;
    int* tmp = new[128] int(){0};
    int i = 101;
    while(blkno>=FileSystem::DATA_ZONE_START_SECTOR){
        if (i==101)
            tmp[i--] = lastblk;
        else if(i>0)
            tmp[i--] = blkno--;
        else{
            tmp[i] = 100-i;
            lastblk = blkno--;
            i = 101;
            memcpy((void *)((char *)g_spb+blkno*BLOCK_SIZE),(void *)tmp,sizeof(int)*128);
        }
    }

    if(i==101 || i==100){
        int* pBuf = (int*)((char*)g_spb+lastblk*BLOCK_SIZE);
        g_spb->s_nfree = *pBuf++;
        Utility::DWordCopy(pBuf,g_spb->s_free,100);
    }
    // tmp[i+1]~tmp[100]为block
    // tmp[101]为lastblk
    else{
        g_spb->s_nfree = 100-i;
        for(int j=i+1;j<=101;j++){
            g_spb->s_free[j-2] = tmp[j];
        }
    }

}

int main(){
    DiskDriver dd;
    dd.Initialize();
    return 0;
}

