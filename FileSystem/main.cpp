#include <iostream>
#include <unordered_map>
#include <string>
using namespace std;
#include "Inode.h"
#include "File.h"
#include "FileSystem.h"
#include "OpenFileManager.h"
#include "FileManager.h"
#include "User.h"

DeviceDriver g_DeviceDriver;
BufferManager g_BufferManager;
OpenFileTable g_OpenFileTable;
SuperBlock g_SuperBlock;
FileSystem g_FileSystem;
InodeTable g_INodeTable;
FileManager g_FileManager;
User g_User;

void man(string command) {

    static string man =
        "******************************* 用户手册 ***************************************\n"
        "欢迎使用用户手册，本系统包含命令如下:\n"
        "man          :  手册 \n"
        "fformat      :  格式化 \n"
        "exit         :  正确退出 \n"
        "mkdir        :  新建目录 \n"
        "cd           :  改变目录 \n"
        "ls           :  列出目录及文件 \n"
        "create       :  新建文件 \n"
        "delete       :  删除文件 \n"
        "open         :  打开文件 \n"
        "close        :  关闭文件 \n"
        "seek         :  移动读写指针 \n"
        "write        :  写入文件 \n"
        "read         :  读取文件 \n"
        "cp           :  复制文件或文件夹\n"
        "mv           :  移动文件或文件夹\n"
        "at|autoTest  :  自动测试 \n"
        "******************************* 使用示例 ***************************************\n"
        "man mkdir    :  man [命令] 会显示命令对应的用户手册\n"
        ;

    static string fformat =
        "******************************* 文件系统格式化 ***************************************\n"
        "Description   :  将整个文件系统进行格式化，即清空所有文件及目录! \n"
        "Usage         :  fformat \n"
        "Parameter     :  无 \n"
        "******************************* 使用示例 ***************************************\n"
        "Usage Demo    :  fformat \n"
        ;

    static string exit =
        "******************************* 文件系统正确退出 ***************************************\n"
        "Description   :  若要退出程序，最好通过exit命令。此时正常退出会调用析构函数，\n"
        "              :  若有在内存中未更新到磁盘上的缓存会及时更新，保证正确性。若点 \n"
        "              :  击窗口关闭按钮，属于给当前程序发信号强制退出，不会调用析构函 \n"
        "              :  数，未写回部分信息，再次启动时可能出现错误! \n"
        "Usage         :  exit \n"
        "Parameter     :  无 \n"
        "******************************* 使用示例 ***************************************\n"
        "Usage Demo    :  exit \n"
        ;

    static string mkdir =
        "******************************* 建立目录 ***************************************\n"
        "Description   :  新建一个目录。若出现错误，会有相应错误提示! \n"
        "Usage         :  mkdir <目录名> \n"
        "Parameter     :  <目录名> 可以是相对路径，也可以是绝对路径 \n"
        "******************************* 使用示例 ***************************************\n"
        "Usage Demo    :  mkdir dirName \n"
        "                 mkdir ../dirName \n"
        "                 mkdir ../../dirName \n"
        "                 mkdir /dirName \n"
        "                 mkdir /dir1/dirName \n"
        "******************************* 注意事项 ***************************************\n"
        "目录名长度规范  :  目录名应小于28个字节 \n"
        "路径规范       :  请使用正确的绝对路径或相对路径 \n"
        "重名文件夹     :   如遇到重名文件夹，会停止创建新文件夹操作 \n"
        ;

    static string ls =
        "******************************* 列出文件夹下属文件或子文件夹 **********************\n"
        "Description   :  列出当前目录中包含的文件名或目录名。若出现错误，会有相应错误提示! \n"
        "Usage         :  ls \n"
        "Parameter     :  无 \n"
        "******************************* 使用示例 ***************************************\n"
        "Usage Demo    :  ls \n"
        "                 ls ../dirName \n"
        "                 ls ../../dirName \n"
        "                 ls /dirName \n"
        "                 ls /dir1/dirName \n"
        ;

    static string cd =
        "******************************* 改变当前文件夹 **********************************\n"
        "Description   :  改变当前工作目录。若出现错误，会有相应错误提示! \n"
        "Usage         :  cd <目录名> \n"
        "Parameter     :  <目录名>默认为当前目录；\n"
        "                 <目录名> 可以是相对路径，也可以是绝对路径 \n"
        "******************************* 使用示例 ***************************************\n"
        "Usage Demo    :  cd dirName \n"
        "                 cd ../dirName \n"
        "                 cd ../../dirName \n"
        "                 cd /dirName \n"
        "                 cd /dir1/dirName \n"
        "******************************* 注意事项 ***************************************\n"
        "目录名长度规范  :  目录名应小于28个字节 \n"
        "路径规范       :  请使用正确的绝对路径或相对路径 \n"
        "子文件夹规范    :  请确保最后一级的文件是文件夹类型 \n"
        ;

    static string create = 
        "******************************* 创建文件 ***************************************\n"
        "Description   :  新建一个文件。若出现错误，会有相应错误提示! \n"
        "Usage         :  create <文件名> <选项> \n"
        "Parameter     :  <文件名> 可以包含相对路径或绝对路径 \n"
        "                 <选项> -r 只读属性 \n"
        "                 <选项> -w 只写属性 \n"
        "                 <选项> -rw == -r -w 读写属性 \n"
        "******************************* 使用示例 ***************************************\n"
        "Usage Demo    :  create newFileName -rw \n"
        "                 create ../newFileName -rw \n"
        "                 create ../../newFileName -rw \n"
        "                 create /newFileName -rw \n"
        "                 create /dir1/newFileName -rw \n"
        "******************************* 注意事项 ***************************************\n"
        "文件名长度规范  :  文件名应小于28个字节 \n"
        "路径规范       :  请使用正确的绝对路径或相对路径 \n"
        "权限规范       :  请输入正确的权限 \n"
        ;

    static string delet =
        "******************************* 删除文件 ***************************************\n"
        "Description   :  删除一个文件。若出现错误，会有相应错误提示! \n"
        "Usage         :  delete <文件名> \n"
        "Parameter     :  <文件名> 可以包含相对路径或绝对路径 \n"
        "******************************* 使用示例 ***************************************\n"
        "Usage Demo    :  delete fileName \n"
        "                 delete ../fileName \n"
        "                 delete ../../fileName \n"
        "                 delete /fileName \n"
        "                 delete /dir1/fileName \n"
        "******************************* 注意事项 ***************************************\n"
        "文件名长度规范  :  文件名应小于28个字节 \n"
        "路径规范       :  请使用正确的绝对路径或相对路径 \n"
        ;

    static string open =
        "******************************* 打开文件 ***************************************\n"
        "Description   :  类Unix|Linux函数open，打开一个文件。若要进行文件的读写必须先open。\n"
        "                 若出现错误，会有相应错误提示! \n"
        "Usage         :  open <文件名> <选项> \n"
        "Parameter     :  <文件名> 可以包含相对路径或绝对路径 \n"
        "                 <选项> -r 只读属性 \n"
        "                 <选项> -w 只写属性 \n"
        "                 <选项> -rw == -r -w 读写属性 \n"
        "******************************* 使用示例 ***************************************\n"
        "Usage Demo    :  open fileName -r \n"
        "                 open ../fileName -w \n"
        "                 open ../../fileName -rw \n"
        "                 open /fileName -r -w \n"
        "                 open /dir1/fileName -rw \n"
        "******************************* 注意事项 ***************************************\n"
        "文件名长度规范  :  文件名应小于28个字节 \n"
        "路径规范       :  请使用正确的绝对路径或相对路径 \n"
        ;

    static string close =
        "******************************* 关闭文件 ***************************************\n"
        "Description   :  类Unix|Linux函数close，关闭一个文件。可以对已经打开的文件进行关闭 \n"
        "                 若出现错误，会有相应错误提示! \n"
        "Usage         :  close <file descriptor> \n"
        "Parameter     :  <file descriptor> 文件描述符 \n"
        "******************************* 使用示例 ***************************************\n"
        "Usage Demo    :  close 1 \n"
        "******************************* 注意事项 ***************************************\n"
        "文件描述符规范  :  请使用正确的文件描述符[0..15] \n"
        ;

    static string seek =
        "******************************* 更改文件偏移指针 *********************************\n"
        "Description   :  类Unix|Linux函数fseek，写入一个已经打开的文件中。若出现错误，会有相应错误提示! \n"
        "Usage         :  seek <file descriptor> <offset> <origin> \n"
        "Parameter     :  <file descriptor> open返回的文件描述符 \n"
        "                 <offset> 指定从 <origin> 开始的偏移量 可正可负 \n"
        "                 <origin> 指定起始位置 可为0(SEEK_SET), 1(SEEK_CUR), 2(SEEK_END) \n"
        "******************************* 使用示例 ***************************************\n"
        "Usage Demo    :  seek 1 500 0 \n"
        "******************************* 注意事项 ***************************************\n"
        "文件描述符规范  :  请使用正确的文件描述符[0..15] \n"
        ;

    static string write =
        "******************************* 写文件 *****************************************\n"
        "Description   :  类Unix|Linux函数write，写入一个已经打开的文件中。若出现错误，会有相应错误提示! \n"
        "Usage         :  write <file descriptor> <InFileName> <size> \n"
        "Parameter     :  <file descriptor> open返回的文件描述符 \n"
        "                 <InFileName> 指定写入内容为文件InFileName中的内容 \n"
        "                 <size> 指定写入字节数，大小为 <size> \n"
        "******************************* 使用示例 ***************************************\n"
        "Usage Demo    :  write 1 InFileName 123 \n"
        "******************************* 注意事项 ***************************************\n"
        "文件描述符规范  :  请使用正确的文件描述符[0..15] \n"
        ;

    static string read =
        "******************************* 读文件 *****************************************\n"
        "Description   :  类Unix|Linux函数read，从一个已经打开的文件中读取。若出现错误，会有相应错误提示! \n"
        "Usage         :  read <file descriptor> [-o <OutFileName>] <size> \n"
        "Parameter     :  <file descriptor> open返回的文件描述符 \n"
        "                 [-o <OutFileName>] -o 指定输出方式为文件，文件名为 <OutFileName> 默认为shell \n"
        "                 <size> 指定读取字节数，大小为 <size> \n"
        "******************************* 使用示例 ***************************************\n"
        "Usage Demo    :  read 1 -o OutFileName 123 \n"
        "                 read 1 123 \n"
        "******************************* 注意事项 ***************************************\n"
        "文件描述符规范  :  请使用正确的文件描述符[0..15] \n"
        ;

    static string copy = 
        "******************************* 复制文件 *****************************************\n"
        "Description   :  类Unix|Linux函数copy -r，复制文件或文件夹。 \n"
        "Usage         :  cp <srcFile> <destPath> \n"
        "Parameter     :  <srcFile> 待复制的源文件或文件夹 \n"
        "                 <destPath> 复制操作的目标路径 \n"
        "******************************* 使用示例 ***************************************\n"
        "Usage Demo    :  cp /home/432 /home/371 \n"
        "                 cp /home/432/test.txt /home/371 \n"
        "******************************* 注意事项 ***************************************\n"
        "路径规范       :  请使用正确的绝对路径或相对路径 \n"
        ;

    static string move = 
        "******************************* 移动文件 *****************************************\n"
        "Description   :  类Unix|Linux函数mv，移动文件或文件夹。 \n"
        "Usage         :  mv <srcFile> <destPath> \n"
        "Parameter     :  <srcFile> 待移动的源文件或文件夹 \n"
        "                 <destPath> 移动操作的目标路径 \n"
        "******************************* 使用示例 ***************************************\n"
        "Usage Demo    :  mv /home/432 /home/371 \n"
        "                 mv /home/432/test.txt /home/371 \n"
        "******************************* 注意事项 ***************************************\n"
        "路径规范       :  请使用正确的绝对路径或相对路径 \n"
        ;

    static string cat = 
        "******************************* 显示文件 *****************************************\n"
        "Description   :  类Unix|Linux函数cat，显示文件到屏幕上。 \n"
        "                 每次默认显示200个字节，若显示完毕则自动退出。\n"
        "                 若未显示完毕，等待用户输入。\n"
        "                 用户输入n继续显示接下来的200个字节。\n"
        "                 用户输入q则退出。\n"
        "Usage         :  cat <fileName> \n"
        "Parameter     :  <fileName> 待显示的文件 \n"
        "******************************* 使用示例 ***************************************\n"
        "Usage Demo    :  cat /home/432/test.txt \n"
        "                 cat test.txt \n"
        "******************************* 注意事项 ***************************************\n"
        "路径规范       :  请使用正确的绝对路径或相对路径 \n"
        ;

    static string autoTest =
        "Command       :  autoTest -自动测试 \n"
        "Description   :  帮助测试，在系统启动初期帮助测试。测试不一定所有命令都是正确的，但是系统具有容错性，\n"
        "              :  不会使系统异常。\n"
        "Usage         :  autoTest | at \n"
        "Parameter     :  无 \n"
        "Usage Demo    :  at \n"
        ;

    static unordered_map<string, string*>manMap({
        { "man", &man },
        { "at", &autoTest },
        { "fformat", &fformat },
        { "exit", &exit },
        { "mkdir", &mkdir },
        { "cd", &cd },
        { "ls", &ls },
        { "create", &create },
        { "delete", &delet },
        { "open", &open },
        { "close", &close },
        { "seek", &seek },
        { "write", &write },
        { "read", &read },
        { "cp", &copy},
        { "mv", &move},
        { "cat", &cat},
        { "autoTest", &autoTest },
        });

    auto it = manMap.find(command);
    if (it == manMap.end()) {
        cout << "shell : " << command << " : don't find this commond \n";
        return;
    }
    cout << *it->second;
}

