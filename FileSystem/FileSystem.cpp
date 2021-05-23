#include "FileSystem.h"
#include "Utility.h"
#include "User.h"

/*==============================class DirectoryEntry===================================*/
void DirectoryEntry::debug(){
    cout<<"name: "<<name<<endl;
    cout<<"inode_num:"<<m_ino<<endl;
}




/*==============================class SuperBlock===================================*/
/* 系统全局超级块SuperBlock对象 */
SuperBlock* g_spb;

SuperBlock::SuperBlock()
{
	//nothing to do here
}

SuperBlock::~SuperBlock()
{
	//nothing to do here
}

void SuperBlock::debug(int i,int j){
    cout<<"外存Inode区占用的盘块数:    "<<s_isize<<endl;
    cout<<"空闲外存Inode数量:          "<<s_ninode<<endl;
    cout<<"盘块总数:                  "<<s_fsize<<endl;
    cout<<"空闲盘块数量:               "<<s_nfree<<endl;
    cout<<"空闲外存Inode索引表第"<<i<<"项:    "<<s_inode[i]<<endl;
    cout<<"空闲盘块索引表第"<<j<<"项:   "<<s_free[j]<<endl;
}

extern DeviceDriver g_DeviceDriver;
extern BufferManager g_BufferManager;
extern SuperBlock g_SuperBlock;
extern InodeTable g_INodeTable;
extern User g_User;

/*==============================class FileSystem===================================*/
FileSystem::FileSystem() {
    deviceDriver = &g_DeviceDriver;
    superBlock = &g_SuperBlock;
	bufferManager = &g_BufferManager;

	// 如果镜像文件没有打开，格式化镜像文件
    if (!deviceDriver->Exists()) {
        FormatDevice();
    }
	// 打开了就加载superBlock
    else {
        LoadSuperBlock();
    }
}

FileSystem::~FileSystem() {
    cout<<"~FileSystem"<<endl;
    Update();
    deviceDriver = NULL;
    superBlock = NULL;
}

/* 格式化SuperBlock */
// 并没有生成空闲盘块表和空闲inode表
void FileSystem::FormatSuperBlock() {
    superBlock->s_isize = FileSystem::INODE_ZONE_SIZE;
    superBlock->s_fsize = FileSystem::DISK_SIZE;
    superBlock->s_nfree = 0;
    superBlock->s_free[0] = -1;
    superBlock->s_ninode = 0;
    superBlock->s_flock = 0;
    superBlock->s_ilock = 0;
    superBlock->s_fmod = 0;
    superBlock->s_ronly = 0;
    time((time_t*)&superBlock->s_time);
}

/* 格式化整个文件系统 */
void FileSystem::FormatDevice() {
    FormatSuperBlock();
	// 打开镜像文件
    deviceDriver->Construct();

    //直接调用写磁盘函数，写入superblock
    deviceDriver->write(superBlock, sizeof(SuperBlock), FileSystem::SUPERBLOCK_START_SECTOR*(FileSystem::BLOCK_SIZE));

    DiskInode emptyDINode, rootDINode;
    
	//根目录DiskNode标已分配和目录
    rootDINode.d_mode |= Inode::IALLOC | Inode::IFDIR;
    rootDINode.d_nlink = 1;
    deviceDriver->write(&rootDINode, sizeof(rootDINode),FileSystem::INODE_ZONE_START_SECTOR*FileSystem::BLOCK_SIZE+FileSystem::ROOT_INODE_NO*sizeof(DiskInode));
    
    //从根目录之后开始初始化，初始化INode_NUMBERS个inode
    for (int i = 1; i < FileSystem::INode_NUMBERS; ++i) {
		// 填充空闲inode表
        if (superBlock->s_ninode < 100) {
            superBlock->s_inode[superBlock->s_ninode++] = i+FileSystem::ROOT_INODE_NO;
        }
		// offset不设置，依次填入
        deviceDriver->write(&emptyDINode, sizeof(emptyDINode));
    }

    //空闲盘块初始化
    char freeBlock[BLOCK_SIZE], freeBlock1[BLOCK_SIZE];
    memset(freeBlock, 0, BLOCK_SIZE);
    memset(freeBlock1, 0, BLOCK_SIZE);

	// 依次填入DATA_ZONE_SIZE个扇区
    for (int i = 0; i <= FileSystem::DATA_ZONE_SIZE; ++i) {
        if (superBlock->s_nfree >= 100) {
			// 404个字节拷给freeBlock1，第1个是num，之后99个为空闲盘块号，最后一个是下一空闲表盘块号
            memcpy(freeBlock1, &superBlock->s_nfree, sizeof(int) + sizeof(superBlock->s_free));
            deviceDriver->write(&freeBlock1, BLOCK_SIZE);
            superBlock->s_nfree = 0;
        }
        else {
            deviceDriver->write(freeBlock, BLOCK_SIZE);
        }
		// 起始的盘块写作0代表没有下一个空闲盘块
        superBlock->s_free[superBlock->s_nfree++] = (i==0) ? 0 : i+DATA_ZONE_START_SECTOR-1;
    }

    time((time_t*)&superBlock->s_time);
    //刷新superblock时间，写入空闲盘块表和空闲inode表
    deviceDriver->write(superBlock, sizeof(SuperBlock), 0);
}

/* 系统初始化时读入SuperBlock */
void FileSystem::LoadSuperBlock() {
    deviceDriver->read(superBlock, sizeof(SuperBlock), SUPERBLOCK_START_SECTOR*BLOCK_SIZE);
}

