
#include"Timestamp.h"
#include"TimerQueue.h"
#include<iostream>

void test_time()
{
    printf("OK\n");
    return;
}
void test_timequeue()
{
    EventLoop test;
    TimerQueue t(&test);
    t.addTimer(test_time,Timestamp::now(),10);
    test.loop();
}
/*int main()
{
    test_timequeue();
}*/