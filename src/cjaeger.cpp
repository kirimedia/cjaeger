#include "cjaeger.h"
#include <jaegertracing/Tracer.h>

#define MAX_SPAN_LOG 10000
#define SPAN_LOG_SPLIT 60000

namespace cjaeger {

template<class T>
class Wrapper {
public:
	Wrapper(T& obj): _obj(std::move(obj)) {}
	T& get() {
		return _obj;
	}
private:
	T _obj;
};

typedef Wrapper<std::shared_ptr<opentracing::Tracer> > Tracer;
typedef Wrapper<std::unique_ptr<opentracing::Span> > Span;

extern "C" void *cjaeger_tracer_create2(const char *service_name, const char *agent_addr, const char *collector_endpoint) {

	try {
		auto config = jaegertracing::Config(
			false,
			false,
			jaegertracing::samplers::Config(
				"const",
				1,
				"",
				0,
				std::chrono::seconds(0)
			),
			jaegertracing::reporters::Config(
				0,
				std::chrono::seconds(0),
				true,
				agent_addr,
				collector_endpoint
			),
			jaegertracing::propagation::HeadersConfig(),
			jaegertracing::baggage::RestrictionsConfig(),
			service_name,
			std::vector<jaegertracing::Tag>(),
			jaegertracing::propagation::Format::JAEGER
		);

		auto tracer = jaegertracing::Tracer::make(config);

		return new Tracer(tracer);
	} catch (...) {
		return NULL;
	}
}

extern "C" void *cjaeger_tracer_create(const char *service_name, const char *agent_addr) {
	return cjaeger_tracer_create2(service_name, agent_addr, "");
}

extern "C" void cjaeger_tracer_destroy(void *tracer) {
	Tracer *_tracer = (Tracer*)tracer;
	try {
		_tracer->get()->Close();
	} catch (...) {
	}
	delete _tracer;
}

extern "C" void *cjaeger_span_start(void *tracer, void *parent, const char *operation_name) {
	return cjaeger_span_start2(tracer, parent, operation_name, strlen(operation_name));
}

extern "C" void *cjaeger_span_start2(void *tracer, void *parent, const char *operation_name, size_t operation_name_len) {
	try {
		opentracing::StartSpanOptions options;
		if (parent) {
			Span *_parent = (Span*)parent;
			opentracing::ChildOf(&_parent->get()->context()).Apply(options);
		}
		Tracer *_tracer = (Tracer*)tracer;
		auto span = _tracer->get()->StartSpanWithOptions(opentracing::string_view(operation_name, operation_name_len), options);
		return new Span(span);
	} catch (...) {
		return NULL;
	}
}

extern "C" void cjaeger_span_finish(void *span) {
	Span *_span = (Span*)span;
	try {
		_span->get()->Finish();
	} catch (...) {
	}
	delete _span;
}


extern "C" void cjaeger_span_log(void *span, const char *key, const char *value) {
	cjaeger_span_log2(span, key, value, strlen(value));
}

extern "C" void cjaeger_span_log2(void *span, const char *key, const char *value, size_t value_len) {
	cjaeger_span_log3(span, key, strlen(key), value, value_len);
}

extern "C" void cjaeger_span_log3(void *span, const char *key, size_t key_len, const char *value, size_t value_len) {
	Span *span_ = (Span*)span;
	try {
		auto key_ = opentracing::string_view(key, key_len);
		if (value_len <= MAX_SPAN_LOG)
			span_->get()->Log({{key_, std::string(value, value_len)}});
		else {
			auto tracer = &span_->get()->tracer();
			auto context = &span_->get()->context();
#define SPAN_LOG_NAME "_log_"
#define SPAN_LOG_NAME_LEN (sizeof(SPAN_LOG_NAME) - 1)
			char span_name[256];
			size_t span_name_len = key_len + SPAN_LOG_NAME_LEN;
			size_t pos = 0;

			if (span_name_len > sizeof(span_name)) {
				key_len = sizeof(span_name) - SPAN_LOG_NAME_LEN;
				span_name_len = key_len + SPAN_LOG_NAME_LEN;
			}

			memcpy(span_name, SPAN_LOG_NAME, SPAN_LOG_NAME_LEN);
			memcpy(span_name + SPAN_LOG_NAME_LEN, key, key_len);

			do {
				opentracing::StartSpanOptions options;
				opentracing::ChildOf(context).Apply(options);
				auto span = tracer->StartSpanWithOptions(opentracing::string_view(span_name, span_name_len), options);
				auto next = std::min(value_len, pos + SPAN_LOG_SPLIT);
				span->Log({{key_, std::string(value + pos, next - pos)}});
				pos = next;
			} while (pos < value_len);
		}
	} catch (...) {
	}
}

}

using namespace cjaeger;
