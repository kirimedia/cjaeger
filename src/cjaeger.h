#ifndef CJAEGER_H
#define CJAEGER_H

#include <stddef.h>

#ifdef  __cplusplus
extern "C" {
#endif

void *cjaeger_tracer_create(const char *service_name, const char *agent_addr);
void cjaeger_tracer_destroy(void *tracer);
void *cjaeger_span_start(void *tracer, void *parent, const char *operation_name);
void cjaeger_span_finish(void *span);
void cjaeger_span_log(void *span, const char *key, const char *value);
void cjaeger_span_log2(void *span, const char *key, const char *value, size_t value_len);

# ifdef  __cplusplus
}
# endif

#endif