// superblock刷入磁盘
// 内存inodeTable刷入磁盘
// 缓存区中所有延迟写的块刷入磁盘
void FileSystem::Update() {
	Buf* pBuffer;
	superBlock->s_fmod = 0;
    superBlock->s_time = (int)time(NULL);
	for (int j = 0; j < 2; j++) {
		int* p = (int *)superBlock + j * 128;

		// 拿到对应缓存
		pBuffer = this->bufferManager->GetBlk(FileSystem::SUPERBLOCK_START_SECTOR + j);
		// 内存写入缓存
		memcpy(pBuffer->b_addr, p, BLOCK_SIZE);

		// 缓存写磁盘
		this->bufferManager->Bwrite(pBuffer);
	}
	// 内存inodeTable写磁盘
	g_INodeTable.UpdateInodeTable();
	// 缓存所有延迟写的缓存块刷入磁盘
	this->bufferManager->Bflush();
}

/* 在存储设备上分配空闲磁盘块 */
Buf* FileSystem::Alloc() {
	int blkno;
	Buf* pBuffer;
	User& u = g_User;

    /* 从索引表“栈顶”获取空闲磁盘块编号 */
	blkno = superBlock->s_free[--superBlock->s_nfree];

    /* 若获取磁盘块编号为零，则表示已分配尽所有的空闲磁盘块。*/
	if (blkno <= 0) {
		superBlock->s_nfree = 0;
        // cout<<"alloc error"<<endl;
		u.u_error = User::U_ENOSPC;
		return NULL;
	}

	/*
	* 栈已空，新分配到空闲磁盘块中记录了下一组空闲磁盘块的编号
	* 将下一组空闲磁盘块的编号读入SuperBlock的空闲磁盘块索引表s_free[100]中。
	*/
    if (superBlock->s_nfree <= 0) {
		pBuffer = this->bufferManager->Bread(blkno);
		int* p = (int *)pBuffer->b_addr;
		superBlock->s_nfree = *p++;
		memcpy(superBlock->s_free, p, sizeof(superBlock->s_free));
		this->bufferManager->Brelse(pBuffer);
	}
	pBuffer = this->bufferManager->GetBlk(blkno);
	if (pBuffer) {
		this->bufferManager->Bclear(pBuffer);
	}

	// 更新了superBlock
	superBlock->s_fmod = 1;
	return pBuffer;
}

/* 在存储设备dev上分配一个空闲外存INode，一般用于创建新的文件。*/
Inode* FileSystem::IAlloc() {
    Buf* pBuffer;
    Inode* pINode;
    User& u = g_User;
    int ino;

    /* SuperBlock直接管理的空闲Inode索引表已空，必须到磁盘上搜索空闲Inode。*/
    if (superBlock->s_ninode <= 0) {
        ino = -1;
        for (int i = 0; i < superBlock->s_isize; ++i) {
            pBuffer = this->bufferManager->Bread(FileSystem::INODE_ZONE_START_SECTOR + i);
            int* p = (int*)pBuffer->b_addr;
            for (int j = 0; j < FileSystem::INODE_NUMBER_PER_SECTOR; ++j) {
                ++ino;
                int mode = *(p + j * sizeof(DiskInode) / sizeof(int));

				// 0号inode永远不用
                if (mode || i*FileSystem::INODE_NUMBER_PER_SECTOR+j < FileSystem::ROOT_INODE_NO) {
                    continue;
                }

                /*
                * 如果外存inode的i_mode==0，此时并不能确定该inode是空闲的，
                * 因为有可能是内存inode没有写到磁盘上,所以要继续搜索内存inode中是否有相应的项
                */
                if (g_INodeTable.IsLoaded(ino) == -1) {
                    superBlock->s_inode[superBlock->s_ninode++] = ino;
                    if (superBlock->s_ninode >= 100) {
                        break;
                    }
                }
            }

            this->bufferManager->Brelse(pBuffer);
            if (superBlock->s_ninode >= 100) {
                break;
            }
        }
        if (superBlock->s_ninode <= 0) {
            // cout<<"ialloc error"<<endl;
            u.u_error = User::U_ENOSPC;
            return NULL;
        }
    }

    ino = superBlock->s_inode[--superBlock->s_ninode];
    pINode = g_INodeTable.IGet(ino);
    if (NULL == pINode) {
        cout << "无空闲内存INode" << endl;
        return NULL;
    }
    
    // 清diskInode原有属性
    pINode->Clean();
    superBlock->s_fmod = 1;
    return pINode;
}

/* 释放编号为number的外存INode，一般用于删除文件。*/
// TODO:不用清0吗？
void FileSystem::IFree(int number) {
	if (superBlock->s_ninode >= 100) {
		return ;
	}
	superBlock->s_inode[superBlock->s_ninode++] = number;
	superBlock->s_fmod = 1;
}

/* 释放存储设备dev上编号为blkno的磁盘块 */
void FileSystem::Free(int blkno) {
	Buf* pBuffer;
	User& u = g_User;

	if (superBlock->s_nfree >=100 ) {
		pBuffer = this->bufferManager->GetBlk(blkno);
		int *p = (int*)pBuffer->b_addr;
		*p++ = superBlock->s_nfree;
		memcpy(p, superBlock->s_free, sizeof(int)*100);
		superBlock->s_nfree = 0;
		this->bufferManager->Bwrite(pBuffer);
	}

	superBlock->s_free[superBlock->s_nfree++] = blkno;
	superBlock->s_fmod = 1;
}
