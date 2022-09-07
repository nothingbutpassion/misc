#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <dirent.h>
#include <pthread.h>
#include <string>
#include <memory>
#include <fstream>
#include <vector>
#include <boost/regex.hpp>


/**
  * @brief  A simple mutex employed to avoid race.
  */
struct Mutex {
	Mutex() { pthread_mutex_init(&mutex, NULL); }
	~Mutex() { pthread_mutex_destroy(&mutex); }
private:
	friend struct Lock;
	pthread_mutex_t mutex;
};

/**
  * @brief  A scoped lock (lock in constructor, unlock in destructor)
  */
struct Lock {
	Lock(Mutex& m) : mutexRef(m) { pthread_mutex_lock(&mutexRef.mutex); }
	~Lock() { pthread_mutex_unlock(&mutexRef.mutex); }
private:
	Mutex& mutexRef;
};



/**
  * @brief  Helper function to execute a shell cmd (by using "/bin/sh -c cmd " ).
  *
  * @param cmd Shell command to be executed.
  * @return The stdout of the command.
  */
inline std::string shell(const std::string& cmd) {
    std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
	std::string result = "";
    if (!pipe) {
		return result;
    }
	
    char buffer[128];
    while (!feof(pipe.get())) {
        if (fgets(buffer, sizeof(buffer), pipe.get()) != NULL) {
            result += buffer;
        }
    }
    return result;
}


/**
  * @brief  Helper function to get the executable file path of current process
  *
  * @return The executable file path if succeed, otherwise return a empty string.
  */
inline std::string exePath() {
	static std::string path;
	if (path.empty()) {
		char buf[256] = { 0 };
    	readlink("/proc/self/exe", buf, sizeof(buf)-1);
		path = buf;
	}
	return path;
}


/**
  * @brief  Helper function to get the dir name of a specified path
  *
  * @param path The absolute/relative path
  *
  * @return The dir name of the specified path, otherwise return current dir(".")
  */
inline std::string dirName(const std::string& path) {
	static std::string dir = ".";
	std::size_t pos = path.rfind("/");
	if (pos != std::string::npos) {
		dir = path.substr(0, pos);
	}
	return dir;
}


/**
  * @brief  Helper function to get the base name of a specified path
  *
  * @param path The absolute/relative path
  * @return The base name of the specified path, otherwise return the input path
  */
inline std::string baseName(const std::string& path) {
	static std::string base = path;
	std::size_t pos = path.rfind("/");
	if (pos != std::string::npos) {
		base = path.substr(pos+1);
	}
	return base;
}


/**
  * @brief  Helper function to creat dir if not exists.
  *
  * @param dir Thid directory to be created.
  *
  * @return true for success, otherwise return false.
  */
inline bool createDir(const std::string& dir) {
	// Not using boost::filesystem::create_directories(dir.c_str());
	int err = mkdir(dir.c_str(), S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
	if (err && errno != EEXIST) {
		return false;
	}
	return true;
}


/**
  * @brief  Helper function to check whethe the specified path is a directory
  *
  * @param path The path to be checked
  *
  * @return true if path is a directory, otherwise false is returned.
  */
inline bool isDir(const std::string& path) {
	struct stat st;
	int err = stat(path.c_str(), &st);
	if (!err && S_ISDIR(st.st_mode)) {
		return true;
	}	
	return false;
}


/**
  * @brief  Helper function to check whethe the specified path is it a regular file
  *
  * @param path The path to be checked
  *
  * @return true if path is a regular file, otherwise false is returned.
  */
inline bool isFile(const std::string& path) {
	struct stat st;
	int err = stat(path.c_str(), &st);
	if (!err && S_ISREG(st.st_mode)) {
		return true;
	}	
	return false;
}


/**
  * @brief  Helper function to check whethe the specified path is exist.
  *
  * @param path The path to be checked
  *
  * @return true if path is exist, otherwise false is returned.
  */
inline bool pathExist(const std::string& path) {
	struct stat st;
	return (stat(path.c_str(), &st) == 0);
}

/**
  * @brief  Helper function to get the full path by specified file name
  *
  * @param file the relative/symbol file name
  *
  * @return the full path if succeed. otherwise a empty string is returned.
  */

inline std::string fullPath(const std::string& file) {
	char real[PATH_MAX];
	if (realpath(file.c_str(), real) == NULL) {
		return "";
	}
	return real;
}


/**
  * @brief  Helper function to delete the specified file/directory path.
  *
  * @param path The file/directory path to be deleted
  *
  * @return true if succeed, otherwise false is returned.
  */
inline bool deletePath(const std::string& path) {
	return (unlink(path.c_str()) == 0);
}

/**
  * @brief  Helper function to read the specified file content
  *
  * @param file The input file to be read
  * @param content used to save the content of the input file.
  *
  * @return true if succeed, otherwise false is returned.
  */
inline bool readFile(const std::string& file, std::string& content) {
	FILE* fp = fopen(file.c_str(), "r");
	if (!fp) {
		return false;
	}

	char buffer[128];
	content = "";
    while (!feof(fp)) {
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            content += buffer;
        }
    }
	fclose(fp);
    return true;
}

