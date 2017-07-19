#ifndef __OCL_BUFFER_HPP_
#define __OCL_BUFFER_HPP_

#include <opencv2/core.hpp>

namespace cv {
namespace ocl {
namespace imvt {


CV_EXPORTS_W bool oclInitBuffers(int nCams, Size optSize, Size nvSize, int& numThreads);


}	// namespace imvt
}	// namespace ocl
}	// namespace cv


#endif	// __OCL_BUFFER_HPP_