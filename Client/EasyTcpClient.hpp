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
	//��ʼ��socket
	void initSocket() {
#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		if (_sock != INVALID_SOCKET) {
			printf("<socket=%d>�رվ�����...\n", _sock);
			close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (INVALID_SOCKET == _sock) {
			printf("create error\n");
		}
		else {
			//printf("create success\n");
		}
	}
	//���ӷ�����
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
			//printf("connect success\n");
		}
		return ret;
	}

	//�ر�socket
	void close() {
		//�ر�win sock 2.x����
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

//��������
//����������Ϣ
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
			if (-1 == RecvData(_sock)) {
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

//�������� ����ճ�� ��ְ�

#define RECV_BUFF_SIZE 10240
//���ܻ�����
char _szRecv[RECV_BUFF_SIZE] = {};

char _szMsgBuf[RECV_BUFF_SIZE*10] = {};

int _lastPos = 0;

int RecvData(SOCKET cSock) {
	int nLen = recv(cSock, _szRecv, RECV_BUFF_SIZE, 0);
	if (nLen <= 0) {
		printf("disconnect server exit\n");
		return -1;
	}
	//����ȡ�������ݿ�������Ϣ������
	memcpy(_szMsgBuf+_lastPos, _szRecv, nLen);
	//��Ϣ������������β��λ�ú���
	_lastPos += nLen;

	while (_lastPos >= sizeof(DataHead)) {
		DataHead* header = (DataHead*)_szMsgBuf;
		//�ж���Ϣ�����������ݳ��ȴ�����Ϣ����
		if (_lastPos >= header->dataLength) {
			//ʣ��δ������Ϣ���������ݵĳ���
			int nSize = _lastPos - header->dataLength;
			//OnNetMsg(header);
			//����Ϣ������ʣ��δ��������ǰ��
			memcpy(_szMsgBuf, _szMsgBuf + header->dataLength, _lastPos - header->dataLength);
			_lastPos = nSize;
		}
		else {
			//ʣ�໺����ʣ�಻��һ��������Ϣ
			break;
		}
	}
	return 0;
}

//��Ӧ������Ϣ
void OnNetMsg(DataHead* header) {
	switch (header->cmd) {
	case CMD_LOGIN_RESULT:
	{
		LoginResult* loginresult = (LoginResult*)header;
		//printf("login result %d %d\n", loginresult->result, loginresult->dataLength);
	}
	break;
	case CMD_LOGOUT_RESULT:
	{
		LogoutResult* logoutresult = (LogoutResult*)header;
		//printf("logout result %d\n", logoutresult->result);
	}
	break;
	case CMD_NEW_USER_JOIN:
	{
		NewUserJoin* userjoin = (NewUserJoin*)header;
		//printf("user join %d\n", userjoin->sock);
	}
	break;
	case CMD_ERROR: {
		NewUserJoin* userjoin = (NewUserJoin*)header;
		printf("error");
	}
	break;
	default:
		printf("no defines");
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