#include "FileManager.h"
#include "User.h"
#include "Utility.h"
#include "BufferManager.h"


extern BufferManager g_BufferManager;
extern FileSystem g_FileSystem;
extern InodeTable g_INodeTable;
extern OpenFileTable g_OpenFileTable;
extern User g_User;
/*==========================class FileManager===============================*/
FileManager::FileManager()
{
	//nothing to do here
	m_FileSystem = &g_FileSystem;
    m_OpenFileTable = &g_OpenFileTable;
    m_InodeTable = &g_INodeTable;
    rootDirInode = m_InodeTable->IGet(FileSystem::ROOT_INODE_NO);
    rootDirInode->i_count += 0xff;
}

FileManager::~FileManager()
{
	//nothing to do here
}

/*
 * 功能：打开文件
 * 效果：建立打开文件结构，内存i节点开锁 、i_count 为正数（i_count ++）
 * */
// 要打开的文件路径保存在u_dirp中
// u_uarg中为文件打开的权限
void FileManager::Open()
{
	Inode* pInode;
	User& u = g_User;
	pInode = this->NameI(FileManager::OPEN);	/* 0 = Open, not create */
	/* 没有找到相应的Inode */
	if ( NULL == pInode )
	{
		return;
	}
	this->Open1(pInode, u.u_arg[1], 0);
}

/*
 * 功能：创建一个新的文件
 * 效果：建立打开文件结构，内存i节点开锁 、i_count 为正数（应该是 1）
 * */
void FileManager::Creat()
{
	Inode* pInode;
	User& u = g_User;
	unsigned int newACCMode = u.u_arg[1];

	/* 搜索目录的模式为1，表示创建；若父目录不可写，出错返回 */
	pInode = this->NameI(FileManager::CREATE);
	// cout<<"after NameI"<<endl;
	// if (pInode!=NULL){
	// 	pInode->debug();
	// }

	/* 没有找到相应的Inode，或NameI出错 */
	if ( NULL == pInode )
	{
		if(u.u_error)
			return;
		/* 创建Inode */
		pInode = this->MakNode(newACCMode);
		// cout<<"after MakNode"<<endl;
		// if (pInode!=NULL){
		// 	pInode->debug();
		// }

		/* 创建失败 */
		if ( NULL == pInode )
		{
			return;
		}

		/* 
		 * 如果所希望的名字不存在，使用参数trf = 2来调用open1()。
		 * 不需要进行权限检查，因为刚刚建立的文件的权限和传入参数mode
		 * 所表示的权限内容是一样的。
		 */
		this->Open1(pInode, File::FWRITE, 2);
	}
	else
	{
		/* 如果NameI()搜索到已经存在要创建的文件，则清空该文件（用算法ITrunc()）。UID没有改变
		 * 原来UNIX的设计是这样：文件看上去就像新建的文件一样。然而，新文件所有者和许可权方式没变。
		 * 也就是说creat指定的RWX比特无效。
		 * 邓蓉认为这是不合理的，应该改变。
		 * 现在的实现：creat指定的RWX比特有效 */
		this->Open1(pInode, File::FWRITE, 1);
		pInode->i_mode |= newACCMode;
	}
}

