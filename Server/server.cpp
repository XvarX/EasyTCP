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

enum CMD {
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,
	CMD_ERROR
};
struct DataHead {
	short dataLength; //数据长度
	short cmd;//命令
};

struct Login : public DataHead {
	Login() {
		dataLength = 0;
		cmd = CMD_LOGIN;
	}
	char userName[32];
	char userPassWord[32];
};

struct LoginResult : public DataHead {
	LoginResult() {
		dataLength = sizeof(LoginResult);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}
	int result;
};

struct Logout : public DataHead {
	Logout() {
		dataLength = sizeof(Logout);
		cmd = CMD_LOGOUT;
	}
	char userName[32];
};

struct LogoutResult : public DataHead {
	LogoutResult() {
		dataLength = sizeof(LogoutResult);
		cmd = CMD_LOGOUT_RESULT;
		result = 0;
	}
	int result;
};

struct NewUserJoin : public DataHead {
	NewUserJoin() {
		dataLength = sizeof(NewUserJoin);
		cmd = CMD_NEW_USER_JOIN;
		sock = 0;
	}
	int sock;
};

std::vector<SOCKET> g_clients;

int processor(SOCKET _cSock) {
	char szRecv[1024] = {};
	printf("sizeof DataHead %d", sizeof(DataHead));
	int nLen = recv(_cSock, szRecv, sizeof(DataHead), 0);
	DataHead* header = (DataHead*)szRecv;
	if (nLen <= 0) {
		printf("client exit %d\n", _cSock);
		return -1;
	}
	//if (nLen >= header->dataLength);

	switch (header->cmd) {
	case CMD_LOGIN:
	{
		recv(_cSock, szRecv + sizeof(DataHead), header->dataLength - sizeof(DataHead), 0);
		Login* login = (Login*)szRecv;
		printf("recv command: %d %d username=%s password=%s\n", CMD_LOGIN, login->dataLength, login->userName, login->userPassWord);
		LoginResult ret;
		send(_cSock, (char*)&ret, sizeof(LoginResult), 0);
	}
	break;
	case CMD_LOGOUT:
	{
		recv(_cSock, szRecv + sizeof(DataHead), header->dataLength - sizeof(DataHead), 0);
		Logout* logout = (Logout*)szRecv;
		printf("recv command: %d %d username=%s\n", CMD_LOGOUT, logout->dataLength, logout->userName);

		LogoutResult ret;
		send(_cSock, (char*)&ret, sizeof(LogoutResult), 0);
	}
	break;
	default:
		DataHead header = {};
		header.cmd = CMD_ERROR;
		header.dataLength = 0;
		send(_cSock, (char*)&header, sizeof(DataHead), 0);
	}
	return 0;
}

int main() {
#ifdef _WIN32
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);
#endif

	//生成套接字，这里面的第一个参数是ipv4/ipv6 第二个是数据类型 第三个是tcp/udp
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//绑定socket，之所以不直接使用sockaddr是因为sockaddr的结构不好手填
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);

#ifdef _WIN32
	_sin.sin_addr.S_un.S_addr = INADDR_ANY;
#else
	_sin.sin_addr.s_addr = INADDR_ANY;
#endif

	if (bind(_sock, (sockaddr*)&_sin, sizeof(_sin)) == SOCKET_ERROR) {
		printf("bind error\n");
	}
	else {
		printf("bind success\n");
	}

	if (listen(_sock, 5) == SOCKET_ERROR) {
		printf("listen error\n");
	}
	else {
		printf("listen success\n");
	}

	while (true) {
		fd_set rdRead;
		fd_set rdWrite;
		fd_set rdExcept;

		FD_ZERO(&rdRead);
		FD_ZERO(&rdWrite);
		FD_ZERO(&rdExcept);

		FD_SET(_sock, &rdRead);
		FD_SET(_sock, &rdWrite);
		FD_SET(_sock, &rdExcept);
		SOCKET maxSock = _sock;
		for (int n = (int)g_clients.size() - 1; n >= 0; n--) {
			FD_SET(g_clients[n], &rdRead);
			if (g_clients[n] > maxSock) {
				maxSock = g_clients[n];
			}
		}

		timeval t = { 0, 0 };
		int ret = select(maxSock + 1, &rdRead, &rdWrite, &rdExcept, &t);
		if (ret < 0) {
			printf("select error\n");
			break;
		}
		if (FD_ISSET(_sock, &rdRead)) {
			FD_CLR(_sock, &rdRead);
			sockaddr_in clientAddr = {};
			int nAddrLen = sizeof(clientAddr);
			SOCKET _cSock = INVALID_SOCKET;
#ifdef _WIN32
			_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
			_cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#endif
			if (INVALID_SOCKET == _cSock) {
				printf("error client socket\n");
			}
			else {
				for (int n = (int)g_clients.size() - 1; n >= 0; n--) {
					NewUserJoin userjoin;
					userjoin.sock = _cSock;
					send(g_clients[n], (char*)&userjoin, sizeof(NewUserJoin), 0);
				}
				g_clients.push_back(_cSock);
				printf("new client socket %d %s\n", (int)_cSock, inet_ntoa(clientAddr.sin_addr));
			}
		}

		for (int n = (int)g_clients.size() - 1; n >= 0; n--) {
			if (FD_ISSET(g_clients[n], &rdRead)) {
				if (-1 == processor(g_clients[n])) {
					auto iter = g_clients.begin() + n;
					if (iter != g_clients.end()) {
						g_clients.erase(iter);
					}
				}
			}
		}
	}
#ifdef _WIN32
	for (int n = (int)g_clients.size() - 1; n >= 0; n--) {
		closesocket(g_clients[n]);
	}
#else
	for (int n = (int)g_clients.size() - 1; n >= 0; n--) {
		close(g_clients[n]);
	}
#endif

#ifdef _WIN32
	closesocket(_sock);
#else
	close(_sock);
#endif

#ifdef _WIN32
	WSACleanup();
#endif
	getchar();
	return 0;
}