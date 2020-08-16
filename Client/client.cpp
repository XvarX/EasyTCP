#include "EasyTcpClient.hpp"

bool g_bRun = true;
void cmdThread(EasyTcpClient* client) {
	while (true) {
		char cmdBuff[256] = {};
		scanf("%s", cmdBuff);
		if (0 == strcmp(cmdBuff, "exit")) {
			printf("exit\n");
			g_bRun = false;
			return;
		}
	}
}

int main() {
	const int cCount = 200;
	EasyTcpClient* client[cCount];

	for (int n = 0; n < cCount; n++) {
		if (!g_bRun) {
			return 0;
		}
		client[n] = new EasyTcpClient();
		client[n]->initSocket();
		client[n]->Connect((char*)"127.0.0.1", 4567);
	}
	//std::thread t1(cmdThread, &client);
	//t1.detach();


	Login login;
	strcpy(login.userName, "lyd");
	strcpy(login.userPassWord, "lydmm");
	while (g_bRun) {
		for (int n = 0; n < cCount; n++) {
			client[n]->OnSelect();
			client[n]->SendData(&login);
		}
	}

	for (int n = 0; n < cCount; n++) {
		client[n]->close();
	}
	getchar();
	return 0;
}