/* 返回NULL表示目录搜索失败，否则是根指针，指向文件的内存打开i节点 ，上锁的内存i节点  */
Inode* FileManager::NameI( enum DirectorySearchMode mode )
{
	User& u = g_User;
	Inode* pInode = u.u_cdir;
	Buf* pBuf;
	BufferManager& bufMgr = g_BufferManager;
	int freeEntryOffset;	/* 以创建文件模式搜索目录时，记录空闲目录项的偏移量 */
	unsigned int index = 0, nindex = 1;

	/* 
	 * 如果该路径是'/'开头的，从根目录开始搜索，
	 * 否则从进程当前工作目录开始搜索。
	 */
	if ( '/' == u.u_dirp[0] )
	{
		index++;
		nindex++;
		pInode = this->rootDirInode;
	}

	/* 外层循环每次处理pathname中一段路径分量 */
	while (true)
	{
		/* 如果出错则释放当前搜索到的目录文件Inode，并退出 */
		if ( u.u_error != User::U_NOERROR )
		{
			break;	/* goto out; */
		}

		/* 整个路径搜索完毕，返回相应Inode指针。目录搜索成功返回。 */
		if ( nindex >= u.u_dirp.length() )
		{
			return pInode;
		}

		/* 如果要进行搜索的不是目录文件，释放相关Inode资源则退出 */
		if ( (pInode->i_mode & Inode::IFMT) != Inode::IFDIR )
		{
			u.u_error = User::U_ENOTDIR;
			break;	/* goto out; */
		}

		nindex = u.u_dirp.find_first_of('/', index);
        memset(u.u_dbuf, 0, sizeof(u.u_dbuf));
        memcpy(u.u_dbuf, u.u_dirp.data() + index, (nindex == (unsigned int)string::npos ? u.u_dirp.length() : nindex) - index);
        index = nindex + 1;

		/* 内层循环部分对于u.u_dbuf[]中的路径名分量，逐个搜寻匹配的目录项 */
		u.u_IOParam.m_Offset = 0;
		/* 设置为目录项个数 ，含空白的目录项*/
		u.u_IOParam.m_Count = pInode->i_size / (DirectoryEntry::DIRSIZ+4);
		freeEntryOffset = 0;
		pBuf = NULL;

		// 内层循环对当前path持续搜索直到
		while (true)
		{
			/* 对目录项已经搜索完毕 */
			if ( 0 == u.u_IOParam.m_Count )
			{
				if ( NULL != pBuf )
				{
					bufMgr.Brelse(pBuf);
				}
				/* 如果是创建新文件 */
				if ( FileManager::CREATE == mode && nindex >= u.u_dirp.length() )
				{
					/* 将父目录Inode指针保存起来，以后写目录项WriteDir()函数会用到 */
					u.u_pdir = pInode;

					if ( freeEntryOffset )	/* 此变量存放了空闲目录项位于目录文件中的偏移量 */
					{
						/* 将空闲目录项偏移量存入u区中，写目录项WriteDir()会用到 */
						u.u_IOParam.m_Offset = freeEntryOffset - (DirectoryEntry::DIRSIZ+4);
					}
					else  /*问题：为何if分支没有置IUPD标志？  这是因为文件的长度没有变呀*/
					{
						pInode->i_flag |= Inode::IUPD;
					}
					/* 找到可以写入的空闲目录项位置，NameI()函数返回 */
					return NULL;
				}
				
				/* 目录项搜索完毕而没有找到匹配项，释放相关Inode资源，并推出 */
				u.u_error = User::U_ENOENT;
				goto out;
			}

			/* 已读完目录文件的当前盘块，需要读入下一目录项数据盘块 */
			if ( 0 == u.u_IOParam.m_Offset % Inode::BLOCK_SIZE )
			{
				if ( NULL != pBuf )
				{
					bufMgr.Brelse(pBuf);
				}
				/* 计算要读的物理盘块号 */
				int phyBlkno = pInode->Bmap(u.u_IOParam.m_Offset / Inode::BLOCK_SIZE );
				pBuf = bufMgr.Bread(phyBlkno);
			}

			/* 没有读完当前目录项盘块，则读取下一目录项至u.u_dent */
            memcpy(&u.u_dent, pBuf->b_addr + (u.u_IOParam.m_Offset % Inode::BLOCK_SIZE), sizeof(u.u_dent));

			u.u_IOParam.m_Offset += DirectoryEntry::DIRSIZ + 4;
			u.u_IOParam.m_Count--;

			/* 如果是空闲目录项，记录该项位于目录文件中偏移量 */
			if ( 0 == u.u_dent.m_ino )
			{
				if ( 0 == freeEntryOffset )
				{
					freeEntryOffset = u.u_IOParam.m_Offset;
				}
				/* 跳过空闲目录项，继续比较下一目录项 */
				continue;
			}

			if (!memcmp(u.u_dbuf, &u.u_dent.name, DirectoryEntry::DIRSIZ)) {
                break;
            }
			
		}

		/* 
		 * 从内层目录项匹配循环跳至此处，说明pathname中
		 * 当前路径分量匹配成功了，还需匹配pathname中下一路径
		 * 分量，直至遇到'\0'结束。
		 */
		if ( NULL != pBuf )
		{
			bufMgr.Brelse(pBuf);
		}

		/* 如果是删除操作，则返回父目录Inode，而要删除文件的Inode号在u.u_dent.m_ino中 */
		if ( FileManager::DELETE == mode && nindex >= u.u_dirp.length() )
		{
			return pInode;
		}

		/* 
		 * 匹配目录项成功，则释放当前目录Inode，根据匹配成功的
		 * 目录项m_ino字段获取相应下一级目录或文件的Inode。
		 */
		this->m_InodeTable->IPut(pInode);
		pInode = this->m_InodeTable->IGet(u.u_dent.m_ino);
		/* 回到外层While(true)循环，继续匹配Pathname中下一路径分量 */

		if ( NULL == pInode )	/* 获取失败 */
		{
			return NULL;
		}
	}
out:
	this->m_InodeTable->IPut(pInode);
	return NULL;
}


