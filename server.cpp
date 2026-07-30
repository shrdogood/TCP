#include <winsock2.h>
#include <iostream>
#include <cstring>  

int main()
{
    SOCKET serversoc;
    SOCKET clientsoc;
    SOCKADDR_IN serveraddr;
    SOCKADDR_IN clientaddr;
    char buf[1024];
    int len;

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 0), &wsa);

    //创建套接字
    if((serversoc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) <= 0)
    {
        std::cout << "套接字socket创建失败" << std::endl;
        WSACleanup();
        return -1;
    }

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
    if(listen(serversoc, 1) != 0)
    {
        printf("监听失败!\n");
        closesocket(serversoc); // 监听失败释放
        WSACleanup();
        return -1;
    }

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

    //发送数据
    std::cout << "输入发送给客户端的信息：" << std::endl;
    std::cin >> buf ;
    // 去掉 +1，不需要发送\0
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