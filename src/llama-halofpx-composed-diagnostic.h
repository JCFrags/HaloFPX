#pragma once

#include <sstream>
#include <string>

#include "ggml.h"

// One formatter owns both the retained result and the early warmup diagnostic.
// Callers supply only closed, source-owned branch/detail strings.
static inline std::string llama_halofpx_composed_failure_text(
        const std::string & branch,
        const std::string & detail,
        uint64_t execution_sequence,
        bool pending,
        ggml_status status) {
    std::ostringstream out;
    out << "version=1|status=failed|branch=" << branch << detail
        << "|execution_sequence=" << execution_sequence
        << "|pending=" << (pending ? 1 : 0)
        << "|ggml_status=" << static_cast<int>(status);
    return out.str();
}

template<class Emit>
static inline void llama_halofpx_record_composed_failure(
        const std::string & branch,
        const std::string & detail,
        uint64_t execution_sequence,
        bool pending,
        ggml_status status,
        std::string & retained,
        Emit && emit) {
    retained = llama_halofpx_composed_failure_text(
        branch, detail, execution_sequence, pending, status);
    emit(retained);
}

template<class Emit, class Abort>
static inline bool llama_halofpx_record_composed_failure_and_abort(
        const std::string & branch,
        const std::string & detail,
        uint64_t execution_sequence,
        bool pending,
        ggml_status status,
        std::string & retained,
        Emit && emit,
        Abort && abort) {
    llama_halofpx_record_composed_failure(
        branch, detail, execution_sequence, pending, status, retained,
        static_cast<Emit &&>(emit));
    const std::string evidence = retained;
    const bool aborted = abort();
    retained = evidence;
    return aborted;
}

static inline const std::string & llama_halofpx_composed_failure_result(
        const std::string & retained) {
    return retained;
}
