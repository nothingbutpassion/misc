#include "clut.h"

using namespace clut;
using namespace std;

int main(int argc, char* argv[]) {

    openCLInit();
    
    char h_src[4][4] = {
        {0,  1,  2,  3},
        {4,  5,  6,  7},
        {8,  9,  10, 11},
        {12, 13, 14, 15}
    };
    
    char h_dst[4][4] = {0};
    
    
    printf("input:\n");
    for (int i=0; i < 4; i++) {
        for (int j=0; j < 4; j++) {
            printf("%d\t", h_src[i][j]);
        }
        printf("\n");
    } 
    
    
    cl_mem d_src = openCLCreateBuffer(sizeof(h_src));
    cl_mem d_dst = openCLCreateBuffer(sizeof(h_dst));
    
    openCLWriteBuffer(d_src, h_src, sizeof(h_src));
    
    cl_program program = openCLCreateProgram("./boxFilter.cl", "-DanX=1 -DanY=1 -DksX=3 -DksY=3 -DBORDER_REPLICATE");
    cl_kernel kernel = openCLCreateKernel(program, "boxFilter_C1_D0");
    
    
    cl_float alpha = 3*3;
    cl_int d_src_offset = 0;
    cl_int d_src_wholerows = 4;
    cl_int d_src_wholecols = 4;
    cl_int d_src_step = 4;
    cl_int d_dst_offset = 0;
    cl_int d_dst_rows = 4;
    cl_int d_dst_cols = 4;
    cl_int d_dst_step = 4;
    vector<pair<size_t , const void *> > args;
    args.push_back(make_pair(sizeof(cl_mem), (void *)&d_src));
    args.push_back(make_pair(sizeof(cl_mem), (void *)&d_dst));
    args.push_back(make_pair(sizeof(cl_float), (void *)&alpha));
    args.push_back(make_pair(sizeof(cl_int), (void *)&d_src_offset));
    args.push_back(make_pair(sizeof(cl_int), (void *)&d_src_wholerows));
    args.push_back(make_pair(sizeof(cl_int), (void *)&d_src_wholecols));
    args.push_back(make_pair(sizeof(cl_int), (void *)&d_src_step));
    args.push_back(make_pair(sizeof(cl_int), (void *)&d_dst_offset));
    args.push_back(make_pair(sizeof(cl_int), (void *)&d_dst_rows));
    args.push_back(make_pair(sizeof(cl_int), (void *)&d_dst_cols));
    args.push_back(make_pair(sizeof(cl_int), (void *)&d_dst_step));
    openCLSetKernelArgs(kernel,args);
    
    size_t globalThreads[3] = { d_dst_cols, d_dst_rows, 1 };
    size_t localThreads[3]  = { 4, 1, 1 };
    openCLExecuteKernel(kernel, globalThreads, localThreads);
   
    openCLReadBuffer(d_dst, h_dst, sizeof(h_dst));
    
   
    printf("output:\n");
    for (int i=0; i < 4; i++) {
        for (int j=0; j < 4; j++) {
            printf("%d\t", h_dst[i][j]);
        }
        printf("\n");
    } 
    
    openCLReleaseMemObject(d_dst);
    openCLReleaseMemObject(d_src);

    openCLReleaseKernel(kernel);
    openCLReleaseProgram(program);
    
    openCLDestroy();
    return 0;
}





