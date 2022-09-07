#ifndef VPU_UTILS_H
#define VPU_UTILS_H


#define LOG_DEBUG       1
#define LOG_INFO        2
#define LOG_WARN        3
#define LOG_ERROR       4

#ifndef LOG_LEVEL       
#define LOG_LEVEL       LOG_DEBUG
#endif

#if LOG_ERROR >= LOG_LEVEL
#define LOGE(tag, ...)   do { vpu_print_log("ERROR", tag, __VA_ARGS__); vpu_set_error(__VA_ARGS__); } while(0)                
#else
#define LOGE(tag, ...)		 vpu_set_error(__VA_ARGS__)
#endif

#if LOG_WARN >= LOG_LEVEL
#define LOGW(tag, ...)   vpu_print_log("WARN ", tag, __VA_ARGS__)                  
#else
#define LOGW(...)
#endif 

#if LOG_INFO >= LOG_LEVEL
#define LOGI(tag, ...)   vpu_print_log("INFO ", tag, __VA_ARGS__)                  
#else
#define LOGI(...)
#endif 

#if LOG_DEBUG >= LOG_LEVEL
#define LOGD(tag, ...)   vpu_print_log("DEBUG", tag,  __VA_ARGS__)                  
#else
#define LOGD(...)
#endif


void vpu_print_log(const char* priority, const char* tag, const char *fmt, ...);
void vpu_set_error(const char *fmt, ...);
const char* vpu_get_error();



#endif	// VPU_UTILS_H 
