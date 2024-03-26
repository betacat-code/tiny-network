# Channel类
Channel 对文件描述符和事件进行了一层封装。平常我们写网络编程相关函数，基本就是创建套接字，绑定地址，转变为可监听状态（这部分在 Socket 类中实现过了，交给 Acceptor 调用即可），然后接受连接。

但是得到了一个初始化好的 socket 还不够，我们是需要监听这个 socket 上的事件并且处理事件的。比如我们在 Reactor 模型中使用了 epoll 监听该 socket 上的事件，我们还需将需要被监视的套接字和监视的事件注册到 epoll 对象中。

可以想到文件描述符和事件和 IO 函数全都混在在了一起，极其不好维护。而 muduo 中的 Channel 类将文件描述符和其感兴趣的事件（需要监听的事件）封装到了一起。而事件监听相关的代码放到了 Poller EPollPoller 类中。

# 成员变量
```cpp
/**
* const int Channel::kNoneEvent = 0;
* const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
* const int Channel::kWriteEvent = EPOLLOUT;
*/
static const int kNoneEvent;
static const int kReadEvent;
static const int kWriteEvent;

EventLoop *loop_;   // 当前Channel属于的EventLoop
const int fd_;      // fd, Poller监听对象
int events_;        // 注册fd感兴趣的事件
int revents_;       // poller返回的具体发生的事件
int index_;         // 在Poller上注册的情况

std::weak_ptr<void> tie_;   // 弱指针指向TcpConnection(必要时升级为shared_ptr多一份引用计数，避免用户误删)
bool tied_;  // 标志此 Channel 是否被调用过 Channel::tie 方法

// 保存着事件到来时的回调函数
ReadEventCallback readCallback_; 	// 读事件回调函数
EventCallback writeCallback_;		// 写事件回调函数
EventCallback closeCallback_;		// 连接关闭回调函数
EventCallback errorCallback_;		// 错误发生回调函数
```

- `int fd_`：这个Channel对象照看的文件描述符
- `int events_`：代表fd感兴趣的事件类型集合
- `int revents_`：代表事件监听器实际监听到该fd发生的事件类型集合，当事件监听器监听到一个fd发生了什么事件，通过`Channel::set_revents()`函数来设置revents值。
- `EventLoop* loop_`：这个 Channel 属于哪个EventLoop对象，因为 muduo 采用的是 one loop per thread 模型，所以我们有不止一个 EventLoop。我们的 manLoop 接收新连接，将新连接相关事件注册到线程池中的某一线程的 subLoop 上（轮询）。我们不希望跨线程的处理函数，所以每个 Channel 都需要记录是哪个 EventLoop 在处理自己的事情，这其中还涉及到了线程判断的问题。
- `read_callback_` 、`write_callback_`、`close_callback_`、`error_callback_`：这些是 std::function 类型，代表着这个Channel为这个文件描述符保存的各事件类型发生时的处理函数。比如这个fd发生了可读事件，需要执行可读事件处理函数，这时候Channel类都替你保管好了这些可调用函数。到时候交给 EventLoop 执行即可。
- `index `：我们使用 index 来记录 channel 与 Poller 相关的几种状态，Poller 类会判断当前 channel 的状态然后处理不同的事情。
   - `kNew`：是否还未被poll监视 
   - `kAdded`：是否已在被监视中 
   - `kDeleted`：是否已被移除
- `kNoneEvent`、`kReadEvent`、`kWriteEvent`：事件状态设置会使用的变量


## 用于增加TcpConnection生命周期的tie方法（防止用户误删操作）
```cpp
// 在TcpConnection建立得时候会调用
void Channel::tie(const std::shared_ptr<void> &obj)
{
    // weak_ptr 指向 obj
    tie_ = obj;
    // 设置tied_标志
    tied_ = true;
}
```
用户使用muduo库的时候，会利用到TcpConnection。用户可以看见 TcpConnection，如果用户注册了要监视的事件和处理的回调函数，并在处理 subLoop 处理过程中「误删」了 TcpConnection 的话会发生什么呢？

总之，EventLoop 肯定不能很顺畅的运行下去。毕竟它的生命周期小于 TcpConnection。为了防止用户误删的情况，TcpConnection 在创建之初 `TcpConnection::connectEstablished` 会调用此函数来提升对象生命周期。

实现方案是在处理事件时，如果对被调用了`tie()`方法的Channel对象，我们让一个共享型智能指针指向它，在处理事件期间延长它的生命周期。哪怕外面「误删」了此对象，也会因为多出来的引用计数而避免销毁操作。


## 根据相应事件执行Channel保存的回调函数
我们的Channel里面保存了许多回调函数，这些都是在对应的事件下被调用的。用户提前设置写好此事件的回调函数，并绑定到Channel的成员里。等到事件发生时，Channel自然的调用事件处理方法。借由回调操作实现了异步的操作。

