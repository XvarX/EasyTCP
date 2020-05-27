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
	}
	//�����˿ں�
	int Listen(int n) {
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret) {
			printf("Socket=<%d>���󣬼�������˿�ʧ��...\n", _sock);
		}
		else {
			printf("Socket=<%d>��������˿ڳɹ�");
		}
		return ret;
	}
	//���ܿͻ�������
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
	//�Ƿ�����
	//��������
	//��Ӧ������Ϣ
	//��������
};

#endif