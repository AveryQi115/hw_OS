#include "Utility.h"
#include "Kernel.h"
#include "User.h"
// #include "PageManager.h"
// #include "Machine.h"
// #include "MemoryDescriptor.h"
// #include "Video.h"
#include "Assembly.h"

void Utility::MemCopy(unsigned long src, unsigned long des, unsigned int count)
{
	unsigned char* psrc = (unsigned char*)src;
	unsigned char* pdes = (unsigned char*)des;
	
	for ( unsigned int i = 0; i < count; i++ ) 
		pdes[i] = psrc[i];
}

int Utility::CaluPageNeed(unsigned int memoryneed, unsigned int pagesize)
{
	int pageRequired = memoryneed / pagesize;
	pageRequired += memoryneed % pagesize ? 1 : 0;

	return pageRequired;
}

void Utility::StringCopy(char* src, char* dst)
{
	while ( (*dst++ = *src++) != 0 ) ;
}

int Utility::StringLength(char* pString)
{
	int length = 0;
	char* pChar = pString;

	while( *pChar++ )
	{
		length++;
	}

	/* 返回字符串长度 */
	return length;
}

short Utility::GetMajor(const short dev)
{
	short major;
	major = dev >> 8;
	return major;
}

short Utility::GetMinor(const short dev)
{
	short minor;
	minor = dev & 0x00FF;
	return minor;
}

short Utility::SetMajor(short dev, const short value)
{
	dev &= 0x00FF;	/*  清除dev中原先高8比特 */
	dev |= (value << 8);
	return dev;
}

short Utility::SetMinor(short dev, const short value)
{
	dev &= 0xFF00;	/*  清除dev中原先低8比特 */
	dev |= (value & 0x00FF);	/* 仅保留value中的低8位 */
	return dev;
}

void Utility::Panic(char* str)
{
	Diagnose::TraceOn();
	Diagnose::Write("%s\n", str);
	X86Assembly::CLI();
	for(;;);
}

void Utility::DWordCopy(int *src, int *dst, int count)
{
	while(count--)
	{
		*dst++ = *src++;
	}
	return;
}

int Utility::Min(int a, int b)
{
	if(a < b)
		return a;
	return b;
}

int Utility::Max(int a, int b)
{
	if(a > b)
		return a;
	return b;
}

void Utility::IOMove(unsigned char* from, unsigned char* to, int count)
{
	while(count--)
	{
		*to++ = *from++;
	}
	return;
}
