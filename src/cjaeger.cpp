#include "cjaeger.h"
#include <jaegertracing/Tracer.h>

#define MAX_SPAN_LOG 10000
#define SPAN_LOG_SPLIT 60000
#define SPAN_CHUNK_MAX 50000

namespace cjaeger {

template<class T>
class Wrapper {
public:
	Wrapper(T&& obj): _obj(std::move(obj)) {}
	T& get() {
		return _obj;
	}
	T& set(T&& obj) {
		return _obj = std::move(obj);
	}
private:
	T _obj;
};

typedef Wrapper<std::shared_ptr<opentracing::Tracer> > Tracer;
typedef Wrapper<std::unique_ptr<opentracing::Span> > Span;

class ChunkedSpan: public Span {
public:
	ChunkedSpan(std::unique_ptr<opentracing::Span>&& obj): Span(std::move(obj)), stored(0) {
		_context = static_cast<jaegertracing::Span&>(*get()).context();
	}
	const jaegertracing::SpanContext& context() {
		return _context;
	}
	void consume(uint64_t amount) {
		static const opentracing::string_view span_chunk_key = "_continue_";

		if ((stored += amount) <= SPAN_CHUNK_MAX)
			return;
		auto tracer = &get()->tracer();
		opentracing::StartSpanOptions options;
		opentracing::ChildOf(&_context).Apply(options);
		if (orig_obj.get() == nullptr)
			orig_obj.swap(get());
		set(tracer->StartSpanWithOptions(span_chunk_key, options));
		stored = amount;
	}
	void Finish() {
		get()->Finish();
		if (orig_obj.get() != nullptr)
			orig_obj->Finish();
	}
private:
	std::unique_ptr<opentracing::Span> orig_obj;
	jaegertracing::SpanContext _context;
	uint64_t stored;
};

class CJaegerHttpReader: public opentracing::HTTPHeadersReader {
public:
	using ForeachKeyFn = std::function<opentracing::expected<void>(opentracing::string_view key, opentracing::string_view value)>;
	CJaegerHttpReader(cjaeger_header_trav_start trav_start, cjaeger_header_trav_each trav_each, void *trav_arg):
		_trav_start(trav_start),
		_trav_each(trav_each),
		_trav_arg(trav_arg)
	{}

	opentracing::expected<void> ForeachKey(ForeachKeyFn fn) const {
		if (_trav_each == NULL)
			return opentracing::make_expected();
		if (_trav_start != NULL && _trav_start(_trav_arg) != 0)
			return opentracing::make_expected();
		const char *name, *value;
		size_t name_len, value_len;
		while (!_trav_each(&name, &name_len, &value, &value_len, _trav_arg)) {
			auto rc = fn(opentracing::string_view(name, name_len), opentracing::string_view(value, value_len));
			if (!rc)
				return rc;
		}
		return opentracing::make_expected();
	}

private:
	cjaeger_header_trav_start _trav_start;
	cjaeger_header_trav_each _trav_each;
	void *_trav_arg;
};

class CJaegerHttpWriter: public opentracing::HTTPHeadersWriter {
public:
	CJaegerHttpWriter(cjaeger_header_set header_set, void *header_set_arg):
		_header_set(header_set),
		_header_set_arg(header_set_arg)
	{}

