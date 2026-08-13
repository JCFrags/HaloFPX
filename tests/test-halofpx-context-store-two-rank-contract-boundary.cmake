if(NOT HALOFPX_CONTEXT_STORE_TWO_RANK_CONTRACT)
    message(FATAL_ERROR "two-rank contract boundary test requires its explicit test-only gate")
endif()

file(READ "${HALOFPX_SOURCE_DIR}/tools/server/CMakeLists.txt" server_cmake)
file(READ "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-two-rank-contract.h" header)
file(READ "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-two-rank-contract.cpp" source)
file(READ "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-state-transformer-v1.cpp" world1_state)
file(READ "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-v1-transformer-codec.cpp" world1_codec)
file(READ "${HALOFPX_SOURCE_DIR}/tools/server/server-context.cpp" server_context)

foreach(required
    "option(HALOFPX_CONTEXT_STORE_TWO_RANK_CONTRACT"
    "Build the isolated test-only HaloKV two-rank coordinator contract"
    "add_library(halofpx-context-store-two-rank-contract STATIC EXCLUDE_FROM_ALL"
    "constexpr uint32_t context_store_two_rank_world_size = 2"
    "constexpr size_t context_store_two_rank_count = 2"
    "expected_ranks = { 0, 1 }"
    "context_store_two_rank_commit_outcome::outcome_unknown"
    "result.recreation_required = true"
    "providers[index]->capture"
    "providers[index]->stage"
    "providers[1]->commit_apply"
    "providers[0]->commit_apply"
    "context_store_two_rank_status::duplicate_receipt"
    "context_store_two_rank_status::attempt_replayed")
    string(FIND "${server_cmake}${header}${source}" "${required}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "missing two-rank contract boundary token: ${required}")
    endif()
endforeach()

string(REGEX MATCH
    "option\\(HALOFPX_CONTEXT_STORE_TWO_RANK_CONTRACT[ \t\r\n]+\"Build the isolated test-only HaloKV two-rank coordinator contract\"[ \t\r\n]+OFF\\)"
    gate_default_off "${server_cmake}")
if(gate_default_off STREQUAL "")
    message(FATAL_ERROR "two-rank contract gate must remain default OFF")
endif()

# The stable world-1 server/cache route remains a separate admitted profile.
foreach(required
    "profile.world_size == 1"
    "profile.rank == 0")
    string(FIND "${world1_state}" "${required}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "world-1 transformer admission changed: ${required}")
    endif()
endforeach()
string(FIND "${world1_codec}" "candidate.world_size() != 1" codec_world1)
if(codec_world1 EQUAL -1)
    message(FATAL_ERROR "world-1 transformer codec refusal boundary changed")
endif()
foreach(required
    "profile.world_size = 1"
    "profile.rank = 0"
    "exact.world_size = 1"
    "exact.rank = 0")
    string(FIND "${server_context}" "${required}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "world-1 server route changed: ${required}")
    endif()
endforeach()

# The new target must remain absent from product/runtime link statements.
foreach(forbidden_link
    "target_link_libraries(server-context PRIVATE halofpx-context-store-two-rank-contract"
    "target_link_libraries(llama-server PRIVATE halofpx-context-store-two-rank-contract"
    "target_link_libraries(llama-common PRIVATE halofpx-context-store-two-rank-contract")
    string(FIND "${server_cmake}" "${forbidden_link}" found)
    if(NOT found EQUAL -1)
        message(FATAL_ERROR "two-rank contract linked into a product target: ${forbidden_link}")
    endif()
endforeach()

foreach(forbidden
    "llama_context"
    "ggml_"
    "server-context"
    "RPC_CMD"
    "filesystem"
    "fstream"
    "socket"
    "thread"
    "sleep"
    "open("
    "rename("
    "unlink("
    "fsync("
    "CachyLlama")
    string(FIND "${header}${source}" "${forbidden}" found)
    if(NOT found EQUAL -1)
        message(FATAL_ERROR "forbidden product/I/O/donor token in test-only contract: ${forbidden}")
    endif()
endforeach()

string(FIND "${source}" "providers[index]->stage" stage_position)
string(FIND "${source}" "providers[1]->commit_apply" remote_commit_position)
string(FIND "${source}" "providers[0]->commit_apply" local_apply_position)
if(stage_position EQUAL -1 OR remote_commit_position EQUAL -1 OR
   local_apply_position EQUAL -1 OR
   NOT stage_position LESS remote_commit_position OR
   NOT remote_commit_position LESS local_apply_position)
    message(FATAL_ERROR "restore ordering is not stage-both then remote-commit then local-apply")
endif()
