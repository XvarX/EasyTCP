#include "EasyTcpServer.hpp"
#include "thread"

bool g_bRun = true;
void cmdThread() {
	while (true) {
		char cmdBuff[256] = {};
		scanf("%s", cmdBuff);
		if (0 == strcmp(cmdBuff, "exit")) {
			g_bRun = false;
			printf("退出cmdThread线程\n");
			break;
		}
		else {
			printf("不支持的命令\n");
		}
	}
}

int main() {
	std::thread t1(cmdThread);
	t1.detach();

	EasyTcpServer server;
	server.InitSocket();
	server.Bind(nullptr, 4567);
	server.Listen(5);
	while (g_bRun) {
		server.OnRun();
	}
	server.Close();
}