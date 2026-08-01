大家好，今天给大家分享一个在Windows环境下用C++实现的极简TCP双向通信Demo，从零上手就能跑通，非常适合入门网络编程的小伙伴参考。
## 一、TCP协议核心特点
TCP是传输控制协议，属于面向连接的可靠传输层协议，核心特性可以简单总结为几点：
- 面向连接：通信双方在传输数据前必须先完成三次握手建立连接，数据传输结束后四次挥手释放连接
- 可靠性保障：通过超时重传、确认应答、拥塞控制等机制，保证数据能无差错、不丢包、按序到达对端
- 面向字节流：数据以字节流的形式传输，没有固定的报文边界
- 全双工通信：建立连接后，通信双方可以同时双向收发数据

## 二、开发环境说明
本次Demo的开发运行环境配置如下：
- 操作系统：Windows 平台
- 开发工具：VSCode
- 编程语言：C++
- 构建工具：CMake
- 编译器：MinGW 64位的g++ 8.1.0版本

## 三、代码用到的关键函数说明
本次实现用到了Windows Socket（Winsock2）的核心API，用法做简要说明：

 1. `WSAStartup`：初始化Windows Socket环境，加载指定版本的Winsock库
 2. `socket` ：创建套接字文件描述符，指定通信协议族、套接字类型
 3. `sockaddr_in`：用于存储IPv4地址、端口等套接字地址信息的结构体
 4. `htons`：将主机字节序的端口转换为网络字节序（大端模式）
 5. `inet_addr`：将点分十进制格式的IP字符串转换为网络字节序的整数形式
 6. `bind`：将创建好的套接字和本地的IP、端口进行绑定
 7. `listen`：将套接字设置为监听状态，等待客户端发起连接请求
 8. `accept`：从监听队列中取出客户端连接请求，返回一个专门和该客户端通信的新套接字
 9. `connect`：客户端向指定地址的服务端发起连接请求
 10. `send`：通过已建立连接的套接字向对端发送数据
 11. `recv`：通过已建立连接的套接字阻塞接收对端发来的数据
 12. `closesocket`：关闭不再使用的套接字，释放资源
 13. `WSACleanup`：卸载Winsock库，清理占用的系统资源

## 四、代码分步解析
### 4.1 服务端代码 server.cpp
首先是服务端的实现逻辑，第一步先引入依赖头文件，定义需要用到的套接字、地址结构体和数据缓冲区，完成Winsock环境的初始化：

```cpp
#include <winsock2.h>
#include <iostream>
#include <cstring>  

int main()
{
    SOCKET serversoc;
    SOCKET clientsoc;
    SOCKADDR_IN serveraddr;
    SOCKADDR_IN clientaddr;
    char buf;
    int len;

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 0), &wsa);
```

接下来创建TCP套接字，判断创建结果，如果失败就直接打印错误并清理资源退出：

```cpp
    //创建套接字 AF_INET表示IPV4协议 
    if((serversoc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) <= 0)
    {
        std::cout << "套接字socket创建失败" << std::endl;
        WSACleanup();
        return -1;
    }
```

随后配置服务端自身要绑定的IP地址和端口信息，这里指定的IP是192.168.31.86（使用ipconfig查看自己主机的ip），端口是9102(最好大于1024)：

```cpp
    //定义服务器自身的 IP + 端口信息
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(9102);
    serveraddr.sin_addr.S_un.S_addr = inet_addr("192.168.31.86");
```

执行绑定操作，把套接字和配置好的地址信息关联，绑定失败直接打印错误释放资源：

```cpp
    //绑定套接字
    if(bind(serversoc, (SOCKADDR *)&serveraddr, sizeof(SOCKADDR_IN)) != 0)
    {
        std::cout << "套接字socket绑定失败" << std::endl;
        closesocket(serversoc); // 绑定失败，释放服务端socket
        WSACleanup();
        return -1;
    }
```

把套接字切换为监听状态，准备接收客户端的连接请求：

```cpp
    std::cout << "开始监听..." << std::endl;
    if(listen(serversoc, 1) != 0)
    {
        std::cout << "监听失败!" << std::endl;
        closesocket(serversoc); // 监听失败释放
        WSACleanup();
        return -1;
    }
```

调用accept函数阻塞等待客户端接入，连接成功后打印客户端的端口信息：

```cpp
    len = sizeof(SOCKADDR_IN);
    if((clientsoc = accept(serversoc, (SOCKADDR *)&clientaddr, &len)) <= 0)
    {
        std::cout << "接受连接失败" << std::endl;
        closesocket(serversoc); // 释放服务端socket
        WSACleanup();
        return -1;
    }
    std::cout << "接受连接成功" << std::endl;
    std::cout << "客户端端口：" << ntohs(clientaddr.sin_port) << std::endl;
```

阻塞接收客户端发来的数据，收到数据后打印到控制台：

