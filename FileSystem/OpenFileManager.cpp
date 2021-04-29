#include "Utility.h"
#include "OpenFileManager.h"
#include "User.h"

extern BufferManager g_BufferManager;
extern FileSystem g_FileSystem;
extern InodeTable g_INodeTable;
extern User g_User;

OpenFileTable::OpenFileTable()
{
	//nothing to do here
}

OpenFileTable::~OpenFileTable()
{
	//nothing to do here
}

void OpenFileTable::Format() {
    File emptyFile;
    for (int i = 0; i < OpenFileTable::NFILE; ++i) {
        memcpy(m_File + i, &emptyFile, sizeof(File));
    }
}

/*作用：进程打开文件描述符表中找的空闲项之下标写入 u_ar0[EAX]*/
File* OpenFileTable::FAlloc()
{
	int fd;
	User& u = g_User;

	/* 在进程打开文件描述符表中获取一个空闲项 */
	fd = u.u_ofiles.AllocFreeSlot();

	if(fd < 0)	/* 如果寻找空闲项失败 */
	{
		return NULL;
	}

	// 在系统全局打开文件表中找到一个空闲的File结构
	for(int i = 0; i < OpenFileTable::NFILE; i++)
	{
		/* f_count==0表示该项空闲 */
		if(this->m_File[i].f_count == 0)
		{
			/* 建立描述符和File结构的勾连关系 */
			u.u_ofiles.SetF(fd, &this->m_File[i]);
			/* 增加对file结构的引用计数 */
			this->m_File[i].f_count++;
			/* 清空文件读、写位置 */
			this->m_File[i].f_offset = 0;
			return (&this->m_File[i]);
		}
	}

	u.u_error = User::U_ENFILE;
	return NULL;
}

// 关闭当前File结构
// 即对f_count-1若f_count减为0，则释放File结构
void OpenFileTable::CloseF(File *pFile)
{
	/* 引用当前File的进程数减1 */
	pFile->f_count--;
	if(pFile->f_count <= 0)
	{
		g_INodeTable.IPut(pFile->f_inode);
	}
}


InodeTable::InodeTable()
{
	//nothing to do here
	m_FileSystem = &g_FileSystem;
}

InodeTable::~InodeTable()
{
	//nothing to do here
}

void InodeTable::Format() {
    Inode emptyINode;
    for (int i = 0; i < InodeTable::NINODE; ++i) {
        memcpy(m_Inode + i, &emptyINode, sizeof(Inode));
    }
}

/*
 * 根据外存INode编号获取对应INode。
 * 如果该INode已经在内存中，返回该内存INode；
 * 如果不在内存中，则将其读入内存后上锁并返回该内存INode
 * 返回NULL:INode Table OverFlow!
 */
Inode* InodeTable::IGet(int inumber)
{
	Inode* pINode;
	User& u = g_User;

	// 如果在内存中，直接获取
	int index = IsLoaded(inumber);
    if (index >= 0) {
        pINode = m_Inode + index;
        ++pINode->i_count;
        return pINode;
    }

	// 在内存InodeTable中找一个空闲的
	pINode = GetFreeInode();
    if (NULL == pINode) {
        cout << "INode Table Overflow !" << endl;
        g_User.u_error = User::U_ENFILE;
        return NULL;
    }

	// 将inumber的外存Inode拷贝进当前pINode
	pINode->i_number = inumber;
    pINode->i_count++;
    BufferManager& bmg = g_BufferManager;
    Buf* pBuffer = bmg.Bread(FileSystem::INODE_ZONE_START_SECTOR + inumber / FileSystem::INODE_NUMBER_PER_SECTOR);
    pINode->ICopy(pBuffer, inumber);
    bmg.Brelse(pBuffer);
    return pINode;
}

/* close文件时会调用Iput
 *      主要做的操作：内存i节点计数 i_count--；若为0，释放内存 i节点、若有改动写回磁盘
 * 搜索文件途径的所有目录文件，搜索经过后都会Iput其内存i节点。路径名的倒数第2个路径分量一定是个
 *   目录文件，如果是在其中创建新文件、或是删除一个已有文件；再如果是在其中创建删除子目录。那么
 *   	必须将这个目录文件所对应的内存 i节点写回磁盘。
 *   	这个目录文件无论是否经历过更改，我们必须将它的i节点写回磁盘。
 * */
void InodeTable::IPut(Inode *pNode)
{
	/* 当前进程为引用该内存Inode的唯一进程，且准备释放该内存Inode */
	if(pNode->i_count == 1)
	{
		/* 该文件已经没有目录路径指向它
			释放该inode相关所有文件
		 */
		if(pNode->i_nlink <= 0)
		{
			/* 释放该文件占据的数据盘块 */
			pNode->ITrunc();
			pNode->i_mode = 0;
			/* 释放对应的外存Inode */
			this->m_FileSystem->IFree(pNode->i_number);
		}

		/* 更新外存Inode信息 */
		pNode->IUpdate((int)time(NULL));

		/* 清除内存Inode的所有标志位 */
		pNode->i_flag = 0;
		/* 这是内存inode空闲的标志之一，另一个是i_count == 0 */
		pNode->i_number = 0;
	}

	/* 减少内存Inode的引用计数，唤醒等待进程 */
	pNode->i_count--;
}

/* 对所有非空闲的Inode更新时间 */
void InodeTable::UpdateInodeTable()
{
	for(int i = 0; i < InodeTable::NINODE; i++)
	{
		if(this->m_Inode[i].i_count != 0)
		{
			this->m_Inode[i].IUpdate((int)time(NULL));
		}
	}
}

int InodeTable::IsLoaded(int inumber)
{
	/* 寻找指定外存Inode的内存拷贝 */
	for(int i = 0; i < NINODE; i++)
	{
		if(m_Inode[i].i_number == inumber && this->m_Inode[i].i_count != 0 )
		{
			return i;	
		}
	}
	return -1;
}

Inode* InodeTable::GetFreeInode()
{
	for(int i = 0; i < InodeTable::NINODE; i++)
	{
		/* 如果该内存Inode引用计数为零，则该Inode表示空闲 */
		if(this->m_Inode[i].i_count == 0)
		{
			return m_Inode+i;
		}
	}
	return NULL;	/* 寻找失败 */
}
