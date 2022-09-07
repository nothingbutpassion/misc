#pragma once

#include <unistd.h>
#include <string>
#include <vector>

class ProcessManager {
public:

	/**
	  * @brief Get the global unique instance of ProcessManager
	  *
	  * @return The reference of the ProcessManager instance.
	  * 
	  */
	static ProcessManager& instance() {
		static ProcessManager pm;
		return pm;
	}

	/**
	  * @brief Create a new process with specified executable file and arguemts, optionally with a stderr redirected file.
	  *
	  * @param exeFile	 The executable file of the new process.
	  * @param args The arguments list for the new process.
	  *
	  * @return The pid of the new process if succeed, otherwise -1.
	  * 
	  * @note the stdout/stderr contents of the new process will be printed to parent's stderr.
	  *
	  */
	pid_t create(const std::string& exeFile, const std::vector<std::string>& args);


    /**
	  * @brief Create a new process with specified executable file and arguemts, optionally with a stderr redirected file.
	  *
	  * @param exeFile	 The executable file of the new process.
	  * @param args The arguments list for the new process.
	  * @param stderrFile The stderr of the new process will be redirect to this file
	  *
	  * @return The pid of the new process if succeed, otherwise -1.
	  * 
	  */
	pid_t create(const std::string& exeFile, const std::vector<std::string>& args, 
				  const std::string& stderrFile);


	/**
	  * @brief Create a new process with specified executable file and arguemts, optionally with a stderr redirected file.
	  *
	  * @param exeFile	 The executable file of the new process.
	  * @param args The arguments list for the new process.
	  * @param stdoutFile  The stdout of the new process will be redirect to this file
	  * @param stderrFile The stderr of the new process will be redirect to this file
	  *
	  * @return The pid of the new process if succeed, otherwise -1.
	  * 
	  */
	pid_t create(const std::string& exeFile, const std::vector<std::string>& args, 
				  const std::string& stdoutFile, const std::string& stderrFile);


	/**
	  * @brief Terminate a specified process.
	  *
	  * @param pid A valid pid which is returned by create()
	  *
	  * @return true if the process is succeefully terminated, otherwise false.
	  *
	  * @note The implementation send SIGTERM to the process if it's alive.
	  */
	bool terminate(pid_t pid);



	/**
	  * @brief Kill a specified process.
	  *
	  * @param pid A valid pid which is returned by create()
	  *
	  * @return true if the process is succeefully terminated, otherwise false.
	  *
	  * @note The implementation send SIGKILL to the process if it's alive.
	  */
	bool kill(pid_t pid);


	/**
	  * @brief Decide whether a specified process is alive.
	  *
	  * @param pid A valid pid which is returned by create().
	  *
	  * @return ture if the process is alive, otherwise false.
	  *
	  */
	bool isAlive(pid_t pid);

	/**
	  * @brief Get the stdin file descriptor of a specified process.
	  *
	  * @param pid A valid pid which is returned by create().
	  *
	  * @return The stdin file descriptor if the process is alive, otherwise -1.
	  *
	  */
	int inFD(pid_t pid);
	

	/**
	  * @brief Get the stdout file descriptor of a specified process.
	  *
	  * @param pid A valid pid which is returned by create().
	  *
	  * @return The stdout file descriptor if the process is alive, otherwise -1.
	  *
	  */
	int outFD(pid_t pid);


	/**
	  * @brief Get the stderr file descriptor of a specified process.
	  *
	  * @param pid A valid pid which is returned by create().
	  *
	  * @return The stderr file descriptor if the process is alive, otherwise -1.
	  *
	  */
	int errFD(pid_t pid);


	
	/**
	  * @brief Read the stdin of a specified process.
	  *
	  * @param pid A valid pid which is returned by create().
	  * @param buf The buffer used to receive the read data from the process' stdout.
	  * @param buflen The size of buf.
	  * @param timeout The timeout miniseconds for reading data from the process' stdout.
	  *
	  * @return -1 if error occurs, 0 if read timeout (or end of file), otherwise the size of the read data.
	  *              
	  */
	ssize_t recvData(pid_t pid, void* buf, size_t buflen, int timeout = 300);

	/**
	  * @brief Write the stdout of a specified process.
	  *
	  * @param pid A valid pid which is returned by create().
	  * @param buf The data to be writted to the process' stdin.
	  * @param buflen The length of buf.
	  * @param timeout The timeout miniseconds for writing data to the process' stdin.
	  *
	  * @return -1 if error occurs, 0 if read timeout (or nothing was written), otherwise the size of the writted data.
	  *              
	  */
	ssize_t sendData(pid_t pid, const void* buf, size_t buflen, int timeout = 300);


		
	enum EnventType {
		FD_READ_ERR,
		FD_WRITE_ERR,
		FD_POLL_ERR,
		SIG_CHLD_EXIT
	};

	union EventData {
		int fd;
		void* ptr;
		pid_t pid;
	};

	struct Event {
		EnventType type;
		EventData data;
	};

	void notify(const Event& event);
	bool start();
	bool stop();
	
	
private:
	ProcessManager() {}
	ProcessManager(const ProcessManager&);
	ProcessManager& operator=(const ProcessManager&);

	ssize_t read(int fd, void* buf, size_t buflen, int timeout);
	ssize_t write(int fd, const void* buf, size_t buflen, int timeout);
	
	bool probeAlive(pid_t pid);
	bool recycle(pid_t pid);
	pid_t owner(int fd);
	bool terminateAll();
	bool killAll();
};


