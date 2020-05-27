#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#ifdef  _WIN32
#include <Windows.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include<unistd.h> //uni std
#include<arpa/inet.h>
#include<string.h>
#define SOCKET int
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#endif
#include <stdio.h>
#include <vector>
#include "MessageHeader.hpp"

class EasyTcpServer {
private:
	SOCKET _sock;
	std::vector<SOCKET> g_clients;

public:
	EasyTcpServer() {

	}
	virtual ~EasyTcpServer() {

	}
	//初始化socket
	SOCKET InitSocket() {
#ifdef _WIN32
		//windows 环境
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		//创建服务端socket
		if (_sock != INVALID_SOCKET) {
			printf("<socket=%d>关闭旧连接...\n", _sock);
			Close();
		}
		//生成套接字，这里面的第一个参数是ipv4/ipv6 第二个是数据类型 第三个是tcp/udp
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (_sock == INVALID_SOCKET) {
			printf("错误，建立Socket失败...\n");
		}
		else {
			printf("建立Socket=<%d>成功...", _sock);
		}
		return _sock;
	}
	//绑定端口号
	int Bind(const char* ip, unsigned short port) {
		//绑定socket，之所以不直接使用sockaddr是因为sockaddr的结构不好手填
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		if (ip) {
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
#else
		if (ip) {
			_sin.sin_addr.saddr = net_addr(ip);
		}
		else {
			_sin.sin_addr.s_addr = INADDR_ANY;
		}
#endif
		int ret = bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
		if (SOCKET_ERROR == ret) {
			printf("错误，绑定网络端口<%d>失败...\n", port);
		}
		else {
			printf("绑定网络端口<%d>成功...\n", port);
		}
	}
	//监听端口号
	int Listen(int n) {
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret) {
			printf("Socket=<%d>错误，监听网络端口失败...\n", _sock);
		}
		else {
			printf("Socket=<%d>监听网络端口成功");
		}
		return ret;
	}
	//接受客户端连接
	//关闭socket
	void Close() {
		if (_sock != INVALID_SOCKET) {
#ifdef _WIN32
			for (int n = (int)g_clients.size() - 1; n >= 0; n--) {
				closesocket(g_clients[n]);
			}
			closesocket(_sock);
			WSACleanup();
#else
			for (int n = (int)g_clients.size() - 1; n >= 0; n--) {
				close(g_clients[n]);
			}
			close(_sock);
#endif
		}
	}
	//处理网络消息
	//是否工作中
	//接收数据
	//响应网络消息
	//发送数据
};

#endif