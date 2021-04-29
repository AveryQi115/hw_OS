#include "BufferManager.h"
#include "console_output.h"

extern DeviceDriver g_DeviceDriver;

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

    if ((bp = find(blkno)) != nullptr) {
        detachNode(bp);
        return bp;
    }
    bp = m_Buf_head->back;
	if (bp == m_Buf_head) {
		Diagnose::Write("无Buffer可供使用");
		return nullptr;
	}
    detachNode(bp);
	
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

Buf* BufferManager::Bread(int blkno)
{
	Buf* bp;
	
	bp = this->GetBlk(blkno);
	
	if(bp->b_flags & (Buf::B_DONE | Buf::B_DELWRI))
	{
		return bp;
	}
	
	m_DeviceDriver->read(bp->b_addr,BUFFER_SIZE,bp->b_blkno*BUFFER_SIZE);
	bp->b_flags |= Buf::B_DONE;

	// GetBlk会自动调用detachNode
	// 读完记得将缓存放入队头
	Brelse(bp);
	return bp;
}

void BufferManager::Bwrite(Buf *bp)
{
	// 同步写，清延迟写的标志
	bp->b_flags &= ~(Buf::B_DELWRI);
	m_DeviceDriver->write(bp->b_addr,BUFFER_SIZE,bp->b_blkno*BUFFER_SIZE);
	bp->b_flags |= Buf::B_DONE;
	this->Brelse(bp);
	return;
}

/* 清空缓冲区内容 */
void BufferManager::Bclear(Buf *bp) {
	memset(bp->b_addr, 0, BUFFER_SIZE);
	return;
}

void BufferManager::Bflush(){
	Buf* pb = nullptr;
	for(int i=0;i<NBUF;i++){
		pb = m_Buf;
		if(pb->b_flags & Buf::B_DELWRI){
			pb->b_flags &= ~(Buf::B_DELWRI);
			m_DeviceDriver->write(pb->b_addr,BUFFER_SIZE,pb->b_blkno*BUFFER_SIZE);
			pb->b_flags |= Buf::B_DONE;
		}
	}
}

