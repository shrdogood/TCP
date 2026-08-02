大家好，今天给大家分享一个在 Windows 环境下用 C++ 实现的多线程 TCP 服务器 Demo。在之前的单线程版本中，服务端一次只能处理一个客户端，处理完才能接受下一个连接，这在实际应用中显然不够用。本次我们在原有基础上引入了 **多线程** 技术，让服务器能够同时为多个客户端提供服务，真正实现并发通信。代码同样从零上手即可跑通，非常适合入门网络编程和多线程编程的小伙伴参考。

## 一、为什么需要多线程？
TCP 服务器通常需要同时处理多个客户端的请求。如果使用单线程模型，主线程在 accept 一个连接后，必须完成该客户端的所有收发操作才能返回去接受下一个连接，这会导致其他客户端长时间等待。通过多线程，我们可以为**每一个新接入的客户端**创建一个独立的工作线程，主线程只负责快速接受连接并分发任务，从而极大提升服务器的并发处理能力。
本 Demo 还引入了 **原子操作**`（std::atomic）`来安全地统计当前在线客户端数量，并设置了最大并发连接数（5 个），超过则拒绝新连接。

## 二、开发环境说明
- **操作系统**：Windows

- **开发工具**：VSCode

- **编程语言**：C++（使用 C++11 标准）

- **构建工具**：CMake

- **编译器**：MinGW 64 位 g++ 8.1.0 或更高版本（需支持 C++11 及 `<thread>` 库）
## 三、新增关键函数与概念说明
本次多线程实现用到了一些 C++ 标准库的新特性，简要介绍如下：
| 概念 / 函数 | 说明 |
|--|--|
| std::thread | C++11 提供的线程类，用于创建和管理线程。 |
|std::thread::detach() | 将线程与主线程分离，使其在后台独立运行，主线程不再等待它结束。 |
|std::thread::detach() | 将线程与主线程分离，使其在后台独立运行，主线程不再等待它结束。 |
|std::this_thread::get_id() | 获取当前线程的唯一 ID，用于日志输出，方便调试。 |
|std::atomic<int> | 原子整型，保证多线程环境下对变量的增减操作是线程安全的，无需额外加锁。 |
|std::atomic::operator++ / operator--| 原子地自增或自减。 |
|SOMAXCONN| Windows 中 listen 的 backlog 参数，表示监听队列的最大长度，一般取系统允许的最大值。 |

此外，Winsock 的 API 依然沿用之前的版本，如 `socket`、`bind`、`listen`、`accept`、`send`、`recv`、`closesocket` 等，用法不变。
## 四、代码分步解析
### 4.1多线程服务端 `server.cpp`
首先引入必要的头文件。除了 Winsock 和标准输入输出外，我们新增了 `<thread>`、`<atomic>` 用于多线程支持：

```cpp
#include <thread>
#include <iostream>
#include <cstring>  
#include <atomic>
#include <winsock2.h>
```
接着定义最大并发客户端数量（这里设为 5），并声明一个原子计数器 `clientCount`，用于记录当前已连接的客户端数：

```cpp
#define MAX_CLIENTS 5
std::atomic<int> clientCount(0);  // 当前客户端数量（线程安全）
```
**客户端处理函数** `handleClient`
这个函数将作为线程的入口函数，每个客户端连接都会创建一个独立线程来执行它。参数是客户端套接字和客户端地址结构体。

```cpp
void handleClient(SOCKET clientSock, SOCKADDR_IN clientaddr)
{
    char buf[1024];
    // 打印客户端信息，并显示当前线程 ID
    std::cout << "[Thread " << std::this_thread::get_id() << "] "
              << "客户端端口:" << ntohs(clientaddr.sin_port) << " 已连接" << std::endl;

    while(true)
    {
        // 接收数据（阻塞）
        int recvRet = recv(clientSock, buf, sizeof(buf), 0);
        if (recvRet <= 0) 
        {
            if(recvRet == 0)
                std::cout << "[Thread " << std::this_thread::get_id() << "] "
                          << "客户端主动断开连接" << std::endl;
            else
                std::cout << "[Thread " << std::this_thread::get_id() << "] "
                          << "接收失败" << std::endl;
            break;
        }

        std::cout << "[Thread " << std::this_thread::get_id() << "] "
                  << "接收来自客户端的信息: " << buf << std::endl;
        
        // 回显数据给客户端
        if (send(clientSock, buf, strlen(buf), 0) <= 0) 
        {
            std::cout << "[Thread " << std::this_thread::get_id() << "] "
                      << "发送失败" << std::endl;
            break;
        }
    }
        
    // 关闭客户端套接字并减少计数
    closesocket(clientSock);
    clientCount--;   // 原子递减
    std::cout << "[Thread " << std::this_thread::get_id() << "] "
              << "客户端断开，当前在线数: " << clientCount << std::endl;
}
```
**重点说明**：

