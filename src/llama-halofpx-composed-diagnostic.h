#pragma once

#include <cstring>
#include <sstream>
#include <string>

#include "ggml.h"

enum class llama_halofpx_preprepare_failure {
    mutable_preflight,
    expected_admission,
    prepared_admission,
    admission_verify,
    mutable_begin,
    resolved_census,
    scheduler_census_lookup,
    scheduler_census_disposition,
    mutable_register,
    mutable_exclude,
    mutable_prepare,
    graph_input_authority,
    admission_consume,
};

static inline const char * llama_halofpx_preprepare_failure_branch(
        llama_halofpx_preprepare_failure failure) {
    switch (failure) {
        case llama_halofpx_preprepare_failure::mutable_preflight:
            return "l44_mutable_preflight_refused";
        case llama_halofpx_preprepare_failure::expected_admission:
            return "l42_expected_admission_refused";
        case llama_halofpx_preprepare_failure::prepared_admission:
            return "l42_prepared_admission_refused";
        case llama_halofpx_preprepare_failure::admission_verify:
            return "l42_admission_verify_refused";
        case llama_halofpx_preprepare_failure::mutable_begin:
            return "l44_mutable_begin_refused";
        case llama_halofpx_preprepare_failure::resolved_census:
            return "l42_resolved_census_refused";
        case llama_halofpx_preprepare_failure::scheduler_census_lookup:
            return "l42_scheduler_census_lookup_refused";
        case llama_halofpx_preprepare_failure::scheduler_census_disposition:
            return "l42_scheduler_census_disposition_refused";
        case llama_halofpx_preprepare_failure::mutable_register:
            return "l44_mutable_register_refused";
        case llama_halofpx_preprepare_failure::mutable_exclude:
            return "l44_mutable_exclude_refused";
        case llama_halofpx_preprepare_failure::mutable_prepare:
            return "l44_mutable_prepare_refused";
        case llama_halofpx_preprepare_failure::graph_input_authority:
            return "l40_graph_input_authority_refused";
        case llama_halofpx_preprepare_failure::admission_consume:
            return "l42_admission_consume_refused";
    }
    return "l83_invalid_preprepare_failure";
}

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

static inline size_t llama_halofpx_copy_composed_failure_result(
        const std::string & retained,
        char * dst,
        size_t capacity) {
    const std::string & value =
        llama_halofpx_composed_failure_result(retained);
    const size_t required = value.size() + 1;
    if (dst != nullptr && capacity >= required) {
        memcpy(dst, value.c_str(), required);
    }
    return required;
}