bool autoTest(string& cmd) {
    static int testNo = 0;
    static char* testCmds[] = {
        "mkdir /dir1",
        "mkdir dir2",
        "create file1 -rw",
        "create file2 -rw",
        "ls",
        "delete /file2", 
        "ls",
        "mkdir /dir1/dir11",
        "create /dir1/file11 -rw",
        "cd dir1",
        "ls",
        "cd ..",
        "ls",
        "open file1 -rw",
        "write 6 file1-2000.txt 800", 
        "seek 6 500 0",
        "read 6 20", 
        "seek 6 500 0",
        "read 6 -o readOut1.txt 20", 
        "seek 6 -20 1",
        "read 6 100", 
        "close 6",
        "cd dir1",
        "ls",
        "open file11 -rw",
        "write 6 file11-100000.txt 100000",
        "seek 6 0 0",
        "read 6 100",
        "read 6 100",
        "seek 6 50000 0",
        "read 6 100"
    };
    int cmdNums = sizeof(testCmds) / sizeof(char*);
    cmd = testCmds[testNo % cmdNums];
    return ++testNo <= cmdNums;
}

void cmdArgs(const string& cmd, vector<string>& args) {
    args.clear();
    string str;
    unsigned int p, q;
    for (p = 0, q = 0; q < cmd.length(); p = q + 1) {
        q = cmd.find_first_of(" \n", p);
        str = cmd.substr(p, q - p);
        if (!str.empty()) {
            args.push_back(str);
        }
        if (q == string::npos)
            return;
    }
}

