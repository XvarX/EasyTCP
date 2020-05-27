#include "EasyTcpServer.hpp"

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
}