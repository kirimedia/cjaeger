#include "cjaeger.h"
#include <jaegertracing/Tracer.h>

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
	try {
		opentracing::StartSpanOptions options;
		if (parent) {
			Span *_parent = (Span*)parent;
			opentracing::ChildOf(&_parent->get()->context()).Apply(options);
		}
		Tracer *_tracer = (Tracer*)tracer;
		auto span = _tracer->get()->StartSpanWithOptions(operation_name, options);
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
	Span *_span = (Span*)span;
	try {
		_span->get()->Log({{opentracing::string_view(key, key_len), std::string(value, value_len)}});
	} catch (...) {
	}
}

}

using namespace cjaeger;