- 每个线程都会打印自己的 `thread::id`，方便我们观察哪个线程在处理哪个客户端。
- 使用 `std::atomic` 确保 `clientCount` 的增减是原子操作，不会出现数据竞争。


**主函数** `main`

```cpp
WSADATA wsa;
WSAStartup(MAKEWORD(2, 0), &wsa);

SOCKET serversoc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
if(serversoc == INVALID_SOCKET) { /* 错误处理 */ }

SOCKADDR_IN serveraddr;
serveraddr.sin_family = AF_INET;
serveraddr.sin_port = htons(9102);
serveraddr.sin_addr.S_un.S_addr = inet_addr("192.168.31.86");

if(bind(serversoc, (SOCKADDR *)&serveraddr, sizeof(SOCKADDR_IN)) != 0) { /* 错误处理 */ }

if(listen(serversoc, SOMAXCONN) != 0) { /* 错误处理 */ }
std::cout << "服务器启动，最大并发连接数: " << MAX_CLIENTS << std::endl;
```
接下来进入主循环，不断调用 accept 接受新连接：

```cpp
while (true) 
{
    SOCKADDR_IN clientaddr;
    int len = sizeof(clientaddr);
    SOCKET clientsoc = accept(serversoc, (SOCKADDR*)&clientaddr, &len);
    if (clientsoc == INVALID_SOCKET) {
        std::cout << "accept 失败，退出主循环" << std::endl;
        break;
    }

    // 判断是否达到最大连接数
    if (clientCount.load() < MAX_CLIENTS) {
        clientCount++;   // 原子增加
        // 创建工作线程，detach 使其独立运行
        std::thread(handleClient, clientsoc, clientaddr).detach();
    } else {
        // 拒绝连接并发送提示
        const char* msg = "服务器当前繁忙，请稍后再试。\n";
        send(clientsoc, msg, strlen(msg), 0);
        closesocket(clientsoc);
        std::cout << "拒绝连接（已达到最大并发数）" << std::endl;
    }
}
```

**重要细节**：
- `clientCount.load()` 原子地读取当前值，判断是否还可以接受新连接。
- 若未达上限，则 `clientCount++`（原子自增），然后创建 `std::thread` 对象并立即调用 `detach()`，让线程在后台运行。

- 若已达上限，则向客户端发送“繁忙”消息后关闭套接字，不创建线程。

 最后，主循环退出时（通常不会发生，除非 **accept 失败**）释放服务器套接字并清理 `Winsock`：

```cpp
closesocket(serversoc);
WSACleanup();
return 0;
```

### 4.2 客户端 `client.cpp`（支持循环交互）
客户端代码也做了增强，现在支持用户多次输入并接收服务器回显，输入 `quit` 可主动断开连接。大部分内容与之前相同，这里只贴出循环部分：

```cpp
while(true)
{
    // 输入消息发送给服务端
    std::cout << "请输入发送给服务器的字符（输入 quit 退出）：" << std::endl;
    std::cin >> buf;

    // 检查是否要主动断开
    if (strcmp(buf, "quit") == 0) 
    {
        std::cout << "主动断开连接" << std::endl;
        break;
    }

    // 发送消息
    if (send(clientsocket, buf, strlen(buf), 0) <= 0) {
        std::cout << "发送失败！" << std::endl;
        break;
    }

    // 接收服务器回复（阻塞）
    int recvRet = recv(clientsocket, buf, sizeof(buf) - 1, 0);
    if (recvRet <= 0) {
        std::cout << "接收失败或服务器断开连接！" << std::endl;
        break;
    }
    std::cout << "服务器回复：" << buf << std::endl;
}
```

