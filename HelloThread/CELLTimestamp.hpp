#ifndef _CELLTimestamp_hpp_
#define _CELLTimestamp_hpp_

#pragma once

#include<chrono>
using namespace std::chrono;

class CELLTimestamp {
public:
	CELLTimestamp() {
		update();
	}
	~CELLTimestamp() {

	}

	void update() {
		_begin = high_resolution_clock::now();
	}

	double getElapsedTimeInSec() {
		return this->getElapsedTimeInMicroSec()*0.000001;
	}

	double getElapseTimeInMilliSec() {
		return this->getElapsedTimeInMicroSec() * 0.001;
	}

	//ªÒ»°Œ¢√Î
	long long getElapsedTimeInMicroSec() {
		auto t = duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
		return t;
	}


protected:
	time_point<high_resolution_clock> _begin;

};

#endif // !_CELLTimestamp_hpp_