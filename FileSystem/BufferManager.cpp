#include "BufferManager.h"
#include "Kernel.h"
#include "Assembly.h"
#include "console_output.h"

extern DiskDriver g_DeviceDriver;

BufferManager::BufferManager()
{
	//new head Node
	m_Buf_head = new Buf;
	if(!m_Buf_head)
		Diagnose::Write("No space for Buffer head Node");
    Initialize();
    this->m_DeviceDriver = &g_DeviceDriver;
}

BufferManager::~BufferManager()
{
	//nothing to do here
	Bflush();
    delete m_Buf_head;
}

void BufferManager::Initialize()
{
	for (int i = 0; i < NBUF; ++i) {
        if (i) {
            m_Buf[i].forw = &m_Buf[i-1];
        }
        else {
            m_Buf[i].forw = m_Buf_head;
            m_Buf_head->back = m_Buf;
        }

        if (i + 1 < NBUF) {
            m_Buf[i].back = &m_Buf[i+1];
        }
        else {
            m_Buf[i].back = m_Buf_head;
            m_Buf_head->forw = &m_Buf[i];
        }
        m_Buf[i].b_addr = Buffer[i];
    }
}

// 实现LRU队列
// 如果缓存命中，取缓存
// 缓存未命中，取尾结点，刷新该结点使用完毕后将其放入头结点
Buf* BufferManager::GetBlk(int blkno)
{
	Buf* bp;
	User& u = Kernel::Instance().GetUser();

    if ((bp = find(blkno)) != nullptr) {
        DetachNode(bp);
        return bp;
    }
    bp = m_Buf_head->back;
	if (bp == m_Buf_head) {
		Diagnose::Write("无Buffer可供使用");
		return nullptr;
	}
    DetachNode(bp);
	
	// 如果有延迟写标志，代表该缓存块之前修改过但还未写入磁盘
	// 先写再返回该缓存块
    if (bp->b_flags & Buf::B_DELWRI) {
        m_DeviceDriver->write(bp->b_addr, BUFFER_SIZE, bp->b_blkno*BUFFER_SIZE);
    }

	// 刷新该缓存块
	bp->b_flags &= ~(Buf::B_DELWRI | Buf::B_DONE);
	bp->b_blkno = blkno;
    return bp;
}

// 释放缓存块记将缓存块插入队列头部
void BufferManager::Brelse(Buf* bp)
{
	insertHead(bp);
	return;
}

void BufferManager::insertHead(Buf* bp) {
    if (bp->back != nullptr) {
        return;
    }
    bp->forw = m_Buf_head->forw;
    bp->back = m_Buf_head;
    m_Buf_head->forw->back = bp;
    m_Buf_head->forw = bp;
}

void BufferManager::detachNode(Buf* bp) {
    if (bp->back == nullptr) {
        return;
    }
    bp->forw->back = bp->back;
    bp->back->forw = bp->forw;
    bp->back = NULL;
    bp->forw = NULL;
}

Buf* BufferManager::find(int blkno){
	Buf* p = m_Buf_head->back;
	while(p!=m_Buf_head){
		if(p->b_blkno==blkno)
			return p;
		p = p->back;
	}
	return nullptr;
}

void BufferManager::IOWait(Buf* bp)
{
	User& u = Kernel::Instance().GetUser();

	/* 这里涉及到临界区
	 * 因为在执行这段程序的时候，很有可能出现硬盘中断，
	 * 在硬盘中断中，将会修改B_DONE如果此时已经进入循环
	 * 则将使得改进程永远睡眠
	 */
	X86Assembly::CLI();
	while( (bp->b_flags & Buf::B_DONE) == 0 )
	{
		Diagnose::Write("Process Sleep in IOWait wating for IO to be done");
		//u.u_procp->Sleep((unsigned long)bp, ProcessManager::PRIBIO);
	}
	X86Assembly::STI();

	this->GetError(bp);
	return;
}

