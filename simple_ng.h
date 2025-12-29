#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct SFieldbus Fieldbus;

typedef void (*process_callback_t)(void *userdata, const void *in, void *out);

Fieldbus *
fieldbus_alloc(void);
void fieldbus_free(Fieldbus *fieldbus);
void fieldbus_initialize(Fieldbus *fieldbus, const char *iface, process_callback_t process_callback, void *userdata);
bool fieldbus_start(Fieldbus *fieldbus);
void fieldbus_loop(Fieldbus *fieldbus, const volatile bool *keep_running);

#ifdef __cplusplus
}
#endif