### 4.3 CMake 构建配置 `CMakeLists.txt`
CMake 配置需要指定 C++ 标准为 C++11（因为使用了 `<thread>` 和 `<atomic>`），同时链接 `ws2_32` 库。如果你使用的是 MinGW，`std::thread` 可能需要链接 `pthread` 库（某些 MinGW 版本默认依赖），因此我们也在服务端链接了 `pthread`：

```c
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_COMPILER "D:/mingw64/x86_64-16.1.0-release-win32-seh-ucrt-rt_v14-rev1/mingw64/bin/g++.exe")
project(TcpDemo)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server server.cpp)
if(WIN32)
    target_link_libraries(server ws2_32 pthread)   # pthread 可能被 std::thread 依赖
endif()

add_executable(client client.cpp)
if(WIN32)
    target_link_libraries(client ws2_32)
endif()
```

## 五、直接使用 g++ 命令行编译的方法
如果你不想使用 CMake，也可以直接在命令行下分别编译服务端和客户端。记得启用 C++11 标准并链接必要的库：
**编译服务端（多线程）**：

```bash
g++ -std=c++11 server.cpp -o server.exe -lws2_32 -lpthread
```

**编译客户端**

```bash
g++ -std=c++11 client.cpp -o client.exe -lws2_32
```

## 六、完整代码如下：

**server.cpp**
```cpp
#include <thread>
#include <iostream>
#include <cstring>  
#include <atomic>
#include <winsock2.h>

#define MAX_CLIENTS 5
std::atomic<int> clientCount(0);  //当前客户端数量（线程安全）

void handleClient(SOCKET clientSock, SOCKADDR_IN clientaddr)
{
    char buf[1024];
    //打印客户端信息（+打印服务器端线程ID）
    std::cout << "[Thread " << std::this_thread::get_id() << "] "
              << "客户端端口:" << ntohs(clientaddr.sin_port) << " 已连接" << std::endl;

    while(true)
    {
        //接收数据、阻塞等待
        int recvRet = recv(clientSock, buf, sizeof(buf), 0);
        if (recvRet <= 0) 
        {
            if(recvRet == 0)
            {
                std::cout << "[Thread " << std::this_thread::get_id() << "] "
                    << "客户端主动断开连接" << std::endl;
            }
            else
            {
                std::cout << "[Thread " << std::this_thread::get_id() << "] "
                    << "接收失败" << std::endl;
            }
            break;
        }

        std::cout << "[Thread " << std::this_thread::get_id() << "] "
                << "接收来自客户端的信息: " << buf << std::endl;
        
        //发送数据(回显)
        if (send(clientSock, buf, strlen(buf), 0) <= 0) 
        {
            std::cout << "[Thread " << std::this_thread::get_id() << "] "
                    << "发送失败" << std::endl;
            break;
        }
    }
        

    // 关闭客户端套接字并减少计数
    closesocket(clientSock);
    clientCount--;
    std::cout << "[Thread " << std::this_thread::get_id() << "] "
              << "客户端断开，当前在线数: " << clientCount << std::endl;
}


int main()
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 0), &wsa);

    //创建套接字
    SOCKET serversoc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(serversoc == INVALID_SOCKET)
    {
        std::cout << "套接字socket创建失败" << std::endl;
        WSACleanup();
        return -1;
    }

    SOCKADDR_IN serveraddr;
    //定义服务器自身的 IP + 端口信息
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(9102);
    serveraddr.sin_addr.S_un.S_addr = inet_addr("192.168.31.86");

    //绑定套接字
    if(bind(serversoc, (SOCKADDR *)&serveraddr, sizeof(SOCKADDR_IN)) != 0)
    {
        std::cout << "套接字socket绑定失败" << std::endl;
        closesocket(serversoc); // 绑定失败，释放服务端socket
        WSACleanup();
        return -1;
    }

    std::cout << "开始监听..." << std::endl;
    if(listen(serversoc, SOMAXCONN) != 0)
    {
        std::cout << "监听失败" << std::endl;
        closesocket(serversoc);
        WSACleanup();
        return -1;
    }
    std::cout << "服务器启动，最大并发连接数: " << MAX_CLIENTS << std::endl;


    // 主循环：接受连接
    while (true) 
    {
        SOCKADDR_IN clientaddr;
        int len = sizeof(clientaddr);
        SOCKET clientsoc = accept(serversoc, (SOCKADDR*)&clientaddr, &len);
        if (clientsoc == INVALID_SOCKET) {
            std::cout << "accept 失败，退出主循环" << std::endl;
            break;
        }

        // 判断是否达到最大连接数
        if (clientCount < MAX_CLIENTS) {
            clientCount++;   // 增加计数
            // 创建工作线程，detach 使其独立运行
            std::thread(handleClient, clientsoc, clientaddr).detach();
        } else {
            // 拒绝连接
            const char* msg = "服务器当前繁忙，请稍后再试。\n";
            send(clientsoc, msg, strlen(msg), 0);
            closesocket(clientsoc);
            std::cout << "拒绝连接（已达到最大并发数）" << std::endl;
        }
    }

    // 主循环退出时释放服务器套接字(实际不会执行到这里)
    closesocket(serversoc);
    WSACleanup();
    return 0;
}
```

