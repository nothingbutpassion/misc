#include <vector>
#include <opencv2/core.hpp>

namespace cv {
namespace ocl {
namespace imvt {

CV_EXPORTS_W UMat oclGammaLUT();
CV_EXPORTS_W UMat oclAntiGammaLUT();
CV_EXPORTS_W void oclAntiGammaAdjust(const UMat& lut_anti_gamma, UMat& image);
CV_EXPORTS_W void oclGammaAdjust(const UMat& lut_gamma, UMat& image);

CV_EXPORTS_W void oclInitGammaLUT();
CV_EXPORTS_W void oclReleaseGammaLUT();

//preColorAdjustByGamma(gpuImages, -1, 180, 0.01, false, false, true);


}	// namespace imvt
}	// namespace ocl
}	// namespace cv