```cpp
    //接收数据
    while(1)
    {
        int recvRet = recv(clientsoc, buf, 1024, 0);
        if(recvRet <= 0)
        {
            std::cout << "关闭连接" << std::endl;
            // 客户端断开，释放两个套接字
            closesocket(clientsoc);
            closesocket(serversoc);
            WSACleanup();
            return -1;
        }
        std::cout << "接收来自客户端的信息:" << buf << std::endl;
        break;
    }
```
最后等待用户输入要回复给客户端的内容，发送完成后统一释放所有套接字和Winsock资源，程序正常退出：

```cpp
    //发送数据
    std::cout << "输入发送给客户端的信息：" << std::endl;
    std::cin >> buf ;
    if(send(clientsoc, buf, strlen(buf), 0) <= 0)
    {
        std::cout << "发送失败" << std::endl;
    }

    // ========== 程序正常结束，统一释放套接字 ==========
    closesocket(clientsoc);
    closesocket(serversoc);
    WSACleanup();
    return 0;
}
```

### 4.2 客户端代码 client.cpp
客户端部分首先引入依赖头文件，定义需要的套接字、地址结构体和缓冲区，初始化Winsock环境：

```cpp
#include <winsock2.h>
#include <iostream>
#include <cstring>  // strlen

int main()
{
    SOCKET clientsocket;
    SOCKADDR_IN serveraddr;
    char buf;

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 0), &wsa);
```

创建客户端的TCP套接字，校验创建结果，失败就清理资源退出：

```cpp
    // 创建客户端套接字
    if ((clientsocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) <= 0)
    {
        std::cout << "套接字socket创建失败" << std::endl;
        WSACleanup();
        return -1;
    }
```

配置要连接的服务端地址信息，和服务端的IP、端口保持一致：

```cpp
    // 配置要连接的服务端地址
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(9102);
    serveraddr.sin_addr.S_un.S_addr = inet_addr("192.168.31.86");
```

调用connect函数向服务端发起连接请求，连接失败打印错误并释放资源：

```cpp
    std::cout << "请求连接服务端..." << std::endl;
    if (connect(clientsocket, (SOCKADDR*)&serveraddr, sizeof(SOCKADDR_IN)) != 0)
    {
        std::cout << "连接失败！" << std::endl;
        closesocket(clientsocket);  // 连接失败，释放套接字
        WSACleanup();
        return -1;
    }
    std::cout << "连接服务端成功！" << std::endl;
```

等待用户输入要发送给服务端的内容，调用send函数完成数据发送：

```cpp
    // 输入消息发送给服务端
    std::cout << "请输入发送给服务器的字符：" << std::endl;
    std::cin >> buf;
    int sendLen = strlen(buf);
    if (send(clientsocket, buf, sendLen, 0) <= 0)
    {
        std::cout << "发送错误！" << std::endl;
        closesocket(clientsocket);
        WSACleanup();
        return -1;
    }
    std::cout << "消息发送完成！" << std::endl;
```

阻塞等待接收服务端返回的应答数据，收到后打印到控制台：

```cpp
    // 阻塞接收服务器回复
    int recvRet;
    while (1)
    {
        recvRet = recv(clientsocket, buf, 1024, 0);
        if (recvRet <= 0)
        {
            std::cout << "接收错误或服务端断开连接！" << std::endl;
            closesocket(clientsocket);
            WSACleanup();
            return -1;
        }
        std::cout << "接收来自服务器的信息：" << buf << std::endl;
        break;
    }
```

所有交互完成后，释放套接字和Winsock资源，客户端正常退出：

```cpp
    // 程序正常退出，释放资源
    closesocket(clientsocket);
    WSACleanup();
    std::cout << "客户端正常退出" << std::endl;
    return 0;
}
```
### 4.3 构建配置文件 CMakeLists.txt
这个是CMake的构建配置脚本，首先指定CMake最低版本要求，配置好我们本地的g++编译器路径，声明项目名称：

```cpp
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_COMPILER "D:/mingw64/x86_64-8.1.0-release-win32-seh-rt_v6-rev0/mingw64/bin/g++.exe")
project(TcpDemo)
```
接下来分别配置服务端和客户端的编译规则，同时在Windows平台下链接Winsock对应的ws2_32库：

```cpp
add_executable(server server.cpp)
if(WIN32)
    target_link_libraries(server ws2_32)
endif()

add_executable(client client.cpp)
if(WIN32)
    target_link_libraries(client ws2_32)
endif()
```
## 五、直接使用g++命令行编译的方法
如果你不想用CMake工具，也可以直接在MinGW的命令行环境下手动执行编译命令，只需要注意主动链接ws2_32系统库即可，示例命令如下：
编译服务端程序：

```bash
g++ server.cpp -o server.exe -lws2_32
```
编译客户端程序：

```cpp
g++ client.cpp -o client.exe -lws2_32
```
执行完成后，就会在当前目录下生成可运行的server.exe和client.exe两个可执行文件，先运行服务端，再运行客户端就能完成通信测试。

