#ifdef NDEBUG
#undef NDEBUG
#endif

#include "halofpx-context-store-registry-lab-linux-initializer-internal.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <linux/memfd.h>
#include <sys/syscall.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

using halofpx::registry_lab::linux_initializer::sealed_input_audit;
using halofpx::registry_lab::linux_initializer::sealed_input_request;
using halofpx::registry_lab::linux_initializer::sealed_input_status;

enum class variant : std::uint8_t {
    success,
    wrong_digest,
    wrong_key,
    wrong_generation,
    credential_name,
    predecessor_name,
    credential_unsealed,
    predecessor_unsealed,
    credential_no_cloexec,
    predecessor_no_cloexec,
    credential_alias,
    predecessor_alias,
    predecessor_bit,
    credential_magic,
    zero_secret,
    missing_credential,
    missing_predecessor,
    credential_too_short,
    credential_oversized,
    predecessor_empty,
    predecessor_oversized,
    credential_extra_seal,
    predecessor_extra_seal,
    credential_non_shmem,
    locked_storage_unavailable,
    external_mutation,
};

std::uint8_t nibble(char c) {
    return static_cast<std::uint8_t>(c <= '9' ? c - '0' : c - 'a' + 10);
}

std::vector<std::uint8_t> hex_bytes(const std::string & text) {
    assert(text.size() % 2 == 0);
    std::vector<std::uint8_t> output(text.size() / 2);
    for (std::size_t i = 0; i < output.size(); ++i) {
        output[i] = static_cast<std::uint8_t>(nibble(text[2 * i]) << 4 |
                                              nibble(text[2 * i + 1]));
    }
    return output;
}

std::string value(const std::string & json, const std::string & key) {
    auto position = json.find("\"" + key + "\"");
    assert(position != std::string::npos);
    position = json.find(':', position) + 1;
    position = json.find('"', position) + 1;
    const auto end = json.find('"', position);
    return json.substr(position, end - position);
}

void write_all(int fd, const std::vector<std::uint8_t> & bytes) {
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const ssize_t count = ::write(fd, bytes.data() + offset, bytes.size() - offset);
        assert(count > 0);
        offset += static_cast<std::size_t>(count);
    }
}

int install_memfd(const char * name, const std::vector<std::uint8_t> & bytes,
                  int target, bool seal, bool cloexec, bool extra_seal = false) {
    int fd = static_cast<int>(::syscall(SYS_memfd_create, name,
                                       MFD_ALLOW_SEALING | MFD_CLOEXEC));
    assert(fd >= 0);
    write_all(fd, bytes);
    if (seal) {
        int seals = F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE;
#ifdef F_SEAL_FUTURE_WRITE
        if (extra_seal) {
            seals |= F_SEAL_FUTURE_WRITE;
        }
#else
        assert(!extra_seal);
#endif
        assert(::fcntl(fd, F_ADD_SEALS, seals) == 0);
    }
    if (fd != target) {
        assert(::dup3(fd, target, O_CLOEXEC) == target);
        assert(::close(fd) == 0);
    }
    assert(::fcntl(target, F_SETFD, cloexec ? FD_CLOEXEC : 0) == 0);
    return target;
}

void install_non_shmem(const std::vector<std::uint8_t> & bytes, int target) {
    char path[] = "/var/tmp/halofpx-m63-input-XXXXXX";
    int fd = ::mkstemp(path);
    assert(fd >= 0);
    assert(::unlink(path) == 0);
    write_all(fd, bytes);
    if (fd != target) {
        assert(::dup3(fd, target, O_CLOEXEC) == target);
        assert(::close(fd) == 0);
    }
    assert(::fcntl(target, F_SETFD, FD_CLOEXEC) == 0);
}

sealed_input_request make_request(const std::vector<std::uint8_t> & package,
                                  const std::vector<std::uint8_t> & digest) {
    sealed_input_request request;
    const std::size_t key_size = static_cast<std::size_t>(package[16] << 8 | package[17]);
    assert(key_size > 0 && key_size <= request.expected_key_id.bytes.size());
    request.expected_key_id.size = static_cast<std::uint8_t>(key_size);
    std::copy_n(package.data() + 18, key_size, request.expected_key_id.bytes.begin());
    std::size_t offset = 18 + key_size;
    for (unsigned i = 0; i < 8; ++i) {
        request.expected_key_generation =
            (request.expected_key_generation << 8) | package[offset + i];
    }
    assert(digest.size() == request.expected_predecessor_digest.size());
    std::copy(digest.begin(), digest.end(), request.expected_predecessor_digest.begin());
    return request;
}

