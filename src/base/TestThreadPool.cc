#include "ThreadPool.h"
#include <iostream>
#include <unistd.h>
#include <sys/syscall.h>
#include "Thread.h"
void MyTaskFunc(int i) 
{
  printf("Info: thread %ld is working on task %d\n", syscall(SYS_gettid), i);
  sleep(1);
  return;
}

void test_pool()
{
  ThreadPool threadpool(20);
  for (int i = 0; i < 100; ++i) {
    threadpool.enqueue(MyTaskFunc, i);
  }
}

void t_print()
{
  for(int i=0;i<2;i++)
  {
    printf("Info: thread %ld is working", syscall(SYS_gettid));
    sleep(1);
  }
}

void test_thread()
{
  std::vector<Thread*> threads;
  for(int i=0;i<5;i++)
  {
    threads.push_back(new Thread(t_print));
    threads[i]->start();
  }
  for(int i=0;i<5;i++)
  {
    threads[i]->join();
  }
}

/*int main() {
  test_thread();
  return 0;
}*/