#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef void (*process_callback_t)(const void *in, void *out);

int plc_thread(const char *iface, process_callback_t process_callback, volatile bool *keep_running);

#ifdef __cplusplus
}
#endif
