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
	EasyTcpClient client2;

	client.initSocket();
	client.Connect((char*)"127.0.0.1", 4567);
	std::thread t1(cmdThread, &client);
	t1.detach();

	client2.Connect((char*)"127.0.0.1", 4567);
	std::thread t2(cmdThread, &client2);
	t2.detach();

	while (client.isRun() || client2.isRun()) {
		client.OnSelect();
		client2.OnSelect();
	}
	client.close();
	client2.close();
	getchar();
	return 0;
}