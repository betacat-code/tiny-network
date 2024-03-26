#include"EventLoop.h"
#include"Channel.h"
#include"../logger/Logging.h"
#include"EPollPoller.h"
#include"Buffer.h"
#include "InetAddress.h"
#include"EventLoop.h"
#include"EventLoopThreadPool.h"
#include"Acceptor.h"
#include"TcpConnection.h"
#include"TcpServer.h"
void test_channel()
{
    EventLoop t;
    Channel test(&t,100);
    test.enableReading();
    LOG_INFO<<"OK\n";
}

void test_EPollPoller()
{
    EventLoop t;
    EPollPoller test(&t);
    LOG_INFO<<"OK\n";
}

void test_buffer()
{
    Buffer in;
    char test[]="testetttt";
    in.append(test);
    std::cout<<in.retrieveAsString(2)<<std::endl;
    std::cout<<in.retrieveAllAsString()<<std::endl;
}

void test_address()
{
    InetAddress addr(8080);
    std::cout << addr.toIpPort() << std::endl;
}

void test_EventLoop()
{
    EventLoop test;
    test.loop();
    test.quit();
}

void test_EventLoopThreadPool()
{
    EventLoop base_loop;
    EventLoopThreadPool test(&base_loop,"tcp");
    test.setThreadNum(3);
    test.start();
}
void test(int sockfd, const InetAddress& tt)
{
    printf("NewConnectionCallback OK\n");
}
void test_Acceptor()
{
    Acceptor::NewConnectionCallback callback=test;
    InetAddress t;
    EventLoop base_loop;
    Acceptor test(&base_loop,t,true);
    test.setNewConnectionCallback(callback);
    test.listen();
}
void test_tcp_connection()
{
    InetAddress t;
    EventLoop base_loop;
    TcpConnection test(&base_loop,"test",90,t,t);
}
void test_call(EventLoop* t)
{
    std::cout<<"EventLoop OK server\n";
}
void test_tcp_server()
{
    EventLoop base_loop;
    TcpServer::ThreadInitCallback test_call_=test_call;
    InetAddress test;
    TcpServer t(&base_loop,test,"test_server");
    t.setThreadInitCallback(test_call_);
    t.start();
    base_loop.loop();
}
/*int main()
{
   test_tcp_server();
}*/