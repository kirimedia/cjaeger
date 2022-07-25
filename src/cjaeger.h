#ifndef CJAEGER_H
#define CJAEGER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct cjaeger_tracer_headers_config {
    const char *jaeger_debug_header;
    const char *jaeger_baggage_header;
    const char *trace_context_header_name;
    const char *trace_baggage_header_prefix;
} cjaeger_tracer_headers_config;

#define CJAEGER_PROPAGATION_JAEGER  (1 << 0)
#define CJAEGER_PROPAGATION_W3C     (1 << 1)
#define CJAEGER_TRACEID_128BIT      (1 << 2)
#define CJAEGER_PROPAGATION_ANY     (CJAEGER_PROPAGATION_JAEGER | CJAEGER_PROPAGATION_W3C)

void *cjaeger_tracer_create3(const char *service_name, const char *agent_addr, const char *collector_endpoint, unsigned flags, const cjaeger_tracer_headers_config *headers_config);
void *cjaeger_tracer_create2(const char *service_name, const char *agent_addr, const char *collector_endpoint, const cjaeger_tracer_headers_config *headers_config);
void *cjaeger_tracer_create(const char *service_name, const char *agent_addr);
void cjaeger_tracer_destroy(void *tracer);

#define CJAEGER_SPAN_DEBUG          (1 << 0)
void *cjaeger_span_start(void *tracer, void *parent, const char *operation_name);
void *cjaeger_span_start2(void *tracer, void *parent, const char *operation_name, size_t operation_name_len);
void *cjaeger_span_start3(void *tracer, void *parent, const char *operation_name, size_t operation_name_len, unsigned flags);
uint64_t cjaeger_span_id(void *span, uint64_t *trace_id_hi, uint64_t *trace_id_lo);
bool cjaeger_span_debug(void *span);
void *cjaeger_span_start_from(void *tracer, uint64_t trace_id_hi, uint64_t trace_id_lo, uint64_t parent_id, const char *operation_name, size_t operation_name_len);
typedef int (*cjaeger_header_set)(const char *name, size_t name_len, const char *value, size_t value_len, void *arg);
typedef int (*cjaeger_header_trav_start)(void *arg);
typedef int (*cjaeger_header_trav_each)(const char **name, size_t *name_len, const char **value, size_t *value_len, void *arg);
int cjaeger_span_headers_set(void *span, cjaeger_header_set header_set, void *header_set_arg);
void *cjaeger_span_start_headers(void *tracer, cjaeger_header_trav_start trav_start, cjaeger_header_trav_each trav_each, void *trav_arg, const char *operation_name, size_t operation_name_len);
void cjaeger_span_finish(void *span);
void cjaeger_span_log(void *span, const char *key, const char *value);
void cjaeger_span_log2(void *span, const char *key, const char *value, size_t value_len);
void cjaeger_span_log3(void *span, const char *key, size_t key_len, const char *value, size_t value_len);
void cjaeger_span_logd(void *span, const char *key, size_t key_len, int64_t value);
void cjaeger_span_logu(void *span, const char *key, size_t key_len, uint64_t value);
void cjaeger_span_logfp(void *span, const char *key, size_t key_len, double value);
void cjaeger_span_logb(void *span, const char *key, size_t key_len, bool value);

# ifdef  __cplusplus
}
# endif

#endif
