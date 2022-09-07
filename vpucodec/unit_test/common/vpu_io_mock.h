extern "C" {
#include "vpu_io.h"
}

int IOGetPhyMem_return;
int IOGetPhyMem(vpu_mem_desc * buff) {
	return IOGetPhyMem_return;
}

int IOFreePhyMem_return = 0;
int IOFreePhyMem(vpu_mem_desc * buff) {
	return IOFreePhyMem_return;
}

int IOGetVirtMem_return = 0;
int IOGetVirtMem(vpu_mem_desc * buff) {
	return IOGetVirtMem_return;
}

int IOFreeVirtMem_return = 0;
int IOFreeVirtMem(vpu_mem_desc * buff) {
	return IOFreeVirtMem_return;
}

