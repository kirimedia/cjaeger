#ifndef CJAEGER_H
#define CJAEGER_H

#include <stddef.h>
#include <stdint.h>

#ifdef  __cplusplus
extern "C" {
#endif

void *cjaeger_tracer_create2(const char *service_name, const char *agent_addr, const char *collector_endpoint);
void *cjaeger_tracer_create(const char *service_name, const char *agent_addr);
void cjaeger_tracer_destroy(void *tracer);
void *cjaeger_span_start(void *tracer, void *parent, const char *operation_name);
void *cjaeger_span_start2(void *tracer, void *parent, const char *operation_name, size_t operation_name_len);
uint64_t cjaeger_span_id(void *span, uint64_t *trace_id_hi, uint64_t *trace_id_lo);
void *cjaeger_span_start_from(void *tracer, uint64_t trace_id_hi, uint64_t trace_id_lo, uint64_t parent_id, const char *operation_name, size_t operation_name_len);
void cjaeger_span_finish(void *span);
void cjaeger_span_log(void *span, const char *key, const char *value);
void cjaeger_span_log2(void *span, const char *key, const char *value, size_t value_len);
void cjaeger_span_log3(void *span, const char *key, size_t key_len, const char *value, size_t value_len);

# ifdef  __cplusplus
}
# endif

#endif