int main() {
    User* user = &g_User;

    string line = "man";
    vector<string> args;
    string cmd, arg1, arg2, arg3;
    int autoFlag = 0;
    // cout << "++++++++++++++++++++ Unix文件系统模拟 ++++++++++++++++++++" << endl;

    while (1) {
        if (line == "")
            goto NEXT_INPUT;

        cmdArgs(line, args);
        cmd = args[0];
        arg1 = args.size() > 1 ? args[1] : "";
        arg2 = args.size() > 2 ? args[2] : "";
        arg3 = args.size() > 3 ? args[3] : "";
        if (cmd == "man") {
            man(arg1.empty() ? "man" : arg1);
        }
        else if (cmd == "fformat") {
            g_OpenFileTable.Format();
            g_INodeTable.Format();
            g_BufferManager.FormatBuffer();
            g_FileSystem.FormatDevice();
            exit(0);
        }
        else if (cmd == "exit") {
            exit(0);
        }
        else if (cmd == "at" || cmd == "autoTest") {
            autoFlag = 1;
            cout << "autoTest begin ... \njust press enter to continue ... \n" ;
        }
        else if (cmd == "mkdir") {
            user->Mkdir(args[1]);
        }
        else if (cmd == "ls") {
            user->Ls();
        }
        else if (cmd == "cd") {
            user->Cd(arg1);
        }
        else if (cmd == "create") {
            //create 没有需要模式的必要
            user->Create(arg1, arg2 + arg3);
        }
        else if (cmd == "delete") {
            user->Delete(arg1);
        }
        else if (cmd == "open") {
            user->Open(arg1, line);
        }
        else if (cmd == "close") {
            user->Close(arg1);
        }
        else if (cmd == "seek") {
            user->Seek(arg1, arg2, arg3);
        }
        else if (cmd == "read") {
            if (arg2 == "-o")
                user->Read(arg1, arg3, args[4]);
            else
                user->Read(arg1, "", arg2);
        }
        else if (cmd == "write") {
            user->Write(arg1, arg2, arg3);
        }
        else if (cmd == "cp") {
            user->Copy(arg1,arg2);
        }
        else if (cmd == "mv") {
            user->Move(arg1,arg2);
        }
        else if (cmd == "cat"){
            user->Cat(arg1);
        }
        else if (cmd != "") {
            cout << "shell : " << cmd << " : don't find this command \n";
        }

    NEXT_INPUT:
        cout << "[qihaoyu " << user->u_curdir << " ]$ ";
        if (autoFlag) {
            if (autoTest(line)) {
                cout << line;
                getchar();
            }
            else {
                cout << "autoTest finished ... \n";
                autoFlag = 0;
                line = "";
            }
        }
        else {
            getline(cin, line);
        }
    }
    return 0;
}