#ifndef __OCL_IMVT_TRACE_HPP__
#define __OCL_IMVT_TRACE_HPP__

#include <string>
#include <map>
#include "opencv2/core.hpp"

namespace cv {
namespace ocl {
namespace imvt {

using std::map;
using std::string;

struct CV_EXPORTS_W MatTrace {
	static MatTrace& instance();
		
	void add(const string& kind, const string& key, const Mat& value);
	void add(const string& kind, const string& key, const UMat& value);
        
	map<string, Mat> get(const string& kind);
    Mat get(const string& kind, const string& key);

	map<string, Mat> diff(const string& kind1, const string& kind2);
    Mat diff(const string& kind1, const string& kind2, const string& key);
            
private:
	map<string, map<string, Mat>> mats;
};

CV_EXPORTS_W string format(const char* fmt, ...);

}	// namespace imvt
}	// namespace ocl
}	// namespace cv



#endif	// __OCL_IMVT_TRACE_HPP__