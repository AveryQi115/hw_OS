#include "DeviceDriver.h"
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

SuperBlock g_spb;
SuperBlock::SuperBlock(){

}
SuperBlock::~SuperBlock(){

}

DiskDriver::DiskDriver(){
    fp = fopen(DISK_FILE_NAME,"rw"); 
}

DiskDriver::~DiskDriver(){
    munmap(&g_spb,2*BLOCK_SIZE);
    fclose(fp);
}

void DiskDriver::Initialize(){
    // superblock区映射
    mmap(NULL,2*BLOCK_SIZE, PROT_READ|PROT_WRITE,MAP_SHARED,fileno(fp),0);
    g_spb.s_isize = 10;
    g_spb.s_fsize = 10;
}

int main(){
    DiskDriver dd;
    dd.Initialize();
    return 0;
}

