#include "llama-halofpx-composed-diagnostic.h"

#include <cstdio>
#include <string>
#include <vector>

int main() {
    const std::vector<std::pair<std::string, std::string>> cases = {
        {"l44_mutable_commit_refused", ""},
        {"l42_scheduler_finalize", ""},
        {"l40_graph_result_reconcile",
         "|subreason=receipt_invalid_argument|expected_parent_uid=26"
         "|expected_split_uid=27|actual_receipt_uid=27"
         "|expected_execution_sequence=1|actual_execution_sequence=1"
         "|backend_ordinal=0|receipt_reason=4"},
        {"l44_session_finalize", ""},
        {"rpc_execution_disarm", ""},
    };
    const std::vector<std::string> receipt_reasons = {
        "receipt_invalid_argument", "receipt_backend_not_rpc",
        "receipt_context_missing", "receipt_status_not_executed",
        "receipt_graph_uid_zero", "receipt_execution_sequence_zero",
        "receipt_reason_out_of_range", "graph_uid_mismatch",
        "execution_sequence_mismatch",
    };
    for (const auto & item : cases) {
        std::string retained;
        std::string emitted;
        bool abort_called = false;
        llama_halofpx_record_composed_failure_and_abort(
            item.first, item.second, 1, true, GGML_STATUS_FAILED, retained,
            [&](const std::string & value) { emitted = value; },
            [&]() {
                abort_called = true;
                retained.clear();
                return true;
            });
        const std::string & after_abort =
            llama_halofpx_composed_failure_result(retained);
        if (emitted != retained || after_abort != retained ||
            !abort_called ||
            retained.find("|execution_sequence=1|pending=1|ggml_status=") ==
                std::string::npos) {
            return 1;
        }
        std::puts(emitted.c_str());
    }
    for (const auto & reason : receipt_reasons) {
        std::string retained;
        std::string emitted;
        const std::string detail =
            "|subreason=" + reason +
            "|expected_parent_uid=26|expected_split_uid=27"
            "|actual_receipt_uid=27|expected_execution_sequence=1"
            "|actual_execution_sequence=1|backend_ordinal=0|receipt_reason=4";
        llama_halofpx_record_composed_failure_and_abort(
            "l40_graph_result_reconcile", detail, 1, true,
            GGML_STATUS_FAILED, retained,
            [&](const std::string & value) { emitted = value; },
            [&]() {
                retained.clear();
                return true;
            });
        const std::string & authority_result =
            llama_halofpx_composed_failure_result(retained);
        if (emitted != authority_result ||
            authority_result.find("|subreason=" + reason + "|") ==
                std::string::npos) {
            return 2;
        }
    }
    return 0;
}