/* 
* trf == 0由open调用
* trf == 1由creat调用，creat文件的时候搜索到同文件名的文件，覆盖该同名文件
* trf == 2由creat调用，creat文件的时候未搜索到同文件名的文件，这是文件创建时更一般的情况
* mode参数：打开文件模式，表示文件操作是 读、写还是读写
*/
void FileManager::Open1(Inode* pInode, int mode, int trf)
{
	User& u = g_User;

	/* 在creat文件的时候搜索到同文件名的文件，释放该文件所占据的所有盘块 */
    if (1 == trf) {
        pInode->ITrunc();
    }

	/* 分配打开文件控制块File结构 */
    File* pFile = this->m_OpenFileTable->FAlloc();
    if (NULL == pFile) {
        this->m_InodeTable->IPut(pInode);
        return;
    }

	/* 设置打开文件方式，建立File结构和内存INode的勾连关系 */
    pFile->f_flag = mode & (File::FREAD | File::FWRITE);
    pFile->f_inode = pInode;

	/* 为打开或者创建文件的各种资源都已成功分配，函数返回 */
    if (u.u_error == 0) {
        return;
    }
    else {  /* 如果出错则释放资源 */
        /* 释放打开文件描述符 */
        int fd = u.u_ar0[User::EAX];
        if (fd != -1) {
            u.u_ofiles.SetF(fd, NULL);
            /* 递减File结构和INode的引用计数 ,File结构没有锁 f_count为0就是释放File结构了*/
            pFile->f_count--;
        }
        this->m_InodeTable->IPut(pInode);
    }
}

/* 由creat调用。
 * 为新创建的文件写新的i节点和新的目录项
 * 返回的pInode是上了锁的内存i节点，其中的i_count是 1。
 *
 * 在程序的最后会调用 WriteDir，在这里把属于自己的目录项写进父目录，修改父目录文件的i节点 、将其写回磁盘。
 *
 */
Inode* FileManager::MakNode( unsigned int mode )
{
	Inode* pInode;
	User& u = g_User;

	/* 分配一个空闲DiskInode，里面内容已全部清空 */
	pInode = this->m_FileSystem->IAlloc();
	if( NULL ==	pInode )
	{
		return NULL;
	}

	pInode->i_flag |= (Inode::IACC | Inode::IUPD);
	pInode->i_mode = mode | Inode::IALLOC;
	pInode->i_nlink = 1;

	/* 将目录项写入u.u_dent，随后写入目录文件 */
	this->WriteDir(pInode);

	return pInode;
}

/* 由creat调用。
 * 把属于自己的目录项写进父目录，修改父目录文件的i节点 、将其写回磁盘。
 */
