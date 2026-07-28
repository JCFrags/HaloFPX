#include "llama-halofpx-composed-diagnostic.h"

#include <cstdio>
#include <string>
#include <utility>
#include <vector>

struct test_case {
    llama_halofpx_preprepare_failure failure;
    const char * branch;
    const char * detail;
};

int main() {
    const std::vector<test_case> cases = {
        {llama_halofpx_preprepare_failure::mutable_preflight,
         "l44_mutable_preflight_refused",
         "|backend_ordinal=1|admitted_backend_count=1"},
        {llama_halofpx_preprepare_failure::expected_admission,
         "l42_expected_admission_refused",
         "|backend_ordinal=1|admitted_backend_count=1"},
        {llama_halofpx_preprepare_failure::prepared_admission,
         "l42_prepared_admission_refused",
         "|backend_ordinal=1|admitted_backend_count=1"},
        {llama_halofpx_preprepare_failure::admission_verify,
         "l42_admission_verify_refused",
         "|backend_ordinal=1|admitted_backend_count=1"},
        {llama_halofpx_preprepare_failure::mutable_begin,
         "l44_mutable_begin_refused",
         "|backend_ordinal=1|admitted_backend_count=1"},
        {llama_halofpx_preprepare_failure::resolved_census,
         "l42_resolved_census_refused", ""},
        {llama_halofpx_preprepare_failure::scheduler_census_lookup,
         "l42_scheduler_census_lookup_refused",
         "|backend_ordinal=1|census_index=7"},
        {llama_halofpx_preprepare_failure::scheduler_census_disposition,
         "l42_scheduler_census_disposition_refused",
         "|backend_ordinal=1|census_index=7|disposition=9|role=4|role_ordinal=12"},
        {llama_halofpx_preprepare_failure::mutable_register,
         "l44_mutable_register_refused",
         "|backend_ordinal=1|census_index=7|disposition=1|role=4|role_ordinal=12"},
        {llama_halofpx_preprepare_failure::mutable_exclude,
         "l44_mutable_exclude_refused",
         "|backend_ordinal=1|census_index=7|disposition=2|role=4|role_ordinal=12"},
        {llama_halofpx_preprepare_failure::mutable_prepare,
         "l44_mutable_prepare_refused",
         "|backend_ordinal=1|session_index=0|result_status=101"
         "|mutation_count=3|census_count=8|set_count=2"
         "|set_hash_hit_count=1|set_hash_miss_count=1"},
        {llama_halofpx_preprepare_failure::graph_input_authority,
         "l40_graph_input_authority_refused", ""},
        {llama_halofpx_preprepare_failure::admission_consume,
         "l42_admission_consume_refused",
         "|backend_ordinal=1|admission_index=0"},
    };

    for (const auto & item : cases) {
        const char * branch =
            llama_halofpx_preprepare_failure_branch(item.failure);
        if (std::string(branch) != item.branch) {
            return 1;
        }
        std::string retained;
        std::string emitted;
        bool abort_called = false;
        llama_halofpx_record_composed_failure_and_abort(
            branch, item.detail, 41, true, GGML_STATUS_FAILED, retained,
            [&](const std::string & value) { emitted = value; },
            [&]() {
                abort_called = true;
                retained.clear();
                return true;
            });
        const std::string & authority_result =
            llama_halofpx_composed_failure_result(retained);
        const std::string expected =
            "version=1|status=failed|branch=" + std::string(item.branch) +
            item.detail +
            "|execution_sequence=41|pending=1|ggml_status=" +
            std::to_string(static_cast<int>(GGML_STATUS_FAILED));
        std::vector<char> copied(expected.size() + 1);
        const size_t required = llama_halofpx_copy_composed_failure_result(
            retained, copied.data(), copied.size());
        if (!abort_called || emitted != expected || retained != expected ||
            authority_result != expected || required != copied.size() ||
            std::string(copied.data()) != expected) {
            return 2;
        }
        std::puts(emitted.c_str());
    }
    return 0;
}
