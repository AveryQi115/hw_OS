#include <iostream>
#include <stdio.h>
#include "FileSystem.h"
#include "DeviceDriver.h"
#include <string>
using namespace std;

void printDiskInode(void* p){
    cout<<"d_mode: "<<*(int*)p<<endl;
    cout<<"d_nlink: "<<*((int*)p+1)<<endl;
    cout<<"d_uid: "<<*((short*)p+4)<<endl;
    cout<<"d_gid: "<<*((short*)p+5)<<endl;
    cout<<"d_size: "<<*((int*)p+3)<<endl;

    for(int i=0;i<10;i++){
        cout<<"d_addr["<<i<<"]: "<<*((int*)p+4+i)<<endl;
    } 
    cout<<"d_atime: "<<*((short*)p+14)<<endl;
    cout<<"d_mtime: "<<*((int*)p+15)<<endl;
    cout<<endl;
}

void printSuperBlock(int* buf){
    int     s_isize = buf[0];
    int		s_fsize = buf[1];		/* 盘块总数 */
	
	int		s_nfree = buf[2];		/* 直接管理的空闲盘块数量 */
	
	int		s_ninode = buf[103];		/* 直接管理的空闲外存Inode数量 */

    cout<<"外存Inode区占用的盘块数:"<<s_isize<<endl;
    cout<<"空闲外存Inode数量:"<<s_ninode<<endl;
    cout<<"盘块总数:"<<s_fsize<<endl;
    cout<<"空闲盘块数量:"<<s_nfree<<endl;

    for(int i=0;i<100;i++){
        cout<<"空闲盘块表第"<<i<<"项："<<buf[3+i]<<endl;
    }

    for(int i=0;i<100;i++){
        cout<<"空闲Inode表第"<<i<<"项："<<buf[104+i]<<endl;
    }
    
}

void printDir(void* buf){
    void* p = buf;
    for(int i=0;i<8;i++){
        cout<<"the"<<i<<"th dir entry is:"<<endl;
        cout<<"m_ino: "<<*(int*)p<<endl;
        cout<<"name: "<<*(char*)p+4<<endl;
        p = (void *)((char *)p+32);
    }
}

int main(int argc,char** argv){
    DeviceDriver dd;
    cout<<argv[0]<<" "<<argv[1]<<endl;

    if (string(argv[1])=="superblock"){
        cout<<"read superblock"<<endl;
        int* buf = new int[2*FileSystem::BLOCK_SIZE/sizeof(int)];
        dd.read(buf,FileSystem::BLOCK_SIZE*2,0);
        printSuperBlock(buf);
    }

    if (string(argv[1])=="Inode"){
        cout<<"read first 8 diskInodes"<<endl;
        char* buf = new char[FileSystem::BLOCK_SIZE];
        dd.read(buf, FileSystem::BLOCK_SIZE, FileSystem::BLOCK_SIZE*FileSystem::INODE_ZONE_START_SECTOR);
        // 512个字节为一个BLOCK
        // 64个字节一个diskInode
        for(int i=0;i<8;i++){
            printDiskInode(buf+i*64);
        }
    }

    if (string(argv[1])=="dir"){
        int dno = atoi(argv[2]);
        cout<<"the content of dno is:"<<endl;
        char* buf = new char[FileSystem::BLOCK_SIZE];
        dd.read(buf, FileSystem::BLOCK_SIZE, FileSystem::BLOCK_SIZE*(dno));
        printDir(buf);
    }

}

