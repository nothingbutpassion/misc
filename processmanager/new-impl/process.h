#pragma once

#include <string>
#include <vector>

enum IOEType {
	IOE_NONE,
	IOE_PIPE,
	IOE_FILE
};

enum { 
	READ,
	WRITE 
};


struct IOE {
	IOEType type = IOE_NONE;
	int fds[2] = {-1, -1};
	std::string name;
};

struct Process {

	bool create(const std::string& file, const std::vector<std::string>& args,
			const std::string& inName, const std::string& outName, const std::string& errName);
	bool isAlive();
	bool kill(int signal = 9);
	void dump();

private:
	int fork();
	int exec();
	bool init();
	void release();
	IOEType toType(const std::string& name);
	
private:
	std::string file;
	std::vector<std::string> args;
	int pid = -1;
	IOE ioes[3];

};
