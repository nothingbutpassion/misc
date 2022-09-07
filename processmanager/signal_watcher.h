#pragma once

class SignalWatcher {
public:
	static SignalWatcher& instance() {
		static SignalWatcher sw;
		return sw;
	}
	bool start();
	bool stop();

private:
	SignalWatcher() ;
	~SignalWatcher();
	SignalWatcher(const SignalWatcher&);
	SignalWatcher& operator=(const SignalWatcher&);
	bool init();
	bool release();
};



