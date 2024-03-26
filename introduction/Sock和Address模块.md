
# InetAddress类

类 `InetAddress` 的作用是用于表示网络地址（IP 地址和端口号）的数据结构，并提供一些操作这些数据的方法

构造函数 `explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1")：`用于初始化 `InetAddress` 对象，可以指定端口号和 IP 地址。

另一个构造函数 `explicit InetAddress(const sockaddr_in &addr)：`用给定的 `sockaddr_in` 结构初始化 `InetAddress` 对象。

`std::string toIp() const：`返回该网络地址的 IP 地址部分。

`std::string toIpPort() const：`返回包含 IP 地址和端口号的字符串表示。

`uint16_t toPort() const：`返回该网络地址的端口号。

`const sockaddr_in *getSockAddr() const：`返回指向 `sockaddr_in` 结构体的指针，该结构体包含了网络地址的详细信息。

`void setSockAddr(const sockaddr_in &addr)：`设置网络地址的详细信息。

# Socket类

用于封装一个套接字文件描述符 `sockfd_`，并提供了一系列成员函数来操作这个套接字。提供了以下接口

`int accept(InetAddress *peeraddr)：`接受客户端的连接请求，并返回新建立的连接的文件描述符，同时将客户端的地址信息存储到 peeraddr 中。

`void shutdownWrite()：`设置套接字 `sockfd_`为半关闭状态，即关闭写端，不再发送数据。

`void setTcpNoDelay(bool on)：`设置 Nagel 算法，控制是否启用 Nagle 算法。Nagle 算法可以减少网络传输中的小数据包数量，提高网络传输效率。

`void setReuseAddr(bool on)：`设置地址复用选项，允许多个套接字绑定到相同的地址和端口上。

`void setReusePort(bool on)：`设置端口复用选项，允许多个套接字监听相同的端口。

`void setKeepAlive(bool on)：`设置长连接选项，启用或禁用 TCP 的 keep-alive 机制，用于检测连接是否仍然有效。

