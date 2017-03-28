#include "precomp.hpp"
#include "opencl_kernels_imvt.hpp"

namespace cv {
namespace imvt {

static bool ocl_remap(InputArray _src, OutputArray _dst, InputArray _map1, InputArray _map2,
                      int interpolation, int borderType, const Scalar& borderValue)
{
    const ocl::Device & dev = ocl::Device::getDefault();
    int cn = _src.channels(), type = _src.type(), depth = _src.depth(),
            rowsPerWI = dev.isIntel() ? 4 : 1;

    if (borderType == BORDER_TRANSPARENT || !(interpolation == INTER_LINEAR || interpolation == INTER_NEAREST)
            || _map1.type() == CV_16SC1 || _map2.type() == CV_16SC1)
        return false;

    UMat src = _src.getUMat(), map1 = _map1.getUMat(), map2 = _map2.getUMat();

    if( (map1.type() == CV_16SC2 && (map2.type() == CV_16UC1 || map2.empty())) ||
        (map2.type() == CV_16SC2 && (map1.type() == CV_16UC1 || map1.empty())) )
    {
        if (map1.type() != CV_16SC2)
            std::swap(map1, map2);
    }
    else
        CV_Assert( map1.type() == CV_32FC2 || (map1.type() == CV_32FC1 && map2.type() == CV_32FC1) );

    _dst.create(map1.size(), type);
    UMat dst = _dst.getUMat();

    String kernelName = "remap";
    if (map1.type() == CV_32FC2 && map2.empty())
        kernelName += "_32FC2";
    else if (map1.type() == CV_16SC2)
    {
        kernelName += "_16SC2";
        if (!map2.empty())
            kernelName += "_16UC1";
    }
    else if (map1.type() == CV_32FC1 && map2.type() == CV_32FC1)
        kernelName += "_2_32FC1";
    else
        CV_Error(Error::StsBadArg, "Unsupported map types");

    static const char * const interMap[] = { "INTER_NEAREST", "INTER_LINEAR", "INTER_CUBIC", "INTER_LINEAR", "INTER_LANCZOS" };
    static const char * const borderMap[] = { "BORDER_CONSTANT", "BORDER_REPLICATE", "BORDER_REFLECT", "BORDER_WRAP",
                           "BORDER_REFLECT_101", "BORDER_TRANSPARENT" };
    String buildOptions = format("-D %s -D %s -D T=%s -D rowsPerWI=%d",
                                 interMap[interpolation], borderMap[borderType],
                                 ocl::typeToStr(type), rowsPerWI);

    if (interpolation != INTER_NEAREST)
    {
        char cvt[3][40];
        int wdepth = std::max(CV_32F, depth);
        buildOptions = buildOptions
                      + format(" -D WT=%s -D convertToT=%s -D convertToWT=%s"
                               " -D convertToWT2=%s -D WT2=%s",
                               ocl::typeToStr(CV_MAKE_TYPE(wdepth, cn)),
                               ocl::convertTypeStr(wdepth, depth, cn, cvt[0]),
                               ocl::convertTypeStr(depth, wdepth, cn, cvt[1]),
                               ocl::convertTypeStr(CV_32S, wdepth, 2, cvt[2]),
                               ocl::typeToStr(CV_MAKE_TYPE(wdepth, 2)));
    }
    int scalarcn = cn == 3 ? 4 : cn;
    int sctype = CV_MAKETYPE(depth, scalarcn);
    buildOptions += format(" -D T=%s -D T1=%s -D cn=%d -D ST=%s -D depth=%d",
                           ocl::typeToStr(type), ocl::typeToStr(depth),
                           cn, ocl::typeToStr(sctype), depth);

    ocl::Kernel k(kernelName.c_str(), ocl::imvt::remap_oclsrc, buildOptions);

    Mat scalar(1, 1, sctype, borderValue);
    ocl::KernelArg srcarg = ocl::KernelArg::ReadOnly(src), dstarg = ocl::KernelArg::WriteOnly(dst),
            map1arg = ocl::KernelArg::ReadOnlyNoSize(map1),
            scalararg = ocl::KernelArg::Constant((void*)scalar.ptr(), scalar.elemSize());

    if (map2.empty())
        k.args(srcarg, dstarg, map1arg, scalararg);
    else
        k.args(srcarg, dstarg, map1arg, ocl::KernelArg::ReadOnlyNoSize(map2), scalararg);

    size_t globalThreads[2] = { (size_t)dst.cols, ((size_t)dst.rows + rowsPerWI - 1) / rowsPerWI };
    return k.run(2, globalThreads, NULL, false);
}


CV_EXPORTS_W void remap(InputArray src, OutputArray dst,
						  InputArray map1, InputArray map2,
						  int interpolation, int borderMode,
						  const Scalar& borderValue) {
	CV_Assert(map1.size().area() > 0);
	CV_Assert(map2.empty() || (map2.size() == map1.size()));
	CV_OCL_RUN(src.dims() <= 2 && dst.isUMat(), 
		ocl_remap(src, dst, map1, map2, interpolation, borderMode, borderValue))

}


}	// end namespace imvt
} 	// end namespace cv