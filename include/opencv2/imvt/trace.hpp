#ifndef __OPENCV_TRACE_HPP__
#define __OPENCV_TRACE_HPP__

#include <string>
#include <map>
#include "opencv2/core.hpp"


namespace cv {
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

}	// end namespace cv
}	// end namespace imvt



#endif	// end __OPENCV_TRACE_HPP__