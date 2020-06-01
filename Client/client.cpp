#include "EasyTcpClient.hpp"

bool g_bRun = true;
void cmdThread(EasyTcpClient* client) {
	while (true) {
		char cmdBuff[256] = {};
		scanf("%s", cmdBuff);
		if (0 == strcmp(cmdBuff, "exit")) {
			printf("exit\n");
			client->close();
			return;
		}
		else if (0 == strcmp(cmdBuff, "login")) {
			Login login;
			strcpy(login.userName, "yxj");
			strcpy(login.userPassWord, "123456");
			client->SendData(&login);
		}
		else if (0 == strcmp(cmdBuff, "logout")) {
			Logout logout;
			strcpy(logout.userName, "yxj");
			client->SendData(&logout);
		}
	}
}

int main() {
	EasyTcpClient client;

	client.initSocket();
	client.Connect((char*)"127.0.0.1", 4567);
	std::thread t1(cmdThread, &client);
	t1.detach();

	while (client.isRun()) {
		client.OnSelect();
	}
	client.close();
	getchar();
	return 0;
}