client.cpp

```cpp
#include <winsock2.h>
#include <iostream>
#include <cstring>  // strlen

int main()
{
    SOCKET clientsocket;
    SOCKADDR_IN serveraddr;
    char buf[1024];

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 0), &wsa);

    // 创建客户端套接字
    if ((clientsocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) <= 0)
    {
        std::cout << "套接字socket创建失败" << std::endl;
        WSACleanup();
        return -1;
    }

    // 配置要连接的服务端地址
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(9102);
    serveraddr.sin_addr.S_un.S_addr = inet_addr("192.168.31.86");

    std::cout << "请求连接服务端..." << std::endl;
    if (connect(clientsocket, (SOCKADDR*)&serveraddr, sizeof(SOCKADDR_IN)) != 0)
    {
        std::cout << "连接失败！" << std::endl;
        closesocket(clientsocket);  // 连接失败，释放套接字
        WSACleanup();
        return -1;
    }
    std::cout << "连接服务端成功！" << std::endl;

    while(true)
    {
        // 输入消息发送给服务端
        std::cout << "请输入发送给服务器的字符（输入 quit 退出）：" << std::endl;
        std::cin >> buf;

        // 检查是否要主动断开
        if (strcmp(buf, "quit") == 0) 
        {
            std::cout << "主动断开连接" << std::endl;
            break;  // 退出循环，随后关闭套接字
        }

        // 发送消息
        if (send(clientsocket, buf, strlen(buf), 0) <= 0) {
            std::cout << "发送失败！" << std::endl;
            break;
        }

        // 接收服务器回复（阻塞）
        int recvRet = recv(clientsocket, buf, sizeof(buf) - 1, 0);
        if (recvRet <= 0) {
            std::cout << "接收失败或服务器断开连接！" << std::endl;
            break;
        }
        std::cout << "服务器回复：" << buf << std::endl;
    }
    
    // 程序正常退出，释放资源
    closesocket(clientsocket);
    WSACleanup();
    std::cout << "客户端正常退出" << std::endl;
    return 0;
}
```

## 七、运行效果

 1. 启动服务器 `server.exe`
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/c102a4abb4bd43f283718dd8eea784ff.png)
2. 启动客户端1 `client.exe`
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/a3ea0cc601c74fe3bb9c213b5f8cf6a3.png)
3. 启动客户端1后，服务器端响应
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/83dd804b9afa4a45bd3c4d88c7b45dce.png)
4. 启动客户端2 `client.exe` 及 服务器端响应
 ![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/3b7ef25cb56949359f935490f5ad5366.png)
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/40f91314dda04f399d9ec9cd21a93e56.png)
5. 客服端1发送字符串：abcd
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/8a81f79d3c4447c698a77b3beb20d2b2.png)
![服务器端](https://i-blog.csdnimg.cn/direct/ba16607ee9194090a351e20d6bc4d830.png)
6. 客户端2发送字符串：12345
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/a48902ddd5df406c968fc16b82bc50df.png)
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/0cdae74195f54e0ea8aaadcacf001375.png)
7. 客户端相继主动断开
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/07856b06d0684a96bcbeabcab9e7f8eb.png)![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/31fce74eaac844e89abbb5b07cf4191f.png)
**希望这篇博客能帮助你快速上手 Windows 下的多线程网络编程。**

