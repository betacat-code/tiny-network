# 缓冲区存在必要性

1. 非阻塞网络编程中应用层buffer是必须的：非阻塞IO的核心思想是避免阻塞在`read()`或`write()`或其他`I/O`系统调用上，这样可以最大限度复用`thread-of-control`，让一个线程能服务于多个`socket`连接。`I/O`线程只能阻塞在`IO-multiplexing`函数上，如`select()/poll()/epoll_wait()`。这样一来，应用层的缓冲是必须的，每个`TCP socket`都要有`inputBuffer`和`outputBuffer`。
2. TcpConnection必须有output buffer：使程序在`write()`操作上不会产生阻塞，当`write()`操作后，操作系统一次性没有接受完时，网络库把剩余数据则放入`outputBuffer`中，然后注册`POLLOUT`事件，一旦`socket`变得可写，则立刻调用`write()`进行写入数据。——应用层`buffer`到操作系统`buffer`
3. TcpConnection必须有input buffer：当发送方`send`数据后，接收方收到数据不一定是整个的数据，网络库在处理`socket`可读事件的时候，必须一次性把`socket`里的数据读完，否则会反复触发`POLLIN`事件，造成`busy-loop`。所以网路库为了应对数据不完整的情况，收到的数据先放到`inputBuffer`里。——操作系统`buffer`到应用层`buffer`。

# Buffer缓冲区设计
muduo 的 Buffer 类作为网络通信的缓冲区，像是 TcpConnection 就拥有 inputBuffer 和 outputBuffer 两个缓冲区成员。而缓冲区的设计特点：

1. 其内部使用`std::vector<char>`保存数据，并提供许多访问方法。并且`std::vector`拥有扩容空间的操作，可以适应数据的不断添加。
2. `std::vector<char>`内部分为三块，头部预留空间，可读空间，可写空间。内部使用索引标注每个空间的起始位置。每次往里面写入数据，就移动`writeIndex`；从里面读取数据，就移动`readIndex`。


## 向Buffer写入数据：readFd
`ssize_t Buffer::readFd(int fd, int* savedErrno)`：表示从 fd 中读取数据到 buffer_ 中。对于 buffer 来说这是写入数据的操作，会改变`writeIndex`。

1. 考虑到 buffer_ 的 writableBytes 空间大小，不能够一次性读完数据，于是内部还在栈上创建了一个临时缓冲区 `char extrabuf[65536];`。如果有多余的数据，就将其读入到临时缓冲区中。
2. 因为可能要写入两个缓冲区，所以使用了更加高效`readv`函数，可以向多个地址写入数据。刚开始会判断需要写入的大小。
   1. 如果一个缓冲区足够，就不必再往临时缓冲区`extrabuf`写入数据了。写入后需要更新`writeIndex`位置，`writerIndex_ += n;`。
   2. 如果一个缓冲区不够，则还需往临时缓冲区`extrabuf`写入数据。原缓冲区直接写满，`writeIndex_ = buffer_.size()`。然后往临时缓冲区写入数据，`append(extrabuf, n - writable);`。

## 空间不够
将数据往前移动：因为每次读取数据，`readIndex`索引都会往后移动，从而导致前面预留的空间逐渐增大。我们需要将后面的元素重新移动到前面。

如果第一种方案的空间仍然不够，那么我们就直接对 buffer_ 进行扩容（`buffer_.resize(len)`）操作。

## 从Buffer中读取数据：retrieveAllAsString()

读取数据会调用`void retrieve(size_t len)`函数，在这之前会判断读取长度是否大于可读取空间

1. 如果小于，则直接后移`readIndex`即可，`readerIndex_ += len;`。
2. 如果大于等于，说明全部数据都读取出来。此时会将buffer置为初始状态：
   1. `readerIndex_ = kCheapPrepend;`
   2. `writerIndex_ = kCheapPrepend;`


# TcpConnection使用Buffer

当服务端接收客户端数据，EventLoop 返回活跃的 Channel，并调用对应的读事件处理函数，即 TcpConnection 从相应的 fd 中读取数据到 inputBuffer 中。在 Buffer 内部 inputBuffer 中的 writeIndex 向后移动。

当服务端向客户端发送数据，TcpConnection 将 outputBuffer 的数据写入到 TCP 发送缓冲区。outputBuffer 内部调用 `retrieve` 方法移动 readIndex 索引。

## TcpConnection接收客户端数据（从客户端sock读取数据到inputBuffer）

调用`inputBuffer_.readFd(channel_->fd(), &savedErrno);`将对端`fd`数据读取到`inputBuffer`中。
   1. 如果读取成功，调用「可读事件发生回调函数」
   2. 如果读取数据长度为`0`，说明对端关闭连接。调用`handleCose()`
   3. 出错，则保存`errno`，调用`handleError()`

## TcpConnection向客户端发送数据（将ouputBuffer数据输出到socket中）

要在`channel_`确实关注写事件的前提下正常发送数据：因为一般有一个`send`函数发送数据，如果TCP接收缓冲区不够接收ouputBuffer的数据，就需要多次写入。需要重新注册写事件


向`channel->fd()`发送outputBuffer中的可读取数据。成功发送数据则移动`readIndex`，并且如果一次性成功写完数据，就不再让此`channel`关注写事件了，并调用写事件完成回调函数没写完则继续关注

这里的`ssize_t Buffer::writeFd(int fd, int *saveErrno)` 注意不能移位，一定要手动移位

# 潜在问题
扩容后读指针，写指针会失效，必须使用buffer类提供的接口，或使用指针就获取一次。