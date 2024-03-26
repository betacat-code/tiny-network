# 基类Poller的设计

负责监听文件描述符事件是否触发以及返回发生事件的文件描述符以及具体事件


```cpp
class Poller : noncopyable
{
 public:
  // Poller关注的Channel
  typedef std::vector<Channel*> ChannelList;

  Poller(EventLoop* loop);
  virtual ~Poller();

  /**
   * 需要交给派生类实现的接口
   * 用于监听感兴趣的事件和fd(封装成了channel)
   * 对于Poller是poll，对于EPollerPoller是epoll_wait
   * 最后返回epoll_wait/poll的返回时间
   */
  virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

  // 需要交给派生类实现的接口（须在EventLoop所在的线程调用）
  // 更新事件，channel::update->eventloop::updateChannel->Poller::updateChannel
  virtual void updateChannel(Channel* channel) = 0;

  // 需要交给派生类实现的接口（须在EventLoop所在的线程调用）
  // 当Channel销毁时移除此Channel
  virtual void removeChannel(Channel* channel) = 0;
  
  // 需要交给派生类实现的接口
  virtual bool hasChannel(Channel* channel) const;
  
  /** 
   * newDefaultPoller获取一个默认的Poller对象（内部实现可能是epoll或poll）
   * 它的实现并不在 Poller.cc 文件中
   * 如果要实现则可以预料其会包含EPollPoller PollPoller
   * 那么外面就会在基类引用派生类的头文件，这个抽象的设计就不好
   * 所以外面会单独创建一个 DefaultPoller.cc 的文件去实现
   */
  static Poller* newDefaultPoller(EventLoop* loop);

  // 断言是否在创建EventLoop的所在线程
  void assertInLoopThread() const
  {
    ownerLoop_->assertInLoopThread();
  }

 protected:
  // 保存fd => Channel的映射
  typedef std::map<int, Channel*> ChannelMap;
  ChannelMap channels_;

 private:
  EventLoop* ownerLoop_;
};
```

- `ChannelMap channels_` 需要存储从 fd -> channel 的映射
- `ownerLoop_` 定义 Poller 所属的事件循环 EventLoop


# EPollPoller类设计
```cpp
class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller() override;

    // 重写基类Poller的抽象方法
    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    // 默认监听事件数量
    static const int kInitEventListSize = 16;

    static const char* operationToString(int op);

    // 填写活跃的连接
    // EventLoop内部调用此方法，会将发生了事件的Channel填充到activeChannels中
    void fillActiveChannels(int numEvents,
                            ChannelList* activeChannels) const;
    // 更新channel通道，本质是调用了epoll_ctl
    void update(int operation, Channel* channel);
    
    // epoll_event数组
    typedef std::vector<struct epoll_event> EventList;

    // epoll句柄
    int epollfd_;
    // epoll_event数组
    EventList events_;
};
```

- `kInitEventListSize` 默监听事件的数量
- `epollfd_` 我们使用 epoll_create 创建的指向 epoll 对象的文件描述符（句柄）
- `EventList events_` 返回事件的数组



## 成员函数
### 返回发生事件的 poll 方法
该方法内部调用 epoll_wait 获取发生的事件，并找到这些事件对应的 Channel 并将这些活跃的 Channel 填充入 activeChannels 中，最后返回一个时间戳。
通过 `numEvents` 的值判断事件情况

-  `numEvents > 0`
   - 事件发生，需要调用 `fillActiveChannels` 填充活的 Channel
- `numEvents == 0`
   - 事件超时了，打印日志。（可以设置定时器操作）
- 其他情况则是出错，打印 `LOG_ERROR`日志

### 填写活跃的连接 fillActiveChannels
通过 epollwait 返回的 events 数组内部有指向 channel 的指针，我们可以通过此指针在 EPollPoller 模块获取对 channel 进行操作。
我们需要更新 channel 的返回事件的设置，并且将此 channel 装入 activeChannels。

### 更新channel在epoll上的状态
我们获取 channel 在 EPollPoller 上的状态，根据状态进行不同操作。最后调用 update 私有方法。

- 如果此 channel 还没有被添加到 epoll 上或者是之前已经被 epoll 上注销，那么此 channel 接下来会进行添加操作`index == kNew || index == kDeleted`
   - 如果是未添加状态，则需要在 map 上增加此 channel
   - 如果是已删除状态，则直接往后执行
   - 设置 channel 状态为 `kAdded` ，然后调用 `update(EPOLL_CTL_ADD, channel);`
- 如果已经在 poller 上注册的状态，则要进行删除或修改操作，需要判断此 channel 是否还有监视的事情（是否还要事件要等着处理）
   - 如果没有则直接删除，调用 `update(EPOLL_CTL_DEL, channel);` 并重新设置状态为 `kDeleted`
   - 如果还有要监视的事情，则说明要进行修改（MOD）操作，调用 `update(EPOLL_CTL_MOD, channel);`其本质是调用 `epoll_ctl` 函数，而里面的操作由之前的 `updateChannel` 所指定。
