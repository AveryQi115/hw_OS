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

## debug过程  

1. - [x] 只有第一次mkdir成功，后面再mkdir则失败  

    > - 第二次写入m_Offset有错，覆盖了前面的
2. - [x] 再次打开磁盘文件，发现磁盘文件没有成功写入，空闲盘块表也没有变化  

    > - 目录文件指向的磁盘块没有成功写入-->异步写，析构函数刷新缓存-->刷新缓存出错
    > - 第二次写入的directory entry写入位置不对

3. - [ ] 在同一次打开行为中先删除，再创建文件夹，ls看不到新创建的文件夹，但是缓存刷入disk中能看到

   > - ls是通过读磁盘文件来显示的，创建操作是异步写  
  
4. - [ ] 自动测试，打开文件之后写文件失败  
    
   > - 不知道原因