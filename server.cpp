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