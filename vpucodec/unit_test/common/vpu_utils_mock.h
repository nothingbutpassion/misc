#include "vpu_utils.h"

void vpu_print_log(const char* priority, const char* tag, const char *fmt, ...) {}
void vpu_set_error(const char *fmt, ...) {}

const char* vpu_get_error_return = "";
const char* vpu_get_error() {
	return vpu_get_error_return;
}