void FileManager::WriteDir(Inode* pInode)
{
	User& u = g_User;

	/* 设置目录项中Inode编号部分 */
	u.u_dent.m_ino = pInode->i_number;

	/* 设置目录项中pathname分量部分 */
	memcpy(u.u_dent.name, u.u_dbuf, DirectoryEntry::DIRSIZ);

	u.u_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;
	u.u_IOParam.m_Base = (unsigned char *)&u.u_dent;



	/* 将目录项写入父目录文件 */
	if (u.u_IOParam.m_Offset < u.u_pdir->i_size){
		u.u_pdir->i_size += DirectoryEntry::DIRSIZ+4;
	}

	// WriteI是写文件的通用函数，只有当写入后offset超过原文件大小时才会更新文件size
	u.u_pdir->WriteI();
	this->m_InodeTable->IPut(u.u_pdir);
}

void FileManager::Close()
{
	User& u = g_User;
	int fd = u.u_arg[0];

	/* 获取打开文件控制块File结构 */
	File* pFile = u.u_ofiles.GetF(fd);
	if ( NULL == pFile )
	{
		return;
	}

	/* 释放打开文件描述符fd，递减File结构引用计数 */
	u.u_ofiles.SetF(fd, NULL);
	this->m_OpenFileTable->CloseF(pFile);
}

// 删除文件调用该函数
void FileManager::UnLink()
{
	Inode* pInode;
	Inode* pDeleteInode;
	User& u = g_User;

	pDeleteInode = this->NameI(FileManager::DELETE);
	// pDeleteInode->debug();

	if ( NULL == pDeleteInode )
	{
		return;
	}

	pInode = this->m_InodeTable->IGet(u.u_dent.m_ino);
	// pInode->debug();
	
	if ( NULL == pInode )
	{
		return;
	}

	/* 写入清零后的目录项 */
	u.u_IOParam.m_Offset -= (DirectoryEntry::DIRSIZ + 4);
	u.u_IOParam.m_Base = (unsigned char *)&u.u_dent;
	u.u_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;
	
	u.u_dent.m_ino = 0;
	memcpy(u.u_dent.name,"",sizeof(char)*DirectoryEntry::DIRSIZ);
	pDeleteInode->WriteI();

	/* 修改inode项 */
	pInode->i_nlink--;
	pInode->i_flag |= Inode::IUPD;

	if (0 == pInode->i_nlink){
		pDeleteInode->i_size -= (DirectoryEntry::DIRSIZ + 4);
	}

	this->m_InodeTable->IPut(pDeleteInode);
	this->m_InodeTable->IPut(pInode);
}

void FileManager::Seek()
{
	File* pFile;
	User& u = g_User;
	int fd = u.u_arg[0];

	pFile = u.u_ofiles.GetF(fd);
	if ( NULL == pFile )
	{
		return;  /* 若FILE不存在，GetF有设出错码 */
	}

	int offset = u.u_arg[1];

	switch ( u.u_arg[2] )
	{
		/* 读写位置设置为offset */
		case 0:
			pFile->f_offset = offset;
			break;
		/* 读写位置加offset(可正可负) */
		case 1:
			pFile->f_offset += offset;
			break;
		/* 读写位置调整为文件长度加offset */
		case 2:
			pFile->f_offset = pFile->f_inode->i_size + offset;
			break;
		default:
        	cout << " origin " << u.u_arg[2] << " is undefined ! \n";
        break;
	}
}

void FileManager::Read()
{
	/* 直接调用Rdwr()函数即可 */
	this->Rdwr(File::FREAD);
}

void FileManager::Write()
{
	/* 直接调用Rdwr()函数即可 */
	this->Rdwr(File::FWRITE);
}

