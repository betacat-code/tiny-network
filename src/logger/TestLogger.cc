#include"LogStream.h"
#include "FileUtil.h"
#include "Logging.h"
#include"LogFile.h"
#include<iostream>
#include"AsyncLogging.h"
 #include <unistd.h>
static const off_t kRollSize = 1*1024*1024;
AsyncLogging* g_asyncLog = NULL;

void test_FileUtil()
{
    std::string temp="test.txt";
    FileUtil test(temp);
    char t[]="test test test";
    test.append(t,sizeof(t));
    test.flush();
}
void test_Logging()
{
    LOG_DEBUG << "debug";
    LOG_INFO << "info";
    LOG_WARN << "warn";
    LOG_ERROR << "error";
    // 注意不能轻易使用 LOG_FATAL, LOG_SYSFATAL, 会导致程序abort

    const int n = 10;
    for (int i = 0; i < n; ++i) {
        LOG_INFO << "Hello, " << i << " abc...xyz";
    }
}

void test_LogFile()
{
    char t[]="testlogfile";
    LogFile test("testlogfile",kRollSize);
    test.append(t,sizeof(t));
}

void test_AsyncLogging()
{
    const int n = 10;
    for (int i = 0; i < n; ++i) {
        LOG_INFO << "Hello, " << i << " abc...xyz";
    }
}

inline AsyncLogging* getAsyncLog()
{
    return g_asyncLog;
}

//写入空间巨大无比 需要找问题
void asyncLog(const char* msg, int len)
{
    AsyncLogging* logging = getAsyncLog();
    if (logging)
    {
        logging->append(msg, len);
    }
}

/*int main()
{
    AsyncLogging log("test_asy", kRollSize);
    //test_Logging();

    sleep(1);

    g_asyncLog = &log;
    Logger::setOutput(asyncLog); // 为Logger设置输出回调, 重新配接输出位置
    log.start(); // 开启日志后端线程

    //test_Logging();
    test_AsyncLogging();

    sleep(1);
    log.stop();
}

*/