void assert_cleanup(const sealed_input_audit & audit) {
    assert(audit.descriptors_closed);
    assert(audit.secure_storage_wiped);
    assert(audit.secure_storage_unlocked);
    assert(audit.secure_storage_unmapped);
    assert(audit.signal_mask_restored);
    assert(!audit.root_or_fixture_accessed);
    assert(audit.root_or_fixture_syscall_count == 0);
    errno = 0;
    assert(::fcntl(3, F_GETFD) == -1 && errno == EBADF);
    errno = 0;
    assert(::fcntl(4, F_GETFD) == -1 && errno == EBADF);
}

void run_child(variant selected, const std::vector<std::uint8_t> & request_package,
               std::vector<std::uint8_t> package,
               std::vector<std::uint8_t> predecessor,
               const std::vector<std::uint8_t> & digest) {
    auto request = make_request(request_package, digest);
    if (selected == variant::wrong_digest) {
        request.expected_predecessor_digest[0] ^= 1;
    } else if (selected == variant::wrong_key) {
        request.expected_key_id.bytes[0] ^= 1;
    } else if (selected == variant::wrong_generation) {
        ++request.expected_key_generation;
    } else if (selected == variant::predecessor_bit) {
        predecessor[predecessor.size() / 2] ^= 1;
    } else if (selected == variant::credential_magic) {
        package[0] ^= 1;
    } else if (selected == variant::zero_secret) {
        std::fill(package.end() - 32, package.end(), 0);
    } else if (selected == variant::credential_too_short) {
        package.resize(60);
    } else if (selected == variant::credential_oversized) {
        package.resize(181);
    } else if (selected == variant::predecessor_empty) {
        predecessor.clear();
    } else if (selected == variant::predecessor_oversized) {
        predecessor.resize(1025);
    }

    if (selected != variant::missing_credential) {
        if (selected == variant::credential_non_shmem) {
            install_non_shmem(package, 3);
        } else {
            install_memfd(selected == variant::credential_name ? "wrong-credential" :
                          "halofpx-registry-lab-credential", package, 3,
                          selected != variant::credential_unsealed,
                          selected != variant::credential_no_cloexec,
                          selected == variant::credential_extra_seal);
        }
    } else {
        ::close(3);
    }
    if (selected != variant::missing_predecessor) {
        install_memfd(selected == variant::predecessor_name ? "wrong-predecessor" :
                      "halofpx-registry-lab-predecessor", predecessor, 4,
                      selected != variant::predecessor_unsealed,
                      selected != variant::predecessor_no_cloexec,
                      selected == variant::predecessor_extra_seal);
    } else {
        ::close(4);
    }

    if (selected == variant::credential_alias) {
        assert(::fcntl(3, F_DUPFD_CLOEXEC, 5) == 5);
    } else if (selected == variant::predecessor_alias) {
        assert(::fcntl(4, F_DUPFD_CLOEXEC, 5) == 5);
    }

    if (selected == variant::locked_storage_unavailable) {
        const struct rlimit limit { 0, 0 };
        assert(::setrlimit(RLIMIT_MEMLOCK, &limit) == 0);
    }

    const auto audit =
        halofpx::registry_lab::linux_initializer::inspect_sealed_inputs_once(request);
    assert_cleanup(audit);
    if (selected == variant::success) {
        assert(audit.result == sealed_input_status::transport_validated_no_root_access);
        assert(audit.secure_storage_locked);
        assert(audit.fd_table_unshared);
        assert(audit.exclusive_execution_context);
        assert(audit.credential_transport_validated);
        assert(audit.predecessor_transport_validated);
        assert(audit.descriptor_identities_distinct);
        assert(audit.descriptor_aliases_absent);
        assert(audit.expected_key_tuple_matched);
        assert(audit.predecessor_digest_matched);
        assert(audit.transport_final_revalidation_matched);
        assert(audit.credential_package_size == package.size());
        assert(audit.predecessor_envelope_size == predecessor.size());
        const auto replay =
            halofpx::registry_lab::linux_initializer::inspect_sealed_inputs_once(request);
        assert(replay.result == sealed_input_status::invalid_request_no_mutation);
        assert(!replay.root_or_fixture_accessed);
        assert(replay.secure_storage_wiped);
        assert(replay.secure_storage_unlocked);
        assert(replay.secure_storage_unmapped);
        assert(replay.signal_mask_restored);
    } else {
        if (selected == variant::locked_storage_unavailable &&
            audit.result == sealed_input_status::transport_validated_no_root_access) {
            // Sanitizer runtimes may intercept mlock despite the lowered limit.
            assert(audit.secure_storage_locked);
            assert(audit.exclusive_execution_context);
            assert(audit.transport_final_revalidation_matched);
            return;
        }
        if (audit.result == sealed_input_status::transport_validated_no_root_access) {
            std::fprintf(stderr, "unexpected transport success for variant=%u\n",
                         static_cast<unsigned>(selected));
        }
        assert(audit.result != sealed_input_status::transport_validated_no_root_access);
    }
}

} // namespace

