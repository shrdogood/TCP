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

    // 程序正常退出，释放资源
    closesocket(clientsocket);
    WSACleanup();
    std::cout << "客户端正常退出" << std::endl;
    return 0;
}