extern "C" {
#include <vpu_lib.h>
}

RetCode vpu_Init_return = RETCODE_SUCCESS;
RetCode vpu_Init(void *) { 
	return vpu_Init_return;
}

void vpu_UnInit() {}


RetCode vpu_EncOpen_return = RETCODE_SUCCESS; 
RetCode vpu_EncOpen(EncHandle *, EncOpenParam *) {
	return vpu_EncOpen_return;
}

RetCode vpu_EncClose_return = RETCODE_SUCCESS;
RetCode vpu_EncClose(EncHandle) {
	return vpu_EncClose_return;
}

RetCode vpu_EncGetInitialInfo_return = RETCODE_SUCCESS;
RetCode vpu_EncGetInitialInfo(EncHandle, EncInitialInfo*) {
	return vpu_EncGetInitialInfo_return;
}

RetCode vpu_EncRegisterFrameBuffer_return = RETCODE_SUCCESS;
RetCode vpu_EncRegisterFrameBuffer(EncHandle handle, FrameBuffer * bufArray,
				   int num, int frameBufStride, int sourceBufStride,
				   PhysicalAddress subSampBaseA, PhysicalAddress subSampBaseB,
				   EncExtBufInfo * pBufInfo) {
	return vpu_EncRegisterFrameBuffer_return;
}

RetCode vpu_EncStartOneFrame_return = RETCODE_SUCCESS;
RetCode vpu_EncStartOneFrame(EncHandle handle, EncParam * param) {
	return vpu_EncStartOneFrame_return;
}

RetCode vpu_EncGetOutputInfo_return = RETCODE_SUCCESS;
EncOutputInfo* vpu_EncGetOutputInfo_EncOutputInfo = NULL;

RetCode vpu_EncGetOutputInfo(EncHandle handle, EncOutputInfo * info) {
	if (vpu_EncGetOutputInfo_EncOutputInfo && info) {
		*info = *vpu_EncGetOutputInfo_EncOutputInfo;
	}
	return vpu_EncGetOutputInfo_return;
}




RetCode vpu_DecOpen_return = RETCODE_SUCCESS;
RetCode vpu_DecOpen(DecHandle *, DecOpenParam *) {
	return vpu_DecOpen_return;
}

RetCode vpu_DecClose_return = RETCODE_SUCCESS;
RetCode vpu_DecClose(DecHandle) {
	return vpu_DecClose_return;
}



RetCode vpu_EncGiveCommand_return = RETCODE_SUCCESS;
RetCode vpu_EncGiveCommand(EncHandle handle, CodecCommand cmd, void *parameter) {
	return vpu_EncGiveCommand_return;
}


RetCode vpu_DecGetBitstreamBuffer_return = RETCODE_SUCCESS;
Uint32* vpu_DecGetBitstreamBuffer_size = NULL;
PhysicalAddress* vpu_DecGetBitstreamBuffer_write = NULL;
PhysicalAddress* vpu_DecGetBitstreamBuffer_read = NULL;
RetCode vpu_DecGetBitstreamBuffer(DecHandle handle, PhysicalAddress * paRdPtr,
				  PhysicalAddress * paWrPtr, Uint32 * size) {
	if (vpu_DecGetBitstreamBuffer_size && size)	{
		*size = *vpu_DecGetBitstreamBuffer_size;
	}
	if (vpu_DecGetBitstreamBuffer_read && paRdPtr)	{
		*paRdPtr = *vpu_DecGetBitstreamBuffer_read;
	}
	if (vpu_DecGetBitstreamBuffer_write && paWrPtr)	{
		*paWrPtr = *vpu_DecGetBitstreamBuffer_write;
	}
	return vpu_DecGetBitstreamBuffer_return;
}

RetCode vpu_DecUpdateBitstreamBuffer_return = RETCODE_SUCCESS;
RetCode vpu_DecUpdateBitstreamBuffer(DecHandle handle, Uint32 size) {
	return vpu_DecUpdateBitstreamBuffer_return;
}

RetCode vpu_DecSetEscSeqInit_return = RETCODE_SUCCESS; 
RetCode vpu_DecSetEscSeqInit(DecHandle handle, int escape) {
	return vpu_DecSetEscSeqInit_return;
}

RetCode vpu_DecRegisterFrameBuffer_return = RETCODE_SUCCESS;
RetCode vpu_DecRegisterFrameBuffer(DecHandle handle,
				   FrameBuffer * bufArray, int num, int stride,
				   DecBufInfo * pBufInfo) {
	return vpu_DecRegisterFrameBuffer_return;
}


RetCode vpu_DecGetInitialInfo_return = RETCODE_SUCCESS;
DecInitialInfo* vpu_DecGetInitialInfo_DecInitialInfo = NULL;
RetCode vpu_DecGetInitialInfo(DecHandle handle, DecInitialInfo * info) {
	if (vpu_DecGetInitialInfo_DecInitialInfo && info) {
		*info = *vpu_DecGetInitialInfo_DecInitialInfo;
	}
	return vpu_DecGetInitialInfo_return;
}

RetCode vpu_DecClrDispFlag_return = RETCODE_SUCCESS;
RetCode vpu_DecClrDispFlag(DecHandle handle, int index) {
	return vpu_DecClrDispFlag_return;
}

RetCode vpu_DecGiveCommand_return = RETCODE_SUCCESS;
RetCode vpu_DecGiveCommand(DecHandle handle, CodecCommand cmd, void *parameter) {
	return vpu_DecGiveCommand_return;
}


RetCode vpu_DecStartOneFrame_return = RETCODE_SUCCESS;
RetCode vpu_DecStartOneFrame(DecHandle handle, DecParam * param) {
	return vpu_DecStartOneFrame_return;
}

RetCode vpu_DecGetOutputInfo_return = RETCODE_SUCCESS;
DecOutputInfo* vpu_DecGetOutputInfo_DecOutputInfo = NULL;

RetCode vpu_DecGetOutputInfo(DecHandle handle, DecOutputInfo * info) {
	if (vpu_DecGetOutputInfo_DecOutputInfo && info) {
		*info = *vpu_DecGetOutputInfo_DecOutputInfo;
	}
	
	return vpu_DecGetOutputInfo_return;
}




RetCode vpu_SWReset_return = RETCODE_SUCCESS;
RetCode vpu_SWReset(DecHandle handle, int index) {
	return vpu_SWReset_return;
}


int vpu_IsBusy_return = 0;
int vpu_IsBusy(void) {
	return vpu_IsBusy_return;
}

int vpu_WaitForInt_return = 0;
int vpu_WaitForInt(int timeout_in_ms) {
	return vpu_WaitForInt_return;
}




