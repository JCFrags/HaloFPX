# dependencies

find_package(Threads REQUIRED)

# third-party

# ...

# flags

llama_add_compile_flags()

# tools

if (EMSCRIPTEN)
else()
    add_subdirectory(batched-bench)
    add_subdirectory(gguf-split)
    add_subdirectory(imatrix)
    add_subdirectory(llama-bench)
    add_subdirectory(completion)
    add_subdirectory(perplexity)
    add_subdirectory(quantize)
    if (LLAMA_BUILD_SERVER)
        add_subdirectory(cli)
        add_subdirectory(server)
    endif()
    add_subdirectory(tokenize)
    add_subdirectory(parser)
    add_subdirectory(tts)
    add_subdirectory(mtmd)
    if (GGML_RPC)
        add_subdirectory(rpc)
    endif()
    if (NOT GGML_BACKEND_DL)
        # these examples use the backends directly and cannot be built with dynamic loading
        add_subdirectory(cvector-generator)
        add_subdirectory(export-lora)
    endif()
    add_subdirectory(fit-params)
    add_subdirectory(results)
endif()

# Finalize the L05o link graph only after every tool target exists. This avoids
# treating later project targets such as mtmd as opaque raw linker inputs.
if(BUILD_TESTING AND TARGET halofpx-context-store-registry-lab-wire AND
        TARGET halofpx-context-store-registry-lab-read-only)
    include(${CMAKE_SOURCE_DIR}/cmake/halofpx-registry-lab-finalize-graph.cmake)
    halofpx_registry_lab_write_reachable_graph(
        "${CMAKE_BINARY_DIR}/halofpx-registry-lab-link-graph.txt"
        halofpx-context-store-registry-lab-wire
        halofpx-context-store-registry-lab-read-only
        server-context llama-server llama-cli)
endif()
