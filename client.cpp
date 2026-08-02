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