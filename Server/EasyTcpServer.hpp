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
		SOCKET _cSock = INVALID_SOCKET;
#ifdef _WIN32
		_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
		_cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#endif
		if (INVALID_SOCKET == _cSock) {
			printf("socket=<%d>���󣬽��ܵ���Ч�ͻ���SOCKET...\n", _sock);
		}
		else {
			NewUserJoin userJoin;
			userJoin.sock = int(_cSock);
			SendDataToAll(&userJoin);
			g_clients.push_back(_cSock);
			printf("socket=<%d>�¿ͻ��˼��룺socket = %d, IP = %s \n", _sock, (int)_cSock, inet_ntoa(clientAddr.sin_addr));
		}
		return _cSock;
	}

	//�ر�socket
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
			for (int n = (int)g_clients.size() - 1; n >= 0; n--) {
				FD_SET(g_clients[n], &fdRead);
				if (maxSock < g_clients[n]) {
					maxSock = g_clients[n];
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
			for (int n = (int)g_clients.size() - 1; n >= 0; n--) {
				if (FD_ISSET(g_clients[n], &fdRead)) {
					if (-1 == RecvData(g_clients[n])) {
						auto iter = g_clients.begin() + n;
						if (iter != g_clients.end()) {
							g_clients.erase(iter);
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
	int RecvData(SOCKET _cSock) {
		//������
		char szRecv[4096] = {};
		int nLen = (int)recv(_cSock, szRecv, sizeof(DataHead), 0);
		DataHead* header = (DataHead*)szRecv;
		if (nLen <= 0) {
			printf("�ͻ���<socket=%d>���˳������������\n", _cSock);
			return -1;
		}
		recv(_cSock, szRecv + sizeof(DataHead), header->dataLength - sizeof(DataHead), 0);
		OnNetMsg(_cSock, header);
		return 0;
	}
	//��Ӧ������Ϣ
	virtual void OnNetMsg(SOCKET _cSock, DataHead* header) {
		switch (header->cmd) {
		case CMD_LOGIN: {
			Login* login = (Login*)header;
			printf("�յ��ͻ���<socket=%d>����CMD_LOGIN�����ݳ���:%d��userName=%s PassWord=%s\n", _cSock, login->dataLength, login->userName, login->userPassWord);
			LoginResult ret;
			send(_cSock, (char*)&ret, sizeof(LoginResult), 0);
		}
					  break;
		case CMD_LOGOUT: {
			Logout* logout = (Logout*)header;
			printf("�յ��ͻ���<socket=%d>����CMD_LOGOUT�����ݳ���:%d��userName=%s", _cSock, logout->dataLength, logout->userName);
			LogoutResult ret;
			send(_cSock, (char*)&ret, sizeof(LogoutResult), 0);
		}
					   break;
		default:
		{
			DataHead header = { 0, CMD_ERROR };
			send(_cSock, (char*)&header, sizeof(header), 0);
		}
		break;
		}
	}
	//����ָ��socket����
	int SendData(SOCKET _cSock, DataHead* header) {
		if (isRun() && header) {
			return send(_cSock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}

	void SendDataToAll(DataHead* header) {
		if (isRun() && header) {
			for (int n = (int)g_clients.size() - 1; n >= 0; n--) {
				SendData(g_clients[n], header);
			}
		}
	}
};

#endif