/**
  * @brief  Helper function to read the specified file lines
  *
  * @param file The input file to be read
  * @param lines used to save the content of the input file.
  *
  * @return true if succeed, otherwise false is returned.
  */
inline bool readFile(const std::string& file, std::vector<std::string>& lines) {
	std::ifstream ifs(file);
	if (!ifs) {
		return false;
	}
	
	std::string line;
	lines = {};
	while (std::getline(ifs, line, '\n')) {
		lines.push_back(line);
	}
    return true;
}


/**
  * @brief  Helper function to write string to file
  *
  * @param file The file to be writted
  * @param content the content to writted to the file
  *
  * @return true if succeed, otherwise false is returned.
  */
inline bool writeFile(const std::string& file, const std::string& content) {
	std::ofstream ofs(file);
	return ofs.write(content.data(), content.size());
}


/**
  * @brief  Helper function to write a vector string  to the specified file
  *
  * @param file The file to be writted
  * @param lines the lines to writted to the file
  *
  * @return true if succeed, otherwise false is returned.
  */
inline bool writeFile(const std::string& file, const std::vector<std::string>& lines) {
	std::ofstream ofs(file);
	if (!ofs) {
		return false;
	}

	for (std::size_t i = 0; i < lines.size(); i++) {
		ofs.write(lines[i].data(), lines[i].size());
		ofs.write("\n", 1);
	}
	
    return ofs;
}

/**
  * @brief  Helper function to get all files/directories entries in the specified dir
  *
  * @param dir The dir from which all file/directories are got
  *
  * @return All entry names found in the specified dir 
  */
inline std::vector<std::string> dirEnties(const std::string& dir) {
	std::vector<std::string> results;
	std::shared_ptr<DIR> dp(opendir(dir.c_str()), closedir);
	if (!dp) {
		return results;
	}

	dirent* ent;
	while ((ent = readdir(dp.get())) != NULL) {
		std::string name(ent->d_name);
		if (name != "." && name != "..") {
			results.push_back(name);
		}
	}
	return results;
}



/**
  * @brief  Helper function to get a formated std::string.
  *
  * @param fmt This is c-style format string.
  *
  * @return The formated std::string
  */
inline std::string format(const char* fmt, ... ) {
    char buf[1 << 10];
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    return std::string(buf);
}


/**
  * @brief  Helper function to search regual expression from src, optionally specify the matched group
  *
  * @param src This string to be searched from
  * @param regex The regual expression
  * @param group The group id of regex
  *
  * @return The matched group string 
  */
inline std::string regexSearch(const std::string& src, const std::string& regex, std::size_t group = 0) {
    boost::smatch m;
    bool find = boost::regex_search(src.begin(), src.end(), m, boost::regex(regex));
    if (find && group < m.size()) {
       return m.str(group); 
    }
    return "";   
}


/**
  * @brief  Helper function to match regual expression with src, optionally specify the matched group
  *
  * @param src This string to be matched
  * @param regex The regual expression
  * @param group The group id of regex
  *
  * @return The matched group string 
  */
inline std::string regexMatch(const std::string& src, const std::string& regex, std::size_t group = 0) {
    boost::smatch m;
    bool find = boost::regex_match(src.begin(), src.end(), m, boost::regex(regex));
    if (find && group < m.size()) {
       return m.str(group); 
    }
    return "";   
}



/**
  * @brief  Helper function to use fmt replace the regex matched part of src
  *
  * @param src This string to be searched from
  * @param fmt The regual expression
  * @param fmt The expression used to replace regex
  * @return The string has been replaced
  */
inline std::string regexReplace(const std::string& src, const std::string& regex, const std::string& fmt) {
    return boost::regex_replace(src, boost::regex(regex), fmt);   
}


