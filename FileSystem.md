# 文件系统的设计
  
## 层次结构  
  
- 内存：InodeTable，OpenFileTable，ProcessOpenFilesTable，内存SuperBlock  
- 缓存：缓存队列  
- 磁盘：DiskInode，DiskSuperBlock  

## 设计要点  
  
- 全局OpenFileTable约束了全局共能同时打开100个文件  
- ProcessOpenFilesTable约束了一个进程只能同时打开15个文件
- ProcessOpenFilesTable下标为fd，值为指向全局OpenFileTable元素的一个指针
- 一个File结构对应一个打开的文件，指向一个内存inode结构，同一个File结构同一个File offset，一个内存inode可能对应多个File结构
- File结构f_count减为0后，释放File结构，对应inode的i_count--