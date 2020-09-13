#include<iostream>
#include<thread>
#include<mutex>//Ëø
#include"CELLTimestamp.hpp"

using namespace std;

mutex m;
const int tCount = 4;
int sum = 0;

void workFun(int index) {
	for (int n = 0; n < 20000000; n++) {
		lock_guard<mutex> lg(m);
		//m.lock();
		sum++;
		//m.unlock();
	}
}

int main() {
	thread t[tCount];

	for (int n = 0; n < tCount; n++) {
		t[n] = thread(workFun, n);
	}
	

	CELLTimestamp tTime;
	for (int n = 0; n < tCount; n++) {
		t[n].join();
	}
	cout << tTime.getElapseTimeInMilliSec() << "sum=" << sum << endl;
	cout << "Hello, main thread" << endl;
	return 0;
}