void FileManager::Rdwr( enum File::FileFlags mode )
{
	File* pFile;
	User& u = g_User;

	/* 根据Read()/Write()的系统调用参数fd获取打开文件控制块结构 */
	pFile = u.u_ofiles.GetF(u.u_arg[0]);	/* fd */
	if ( NULL == pFile )
	{
		/* 不存在该打开文件，GetF已经设置过出错码，所以这里不需要再设置了 */
		/*	u.u_error = User::EBADF;	*/
		return;
	}


	/* 读写的模式不正确 */
	if ( (pFile->f_flag & mode) == 0 )
	{
		u.u_error = User::U_EACCES;
		return;
	}

	u.u_IOParam.m_Base = (unsigned char *)u.u_arg[1];	/* 目标缓冲区首址 */
	u.u_IOParam.m_Count = u.u_arg[2];		/* 要求读/写的字节数 */
	u.u_IOParam.m_Offset = pFile->f_offset;
	
	
	if ( File::FREAD == mode )
	{
		pFile->f_inode->ReadI();
	}
	else
	{
		pFile->f_inode->WriteI();
	}

	/* 根据读写字数，移动文件读写偏移指针 */
	pFile->f_offset += (u.u_arg[2] - u.u_IOParam.m_Count);

	/* 返回实际读写的字节数，修改存放系统调用返回值的核心栈单元 */
	u.u_ar0[User::EAX] = u.u_arg[2] - u.u_IOParam.m_Count;
}

void FileManager::Ls() {
    User &u = g_User;
    BufferManager& bufferManager = g_BufferManager;

    Inode* pInode = u.u_cdir;

    Buf* pBuffer = NULL;

	// 当前盘块总目录项数
    u.u_IOParam.m_Offset = 0;
    u.u_IOParam.m_Count = pInode->i_size / sizeof(DirectoryEntry);

	std::ostringstream stringStream;
  	stringStream << "FileMode" << std::setw(10) << "InodeNum" << std::setw(10) << "FileSize" << std::setw(30) << "FileName" << endl;

    while (u.u_IOParam.m_Count) {
        if (0 == u.u_IOParam.m_Offset%Inode::BLOCK_SIZE) {
			// 当前盘块遍历完读入下一个盘块
            if (pBuffer) {
                bufferManager.Brelse(pBuffer);
            }
            int phyBlkno = pInode->Bmap(u.u_IOParam.m_Offset / Inode::BLOCK_SIZE);
            pBuffer = bufferManager.Bread(phyBlkno);
        }
        memcpy(&u.u_dent, pBuffer->b_addr + (u.u_IOParam.m_Offset % Inode::BLOCK_SIZE), sizeof(u.u_dent));
        u.u_IOParam.m_Offset += sizeof(DirectoryEntry);

		// 遇到空目录项则继续
        if (0 == u.u_dent.m_ino)
            continue;

		Inode* fileInode = this->m_InodeTable->IGet(u.u_dent.m_ino);
		if (fileInode == NULL){
			u.u_error = User::U_ENOENT;
			return;
		}

		stringStream << fileInode->getFileType() << fileInode->getFileMode() << std::setw(10) << to_string(u.u_dent.m_ino) << std::setw(10) << to_string(fileInode->i_size) << "B" << std::setw(30) << u.u_dent.name << endl;
		u.u_IOParam.m_Count--;
    }

    if (pBuffer) {
        bufferManager.Brelse(pBuffer);
    }

	u.ls = stringStream.str();
}

void FileManager::ChDir()
{
	Inode* pInode;
	User& u = g_User;

	pInode = this->NameI(FileManager::OPEN);
	if ( NULL == pInode )
	{
		return;
	}

	/* 搜索到的文件不是目录文件，返回错误，释放当前inode */
	if ( (pInode->i_mode & Inode::IFMT) != Inode::IFDIR )
	{
		u.u_error = User::U_ENOTDIR;
		this->m_InodeTable->IPut(pInode);
		return;
	}

	u.u_cdir = pInode;

	if (u.u_dirp[0] != '/') {
        u.u_curdir += u.u_dirp;
    }
    else {
        u.u_curdir = u.u_dirp;
    }
    if (u.u_curdir.back() != '/')
        u.u_curdir.push_back('/');
}