int main(int argc, char ** argv) {
    assert(argc == 2);
    std::ifstream input(argv[1], std::ios::binary);
    const std::string json((std::istreambuf_iterator<char>(input)), {});
    const auto package = hex_bytes(value(json, "credential_package_hex"));
    const auto predecessor = hex_bytes(value(json, "predecessor_registry_envelope_hex"));
    const auto digest = hex_bytes(value(json, "predecessor_registry_envelope_digest_hex"));

    constexpr std::array<variant, 25> variants = {
        variant::success,
        variant::wrong_digest,
        variant::wrong_key,
        variant::wrong_generation,
        variant::credential_name,
        variant::predecessor_name,
        variant::credential_unsealed,
        variant::predecessor_unsealed,
        variant::credential_no_cloexec,
        variant::predecessor_no_cloexec,
        variant::credential_alias,
        variant::predecessor_alias,
        variant::predecessor_bit,
        variant::credential_magic,
        variant::zero_secret,
        variant::missing_credential,
        variant::missing_predecessor,
        variant::credential_too_short,
        variant::credential_oversized,
        variant::predecessor_empty,
        variant::predecessor_oversized,
        variant::credential_extra_seal,
        variant::predecessor_extra_seal,
        variant::credential_non_shmem,
        variant::locked_storage_unavailable,
    };
    for (const auto selected : variants) {
        const pid_t child = ::fork();
        assert(child >= 0);
        if (child == 0) {
            run_child(selected, package, package, predecessor, digest);
            _exit(0);
        }
        int status = 0;
        assert(::waitpid(child, &status, 0) == child);
        assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    }

    auto run_external_mutation = [&](std::vector<std::uint8_t> changed_package,
                                     std::vector<std::uint8_t> changed_predecessor) {
        const pid_t child = ::fork();
        assert(child >= 0);
        if (child == 0) {
            run_child(variant::external_mutation, package, std::move(changed_package),
                      std::move(changed_predecessor), digest);
            _exit(0);
        }
        int status = 0;
        assert(::waitpid(child, &status, 0) == child);
        assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    };

    // Exact pinned-digest rejection for every single predecessor bit.
    for (std::size_t byte = 0; byte < predecessor.size(); ++byte) {
        for (unsigned bit = 0; bit < 8; ++bit) {
            auto changed = predecessor;
            changed[byte] ^= static_cast<std::uint8_t>(1U << bit);
            run_external_mutation(package, std::move(changed));
        }
    }

    // The unauthenticated secret is deliberately outside this transport-only
    // milestone; every bit in the structural/key-tuple prefix must still fail.
    assert(package.size() > 32);
    for (std::size_t byte = 0; byte < package.size() - 32; ++byte) {
        for (unsigned bit = 0; bit < 8; ++bit) {
            auto changed = package;
            changed[byte] ^= static_cast<std::uint8_t>(1U << bit);
            run_external_mutation(std::move(changed), predecessor);
        }
    }

    // A shared fd table is rejected before any descriptor is inspected or
    // closed. The qualification controller must launch a fresh single-task child.
    {
        install_memfd("halofpx-registry-lab-credential", package, 3, true, true);
        install_memfd("halofpx-registry-lab-predecessor", predecessor, 4, true, true);
        std::atomic<bool> stop { false };
        std::thread peer([&]() {
            while (!stop.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
        });
        const auto request = make_request(package, digest);
        const auto audit =
            halofpx::registry_lab::linux_initializer::inspect_sealed_inputs_once(request);
        assert(audit.result == sealed_input_status::unsupported_no_mutation);
        assert(audit.fd_table_unshared);
        assert(!audit.exclusive_execution_context);
        assert(audit.descriptors_closed);
        assert(!audit.root_or_fixture_accessed);
        assert(audit.signal_mask_restored);
        stop.store(true, std::memory_order_release);
        peer.join();
        errno = 0;
        assert(::close(3) == -1 && errno == EBADF);
        errno = 0;
        assert(::close(4) == -1 && errno == EBADF);
    }
}
