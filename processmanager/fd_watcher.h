#pragma once

class FDWatcher {
public:
	enum FDType {
		FD_READ,
		FD_WRITE
	};

	struct FDResquest {
		FDType type;
		int fd;
		int request;
	};

	
	static FDWatcher& instance() {
		static FDWatcher fdw;
		return fdw;
	}

	bool start();
	bool stop();
	bool add(int type, int fd);


private:
	FDWatcher();
	~FDWatcher();
	FDWatcher(const FDWatcher&);
	FDWatcher& operator=(const FDWatcher&);
	bool init();
	bool release();

};





