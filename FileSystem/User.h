#ifndef USER_H
#define USER_H

#include "FileManager.h"
#include <string>
using namespace std;

/*
 * @comment 该类与Unixv6中 struct user结构对应，因此只改变
 * 类名，不修改成员结构名字，关于数据类型的对应关系如下:
 */
class User
{
public:
	static const int EAX = 0;	/* u.u_ar0[EAX]；访问现场保护区中EAX寄存器的偏移量 */
	
	/* u_error's Error Code */
	/* 1~32 来自linux 的内核代码中的/usr/include/asm/errno.h, 其余for V6++ */
	enum ErrorCode
	{
		U_NOERROR	= 0,	/* No error */
		// EPERM	= 1,	/* Operation not permitted */
		U_ENOENT	= 2,	/* No such file or directory */
		// ESRCH	= 3,	/* No such process */
		// EINTR	= 4,	/* Interrupted system call */
		// EIO		= 5,	/* I/O error */
		// ENXIO	= 6,	/* No such device or address */
		// E2BIG	= 7,	/* Arg list too long */
		// ENOEXEC	= 8,	/* Exec format error */
		U_EBADF	= 9,	/* Bad file number */
		// ECHILD	= 10,	/* No child processes */
		// EAGAIN	= 11,	/* Try again */
		// ENOMEM	= 12,	/* Out of memory */
		U_EACCES	= 13,	/* Permission denied */
		// EFAULT  = 14,	/* Bad address */
		// ENOTBLK	= 15,	/* Block device required */
		// EBUSY 	= 16,	/* Device or resource busy */
		// EEXIST	= 17,	/* File exists */
		// EXDEV	= 18,	/* Cross-device link */
		// ENODEV	= 19,	/* No such device */
		U_ENOTDIR	= 20,	/* Not a directory */
		// EISDIR	= 21,	/* Is a directory */
		// EINVAL	= 22,	/* Invalid argument */
		U_ENFILE	= 23,	/* File table overflow */
		U_EMFILE	= 24,	/* Too many open files */
		// ENOTTY	= 25,	/* Not a typewriter(terminal) */
		// ETXTBSY	= 26,	/* Text file busy */
		U_EFBIG	= 27,	/* File too large */
		U_ENOSPC	= 28,	/* No space left on device */
		// ESPIPE	= 29,	/* Illegal seek */
		// EROFS	= 30,	/* Read-only file system */
		// EMLINK	= 31,	/* Too many links */
		// EPIPE	= 32,	/* Broken pipe */
		// ENOSYS	= 100,
		//EFAULT	= 106
	};

public:
	long u_arg[5];				/* 存放当前系统调用参数 */
	string u_dirp;				/* 系统调用参数(一般用于Pathname)的指针 */
	unsigned int	u_ar0[5];
	
	/* 文件系统相关成员 */
	Inode* u_cdir;		/* 指向当前目录的Inode指针 */
	Inode* u_pdir;		/* 指向父目录的Inode指针 */

	DirectoryEntry u_dent;					/* 当前目录的目录项 */
	char u_dbuf[DirectoryEntry::DIRSIZ];	/* 当前路径分量 */
	string u_curdir;						/* 当前工作目录完整路径 */

	ErrorCode u_error;			/* 存放错误码 */
	
	/* 文件系统相关成员 */
	OpenFiles u_ofiles;		/* 进程打开文件描述符表对象 */

	/* 文件I/O操作 */
	IOParameter u_IOParam;	/* 记录当前读、写文件的偏移量，用户目标区域和剩余字节数参数 */

	FileManager* fileManager;

	string ls;

	/* Member Functions */
public:
	User();
	~User();

	void Ls();
    void Cd(string dirName);
    void Mkdir(string dirName);
    void Create(string fileName, string mode);
    void Delete(string fileName);
    void Open(string fileName, string mode);
    void Close(string fd);
    void Seek(string fd, string offset, string origin);
    void Write(string fd, string inFile, string size);
    void Read(string fd, string outFile, string size);
	void Copy(string srcFile, string desPath);
	void Move(string srcFile, string desPath);
	void Cat(string srcFile);
	void debug();
    //void Pwd();
    
private:
    bool IsError();
    void EchoError(enum ErrorCode err);
    int INodeMode(string mode);
    int FileMode(string mode);
    bool checkPathName(string path);
};

#endif