void BufferManager::IODone(Buf* bp)
{
	/* 置上I/O完成标志 */
	bp->b_flags |= Buf::B_DONE;
	if(bp->b_flags & Buf::B_ASYNC)
	{
		/* 如果是异步操作,立即释放缓存块 */
		this->Brelse(bp);
	}
	else
	{
		/* 清除B_WANTED标志位 */
		bp->b_flags &= (~Buf::B_WANTED);
		//Kernel::Instance().GetProcessManager().WakeUpAll((unsigned long)bp);
	}
	return;
}

Buf* BufferManager::Bread(short dev, int blkno)
{
	Buf* bp;
	/* 根据设备号，字符块号申请缓存 */
	bp = this->GetBlk(dev, blkno);
	/* 如果在设备队列中找到所需缓存，即B_DONE已设置，就不需进行I/O操作 */
	if(bp->b_flags & Buf::B_DONE)
	{
		return bp;
	}
	/* 没有找到相应缓存，构成I/O读请求块 */
	bp->b_flags |= Buf::B_READ;
	bp->b_wcount = BufferManager::BUFFER_SIZE;

	/* 
	 * 将I/O请求块送入相应设备I/O请求队列，如无其它I/O请求，则将立即执行本次I/O请求；
	 * 否则等待当前I/O请求执行完毕后，由中断处理程序启动执行此请求。
	 * 注：Strategy()函数将I/O请求块送入设备请求队列后，不等I/O操作执行完毕，就直接返回。
	 */
	this->m_DeviceManager->GetBlockDevice(Utility::GetMajor(dev)).Strategy(bp);
	/* 同步读，等待I/O操作结束 */
	this->IOWait(bp);
	return bp;
}

void BufferManager::Bwrite(Buf *bp)
{
	unsigned int flags;

	flags = bp->b_flags;
	bp->b_flags &= ~(Buf::B_READ | Buf::B_DONE | Buf::B_ERROR | Buf::B_DELWRI);
	bp->b_wcount = BufferManager::BUFFER_SIZE;		/* 512字节 */

	this->m_DeviceManager->GetBlockDevice(Utility::GetMajor(bp->b_dev)).Strategy(bp);

	if( (flags & Buf::B_ASYNC) == 0 )
	{
		/* 同步写，需要等待I/O操作结束 */
		this->IOWait(bp);
		this->Brelse(bp);
	}
	else if( (flags & Buf::B_DELWRI) == 0)
	{
	/* 
	 * 如果不是延迟写，则检查错误；否则不检查。
	 * 这是因为如果延迟写，则很有可能当前进程不是
	 * 操作这一缓存块的进程，而在GetError()主要是
	 * 给当前进程附上错误标志。
	 */
		this->GetError(bp);
	}
	return;
}

void BufferManager::ClrBuf(Buf *bp)
{
	int* pInt = (int *)bp->b_addr;

	/* 将缓冲区中数据清零 */
	for(unsigned int i = 0; i < BufferManager::BUFFER_SIZE / sizeof(int); i++)
	{
		pInt[i] = 0;
	}
	return;
}

void BufferManager::GetError(Buf* bp)
{
	User& u = Kernel::Instance().GetUser();

	if (bp->b_flags & Buf::B_ERROR)
	{
		u.u_error = User::EIO;
	}
	return;
}

void BufferManager::NotAvail(Buf *bp)
{
	X86Assembly::CLI();		/* spl6();  UNIX V6的做法 */
	/* 从自由队列中取出 */
	bp->av_back->av_forw = bp->av_forw;
	bp->av_forw->av_back = bp->av_back;
	/* 设置B_BUSY标志 */
	bp->b_flags |= Buf::B_BUSY;
	X86Assembly::STI();
	return;
}

Buf* BufferManager::InCore(short adev, int blkno)
{
	Buf* bp;
	Devtab* dp;
	short major = Utility::GetMajor(adev);

	dp = this->m_DeviceManager->GetBlockDevice(major).d_tab;
	for(bp = dp->b_forw; bp != (Buf *)dp; bp = bp->b_forw)
	{
		if(bp->b_blkno == blkno && bp->b_dev == adev)
			return bp;
	}
	return NULL;
}

Buf& BufferManager::GetBFreeList()
{
	return this->bFreeList;
}

