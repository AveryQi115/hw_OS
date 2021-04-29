#include "bits/stdc++.h"
using namespace std;
#ifndef device_driver_h
#define device_driver_h


class DeviceDriver {
public:
    /* 磁盘镜像文件名 */
    static const char* DISK_FILE_NAME;

private:
    /* 磁盘文件指针 */
    FILE* fp;

public:
    DeviceDriver();
    ~DeviceDriver();

    /* 检查镜像文件是否存在 */
    bool Exists();

    /* 打开镜像文件 */
    void Construct();

    /* 实际写磁盘函数 */
    void write(const void* buffer, unsigned int size,
        int offset = -1, unsigned int origin = SEEK_SET);

    /* 实际写磁盘函数 */
    void read(void* buffer, unsigned int size, 
        int offset = -1, unsigned int origin = SEEK_SET);
};

const char* DiskDriver::DISK_FILE_NAME = "disk.img";


#endif