#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#ifdef  _WIN32
#define FD_SETSIZE 1024
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

#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#endif

class ClientSocket {
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET) {
		_sockfd = sockfd;
		memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
		_lastPos = 0;
	}
	SOCKET sockfd() {
		return _sockfd;
	}
	char* msgBuf() {
		return _szMsgBuf;
	}

	int getLastPos() {
		return _lastPos;
	}
	void setLastPos(int pos) {
		_lastPos = pos;
	}

private:
	//socket fd_set file desc set
	SOCKET _sockfd;
	//第二缓冲区 消息缓冲区
	char _szMsgBuf[RECV_BUFF_SIZE * 10];
	//消息缓冲区的数据尾部位置
	int _lastPos;
};

//new 堆内存 非new 栈内存
class EasyTcpServer {
private:
	SOCKET _sock;
	std::vector<ClientSocket*> _clients;

public:
	EasyTcpServer() {
		_sock = INVALID_SOCKET;
	}
	virtual ~EasyTcpServer() {
		Close();
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
		return _sock;
	}
	//监听端口号
	int Listen(int n) {
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret) {
			printf("Socket=<%d>错误，监听网络端口失败...\n", _sock);
		}
		else {
			printf("Socket=<%d>监听网络端口成功\n", _sock);
		}
		return ret;
	}
	//接受客户端连接
	SOCKET Accept() {
		sockaddr_in clientAddr = {};
		int nAddrLen = sizeof(sockaddr_in);
		SOCKET cSock = INVALID_SOCKET;
#ifdef _WIN32
		cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
		cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#endif
		if (INVALID_SOCKET == cSock) {
			printf("socket=<%d>错误，接受到无效客户端SOCKET...\n", _sock);
		}
		else {
			NewUserJoin userJoin;
			userJoin.sock = int(cSock);
			SendDataToAll(&userJoin);
			_clients.push_back(new ClientSocket(cSock));
			printf("socket=<%d>新客户端加入<%d>：socket = %d, IP = %s \n", _sock, _clients.size(), (int)cSock, inet_ntoa(clientAddr.sin_addr));
		}
		return cSock;
	}

	//关闭socket
	void Close() {
		if (_sock != INVALID_SOCKET) {
#ifdef _WIN32
			for (int n = (int)_clients.size() - 1; n >= 0; n--) {
				closesocket(_clients[n]->sockfd());
				delete _clients[n];
			}
			closesocket(_sock);
			WSACleanup();
#else
			for (int n = (int)_clients.size() - 1; n >= 0; n--) {
				close(_clients[n]->sockfd());
				delete _clients[n];
			}
			close(_sock);
#endif
			_clients.clear();
		}
	}
	//处理网络消息
	bool OnRun() {
		if (isRun()) {
			//伯克利套接字 BSD socket
			fd_set fdRead; //描述符（socket）集合
			fd_set fdWrite;
			fd_set fdExp;
			//清理集合
			FD_ZERO(&fdRead);
			FD_ZERO(&fdWrite);
			FD_ZERO(&fdExp);

			//将描述符socket加入集合
			FD_SET(_sock, &fdRead);
			FD_SET(_sock, &fdWrite);
			FD_SET(_sock, &fdExp);

			SOCKET maxSock = _sock;
			for (int n = (int)_clients.size() - 1; n >= 0; n--) {
				FD_SET(_clients[n]->sockfd(), &fdRead);
				if (maxSock < _clients[n]->sockfd()) {
					maxSock = _clients[n]->sockfd();
				}
			}
			//nfds 是一个整数值，是指fd_set集合中所有描述符socket的范围，而不是数量
			//即是所有文件描述符最大值+1 在Windows中这个参数可以写0
			timeval t = { 1, 0 };
			int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);
			if (ret < 0) {
				printf("select任务结束。\n");
				Close();
				return false;
			}
			if (FD_ISSET(_sock, &fdRead)) {
				FD_CLR(_sock, &fdRead);
				Accept();
			}
			for (int n = (int)_clients.size() - 1; n >= 0; n--) {
				if (FD_ISSET(_clients[n]->sockfd(), &fdRead)) {
					if (-1 == RecvData(_clients[n])) {
						auto iter = _clients.begin() + n;
						if (iter != _clients.end()) {
							delete _clients[n];
							_clients.erase(iter);
						}
					}
				}
			}

			return true;
		}
		return false;
	}
	//是否工作中
	bool isRun() {
		return _sock != INVALID_SOCKET;
	}
	//接收数据
	char _szRecv[RECV_BUFF_SIZE] = {};
	int RecvData(ClientSocket* pClient) {

		int nLen = (int)recv(pClient->sockfd(), _szRecv, RECV_BUFF_SIZE, 0);
		if (nLen <= 0) {
			printf("客户端<socket=%d>已退出，任务结束。\n", pClient->sockfd());
			return -1;
		}
		memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);

		pClient->setLastPos(pClient->getLastPos() + nLen);
		while (pClient->getLastPos() >= sizeof(DataHead)) {
			DataHead* header = (DataHead*)pClient->msgBuf();
			if (pClient->getLastPos() >= header->dataLength) {
				int nSize = pClient->getLastPos() - header->dataLength;
				OnNetMsg(pClient->sockfd(), header);
				memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);
				pClient->setLastPos(nSize);
			}
			else {
				break;
			}
		}
		return 0;
	}
	//响应网络消息
	virtual void OnNetMsg(SOCKET cSock, DataHead* header) {
		switch (header->cmd) {
		case CMD_LOGIN: {
			Login* login = (Login*)header;
			//printf("收到客户端<socket=%d>请求：CMD_LOGIN，数据长度:%d，userName=%s PassWord=%s\n", cSock, login->dataLength, login->userName, login->userPassWord);
			LoginResult ret;
			SendData(cSock, &ret);
		}
					  break;
		case CMD_LOGOUT: {
			Logout* logout = (Logout*)header;
			//printf("收到客户端<socket=%d>请求：CMD_LOGOUT，数据长度:%d，userName=%s", cSock, logout->dataLength, logout->userName);
			LogoutResult ret;
			SendData(cSock, &ret);
		}
					   break;
		default:
		{
			printf("<socket=%d>收到未定义消息，数据长度：%d\n", cSock, header->dataLength);
			//DataHead ret;
			//send(cSock, (char*)&header, sizeof(header), 0);
		}
		break;
		}
	}
	//发送指定socket数据
	int SendData(SOCKET cSock, DataHead* header) {
		if (isRun() && header) {
			return send(cSock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}

	void SendDataToAll(DataHead* header) {
		if (isRun() && header) {
			for (int n = (int)_clients.size() - 1; n >= 0; n--) {
				SendData(_clients[n]->sockfd(), header);
			}
		}
	}
};

#endif