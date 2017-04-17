#include <cstdarg>
#include <iostream>
#include "opencv2/imvt/trace.hpp"

namespace cv {
namespace imvt {


MatTrace& MatTrace::instance() {
	static MatTrace mt;
	return mt;
}

void MatTrace::add(const string& kind, const string& key, const Mat& value) {
	//std::cout << "add mat " << kind << key << " cols=" << value.cols << " rows=" << value.rows 
	//	<< "channels=" << value.channels() << " type=" << value.type() << std::endl;
	value.copyTo(mats[kind][key]);
}

void MatTrace::add(const string& kind, const string& key, const UMat& value) {
	//std::cout << "add umat " << kind << key << " cols=" << value.cols << " rows=" << value.rows 
	//	<< " channels=" << value.channels() << " type=" << value.type() << std::endl;
	value.copyTo(mats[kind][key]);
}

map<string, Mat> MatTrace::get(const string& kind) {
	CV_Assert(mats.count(kind) > 0);
	return mats[kind];
}

Mat MatTrace::get(const string& kind, const string& key) {
	CV_Assert(mats.count(kind) > 0 && mats[kind].count(key) > 0);
	return mats[kind][key];
}

Mat MatTrace::diff(const string& kind1, const string& kind2, const string& key) {
	CV_Assert(mats.count(kind1) > 0 && mats[kind1].count(key) > 0);
	CV_Assert(mats.count(kind2) > 0 && mats[kind2].count(key) > 0);
	return mats[kind1][key] - mats[kind2][key];
}

map<string, Mat> MatTrace::diff(const string& kind1, const string& kind2) {
	CV_Assert(mats.count(kind1) > 0);
	CV_Assert(mats.count(kind2) > 0);
	CV_Assert(mats[kind1].size() == mats[kind2].size());

    map<string, Mat>& mats1 = mats[kind1];
    map<string, Mat>& mats2 = mats[kind2];
	map<string, Mat> results;
    for (const auto& m : mats1) {
		CV_Assert(mats2.count(m.first) > 0);
        results[m.first] = mats1[m.first] - mats2[m.first];
    }
	return results;
}


CV_EXPORTS_W string format(const char* fmt, ...) {
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	return buf;
}


}	// end namespace imvt
} 	// end namespace cv