	opentracing::expected<void> Set(opentracing::string_view key, opentracing::string_view value) const {
		if (_header_set == NULL)
			return opentracing::make_expected();
		if (_header_set(key.data(), key.length(), value.data(), value.length(), _header_set_arg) < 0)
			throw std::runtime_error("Cannot set header");
		return opentracing::make_expected();
	}

private:
	cjaeger_header_set _header_set;
	void *_header_set_arg;
};

extern "C" void *cjaeger_tracer_create3(const char *service_name, const char *agent_addr, const char *collector_endpoint, unsigned flags, const cjaeger_tracer_headers_config *headers_config) {

	try {
		jaegertracing::propagation::Format propagationFormat = jaegertracing::propagation::Format::JAEGER;

		if (!!(flags & CJAEGER_PROPAGATION_ANY)) {
			switch (flags & CJAEGER_PROPAGATION_ANY) {
			case CJAEGER_PROPAGATION_JAEGER: propagationFormat = jaegertracing::propagation::Format::JAEGER; break;
			case CJAEGER_PROPAGATION_W3C: propagationFormat = jaegertracing::propagation::Format::W3C; break;
			default:
				return NULL;
			}
		}

		auto config = jaegertracing::Config(
			false,
			!!(flags & CJAEGER_TRACEID_128BIT),
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
			jaegertracing::propagation::HeadersConfig(
				headers_config ? headers_config->jaeger_debug_header : "",
				headers_config ? headers_config->jaeger_baggage_header : "",
				headers_config ? headers_config->trace_context_header_name : "",
				headers_config ? headers_config->trace_baggage_header_prefix : ""),
			jaegertracing::baggage::RestrictionsConfig(),
			service_name,
			std::vector<jaegertracing::Tag>(),
			propagationFormat
		);

		return new Tracer(jaegertracing::Tracer::make(config));
	} catch (...) {
		return NULL;
	}
}

extern "C" void *cjaeger_tracer_create2(const char *service_name, const char *agent_addr, const char *collector_endpoint, const cjaeger_tracer_headers_config *headers_config) {
	return cjaeger_tracer_create3(service_name, agent_addr, collector_endpoint, CJAEGER_PROPAGATION_JAEGER, headers_config);
}

extern "C" void *cjaeger_tracer_create(const char *service_name, const char *agent_addr) {
	return cjaeger_tracer_create2(service_name, agent_addr, "", NULL);
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
	return cjaeger_span_start3(tracer, parent, operation_name, operation_name_len, 0);
}

extern "C" void *cjaeger_span_start3(void *tracer, void *parent, const char *operation_name, size_t operation_name_len, unsigned flags) {
	try {
		opentracing::StartSpanOptions options;
		static jaegertracing::SpanContext debug_root_context(jaegertracing::TraceID(0, 0), 0, 0, 0, jaegertracing::SpanContext::StrMap(), "1");

		if (parent) {
			auto _parent = (ChunkedSpan*)parent;
			opentracing::ChildOf(&_parent->context()).Apply(options);
		} else if (!!(flags & CJAEGER_SPAN_DEBUG)) {
			opentracing::ChildOf(&debug_root_context).Apply(options);
		}
		Tracer *_tracer = (Tracer*)tracer;
		return new ChunkedSpan(_tracer->get()->StartSpanWithOptions(opentracing::string_view(operation_name, operation_name_len), options));
	} catch (...) {
		return NULL;
	}
}

extern "C" uint64_t cjaeger_span_id(void *span, uint64_t *trace_id_hi, uint64_t *trace_id_lo) {
	auto _span = (ChunkedSpan*)span;
	uint64_t span_id;

	try {
		auto context = _span->context();
		span_id = context.spanID();
		const jaegertracing::TraceID &traceID = context.traceID();

		if (trace_id_hi != NULL)
			*trace_id_hi = traceID.high();
		if (trace_id_lo != NULL)
			*trace_id_lo = traceID.low();
	} catch (...) {
		return 0;
	}
	return span_id;
}

extern "C" bool cjaeger_span_debug(void *span) {
	auto _span = (ChunkedSpan*)span;

	try {
		auto context = _span->context();
		return context.isDebug();
	} catch (...) {
		return false;
	}
}

extern "C" void *cjaeger_span_start_from(void *tracer, uint64_t trace_id_hi, uint64_t trace_id_lo, uint64_t parent_id, const char *operation_name, size_t operation_name_len) {
	try {
		// trace_id_hi is zero unless 128-bit traces are enabled
		if (!trace_id_lo)
			return NULL;

		if (!parent_id)
			parent_id = trace_id_lo;

		unsigned char flags = static_cast<unsigned char>(jaegertracing::SpanContext::Flag::kSampled);
		jaegertracing::SpanContext context(jaegertracing::TraceID(trace_id_hi, trace_id_lo), parent_id, 0, flags, jaegertracing::SpanContext::StrMap());
		opentracing::StartSpanOptions options;
		opentracing::ChildOf(&context).Apply(options);
		Tracer *_tracer = (Tracer*)tracer;
		return new ChunkedSpan(_tracer->get()->StartSpanWithOptions(opentracing::string_view(operation_name, operation_name_len), options));
	} catch (...) {
		return NULL;
	}
}

extern "C" int cjaeger_span_headers_set(void *span, cjaeger_header_set header_set, void *header_set_arg) {
	auto span_ = (ChunkedSpan*)span;
	try {
		auto tracer = &span_->get()->tracer();
		CJaegerHttpWriter writer(header_set, header_set_arg);

		auto success = tracer->Inject(span_->context(), writer);

		return success ? 0 : -1;
	} catch (...) {
		return -1;
	}
}

extern "C" void *cjaeger_span_start_headers(void *tracer, cjaeger_header_trav_start trav_start, cjaeger_header_trav_each trav_each, void *trav_arg, const char *operation_name, size_t operation_name_len) {
	Tracer *tracer_ = (Tracer*)tracer;
	try {
		CJaegerHttpReader reader(trav_start, trav_each, trav_arg);

		auto context_may_be = tracer_->get()->Extract(reader);
		if (!context_may_be)
			return NULL;
		opentracing::StartSpanOptions options;
		opentracing::ChildOf(context_may_be->get()).Apply(options);
		return new ChunkedSpan(tracer_->get()->StartSpanWithOptions(opentracing::string_view(operation_name, operation_name_len), options));
	} catch (...) {
		return NULL;
	}
}

extern "C" void cjaeger_span_finish(void *span) {
	auto _span = (ChunkedSpan*)span;
	try {
		_span->Finish();
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
	auto span_ = (ChunkedSpan*)span;
	try {
		auto key_ = opentracing::string_view(key, key_len);
		if (value_len <= MAX_SPAN_LOG) {
			span_->consume(key_len + value_len + 8);
			span_->get()->Log({{key_, std::string(value, value_len)}});
		} else {
			auto tracer = &span_->get()->tracer();
			auto context = &span_->context();
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
