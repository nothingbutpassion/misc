#pragma once

#include <vector>
#include <set>
#include <string>

class Process {
public:
	Process(): pid(-1), infd(-1), outfd(-1), errfd(-1) {}	
	bool start(const std::string& file, const std::vector<std::string>& args, 
		       const std::string& stdinFile, const std::string& stdoutFile, const std::string& stderrFile);

	int inFD() const { return infd; }
	int outFD() const { return outfd; }
	int errFD() const { return errfd; }
	pid_t id() const { return pid; }

	void dump();

private:
	pid_t fork(const std::string& stdinFile, const std::string& stdoutFile, const std::string& stderrFile);
	bool closeAllFDs(const std::set<int>& excludeFDs);
	void closeFDs(const std::vector<int>& fds);

private:
	pid_t pid;
	int infd;
	int outfd;
	int errfd;
	std::string file;
	std::vector<std::string> args;
};


