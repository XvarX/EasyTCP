#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_

#ifdef  _WIN32
#define WIN32_LEAN_AND_MEAN
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

#include <thread>
#include "stdio.h"
#include "MessageHeader.hpp"

class EasyTcpClient {
	SOCKET _sock = INVALID_SOCKET;
public:
	EasyTcpClient() {

	}
	virtual ~EasyTcpClient() {
		close();
	}
	//初始化socket
	void initSocket() {
#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		if (_sock != INVALID_SOCKET) {
			printf("<socket=%d>关闭旧链接...\n", _sock);
			close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (INVALID_SOCKET == _sock) {
			printf("create error\n");
		}
		else {
			printf("create success\n");
		}
	}
	//连接服务器
	int Connect(char* ip, unsigned short port) {
		if (_sock == INVALID_SOCKET) {
			initSocket();
		}

		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
		_sin.sin_addr.s_addr = inet_addr(ip);
#endif

		int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));

		if (ret == SOCKET_ERROR) {
			printf("connect error\n");
		}
		else {
			printf("connect success\n");
		}
		return ret;
	}

	//关闭socket
	void close() {
		//关闭win sock 2.x环境
#ifdef _WIN32
		if (_sock != INVALID_SOCKET) {
			closesocket(_sock);
			WSACleanup();
#else
		close(_sock);
#endif
		_sock = INVALID_SOCKET;
		}
	}

//发送数据
//处理网络消息
bool OnSelect() {
	if (isRun()) {
		fd_set rdRead;

		FD_ZERO(&rdRead);

		FD_SET(_sock, &rdRead);

		timeval t = { 0, 0 };
		int ret = select(_sock + 1, &rdRead, NULL, NULL, &t);
		if (ret < 0) {
			printf("<socket=%d>select end\n", _sock);
			return false;
		}

		if (FD_ISSET(_sock, &rdRead)) {
			FD_CLR(_sock, &rdRead);
			if (-1 == RecvData()) {
				printf("<socket=%d>server exit", _sock);
				return false;
			}
		}
		return true;
	}
	return false;
}

bool isRun() {
	return _sock != INVALID_SOCKET;
}

//接收数据 处理粘包 拆分包
int RecvData() {
	char szRecv[1024] = {};
	int nLen = recv(_sock, szRecv, sizeof(DataHead), 0);
	DataHead* header = (DataHead*)szRecv;
	if (nLen <= 0) {
		printf("disconnect server exit\n");
		return -1;
	}
	recv(_sock, szRecv + sizeof(DataHead), header->dataLength - sizeof(DataHead), 0);
	OnNetMsg(header);
	return 0;
}

//响应网络消息
void OnNetMsg(DataHead* header) {
	switch (header->cmd) {
	case CMD_LOGIN_RESULT:
	{
		LoginResult* loginresult = (LoginResult*)header;
		printf("login result %d\n", loginresult->result);
	}
	break;
	case CMD_LOGOUT_RESULT:
	{
		LogoutResult* logoutresult = (LogoutResult*)header;
		printf("logout result %d\n", logoutresult->result);
	}
	break;
	case CMD_NEW_USER_JOIN:
	{
		NewUserJoin* userjoin = (NewUserJoin*)header;
		printf("user join %d\n", userjoin->sock);
	}
	default:
		DataHead header = {};
		header.cmd = CMD_ERROR;
		header.dataLength = 0;
	}
}

int SendData(DataHead* header) {
	if (isRun() && header) {
		send(_sock, (const char*)header, header->dataLength, 0);
	}
	else {
		return SOCKET_ERROR;
	}
}
private:

};
#endif