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
	//�ڶ������� ��Ϣ������
	char _szMsgBuf[RECV_BUFF_SIZE * 10];
	//��Ϣ������������β��λ��
	int _lastPos;
};

//new ���ڴ� ��new ջ�ڴ�
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
	//��ʼ��socket
	SOCKET InitSocket() {
#ifdef _WIN32
		//windows ����
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		//���������socket
		if (_sock != INVALID_SOCKET) {
			printf("<socket=%d>�رվ�����...\n", _sock);
			Close();
		}
		//�����׽��֣�������ĵ�һ��������ipv4/ipv6 �ڶ������������� ��������tcp/udp
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (_sock == INVALID_SOCKET) {
			printf("���󣬽���Socketʧ��...\n");
		}
		else {
			printf("����Socket=<%d>�ɹ�...", _sock);
		}
		return _sock;
	}
	//�󶨶˿ں�
	int Bind(const char* ip, unsigned short port) {
		//��socket��֮���Բ�ֱ��ʹ��sockaddr����Ϊsockaddr�Ľṹ��������
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
			printf("���󣬰�����˿�<%d>ʧ��...\n", port);
		}
		else {
			printf("������˿�<%d>�ɹ�...\n", port);
		}
		return _sock;
	}
	//�����˿ں�
	int Listen(int n) {
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret) {
			printf("Socket=<%d>���󣬼�������˿�ʧ��...\n", _sock);
		}
		else {
			printf("Socket=<%d>��������˿ڳɹ�\n", _sock);
		}
		return ret;
	}
	//���ܿͻ�������
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
			printf("socket=<%d>���󣬽��ܵ���Ч�ͻ���SOCKET...\n", _sock);
		}
		else {
			NewUserJoin userJoin;
			userJoin.sock = int(cSock);
			SendDataToAll(&userJoin);
			_clients.push_back(new ClientSocket(cSock));
			printf("socket=<%d>�¿ͻ��˼���<%d>��socket = %d, IP = %s \n", _sock, _clients.size(), (int)cSock, inet_ntoa(clientAddr.sin_addr));
		}
		return cSock;
	}

	//�ر�socket
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
	//����������Ϣ
	bool OnRun() {
		if (isRun()) {
			//�������׽��� BSD socket
			fd_set fdRead; //��������socket������
			fd_set fdWrite;
			fd_set fdExp;
			//������
			FD_ZERO(&fdRead);
			FD_ZERO(&fdWrite);
			FD_ZERO(&fdExp);

			//��������socket���뼯��
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
			//nfds ��һ������ֵ����ָfd_set����������������socket�ķ�Χ������������
			//���������ļ����������ֵ+1 ��Windows�������������д0
			timeval t = { 1, 0 };
			int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);
			if (ret < 0) {
				printf("select���������\n");
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
	//�Ƿ�����
	bool isRun() {
		return _sock != INVALID_SOCKET;
	}
	//��������
	char _szRecv[RECV_BUFF_SIZE] = {};
	int RecvData(ClientSocket* pClient) {

		int nLen = (int)recv(pClient->sockfd(), _szRecv, RECV_BUFF_SIZE, 0);
		if (nLen <= 0) {
			printf("�ͻ���<socket=%d>���˳������������\n", pClient->sockfd());
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
	//��Ӧ������Ϣ
	virtual void OnNetMsg(SOCKET cSock, DataHead* header) {
		switch (header->cmd) {
		case CMD_LOGIN: {
			Login* login = (Login*)header;
			//printf("�յ��ͻ���<socket=%d>����CMD_LOGIN�����ݳ���:%d��userName=%s PassWord=%s\n", cSock, login->dataLength, login->userName, login->userPassWord);
			LoginResult ret;
			SendData(cSock, &ret);
		}
					  break;
		case CMD_LOGOUT: {
			Logout* logout = (Logout*)header;
			//printf("�յ��ͻ���<socket=%d>����CMD_LOGOUT�����ݳ���:%d��userName=%s", cSock, logout->dataLength, logout->userName);
			LogoutResult ret;
			SendData(cSock, &ret);
		}
					   break;
		default:
		{
			printf("<socket=%d>�յ�δ������Ϣ�����ݳ��ȣ�%d\n", cSock, header->dataLength);
			//DataHead ret;
			//send(cSock, (char*)&header, sizeof(header), 0);
		}
		break;
		}
	}
	//����ָ��socket����
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