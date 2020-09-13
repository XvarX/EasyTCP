#include<iostream>
#include<thread>
#include<mutex>//Ëø
using namespace std;
mutex m;
void workFun(int index) {
	for (int n = 0; n < 100; n++) {
		m.lock();
		cout << index << "Hello, other thread" << n << endl;
		m.unlock();
	}
}

int main() {
	thread t[3];
	for (int n = 0; n < 3; n++) {
		t[n] = thread(workFun, n);
	}
	//t.join();
	for (int n = 0; n < 3; n++) {
		t[n].join();
	}

	for (int n = 0; n < 4; n++) {
		cout << "Hello, main Thread" << endl;
	}

	while (true) {

	}
	return 0;
}