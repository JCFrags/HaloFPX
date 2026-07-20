
#if !defined(__linux__) || !defined(__x86_64__)
#error "The L05z ptrace controller currently requires Linux x86-64"
#endif

// Reuse the accepted L05y/L05x controller's authority, receipt, ptrace,
// directory, and OFD-lock primitives.  This remains a separately hashed,
// excluded test executable and adds no hook or reverse edge to the product.
#pragma GCC diagnostic ignored "-Wunused-function"
#if !defined(__linux__) || !defined(__x86_64__)
#error "The L05y ptrace controller currently requires Linux x86-64"
#endif

// L05y deliberately reuses the accepted L05x controller's authority-pinning,
// receipt, syscall-decoding, directory-prefix, and OFD-lock proofs.  Renaming
// its entry point keeps this translation unit an external controller while the
// production initializer remains free of crash hooks.
int halofpx_l05x_controller_unused_main(int argc, char ** argv);
#define main halofpx_l05x_controller_unused_main
#include "halofpx-l05x-ptrace-controller.cpp"
#undef main

#include <linux/magic.h>
#include <sys/vfs.h>

namespace {

enum class marker_operation {
    directory_prefix,
    open_temporary,
    fchmod_temporary,

    pwrite_temporary,
    fsync_temporary,
    publish_no_replace,
    fsync_root_after_publish,
    fsync_staging_after_publish,
};

struct marker_boundary {
    marker_operation op = marker_operation::directory_prefix;
    boundary prefix {};
    std::uint64_t occurrence = 0;
};

struct marker_options {
    const char * target = nullptr;
    const char * golden = nullptr;
    const char * parent = nullptr;
    const char * root = nullptr;
    const char * fixture = nullptr;
    const char * receipt = nullptr;
    marker_boundary point {};
    phase when = phase::entry;
    mode_t tracee_umask = 0;
    bool point_set = false;
    bool phase_set = false;
    bool umask_set = false;
    bool occurrence_set = false;
};

struct marker_state {

    bool pinned = false;
    struct stat identity {};
    std::uint64_t mount_id = 0;
    int child_fd = -1;
    std::vector<unsigned char> bytes;
    std::uint64_t write_occurrences = 0;
    std::size_t readback_offset = 0;
    bool readback_eof = false;
    bool renamed = false;
};

constexpr std::size_t marker_capacity = 1024;
constexpr char temporary_name[] = "initialize-root.tmp";
constexpr char published_name[] = "root.marker";

[[noreturn]] void marker_usage(const char * program) {
    std::fprintf(stderr,
        "usage: %s --target PATH --golden PATH --parent PATH --root PATH "
        "--fixture PATH --boundary mkdirat-envelopes|mkdirat-attempts|"
        "mkdirat-staging|fchmodat2-envelopes|fchmodat2-attempts|"
        "fchmodat2-staging|fsync-envelopes|fsync-attempts|fsync-staging|"
        "fsync-root|openat2-initialize-root|fchmod-initialize-root|"
        "pwrite-initialize-root|fsync-initialize-root|"
        "renameat2-initialize-root|fsync-root-marker|fsync-staging-marker "
        "--phase entry|exit --tracee-umask OCTAL [--occurrence POSITIVE] "
        "--receipt NEW-PATH\n", program);
    std::exit(2);
}

bool parse_positive(const char * value, std::uint64_t & output) {

    if (value == nullptr || value[0] == '\0') return false;
    std::uint64_t result = 0;
    for (const char * cursor = value; *cursor != '\0'; ++cursor) {
        if (*cursor < '0' || *cursor > '9') return false;
        const unsigned digit = static_cast<unsigned>(*cursor - '0');
        if (result > (UINT64_MAX - digit) / 10U) return false;
        result = result * 10U + digit;
    }
    if (result == 0) return false;
    output = result;
    return true;
}

bool parse_marker_boundary(const char * value, marker_boundary & output) {
    boundary prefix {};
    if (parse_boundary(value, prefix)) {
        output.op = marker_operation::directory_prefix;
        output.prefix = prefix;
        return true;
    }
    const struct named_boundary { const char * name; marker_operation op; } names[] = {
        { "openat2-initialize-root", marker_operation::open_temporary },
        { "fchmod-initialize-root", marker_operation::fchmod_temporary },
        { "pwrite-initialize-root", marker_operation::pwrite_temporary },
        { "fsync-initialize-root", marker_operation::fsync_temporary },
        { "renameat2-initialize-root", marker_operation::publish_no_replace },
        { "fsync-root-marker", marker_operation::fsync_root_after_publish },
        { "fsync-staging-marker", marker_operation::fsync_staging_after_publish },
    };
    for (const auto & named : names) {

        if (std::strcmp(value, named.name) == 0) {
            output.op = named.op;
            return true;
        }
    }
    return false;
}

bool parse_marker(int argc, char ** argv, marker_options & output) {
    for (int i = 1; i < argc; i += 2) {
        if (i + 1 >= argc) return false;
        const char * key = argv[i];
        const char * value = argv[i + 1];
        if (std::strcmp(key, "--target") == 0) {
            if (!set_once(output.target, value)) return false;
        } else if (std::strcmp(key, "--golden") == 0) {
            if (!set_once(output.golden, value)) return false;
        } else if (std::strcmp(key, "--parent") == 0) {
            if (!set_once(output.parent, value)) return false;
        } else if (std::strcmp(key, "--root") == 0) {
            if (!set_once(output.root, value)) return false;
        } else if (std::strcmp(key, "--fixture") == 0) {
            if (!set_once(output.fixture, value)) return false;
        } else if (std::strcmp(key, "--receipt") == 0) {
            if (!set_once(output.receipt, value)) return false;
        } else if (std::strcmp(key, "--boundary") == 0) {
            if (output.point_set || !parse_marker_boundary(value, output.point)) return false;
            output.point_set = true;
        } else if (std::strcmp(key, "--phase") == 0) {
            if (output.phase_set) return false;

            if (std::strcmp(value, "entry") == 0) output.when = phase::entry;
            else if (std::strcmp(value, "exit") == 0) output.when = phase::exit;
            else return false;
            output.phase_set = true;
        } else if (std::strcmp(key, "--tracee-umask") == 0) {
            if (output.umask_set || !parse_umask(value, output.tracee_umask)) return false;
            output.umask_set = true;
        } else if (std::strcmp(key, "--occurrence") == 0) {
            if (output.occurrence_set || !parse_positive(value, output.point.occurrence)) return false;
            output.occurrence_set = true;
        } else return false;
    }
    const bool pwrite = output.point.op == marker_operation::pwrite_temporary;
    return output.target && output.golden && output.parent && output.root &&
           output.fixture && output.receipt && output.point_set && output.phase_set &&
           output.umask_set && output.tracee_umask == 0777 &&
           (pwrite == output.occurrence_set);
}

const char * marker_boundary_name(const marker_boundary & point) {
    switch (point.op) {
        case marker_operation::directory_prefix: return boundary_name(point.prefix);
        case marker_operation::open_temporary: return "openat2(staging/initialize-root.tmp)";
        case marker_operation::fchmod_temporary: return "fchmod(initialize-root.tmp)";
        case marker_operation::pwrite_temporary: return "pwrite64(initialize-root.tmp)";
        case marker_operation::fsync_temporary: return "fsync(initialize-root.tmp)";
        case marker_operation::publish_no_replace:
            return "renameat2(initialize-root.tmp,root.marker,RENAME_NOREPLACE)";
        case marker_operation::fsync_root_after_publish: return "fsync(root-after-marker)";
        case marker_operation::fsync_staging_after_publish: return "fsync(staging-after-marker)";

    }
    return "unknown";
}

bool exact_live_marker_child_argv(pid_t pid) {
    char path[64];
    const int path_size = std::snprintf(path, sizeof(path), "/proc/%ld/cmdline",
                                        static_cast<long>(pid));
    if (path_size <= 0 || static_cast<std::size_t>(path_size) >= sizeof(path)) return false;
    const int fd = ::open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return false;
    char bytes[160] {};
    ssize_t count;
    do { count = ::read(fd, bytes, sizeof(bytes)); } while (count < 0 && errno == EINTR);
    const bool closed = ::close(fd) == 0;
    constexpr char expected[] =
        "halofpx-l05y-live-child\0--live-marker-child\0";
    return closed && count == static_cast<ssize_t>(sizeof(expected) - 1) &&
           std::memcmp(bytes, expected, sizeof(expected) - 1) == 0;
}

bool exact_file_fd(pid_t pid, int fd, const marker_state & marker) {
    struct stat observed {};
    std::uint64_t observed_mount = 0;
    return marker.pinned && fd == marker.child_fd &&
           tracee_fd_identity(pid, fd, observed, observed_mount) &&
           S_ISREG(observed.st_mode) && same_object(marker.identity, observed) &&
           observed_mount == marker.mount_id;
}


bool open_marker_entry(pid_t pid, const tracee_state & state,
                       const pinned_directory & staging) {
    if (state.nr != SYS_openat2 || state.args[3] != sizeof(struct open_how)) return false;
    std::string name;
    struct open_how how {};
    constexpr std::uint64_t flags =
        O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC | O_NOFOLLOW;
    constexpr std::uint64_t resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
        RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
    return read_tracee_string(pid, state.args[1], name) && name == temporary_name &&
           read_tracee(pid, state.args[2], &how, sizeof(how)) &&
           how.flags == flags && how.mode == 0600 && how.resolve == resolve &&
           exact_directory_fd(pid, static_cast<int>(state.args[0]), staging);
}

bool fchmod_marker_entry(pid_t pid, const tracee_state & state,
                         const marker_state & marker) {
    return state.nr == SYS_fchmod && state.args[1] == 0600 &&
           exact_file_fd(pid, static_cast<int>(state.args[0]), marker);
}

bool pwrite_marker_entry(pid_t pid, const tracee_state & state,
                         const marker_state & marker) {
    return state.nr == SYS_pwrite64 && marker.bytes.size() < marker_capacity &&
           state.args[2] > 0 && state.args[2] <= marker_capacity - marker.bytes.size() &&
           state.args[3] == marker.bytes.size() &&
           exact_file_fd(pid, static_cast<int>(state.args[0]), marker);
}

bool fsync_marker_entry(pid_t pid, const tracee_state & state,

                        const marker_state & marker) {
    return state.nr == SYS_fsync && exact_file_fd(
        pid, static_cast<int>(state.args[0]), marker);
}

bool rename_marker_entry(pid_t pid, const tracee_state & state,
                         const pinned_directory & staging,
                         const struct stat & root, std::uint64_t root_mount) {
    if (state.nr != SYS_renameat2 || state.args[4] != RENAME_NOREPLACE) return false;
    std::string old_name;
    std::string new_name;
    struct stat observed_root {};
    std::uint64_t observed_mount = 0;
    return read_tracee_string(pid, state.args[1], old_name) &&
           old_name == temporary_name &&
           read_tracee_string(pid, state.args[3], new_name) &&
           new_name == published_name &&
           exact_directory_fd(pid, static_cast<int>(state.args[0]), staging) &&
           tracee_fd_identity(pid, static_cast<int>(state.args[2]),
                              observed_root, observed_mount) &&
           S_ISDIR(observed_root.st_mode) && same_object(root, observed_root) &&
           observed_mount == root_mount;
}

bool exact_root_fsync(pid_t pid, const tracee_state & state,
                      const struct stat & root, std::uint64_t root_mount) {
    struct stat observed {};
    std::uint64_t observed_mount = 0;
    return state.nr == SYS_fsync && tracee_fd_identity(
        pid, static_cast<int>(state.args[0]), observed, observed_mount) &&

        S_ISDIR(observed.st_mode) && same_object(root, observed) &&
        observed_mount == root_mount;
}

bool marker_selected_entry(pid_t pid, const tracee_state & state,
                           const marker_boundary & point,
                           const struct stat & root, std::uint64_t root_mount,
                           const std::array<pinned_directory, 3> & dirs,
                           const marker_state & marker) {
    switch (point.op) {
        case marker_operation::directory_prefix:
            return selected_entry(pid, state, point.prefix, root, root_mount, dirs);
        case marker_operation::open_temporary:
            return open_marker_entry(pid, state, dirs[2]);
        case marker_operation::fchmod_temporary:
            return fchmod_marker_entry(pid, state, marker);
        case marker_operation::pwrite_temporary:
            return pwrite_marker_entry(pid, state, marker) &&
                   marker.write_occurrences == point.occurrence;
        case marker_operation::fsync_temporary:
            return !marker.bytes.empty() && marker.readback_eof &&
                   fsync_marker_entry(pid, state, marker);
        case marker_operation::publish_no_replace:
            return !marker.bytes.empty() && marker.readback_eof && rename_marker_entry(
                pid, state, dirs[2], root, root_mount);
        case marker_operation::fsync_root_after_publish:
            return marker.renamed && exact_root_fsync(pid, state, root, root_mount);
        case marker_operation::fsync_staging_after_publish:
            return marker.renamed && state.nr == SYS_fsync && exact_directory_fd(
                pid, static_cast<int>(state.args[0]), dirs[2]);

    }
    return false;
}

bool pin_marker_from_tracee(pid_t pid, int fd, std::uint64_t root_mount,
                            marker_state & marker) {
    struct stat observed {};
    std::uint64_t observed_mount = 0;
    if (!tracee_fd_identity(pid, fd, observed, observed_mount) ||
        !S_ISREG(observed.st_mode) || observed.st_nlink != 1 ||
        observed.st_size != 0 || observed.st_uid != ::geteuid() ||
        observed_mount != root_mount) return false;
    marker.pinned = true;
    marker.identity = observed;
    marker.mount_id = observed_mount;
    marker.child_fd = fd;
    return true;
}

bool read_exact_file(int directory_fd, const char * name, const marker_state & marker,
                     mode_t expected_mode) {
    int fd = -1;
    const bool metadata_only = expected_mode == 0 && marker.bytes.empty();
    if (!open_named(directory_fd, name, metadata_only ? O_PATH : O_RDONLY, fd)) return false;
    struct stat observed {};
    std::uint64_t observed_mount = 0;
    std::vector<unsigned char> bytes(marker.bytes.size());
    std::size_t done = 0;
    while (done < bytes.size()) {
        ssize_t count;

        do { count = ::pread(fd, bytes.data() + done, bytes.size() - done,
                             static_cast<off_t>(done)); } while (count < 0 && errno == EINTR);
        if (count <= 0) break;
        done += static_cast<std::size_t>(count);
    }
    unsigned char extra = 0;
    ssize_t eof = 0;
    if (!metadata_only) {
        do { eof = ::pread(fd, &extra, 1, static_cast<off_t>(bytes.size())); }
        while (eof < 0 && errno == EINTR);
    }
    const bool valid = ::fstat(fd, &observed) == 0 && fd_mount_id(fd, observed_mount) &&
        S_ISREG(observed.st_mode) && same_object(marker.identity, observed) &&
        observed_mount == marker.mount_id && observed.st_nlink == 1 &&
        observed.st_uid == ::geteuid() &&
        (observed.st_mode & 07777) == expected_mode &&
        observed.st_size == static_cast<off_t>(marker.bytes.size()) &&
        done == bytes.size() && eof == 0 && bytes == marker.bytes;
    const bool closed = ::close(fd) == 0;
    return valid && closed;
}

bool exact_names(int directory_fd, std::vector<std::string> expected) {
    const int duplicate = ::openat(
        directory_fd, ".", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (duplicate < 0) return false;
    DIR * stream = ::fdopendir(duplicate);
    if (stream == nullptr) { (void) ::close(duplicate); return false; }
    std::vector<std::string> observed;
    errno = 0;

    for (;;) {
        dirent * entry = ::readdir(stream);
        if (entry == nullptr) break;
        if (std::strcmp(entry->d_name, ".") != 0 &&
            std::strcmp(entry->d_name, "..") != 0) observed.emplace_back(entry->d_name);
        errno = 0;
    }
    const bool read_ok = errno == 0;
    const bool closed = ::closedir(stream) == 0;
    std::sort(observed.begin(), observed.end());
    std::sort(expected.begin(), expected.end());
    return read_ok && closed && observed == expected;
}

bool marker_root_inventory(int root_fd, const struct stat & writer,
                           std::array<pinned_directory, 3> & dirs,
                           const marker_boundary & point, phase when,
                           mode_t tracee_umask, const marker_state & marker) {
    if (point.op == marker_operation::directory_prefix) {
        return inventory_root(root_fd, expected_prefix(point.prefix, when), writer,
                              dirs, point.prefix, when, tracee_umask);
    }
    const bool before_create = point.op == marker_operation::open_temporary &&
                               when == phase::entry;
    const bool published = marker.renamed;
    std::vector<std::string> root_names {
        "attempts", "envelopes", "staging", "writer.lock"
    };
    if (published) root_names.emplace_back(published_name);
    if (!exact_names(root_fd, root_names)) return false;

    int staging_fd = -1;
    if (!open_named(root_fd, "staging", O_RDONLY | O_DIRECTORY, staging_fd)) return false;
    const std::vector<std::string> staging_names =
        !before_create && !published ? std::vector<std::string>{ temporary_name }
                                     : std::vector<std::string>{};
    bool valid = exact_names(staging_fd, staging_names);
    mode_t expected_mode = 0600;
    if (point.op == marker_operation::open_temporary && when == phase::exit) {
        expected_mode = static_cast<mode_t>(0600 & ~tracee_umask);
    } else if (point.op == marker_operation::fchmod_temporary && when == phase::entry) {
        expected_mode = static_cast<mode_t>(0600 & ~tracee_umask);
    }
    if (valid && !before_create) {
        valid = marker.pinned && read_exact_file(
            published ? root_fd : staging_fd,
            published ? published_name : temporary_name, marker, expected_mode);
    }
    const bool staging_closed = ::close(staging_fd) == 0;
    if (!valid || !staging_closed) return false;

    // Reuse L05x's complete writer and three-directory proof after temporarily
    // accounting for the single admitted marker residue with the checks above.
    int writer_fd = -1;
    if (!open_named(root_fd, "writer.lock", O_RDONLY, writer_fd)) return false;
    struct stat writer_now {};
    const bool writer_ok = ::fstat(writer_fd, &writer_now) == 0 &&
        S_ISREG(writer_now.st_mode) && same_object(writer, writer_now) &&
        writer_now.st_nlink == 1 && writer_now.st_size == 0 &&
        writer_now.st_uid == ::geteuid() && (writer_now.st_mode & 07777) == 0600;
    const bool writer_closed = ::close(writer_fd) == 0;

    if (!writer_ok || !writer_closed) return false;
    for (int index = 0; index < 3; ++index) {
        int fd = -1;
        if (!open_named(root_fd, directory_names[static_cast<std::size_t>(index)],
                        O_RDONLY | O_DIRECTORY, fd)) return false;
        struct stat observed {};
        std::uint64_t mount = 0;
        const std::vector<std::string> expected_names =
            index == 2 ? staging_names : std::vector<std::string> {};
        const bool ok = dirs[static_cast<std::size_t>(index)].pinned &&
            ::fstat(fd, &observed) == 0 && fd_mount_id(fd, mount) &&
            S_ISDIR(observed.st_mode) &&
            same_object(dirs[static_cast<std::size_t>(index)].identity, observed) &&
            mount == dirs[static_cast<std::size_t>(index)].mount_id &&
            observed.st_uid == ::geteuid() && (observed.st_mode & 07777) == 0700 &&
            exact_names(fd, expected_names);
        const bool closed = ::close(fd) == 0;
        if (!ok || !closed) return false;
    }
    return before_create || (marker.bytes.size() <= marker_capacity &&
           (marker.bytes.empty() || marker.readback_eof ||
            point.op == marker_operation::pwrite_temporary ||
            point.op == marker_operation::fchmod_temporary ||
            point.op == marker_operation::open_temporary));
}

int marker_controller(const marker_options & input) {
    std::string receipt_parent_path;
    std::string receipt_name;
    if (!canonical_existing_path(input.target) || !canonical_existing_path(input.golden) ||

        !canonical_existing_path(input.parent) || !canonical_existing_path(input.root) ||
        !canonical_existing_path(input.fixture) || !direct_child(input.root, input.parent) ||
        !direct_child(input.fixture, input.parent) ||
        !split_receipt_path(input.receipt, receipt_parent_path, receipt_name) ||
        !canonical_existing_path(receipt_parent_path.c_str()) ||
        same_or_descendant(receipt_parent_path.c_str(), input.parent)) {
        std::fprintf(stderr, "non-canonical authority or non-disposable receipt scope\n");
        return 2;
    }
    receipt_ok = true;
    receipt_sequence = 0;
    const int receipt_parent_fd = open_absolute_directory_no_symlink(receipt_parent_path.c_str());
    struct stat receipt_parent_stat {};
    std::uint64_t receipt_parent_mount = 0;
    struct open_how receipt_how {};
    receipt_how.flags = O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW;
    receipt_how.mode = 0600;
    receipt_how.resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
                          RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
    if (receipt_parent_fd < 0 || ::fstat(receipt_parent_fd, &receipt_parent_stat) != 0 ||
        !S_ISDIR(receipt_parent_stat.st_mode) || receipt_parent_stat.st_uid != ::geteuid() ||
        (receipt_parent_stat.st_mode & 07777) != 0700 ||
        !fd_mount_id(receipt_parent_fd, receipt_parent_mount)) {
        if (receipt_parent_fd >= 0) (void) ::close(receipt_parent_fd);
        return 2;
    }
    do {
        receipt_fd = static_cast<int>(::syscall(SYS_openat2, receipt_parent_fd,
            receipt_name.c_str(), &receipt_how, sizeof(receipt_how)));
    } while (receipt_fd < 0 && errno == EINTR);

    if (receipt_fd < 0 || ::fchmod(receipt_fd, 0600) != 0) {
        if (receipt_fd >= 0) (void) ::close(receipt_fd);
        receipt_fd = -1;
        (void) ::close(receipt_parent_fd);
        return 2;
    }

    const int target_fd = ::open(input.target, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    const int parent_fd = ::open(input.parent, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    const int root_fd = ::open(input.root, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    const int fixture_fd = ::open(input.fixture, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    const int fixture_lock_fd = fixture_fd >= 0
        ? ::openat(fixture_fd, "primitive.lock", O_RDONLY | O_CLOEXEC | O_NOFOLLOW) : -1;
    struct stat target_stat {}, parent_stat {}, root_stat {}, fixture_stat {}, fixture_lock_stat {};
    std::uint64_t parent_mount = 0, root_mount = 0, fixture_mount = 0;
    struct statfs root_fs {};
    if (target_fd < 0 || ::fstat(target_fd, &target_stat) != 0 ||
        !S_ISREG(target_stat.st_mode) || (target_stat.st_mode & 0111) == 0 ||
        parent_fd < 0 || ::fstat(parent_fd, &parent_stat) != 0 ||
        !fd_mount_id(parent_fd, parent_mount) || !S_ISDIR(parent_stat.st_mode) ||
        parent_stat.st_uid != ::geteuid() || (parent_stat.st_mode & 07777) != 0700 ||
        root_fd < 0 || ::fstat(root_fd, &root_stat) != 0 ||
        !fd_mount_id(root_fd, root_mount) || !S_ISDIR(root_stat.st_mode) ||
        root_stat.st_uid != ::geteuid() || (root_stat.st_mode & 07777) != 0700 ||
        ::fstatfs(root_fd, &root_fs) != 0 ||
        static_cast<unsigned long>(root_fs.f_type) !=
            static_cast<unsigned long>(BTRFS_SUPER_MAGIC) || !directory_empty(root_fd) ||
        fixture_fd < 0 || ::fstat(fixture_fd, &fixture_stat) != 0 ||
        !fd_mount_id(fixture_fd, fixture_mount) || !S_ISDIR(fixture_stat.st_mode) ||
        fixture_stat.st_uid != ::geteuid() || (fixture_stat.st_mode & 07777) != 0700 ||

        fixture_lock_fd < 0 || ::fstat(fixture_lock_fd, &fixture_lock_stat) != 0 ||
        !S_ISREG(fixture_lock_stat.st_mode) || fixture_lock_stat.st_nlink != 1 ||
        fixture_lock_stat.st_size != 0 || fixture_lock_stat.st_uid != ::geteuid() ||
        (fixture_lock_stat.st_mode & 07777) != 0600) {
        emit("\"event\":\"invalid-target\"");
        if (fixture_lock_fd >= 0) (void) ::close(fixture_lock_fd);
        if (fixture_fd >= 0) (void) ::close(fixture_fd);
        if (root_fd >= 0) (void) ::close(root_fd);
        if (parent_fd >= 0) (void) ::close(parent_fd);
        if (target_fd >= 0) (void) ::close(target_fd);
        return finalize_receipt(receipt_parent_fd, receipt_parent_stat,
                                receipt_parent_mount, receipt_name, 2);
    }
    (void) parent_mount;
    (void) ::close(fixture_lock_fd);
    if (!emit(std::string("\"event\":\"start\",\"boundary\":\"") +
              marker_boundary_name(input.point) + "\",\"phase\":\"" +
              (input.when == phase::entry ? "entry" : "exit") +
              "\",\"occurrence\":" + std::to_string(input.point.occurrence) +
              ",\"classification\":\"whole-root-discard-only\"")) {
        (void) ::close(target_fd); (void) ::close(fixture_fd);
        (void) ::close(root_fd); (void) ::close(parent_fd);
        return finalize_receipt(receipt_parent_fd, receipt_parent_stat,
                                receipt_parent_mount, receipt_name, 2);
    }

    const pid_t launcher = ::fork();
    if (launcher == 0) {
        (void) ::umask(input.tracee_umask);
        if (::ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) != 0) _exit(125);

        if (::raise(SIGSTOP) != 0) _exit(125);
        char executable[64];
        const int count = std::snprintf(executable, sizeof(executable),
                                        "/proc/self/fd/%d", target_fd);
        if (count <= 0 || static_cast<std::size_t>(count) >= sizeof(executable)) _exit(125);
        ::execl(executable, input.target, "--live-marker-controller",
                input.golden, input.parent, input.root, input.fixture,
                static_cast<char *>(nullptr));
        _exit(127);
    }
    (void) ::close(target_fd);
    if (launcher < 0) {
        emit("\"event\":\"fork-failed\"");
        (void) ::close(fixture_fd); (void) ::close(root_fd); (void) ::close(parent_fd);
        return finalize_receipt(receipt_parent_fd, receipt_parent_stat,
                                receipt_parent_mount, receipt_name, 2);
    }
    int status = 0;
    if (::waitpid(launcher, &status, 0) != launcher || !WIFSTOPPED(status)) {
        emit("\"event\":\"initial-stop-failed\"");
        (void) ::kill(launcher, SIGKILL); (void) ::waitpid(launcher, nullptr, 0);
        (void) ::close(fixture_fd); (void) ::close(root_fd); (void) ::close(parent_fd);
        return finalize_receipt(receipt_parent_fd, receipt_parent_stat,
                                receipt_parent_mount, receipt_name, 2);
    }
    constexpr long ptrace_options = PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK |
        PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE | PTRACE_O_TRACEEXEC | PTRACE_O_EXITKILL;
    if (::ptrace(PTRACE_SETOPTIONS, launcher, nullptr,
                 reinterpret_cast<void *>(ptrace_options)) != 0 || !resume_syscalls(launcher)) {
        emit("\"event\":\"ptrace-setup-failed\"");

        (void) ::kill(launcher, SIGKILL); (void) ::waitpid(launcher, nullptr, 0);
        (void) ::close(fixture_fd); (void) ::close(root_fd); (void) ::close(parent_fd);
        return finalize_receipt(receipt_parent_fd, receipt_parent_stat,
                                receipt_parent_mount, receipt_name, 2);
    }

    std::unordered_map<pid_t, tracee_state> tracees;
    tracees.emplace(launcher, tracee_state {});
    std::array<pinned_directory, 3> dirs {};
    marker_state marker {};
    pid_t live_child = -1;
    int writer_fd_number = -1, fixture_child_fd_number = -1;
    struct stat writer_identity {};
    std::uint64_t writer_mount = 0;
    bool writer_pinned = false, fixture_child_pinned = false;
    bool fixture_child_lock_acquired = false, writer_child_lock_acquired = false;
    bool injected = false, target_sigkilled = false;
    bool launcher_exited = false, launcher_wifexited = false, launcher_exec_seen = false;
    int launcher_exit = -1;
    bool controller_error = false, bounded_process_cleanup = false;
    std::vector<unsigned char> pending_write;
    std::size_t pending_read_offset = 0;
    std::size_t pending_read_size = 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);

    while (!tracees.empty() && std::chrono::steady_clock::now() < deadline) {
        const pid_t pid = ::waitpid(-1, &status, __WALL | WNOHANG);
        if (pid == 0) { const struct timespec pause { 0, 1000000 };
            (void) ::nanosleep(&pause, nullptr); continue; }
        if (pid < 0) { if (errno == EINTR) continue; if (errno == ECHILD) break;

            controller_error = true; break; }
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            if (pid == live_child && WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL) {
                target_sigkilled = true;
                emit("\"event\":\"target-exit\",\"wifsignaled\":true,\"signal\":9");
            } else if (pid == launcher) {
                launcher_exited = true; launcher_wifexited = WIFEXITED(status);
                launcher_exit = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
                emit(std::string("\"event\":\"launcher-exit\",\"wifexited\":") +
                    (launcher_wifexited ? "true" : "false") + ",\"status\":" +
                    std::to_string(launcher_exit));
            }
            tracees.erase(pid); continue;
        }
        if (!WIFSTOPPED(status)) continue;
        const int stop_signal = WSTOPSIG(status);
        const unsigned event = static_cast<unsigned>(status) >> 16;
        if (stop_signal == SIGTRAP && event != 0) {
            if (event == PTRACE_EVENT_FORK || event == PTRACE_EVENT_VFORK ||
                event == PTRACE_EVENT_CLONE) {
                unsigned long child_value = 0;
                if (::ptrace(PTRACE_GETEVENTMSG, pid, nullptr, &child_value) != 0) {
                    controller_error = true; break;
                }
                auto & child_state = tracees[static_cast<pid_t>(child_value)];
                child_state.initial_sigstop_expected = !child_state.initial_sigstop_observed;
                emit(std::string("\"event\":\"descendant\",\"parent\":") +
                     std::to_string(pid) + ",\"pid\":" + std::to_string(child_value));
            } else if (event == PTRACE_EVENT_EXEC && pid == launcher) {
                if (launcher_exec_seen || !same_executable(pid, target_stat)) {

                    controller_error = true; break;
                }
                launcher_exec_seen = true;
            } else if (event == PTRACE_EVENT_EXEC) {
                if (live_child != -1 || !same_executable(pid, target_stat) ||
                    !exact_live_marker_child_argv(pid)) {
                    controller_error = true; break;
                }
                live_child = pid; tracees[pid].live_child = true;
                emit(std::string("\"event\":\"live-child-exec\",\"pid\":") +
                     std::to_string(pid));
            }
            if (!resume_syscalls(pid)) { controller_error = true; break; }
            continue;
        }
        if (stop_signal == (SIGTRAP | 0x80)) {
            struct __ptrace_syscall_info info {};
            const long info_size = ::ptrace(PTRACE_GET_SYSCALL_INFO, pid, sizeof(info), &info);
            if (info_size < 0 || info.arch != AUDIT_ARCH_X86_64) {
                controller_error = true; break;
            }
            auto & state = tracees[pid];
            if (info.op == PTRACE_SYSCALL_INFO_ENTRY) {
                state.have_entry = true; state.nr = info.entry.nr;
                std::memcpy(state.args, info.entry.args, sizeof(state.args));
                if (state.live_child) {
                    emit(std::string("\"event\":\"syscall-entry\",\"pid\":") +
                         std::to_string(pid) + ",\"nr\":" + std::to_string(state.nr) +
                         ",\"arg0\":" + std::to_string(state.args[0]) +
                         ",\"arg1\":" + std::to_string(state.args[1]) +

                         ",\"arg2\":" + std::to_string(state.args[2]) +
                         ",\"arg3\":" + std::to_string(state.args[3]) +
                         ",\"arg4\":" + std::to_string(state.args[4]) +
                         ",\"arg5\":" + std::to_string(state.args[5]));
                    if (pwrite_marker_entry(pid, state, marker)) {
                        ++marker.write_occurrences;
                        pending_write.resize(static_cast<std::size_t>(state.args[2]));
                        if (!read_tracee(pid, state.args[1], pending_write.data(),
                                         pending_write.size())) { controller_error = true; break; }
                    } else if (state.nr == SYS_pread64 && marker.pinned &&
                               exact_file_fd(pid, static_cast<int>(state.args[0]), marker) &&
                               state.args[2] <= marker_capacity && state.args[3] <= marker_capacity) {
                        pending_read_offset = static_cast<std::size_t>(state.args[3]);
                        pending_read_size = static_cast<std::size_t>(state.args[2]);
                    }
                    const bool match = marker_selected_entry(pid, state, input.point,
                        root_stat, root_mount, dirs, marker);
                    if (match) {
                        emit(std::string("\"event\":\"boundary-entry\",\"name\":\"") +
                             marker_boundary_name(input.point) + "\",\"occurrence\":" +
                             std::to_string(marker.write_occurrences));
                        if (input.when == phase::entry) {
                            injected = ::kill(pid, SIGKILL) == 0;
                            emit(std::string("\"event\":\"inject\",\"ok\":") +
                                 (injected ? "true" : "false"));
                        }
                    }
                }
            } else if (info.op == PTRACE_SYSCALL_INFO_EXIT) {
                if (state.live_child && state.have_entry) {

                    emit(std::string("\"event\":\"syscall-exit\",\"pid\":") +
                         std::to_string(pid) + ",\"nr\":" + std::to_string(state.nr) +
                         ",\"rval\":" + std::to_string(info.exit.rval) +
                         ",\"is_error\":" +
                         (info.exit.is_error ? "true" : "false"));
                    const bool match = marker_selected_entry(pid, state, input.point,
                        root_stat, root_mount, dirs, marker);
                    if (state.nr == SYS_openat2 && writer_openat2_entry(
                            pid, state, root_stat, root_mount) &&
                        !info.exit.is_error && info.exit.rval >= 0) {
                        writer_fd_number = static_cast<int>(info.exit.rval);
                        if (!tracee_fd_identity(pid, writer_fd_number, writer_identity,
                                               writer_mount) || writer_mount != root_mount) {
                            controller_error = true; break;
                        }
                        writer_pinned = true;
                    }
                    if (state.nr == SYS_openat2 && fixture_child_fd_number < 0 &&
                        fixture_openat2_entry(pid, state, fixture_stat, fixture_mount) &&
                        !info.exit.is_error && info.exit.rval >= 0) {
                        fixture_child_fd_number = static_cast<int>(info.exit.rval);
                        struct stat observed {}; std::uint64_t mount = 0;
                        if (!tracee_fd_identity(pid, fixture_child_fd_number, observed, mount) ||
                            !same_object(fixture_lock_stat, observed) || mount != fixture_mount) {
                            controller_error = true; break;
                        }
                        fixture_child_pinned = true;
                    }
                    if (ofd_write_lock_entry(pid, state, fixture_child_fd_number,
                                             fixture_lock_stat, fixture_mount) &&

                        !info.exit.is_error && info.exit.rval == 0) fixture_child_lock_acquired = true;
                    if (ofd_write_lock_entry(pid, state, writer_fd_number,
                                             writer_identity, writer_mount) &&
                        !info.exit.is_error && info.exit.rval == 0) writer_child_lock_acquired = true;
                    if (state.nr == SYS_mkdirat && !info.exit.is_error && info.exit.rval == 0) {
                        std::string name;
                        if (read_tracee_string(pid, state.args[1], name)) {
                            for (int index = 0; index < 3; ++index) {
                                if (name == directory_names[static_cast<std::size_t>(index)] &&
                                    !pin_created_directory(root_fd, index,
                                        static_cast<mode_t>(0700 & ~input.tracee_umask),
                                        dirs[static_cast<std::size_t>(index)])) {
                                    controller_error = true; break;
                                }
                            }
                        }
                        if (controller_error) break;
                    }
                    if (state.nr == SYS_openat2 && open_marker_entry(pid, state, dirs[2]) &&
                        !info.exit.is_error && info.exit.rval >= 0 &&
                        !pin_marker_from_tracee(pid, static_cast<int>(info.exit.rval),
                                               root_mount, marker)) {
                        controller_error = true; break;
                    }
                    if (state.nr == SYS_pwrite64 && marker.pinned &&
                        exact_file_fd(pid, static_cast<int>(state.args[0]), marker)) {
                        if (info.exit.is_error || info.exit.rval <= 0 ||
                            static_cast<std::uint64_t>(info.exit.rval) > state.args[2] ||
                            state.args[3] != marker.bytes.size() ||
                            pending_write.size() != state.args[2]) {

                            controller_error = true; break;
                        }
                        marker.bytes.insert(marker.bytes.end(), pending_write.begin(),
                            pending_write.begin() +
                                static_cast<std::ptrdiff_t>(info.exit.rval));
                    }
                    if (state.nr == SYS_pread64 && marker.pinned &&
                        exact_file_fd(pid, static_cast<int>(state.args[0]), marker) &&
                        pending_read_offset == state.args[3] && pending_read_size == state.args[2] &&
                        !info.exit.is_error && info.exit.rval >= 0) {
                        if (info.exit.rval == 0) {
                            if (pending_read_offset == marker.bytes.size()) marker.readback_eof = true;
                        } else {
                            const std::size_t count = static_cast<std::size_t>(info.exit.rval);
                            std::vector<unsigned char> observed(count);
                            if (count > pending_read_size || pending_read_offset != marker.readback_offset ||
                                pending_read_offset + count > marker.bytes.size() ||
                                !read_tracee(pid, state.args[1], observed.data(), count) ||
                                !std::equal(observed.begin(), observed.end(),
                                            marker.bytes.begin() +
                                                static_cast<std::ptrdiff_t>(pending_read_offset))) {
                                controller_error = true; break;
                            }
                            marker.readback_offset += count;
                        }
                    }
                    if (state.nr == SYS_renameat2 && rename_marker_entry(
                            pid, state, dirs[2], root_stat, root_mount) &&
                        !info.exit.is_error && info.exit.rval == 0) marker.renamed = true;
                    if (match && input.when == phase::exit && !info.exit.is_error &&

                        info.exit.rval == 0) {
                        emit(std::string("\"event\":\"boundary-exit\",\"name\":\"") +
                             marker_boundary_name(input.point) + "\"");
                        injected = ::kill(pid, SIGKILL) == 0;
                        emit(std::string("\"event\":\"inject\",\"ok\":") +
                             (injected ? "true" : "false"));
                    } else if (input.point.op == marker_operation::open_temporary &&
                               input.when == phase::exit && state.nr == SYS_openat2 &&
                               open_marker_entry(pid, state, dirs[2]) &&
                               !info.exit.is_error && info.exit.rval >= 0) {
                        emit(std::string("\"event\":\"boundary-exit\",\"name\":\"") +
                             marker_boundary_name(input.point) + "\"");
                        injected = ::kill(pid, SIGKILL) == 0;
                        emit(std::string("\"event\":\"inject\",\"ok\":") +
                             (injected ? "true" : "false"));
                    } else if (input.point.op == marker_operation::pwrite_temporary &&
                               input.when == phase::exit &&
                               state.nr == SYS_pwrite64 &&
                               marker.write_occurrences == input.point.occurrence &&
                               !info.exit.is_error && info.exit.rval > 0) {
                        emit(std::string("\"event\":\"boundary-exit\",\"name\":\"") +
                             marker_boundary_name(input.point) + "\"");
                        injected = ::kill(pid, SIGKILL) == 0;
                        emit(std::string("\"event\":\"inject\",\"ok\":") +
                             (injected ? "true" : "false"));
                    }
                }
                state.have_entry = false;
            } else { controller_error = true; break; }
            if (!resume_syscalls(pid) && !(injected && errno == ESRCH)) controller_error = true;

            continue;
        }
        auto iterator = tracees.find(pid);
        if (stop_signal == SIGSTOP && iterator == tracees.end()) {
            tracee_state newborn {}; newborn.initial_sigstop_observed = true;
            tracees.emplace(pid, newborn);
            if (!resume_syscalls(pid)) controller_error = true;
            continue;
        }
        if (stop_signal == SIGSTOP && iterator != tracees.end() &&
            iterator->second.initial_sigstop_expected) {
            iterator->second.initial_sigstop_expected = false;
            iterator->second.initial_sigstop_observed = true;
            if (!resume_syscalls(pid)) controller_error = true;
            continue;
        }
        if (stop_signal == SIGSTOP || stop_signal == SIGTRAP) {
            controller_error = true; break;
        }
        if (!resume_syscalls(pid, stop_signal) && !(injected && errno == ESRCH)) {
            controller_error = true;
        }
    }
    if (!tracees.empty()) {
        bounded_process_cleanup = true;
        emit("\"event\":\"bounded-process-cleanup\"");
        for (const auto & item : tracees) (void) ::kill(item.first, SIGKILL);
        const auto reap_deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (!tracees.empty() && std::chrono::steady_clock::now() < reap_deadline) {

            const pid_t pid = ::waitpid(-1, &status, __WALL | WNOHANG);
            if (pid > 0) tracees.erase(pid);
            else if (pid == 0) {
                const struct timespec pause { 0, 1000000 };
                (void) ::nanosleep(&pause, nullptr);
            } else if (errno != EINTR) break;
        }
    }

    struct stat root_now {}, fixture_now {};
    std::uint64_t root_mount_now = 0, fixture_mount_now = 0;
    const bool authorities_pinned = ::fstat(root_fd, &root_now) == 0 &&
        fd_mount_id(root_fd, root_mount_now) && ::fstat(fixture_fd, &fixture_now) == 0 &&
        fd_mount_id(fixture_fd, fixture_mount_now) && same_object(root_stat, root_now) &&
        root_mount_now == root_mount && root_now.st_uid == root_stat.st_uid &&
        (root_now.st_mode & 07777) == (root_stat.st_mode & 07777) &&
        same_object(fixture_stat, fixture_now) && fixture_mount_now == fixture_mount &&
        fixture_now.st_uid == fixture_stat.st_uid &&
        (fixture_now.st_mode & 07777) == (fixture_stat.st_mode & 07777);
    const bool inventory_exact = authorities_pinned && writer_pinned &&
        marker_root_inventory(root_fd, writer_identity, dirs, input.point, input.when,
                              input.tracee_umask, marker);
    const bool fixture_released = prove_ofd_released(fixture_fd, "primitive.lock",
                                                     fixture_lock_stat);
    const bool writer_released = writer_pinned &&
        prove_ofd_released(root_fd, "writer.lock", writer_identity);
    emit(std::string("\"event\":\"post-crash\",\"authorities_pinned\":") +
        (authorities_pinned ? "true" : "false") + ",\"inventory_exact\":" +
        (inventory_exact ? "true" : "false") + ",\"fixture_lock_released\":" +
        (fixture_released ? "true" : "false") + ",\"writer_lock_released\":" +

        (writer_released ? "true" : "false") + ",\"marker_size\":" +
        std::to_string(marker.bytes.size()) +
        ",\"classification\":\"whole-root-discard-only\"");
    emit(std::string("\"event\":\"identity-inventory\",\"root_device\":") +
        std::to_string(static_cast<std::uint64_t>(root_stat.st_dev)) +
        ",\"root_inode\":" +
        std::to_string(static_cast<std::uint64_t>(root_stat.st_ino)) +
        ",\"root_mount_id\":" + std::to_string(root_mount) +
        ",\"marker_pinned\":" + (marker.pinned ? "true" : "false") +
        ",\"marker_device\":" + std::to_string(marker.pinned
            ? static_cast<std::uint64_t>(marker.identity.st_dev) : 0) +
        ",\"marker_inode\":" + std::to_string(marker.pinned
            ? static_cast<std::uint64_t>(marker.identity.st_ino) : 0) +
        ",\"marker_mount_id\":" + std::to_string(marker.mount_id) +
        ",\"marker_published\":" + (marker.renamed ? "true" : "false") +
        ",\"pwrite_occurrences\":" + std::to_string(marker.write_occurrences));
    const bool pass = !controller_error && receipt_ok && injected && target_sigkilled &&
        launcher_exec_seen && !bounded_process_cleanup && launcher_exited &&
        launcher_wifexited && launcher_exit != 0 && tracees.empty() &&
        fixture_child_pinned && fixture_child_lock_acquired && writer_child_lock_acquired &&
        inventory_exact && fixture_released && writer_released;
    emit(std::string("\"event\":\"summary\",\"pass\":") +
        (pass ? "true" : "false") + ",\"target_wifsignaled\":" +
        (target_sigkilled ? "true" : "false") + ",\"launcher_status\":" +
        std::to_string(launcher_exit) +
        ",\"classification\":\"whole-root-discard-only\"");
    const bool fixture_closed = ::close(fixture_fd) == 0;
    const bool root_closed = ::close(root_fd) == 0;
    const bool parent_closed = ::close(parent_fd) == 0;
    const int intended = pass && fixture_closed && root_closed && parent_closed ? 0 : 1;

    return finalize_receipt(receipt_parent_fd, receipt_parent_stat,
                            receipt_parent_mount, receipt_name, intended);
}

} // namespace


namespace {

enum class envelope_operation {
    open_temporary,
    fchmod_temporary,
    pwrite_temporary,
    fsync_temporary,
    publish_no_replace,
    fsync_envelopes_after_publish,
    fsync_staging_after_publish,
};

struct envelope_boundary {
    envelope_operation op = envelope_operation::open_temporary;
    std::uint64_t occurrence = 0;
};

struct envelope_options {
    const char * target = nullptr;
    const char * golden = nullptr;
    const char * parent = nullptr;
    const char * root = nullptr;
    const char * fixture = nullptr;

    const char * receipt = nullptr;
    envelope_boundary point {};
    phase when = phase::entry;
    mode_t tracee_umask = 0;
    bool point_set = false;
    bool phase_set = false;
    bool umask_set = false;
    bool occurrence_set = false;
};

struct traced_regular {
    bool pinned = false;
    struct stat identity {};
    std::uint64_t mount_id = 0;
    int child_fd = -1;
    std::vector<unsigned char> bytes;
    std::uint64_t write_occurrences = 0;
    bool exact_eof_seen = false;
    std::uint64_t exact_eof_completions = 0;
    bool readonly_alias_seen = false;
    bool renamed = false;
    std::vector<unsigned char> authority_bytes;
    std::string authority_name;
};

constexpr std::size_t envelope_capacity = 1024;
constexpr char marker_temporary_name[] = "initialize-root.tmp";
constexpr char marker_published_name[] = "root.marker";
constexpr char envelope_temporary_name[] = "initialize-envelope.tmp";

[[noreturn]] void envelope_usage(const char * program) {
    std::fprintf(stderr,
        "usage: %s --target PATH --golden PATH --parent PATH --root PATH "
        "--fixture PATH --boundary openat2-initialize-envelope|"

        "fchmod-initialize-envelope|pwrite-initialize-envelope|"
        "fsync-initialize-envelope|renameat2-initialize-envelope|"
        "fsync-envelopes-envelope|fsync-staging-envelope "
        "--phase entry|exit --tracee-umask OCTAL [--occurrence POSITIVE] "
        "--receipt NEW-PATH\n", program);
    std::exit(2);
}

bool parse_envelope_boundary(const char * value, envelope_boundary & output) {
    const struct named_boundary {
        const char * name;
        envelope_operation op;
    } names[] = {
        { "openat2-initialize-envelope", envelope_operation::open_temporary },
        { "fchmod-initialize-envelope", envelope_operation::fchmod_temporary },
        { "pwrite-initialize-envelope", envelope_operation::pwrite_temporary },
        { "fsync-initialize-envelope", envelope_operation::fsync_temporary },
        { "renameat2-initialize-envelope", envelope_operation::publish_no_replace },
        { "fsync-envelopes-envelope", envelope_operation::fsync_envelopes_after_publish },
        { "fsync-staging-envelope", envelope_operation::fsync_staging_after_publish },
    };
    for (const auto & named : names) {
        if (std::strcmp(value, named.name) == 0) {
            output.op = named.op;
            return true;
        }
    }
    return false;
}


bool parse_envelope(int argc, char ** argv, envelope_options & output) {
    for (int index = 1; index < argc; index += 2) {
        if (index + 1 >= argc) return false;
        const char * key = argv[index];
        const char * value = argv[index + 1];
        if (std::strcmp(key, "--target") == 0) {
            if (!set_once(output.target, value)) return false;
        } else if (std::strcmp(key, "--golden") == 0) {
            if (!set_once(output.golden, value)) return false;
        } else if (std::strcmp(key, "--parent") == 0) {
            if (!set_once(output.parent, value)) return false;
        } else if (std::strcmp(key, "--root") == 0) {
            if (!set_once(output.root, value)) return false;
        } else if (std::strcmp(key, "--fixture") == 0) {
            if (!set_once(output.fixture, value)) return false;
        } else if (std::strcmp(key, "--receipt") == 0) {
            if (!set_once(output.receipt, value)) return false;
        } else if (std::strcmp(key, "--boundary") == 0) {
            if (output.point_set || !parse_envelope_boundary(value, output.point)) return false;
            output.point_set = true;
        } else if (std::strcmp(key, "--phase") == 0) {
            if (output.phase_set) return false;
            if (std::strcmp(value, "entry") == 0) output.when = phase::entry;
            else if (std::strcmp(value, "exit") == 0) output.when = phase::exit;
            else return false;
            output.phase_set = true;
        } else if (std::strcmp(key, "--tracee-umask") == 0) {
            if (output.umask_set || !parse_umask(value, output.tracee_umask)) return false;
            output.umask_set = true;
        } else if (std::strcmp(key, "--occurrence") == 0) {

            if (output.occurrence_set || !parse_positive(value, output.point.occurrence)) return false;
            output.occurrence_set = true;
        } else return false;
    }
    const bool pwrite = output.point.op == envelope_operation::pwrite_temporary;
    return output.target && output.golden && output.parent && output.root &&
           output.fixture && output.receipt && output.point_set && output.phase_set &&
           output.umask_set && output.tracee_umask == 0777 &&
           (pwrite == output.occurrence_set);
}

const char * envelope_boundary_name(const envelope_boundary & point) {
    switch (point.op) {
        case envelope_operation::open_temporary:
            return "openat2(staging/initialize-envelope.tmp)";
        case envelope_operation::fchmod_temporary:
            return "fchmod(initialize-envelope.tmp)";
        case envelope_operation::pwrite_temporary:
            return "pwrite64(initialize-envelope.tmp)";
        case envelope_operation::fsync_temporary:
            return "fsync(initialize-envelope.tmp)";
        case envelope_operation::publish_no_replace:
            return "renameat2(staging/initialize-envelope.tmp,envelopes/e-digest.cbor,RENAME_NOREPLACE)";
        case envelope_operation::fsync_envelopes_after_publish:
            return "fsync(envelopes-after-envelope)";
        case envelope_operation::fsync_staging_after_publish:
            return "fsync(staging-after-envelope)";
    }
    return "unknown";
}


bool exact_live_envelope_child_argv(pid_t pid) {
    char path[64];
    const int size = std::snprintf(path, sizeof(path), "/proc/%ld/cmdline",
                                   static_cast<long>(pid));
    if (size <= 0 || static_cast<std::size_t>(size) >= sizeof(path)) return false;
    const int fd = ::open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return false;
    char bytes[160] {};
    ssize_t count;
    do { count = ::read(fd, bytes, sizeof(bytes)); } while (count < 0 && errno == EINTR);
    const bool closed = ::close(fd) == 0;
    constexpr char expected[] =
        "halofpx-l05z-live-child\0--live-envelope-child\0";
    return closed && count == static_cast<ssize_t>(sizeof(expected) - 1) &&
           std::memcmp(bytes, expected, sizeof(expected) - 1) == 0;
}

bool exact_regular_fd(pid_t pid, int fd, const traced_regular & file) {
    struct stat observed {};
    std::uint64_t mount = 0;
    return file.pinned && fd == file.child_fd &&
           tracee_fd_identity(pid, fd, observed, mount) &&
           S_ISREG(observed.st_mode) && same_object(file.identity, observed) &&
           mount == file.mount_id;
}

bool regular_fd_alias(pid_t pid, int fd, const traced_regular & file) {
    struct stat observed {};
    std::uint64_t mount = 0;
    return file.pinned && tracee_fd_identity(pid, fd, observed, mount) &&
           S_ISREG(observed.st_mode) && same_object(file.identity, observed) &&
           mount == file.mount_id;
}

bool tracee_fd_is_readonly(pid_t pid, int fd) {
    char path[96];
    const int size = std::snprintf(path, sizeof(path), "/proc/%ld/fdinfo/%d",
                                   static_cast<long>(pid), fd);
    if (size <= 0 || static_cast<std::size_t>(size) >= sizeof(path)) return false;
    FILE * stream = std::fopen(path, "re");
    if (stream == nullptr) return false;
    char line[160] {};
    unsigned flags = UINT_MAX;
    while (std::fgets(line, sizeof(line), stream) != nullptr) {
        if (std::sscanf(line, "flags:\t%o", &flags) == 1) break;
    }
    const bool closed = std::fclose(stream) == 0;
    return closed && flags != UINT_MAX && (flags & O_ACCMODE) == O_RDONLY;
}

bool name_based_syscall(long number) {
    return number == SYS_openat || number == SYS_openat2 ||
           number == SYS_newfstatat || number == SYS_statx ||
           number == SYS_readlinkat || number == SYS_mkdirat ||
           number == SYS_unlinkat || number == SYS_renameat2;
}

bool open_temporary_entry(pid_t pid, const tracee_state & state,
                          const pinned_directory & staging,
                          const char * expected_name) {

    if (state.nr != SYS_openat2 || state.args[3] != sizeof(struct open_how)) return false;
    std::string name;
    struct open_how how {};
    constexpr std::uint64_t flags =
        O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC | O_NOFOLLOW;
    constexpr std::uint64_t resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
        RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
    return read_tracee_string(pid, state.args[1], name) && name == expected_name &&
           read_tracee(pid, state.args[2], &how, sizeof(how)) &&
           how.flags == flags && how.mode == 0600 && how.resolve == resolve &&
           exact_directory_fd(pid, static_cast<int>(state.args[0]), staging);
}

bool pwrite_entry(pid_t pid, const tracee_state & state,
                  const traced_regular & file) {
    return state.nr == SYS_pwrite64 && file.bytes.size() < envelope_capacity &&
           state.args[2] > 0 && state.args[2] <= envelope_capacity - file.bytes.size() &&
           state.args[3] == file.bytes.size() &&
           exact_regular_fd(pid, static_cast<int>(state.args[0]), file);
}

bool fchmod_entry(pid_t pid, const tracee_state & state,
                  const traced_regular & file) {
    return state.nr == SYS_fchmod && state.args[1] == 0600 &&
           exact_regular_fd(pid, static_cast<int>(state.args[0]), file);
}

bool fsync_regular_entry(pid_t pid, const tracee_state & state,
                         const traced_regular & file) {
    return state.nr == SYS_fsync &&

           exact_regular_fd(pid, static_cast<int>(state.args[0]), file);
}

bool digest_basename_shape(const std::string & name) {
    if (name.size() != 71 || name.compare(0, 2, "e-") != 0 ||
        name.compare(66, 5, ".cbor") != 0) return false;
    for (std::size_t index = 2; index < 66; ++index) {
        const char value = name[index];
        if (!((value >= '0' && value <= '9') || (value >= 'a' && value <= 'f'))) return false;
    }
    return true;
}

bool exact_digest_basename_bytes(const std::array<char, 72> & bytes,
                                 std::string & output) {
    if (bytes[71] != '\0' ||
        std::find(bytes.begin(), bytes.begin() + 71, '\0') != bytes.begin() + 71)
        return false;
    output.assign(bytes.data(), 71);
    return digest_basename_shape(output);
}

bool read_tracee_digest_basename(pid_t pid, std::uint64_t address,
                                 std::string & output) {
    std::array<char, 72> bytes {};
    return read_tracee(pid, address, bytes.data(), bytes.size()) &&
           exact_digest_basename_bytes(bytes, output);
}

bool digest_basename_reader_self_test() {
    std::array<char, 72> valid {};
    const std::string name = "e-" + std::string(64, '0') + ".cbor";
    std::copy(name.begin(), name.end(), valid.begin());
    std::string output;
    if (!exact_digest_basename_bytes(valid, output) || output != name) return false;
    auto early_nul = valid;
    early_nul[70] = '\0';
    auto missing_nul = valid;
    missing_nul[71] = 'x';
    auto wrong_width = valid;
    wrong_width[69] = '\0';
    auto wrong_case = valid;
    wrong_case[2] = 'A';
    auto nonhex = valid;
    nonhex[2] = 'g';
    auto wrong_suffix = valid;
    wrong_suffix[70] = 'x';
    return !exact_digest_basename_bytes(early_nul, output) &&
           !exact_digest_basename_bytes(missing_nul, output) &&
           !exact_digest_basename_bytes(wrong_width, output) &&
           !exact_digest_basename_bytes(wrong_case, output) &&
           !exact_digest_basename_bytes(nonhex, output) &&
           !exact_digest_basename_bytes(wrong_suffix, output);
}

// Small independent SHA-256 used only to reconstruct the target-owned object
// name from the exact traced bytes.  It has no product dependency or hook.
std::array<unsigned char, 32> envelope_sha256(
        const unsigned char * bytes, std::size_t size) {
    constexpr std::array<std::uint32_t, 64> k {
        0x428a2f98U,0x71374491U,0xb5c0fbcfU,0xe9b5dba5U,0x3956c25bU,0x59f111f1U,0x923f82a4U,0xab1c5ed5U,
        0xd807aa98U,0x12835b01U,0x243185beU,0x550c7dc3U,0x72be5d74U,0x80deb1feU,0x9bdc06a7U,0xc19bf174U,
        0xe49b69c1U,0xefbe4786U,0x0fc19dc6U,0x240ca1ccU,0x2de92c6fU,0x4a7484aaU,0x5cb0a9dcU,0x76f988daU,
        0x983e5152U,0xa831c66dU,0xb00327c8U,0xbf597fc7U,0xc6e00bf3U,0xd5a79147U,0x06ca6351U,0x14292967U,
        0x27b70a85U,0x2e1b2138U,0x4d2c6dfcU,0x53380d13U,0x650a7354U,0x766a0abbU,0x81c2c92eU,0x92722c85U,
        0xa2bfe8a1U,0xa81a664bU,0xc24b8b70U,0xc76c51a3U,0xd192e819U,0xd6990624U,0xf40e3585U,0x106aa070U,
        0x19a4c116U,0x1e376c08U,0x2748774cU,0x34b0bcb5U,0x391c0cb3U,0x4ed8aa4aU,0x5b9cca4fU,0x682e6ff3U,
        0x748f82eeU,0x78a5636fU,0x84c87814U,0x8cc70208U,0x90befffaU,0xa4506cebU,0xbef9a3f7U,0xc67178f2U,
    };
    std::vector<unsigned char> message(bytes, bytes + size);
    const std::uint64_t bit_size = static_cast<std::uint64_t>(size) * 8U;
    message.push_back(0x80);

    while ((message.size() % 64U) != 56U) message.push_back(0);
    for (int shift = 56; shift >= 0; shift -= 8) {
        message.push_back(static_cast<unsigned char>((bit_size >> shift) & 0xffU));
    }
    std::array<std::uint32_t, 8> hash {
        0x6a09e667U,0xbb67ae85U,0x3c6ef372U,0xa54ff53aU,
        0x510e527fU,0x9b05688cU,0x1f83d9abU,0x5be0cd19U,
    };
    const auto rotate = [](std::uint32_t value, unsigned count) {
        return (value >> count) | (value << (32U - count));
    };
    for (std::size_t offset = 0; offset < message.size(); offset += 64) {
        std::array<std::uint32_t, 64> words {};
        for (std::size_t index = 0; index < 16; ++index) {
            const std::size_t at = offset + index * 4;
            words[index] = (static_cast<std::uint32_t>(message[at]) << 24) |
                           (static_cast<std::uint32_t>(message[at + 1]) << 16) |
                           (static_cast<std::uint32_t>(message[at + 2]) << 8) |
                           static_cast<std::uint32_t>(message[at + 3]);
        }
        for (std::size_t index = 16; index < 64; ++index) {
            const std::uint32_t s0 = rotate(words[index - 15], 7) ^
                rotate(words[index - 15], 18) ^ (words[index - 15] >> 3);
            const std::uint32_t s1 = rotate(words[index - 2], 17) ^
                rotate(words[index - 2], 19) ^ (words[index - 2] >> 10);
            words[index] = words[index - 16] + s0 + words[index - 7] + s1;
        }
        std::uint32_t a=hash[0], b=hash[1], c=hash[2], d=hash[3];
        std::uint32_t e=hash[4], f=hash[5], g=hash[6], h=hash[7];
        for (std::size_t index = 0; index < 64; ++index) {

            const std::uint32_t s1 = rotate(e,6) ^ rotate(e,11) ^ rotate(e,25);
            const std::uint32_t choose = (e & f) ^ ((~e) & g);
            const std::uint32_t t1 = h + s1 + choose + k[index] + words[index];
            const std::uint32_t s0 = rotate(a,2) ^ rotate(a,13) ^ rotate(a,22);
            const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t t2 = s0 + majority;
            h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
        }
        hash[0]+=a; hash[1]+=b; hash[2]+=c; hash[3]+=d;
        hash[4]+=e; hash[5]+=f; hash[6]+=g; hash[7]+=h;
    }
    std::array<unsigned char, 32> result {};
    for (std::size_t index = 0; index < hash.size(); ++index) {
        result[index*4] = static_cast<unsigned char>(hash[index] >> 24);
        result[index*4+1] = static_cast<unsigned char>(hash[index] >> 16);
        result[index*4+2] = static_cast<unsigned char>(hash[index] >> 8);
        result[index*4+3] = static_cast<unsigned char>(hash[index]);
    }
    return result;
}

std::string exact_envelope_basename(const traced_regular & envelope) {
    constexpr unsigned char domain[] =
        "halofpx.registry-lab-registry-envelope.v1\0";
    std::vector<unsigned char> preimage(domain, domain + sizeof(domain) - 1);
    preimage.insert(preimage.end(), envelope.bytes.begin(), envelope.bytes.end());
    const auto digest = envelope_sha256(preimage.data(), preimage.size());
    constexpr char hex[] = "0123456789abcdef";
    std::string name = "e-";
    for (unsigned char byte : digest) {

        name.push_back(hex[byte >> 4]);
        name.push_back(hex[byte & 0x0f]);
    }
    name += ".cbor";
    return name;
}

bool decode_hex(const std::string & input, std::vector<unsigned char> & output) {
    if (input.empty() || input.size() > envelope_capacity * 2 ||
        (input.size() & 1U) != 0) return false;
    output.clear();
    output.reserve(input.size() / 2);
    const auto nibble = [](char value) -> int {
        if (value >= '0' && value <= '9') return value - '0';
        if (value >= 'a' && value <= 'f') return value - 'a' + 10;
        return -1;
    };
    for (std::size_t index = 0; index < input.size(); index += 2) {
        const int high = nibble(input[index]);
        const int low = nibble(input[index + 1]);
        if (high < 0 || low < 0) return false;
        output.push_back(static_cast<unsigned char>((high << 4) | low));
    }
    return true;
}

bool unique_json_string(const std::string & json, const char * key,
                        std::string & output) {
    const std::string token = std::string("\"") + key + "\": \"";
    const std::size_t first = json.find(token);
    if (first == std::string::npos || json.find(token, first + 1) != std::string::npos)
        return false;
    const std::size_t begin = first + token.size();
    const std::size_t end = json.find('"', begin);
    if (end == std::string::npos || end == begin) return false;
    output.assign(json, begin, end - begin);
    return true;
}

bool read_golden_authority(int fd, std::string & json) {
    struct stat observed {};
    if (::fstat(fd, &observed) != 0 || !S_ISREG(observed.st_mode) ||
        observed.st_size <= 0 || observed.st_size > 2 * 1024 * 1024) return false;
    json.assign(static_cast<std::size_t>(observed.st_size), '\0');
    std::size_t offset = 0;
    while (offset < json.size()) {
        ssize_t count;
        do { count = ::pread(fd, json.data() + offset, json.size() - offset,
                             static_cast<off_t>(offset)); } while (count < 0 && errno == EINTR);
        if (count <= 0) return false;
        offset += static_cast<std::size_t>(count);
    }
    char extra = 0;
    ssize_t eof = 0;
    do { eof = ::pread(fd, &extra, 1, static_cast<off_t>(json.size())); }
    while (eof < 0 && errno == EINTR);
    return eof == 0;
}

bool derive_golden_predecessor(int fd, traced_regular & envelope,
                               std::string & golden_json) {
    std::string envelope_hex;
    std::string digest_hex;
    if (!read_golden_authority(fd, golden_json) ||
        !unique_json_string(golden_json, "predecessor_registry_envelope_hex",
                            envelope_hex) ||
        !unique_json_string(golden_json,
                            "predecessor_registry_envelope_digest_hex",
                            digest_hex) || digest_hex.size() != 64 ||
        !decode_hex(envelope_hex, envelope.authority_bytes)) return false;
    traced_regular authority {};
    authority.bytes = envelope.authority_bytes;
    envelope.authority_name = exact_envelope_basename(authority);
    return digest_basename_shape(envelope.authority_name) &&
           envelope.authority_name.compare(2, 64, digest_hex) == 0;
}

bool golden_authority_unchanged(int fd, const struct stat & expected,
                                std::uint64_t expected_mount,
                                const std::string & expected_json) {
    struct stat observed {};
    std::uint64_t mount = 0;
    std::string json;
    return ::fstat(fd, &observed) == 0 && fd_mount_id(fd, mount) &&
           S_ISREG(observed.st_mode) && same_object(expected, observed) &&
           mount == expected_mount && observed.st_size == expected.st_size &&
           read_golden_authority(fd, json) && json == expected_json;
}

bool rename_marker_entry_l05z(pid_t pid, const tracee_state & state,
                              const pinned_directory & staging,
                              const struct stat & root, std::uint64_t root_mount) {
    return rename_marker_entry(pid, state, staging, root, root_mount);
}

bool rename_envelope_entry(pid_t pid, const tracee_state & state,
                           const pinned_directory & staging,
                           const pinned_directory & envelopes,
                           const traced_regular & envelope,
                           std::string & observed_name) {
    if (state.nr != SYS_renameat2 || state.args[4] != RENAME_NOREPLACE ||
        envelope.bytes.empty() || envelope.bytes != envelope.authority_bytes ||
        !envelope.exact_eof_seen ||
        envelope.exact_eof_completions != 2 || !envelope.readonly_alias_seen) return false;
    std::string old_name;
    std::string new_name;
    if (!read_tracee_string(pid, state.args[1], old_name) ||
        !read_tracee_digest_basename(pid, state.args[3], new_name) ||
        old_name != envelope_temporary_name || !digest_basename_shape(new_name) ||
        new_name != envelope.authority_name ||
        !exact_directory_fd(pid, static_cast<int>(state.args[0]), staging) ||
        !exact_directory_fd(pid, static_cast<int>(state.args[2]), envelopes)) return false;
    observed_name = new_name;
    return true;

}

bool selected_envelope_entry(pid_t pid, const tracee_state & state,
                             const envelope_boundary & point,
                             const std::array<pinned_directory, 3> & dirs,
                             const traced_regular & envelope,
                             std::string & published_name,
                             bool envelopes_fsync_succeeded) {
    switch (point.op) {
        case envelope_operation::open_temporary:
            return open_temporary_entry(pid, state, dirs[2], envelope_temporary_name);
        case envelope_operation::fchmod_temporary:
            return fchmod_entry(pid, state, envelope);
        case envelope_operation::pwrite_temporary:
            return pwrite_entry(pid, state, envelope) &&
                   envelope.write_occurrences == point.occurrence;
        case envelope_operation::fsync_temporary:
            return envelope.bytes == envelope.authority_bytes &&
                   envelope.exact_eof_seen &&
                   fsync_regular_entry(pid, state, envelope);
        case envelope_operation::publish_no_replace:
            return rename_envelope_entry(pid, state, dirs[2], dirs[0], envelope,
                                         published_name);
        case envelope_operation::fsync_envelopes_after_publish:
            return envelope.renamed && state.nr == SYS_fsync &&
                   exact_directory_fd(pid, static_cast<int>(state.args[0]), dirs[0]);
        case envelope_operation::fsync_staging_after_publish:
            return envelope.renamed && envelopes_fsync_succeeded &&
                   state.nr == SYS_fsync &&
                   exact_directory_fd(pid, static_cast<int>(state.args[0]), dirs[2]);
    }
    return false;
}


bool pin_regular_from_tracee(pid_t pid, int fd, std::uint64_t root_mount,
                             traced_regular & file) {
    struct stat observed {};
    std::uint64_t mount = 0;
    if (file.pinned || !tracee_fd_identity(pid, fd, observed, mount) ||
        !S_ISREG(observed.st_mode) || observed.st_nlink != 1 || observed.st_size != 0 ||
        observed.st_uid != ::geteuid() || mount != root_mount) return false;
    file.pinned = true;
    file.identity = observed;
    file.mount_id = mount;
    file.child_fd = fd;
    return true;
}

bool exact_retained_regular(int directory_fd, const char * name,
                            const traced_regular & file, mode_t mode) {
    if (!file.pinned) return false;
    int fd = -1;
    const bool metadata_only = mode == 0 && file.bytes.empty();
    if (!open_named(directory_fd, name, metadata_only ? O_PATH : O_RDONLY, fd))
        return false;
    struct stat observed {};
    std::uint64_t mount = 0;
    std::vector<unsigned char> bytes(file.bytes.size());
    std::size_t offset = 0;
    while (!metadata_only && offset < bytes.size()) {
        ssize_t count;
        do { count = ::pread(fd, bytes.data() + offset, bytes.size() - offset,
                             static_cast<off_t>(offset)); } while (count < 0 && errno == EINTR);
        if (count <= 0) break;
        offset += static_cast<std::size_t>(count);

    }
    unsigned char extra = 0;
    ssize_t eof = 0;
    if (!metadata_only) {
        do { eof = ::pread(fd, &extra, 1, static_cast<off_t>(bytes.size())); }
        while (eof < 0 && errno == EINTR);
    }
    const bool valid = ::fstat(fd, &observed) == 0 && fd_mount_id(fd, mount) &&
        S_ISREG(observed.st_mode) && same_object(file.identity, observed) &&
        mount == file.mount_id && observed.st_nlink == 1 &&
        observed.st_uid == ::geteuid() && (observed.st_mode & 07777) == mode &&
        observed.st_size == static_cast<off_t>(file.bytes.size()) &&
        offset == bytes.size() && eof == 0 && bytes == file.bytes;
    const bool closed = ::close(fd) == 0;
    return valid && closed;
}

bool envelope_root_inventory(int root_fd, const struct stat & writer,
                             const std::array<pinned_directory, 3> & dirs,
                             const envelope_boundary & point, phase when,
                             mode_t tracee_umask,
                             const traced_regular & marker,
                             const traced_regular & envelope,
                             const std::string & published_name) {
    if (envelope.authority_bytes.empty() ||
        envelope.bytes.size() > envelope.authority_bytes.size() ||
        !std::equal(envelope.bytes.begin(), envelope.bytes.end(),
                    envelope.authority_bytes.begin()) ||
        !marker.renamed || marker.bytes.empty() || !marker.exact_eof_seen ||
        !exact_names(root_fd, { "attempts", "envelopes", "root.marker", "staging", "writer.lock" }) ||
        !exact_retained_regular(root_fd, marker_published_name, marker, 0600)) return false;
    int directory_fds[3] { -1, -1, -1 };
    bool valid = true;
    for (int index = 0; index < 3; ++index) {
        directory_fds[index] = -1;
        if (!open_named(root_fd, directory_names[static_cast<std::size_t>(index)],

                        O_RDONLY | O_DIRECTORY, directory_fds[index])) {
            valid = false;
            break;
        }
        struct stat observed {};
        std::uint64_t mount = 0;
        if (!dirs[static_cast<std::size_t>(index)].pinned ||
            ::fstat(directory_fds[index], &observed) != 0 ||
            !fd_mount_id(directory_fds[index], mount) || !S_ISDIR(observed.st_mode) ||
            !same_object(dirs[static_cast<std::size_t>(index)].identity, observed) ||
            mount != dirs[static_cast<std::size_t>(index)].mount_id ||
            observed.st_uid != ::geteuid() || (observed.st_mode & 07777) != 0700) {
            valid = false;
            break;
        }
    }
    const bool before_create = point.op == envelope_operation::open_temporary &&
                               when == phase::entry;
    const bool published = envelope.renamed;
    if (valid) {
        valid = exact_names(directory_fds[0], published
            ? std::vector<std::string>{ published_name } : std::vector<std::string>{}) &&
            exact_names(directory_fds[1], {}) &&
            exact_names(directory_fds[2], !before_create && !published
                ? std::vector<std::string>{ envelope_temporary_name }
                : std::vector<std::string>{});
    }
    mode_t envelope_mode = 0600;
    if ((point.op == envelope_operation::open_temporary && when == phase::exit) ||
        (point.op == envelope_operation::fchmod_temporary && when == phase::entry)) {

        envelope_mode = static_cast<mode_t>(0600 & ~tracee_umask);
    }
    if (valid && !before_create) {
        const int directory_fd = published ? directory_fds[0] : directory_fds[2];
        const char * name = published ? published_name.c_str() : envelope_temporary_name;
        valid = !published ||
            (envelope.bytes == envelope.authority_bytes &&
             digest_basename_shape(published_name) &&
             published_name == envelope.authority_name);
        valid = valid && exact_retained_regular(directory_fd, name, envelope, envelope_mode);
    }
    for (int & fd : directory_fds) {
        if (fd >= 0 && ::close(fd) != 0) valid = false;
    }
    int writer_fd = -1;
    if (!open_named(root_fd, "writer.lock", O_RDONLY, writer_fd)) return false;
    struct stat writer_now {};
    const bool writer_ok = ::fstat(writer_fd, &writer_now) == 0 &&
        S_ISREG(writer_now.st_mode) && same_object(writer, writer_now) &&
        writer_now.st_nlink == 1 && writer_now.st_size == 0 &&
        writer_now.st_uid == ::geteuid() && (writer_now.st_mode & 07777) == 0600;
    const bool writer_closed = ::close(writer_fd) == 0;
    return valid && writer_ok && writer_closed &&
           (before_create || (envelope.pinned && envelope.bytes.size() <= envelope_capacity));
}

int envelope_controller(const envelope_options & input) {
    std::string receipt_parent_path;
    std::string receipt_name;
    if (!canonical_existing_path(input.target) || !canonical_existing_path(input.golden) ||
        !canonical_existing_path(input.parent) || !canonical_existing_path(input.root) ||
        !canonical_existing_path(input.fixture) || !direct_child(input.root, input.parent) ||

        !direct_child(input.fixture, input.parent) ||
        !split_receipt_path(input.receipt, receipt_parent_path, receipt_name) ||
        !canonical_existing_path(receipt_parent_path.c_str()) ||
        same_or_descendant(receipt_parent_path.c_str(), input.parent)) {
        std::fprintf(stderr, "non-canonical authority or non-disposable receipt scope\n");
        return 2;
    }
    receipt_ok = true;
    receipt_sequence = 0;
    const int receipt_parent_fd = open_absolute_directory_no_symlink(receipt_parent_path.c_str());
    struct stat receipt_parent_stat {};
    std::uint64_t receipt_parent_mount = 0;
    struct open_how receipt_how {};
    receipt_how.flags = O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW;
    receipt_how.mode = 0600;
    receipt_how.resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
                          RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
    if (receipt_parent_fd < 0 || ::fstat(receipt_parent_fd, &receipt_parent_stat) != 0 ||
        !S_ISDIR(receipt_parent_stat.st_mode) || receipt_parent_stat.st_uid != ::geteuid() ||
        (receipt_parent_stat.st_mode & 07777) != 0700 ||
        !fd_mount_id(receipt_parent_fd, receipt_parent_mount)) {
        if (receipt_parent_fd >= 0) (void) ::close(receipt_parent_fd);
        return 2;
    }
    do {
        receipt_fd = static_cast<int>(::syscall(SYS_openat2, receipt_parent_fd,
            receipt_name.c_str(), &receipt_how, sizeof(receipt_how)));
    } while (receipt_fd < 0 && errno == EINTR);
    if (receipt_fd < 0 || ::fchmod(receipt_fd, 0600) != 0) {
        if (receipt_fd >= 0) (void) ::close(receipt_fd);

        receipt_fd = -1;
        (void) ::close(receipt_parent_fd);
        return 2;
    }

    traced_regular envelope {};
    std::string golden_json;
    const int golden_fd = ::open(input.golden, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    const int target_fd = ::open(input.target, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    const int parent_fd = ::open(input.parent, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    const int root_fd = ::open(input.root, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    const int fixture_fd = ::open(input.fixture, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    const int fixture_lock_fd = fixture_fd >= 0
        ? ::openat(fixture_fd, "primitive.lock", O_RDONLY | O_CLOEXEC | O_NOFOLLOW) : -1;
    struct stat golden_stat {}, target_stat {}, parent_stat {}, root_stat {}, fixture_stat {}, fixture_lock_stat {};
    std::uint64_t golden_mount = 0, parent_mount = 0, root_mount = 0, fixture_mount = 0;
    struct statfs root_fs {};
    if (golden_fd < 0 || ::fstat(golden_fd, &golden_stat) != 0 ||
        !S_ISREG(golden_stat.st_mode) || !fd_mount_id(golden_fd, golden_mount) ||
        !derive_golden_predecessor(golden_fd, envelope, golden_json) ||
        target_fd < 0 || ::fstat(target_fd, &target_stat) != 0 ||
        !S_ISREG(target_stat.st_mode) || (target_stat.st_mode & 0111) == 0 ||
        parent_fd < 0 || ::fstat(parent_fd, &parent_stat) != 0 ||
        !fd_mount_id(parent_fd, parent_mount) || !S_ISDIR(parent_stat.st_mode) ||
        parent_stat.st_uid != ::geteuid() || (parent_stat.st_mode & 07777) != 0700 ||
        root_fd < 0 || ::fstat(root_fd, &root_stat) != 0 ||
        !fd_mount_id(root_fd, root_mount) || !S_ISDIR(root_stat.st_mode) ||
        root_stat.st_uid != ::geteuid() || (root_stat.st_mode & 07777) != 0700 ||
        ::fstatfs(root_fd, &root_fs) != 0 ||
        static_cast<unsigned long>(root_fs.f_type) !=
            static_cast<unsigned long>(BTRFS_SUPER_MAGIC) || !directory_empty(root_fd) ||
        fixture_fd < 0 || ::fstat(fixture_fd, &fixture_stat) != 0 ||
        !fd_mount_id(fixture_fd, fixture_mount) || !S_ISDIR(fixture_stat.st_mode) ||
        fixture_stat.st_uid != ::geteuid() || (fixture_stat.st_mode & 07777) != 0700 ||
        fixture_lock_fd < 0 || ::fstat(fixture_lock_fd, &fixture_lock_stat) != 0 ||
        !S_ISREG(fixture_lock_stat.st_mode) || fixture_lock_stat.st_nlink != 1 ||

        fixture_lock_stat.st_size != 0 || fixture_lock_stat.st_uid != ::geteuid() ||
        (fixture_lock_stat.st_mode & 07777) != 0600) {
        emit("\"event\":\"invalid-target\"");
        if (fixture_lock_fd >= 0) (void) ::close(fixture_lock_fd);
        if (fixture_fd >= 0) (void) ::close(fixture_fd);
        if (root_fd >= 0) (void) ::close(root_fd);
        if (parent_fd >= 0) (void) ::close(parent_fd);
        if (target_fd >= 0) (void) ::close(target_fd);
        if (golden_fd >= 0) (void) ::close(golden_fd);
        return finalize_receipt(receipt_parent_fd, receipt_parent_stat,
                                receipt_parent_mount, receipt_name, 2);
    }
    (void) parent_mount;
    (void) ::close(fixture_lock_fd);
    if (!emit(std::string("\"event\":\"start\",\"boundary\":\"") +
              envelope_boundary_name(input.point) + "\",\"phase\":\"" +
              (input.when == phase::entry ? "entry" : "exit") +
              "\",\"occurrence\":" + std::to_string(input.point.occurrence) +
              ",\"classification\":\"whole-root-discard-only\"")) {
        (void) ::close(target_fd); (void) ::close(golden_fd); (void) ::close(fixture_fd);
        (void) ::close(root_fd); (void) ::close(parent_fd);
        return finalize_receipt(receipt_parent_fd, receipt_parent_stat,
                                receipt_parent_mount, receipt_name, 2);
    }

    const pid_t launcher = ::fork();
    if (launcher == 0) {
        (void) ::umask(input.tracee_umask);
        if (::ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) != 0) _exit(125);
        if (::raise(SIGSTOP) != 0) _exit(125);
        char executable[64];

        const int count = std::snprintf(executable, sizeof(executable),
                                        "/proc/self/fd/%d", target_fd);
        if (count <= 0 || static_cast<std::size_t>(count) >= sizeof(executable)) _exit(125);
        ::execl(executable, input.target, "--live-envelope-controller",
                input.golden, input.parent, input.root, input.fixture,
                static_cast<char *>(nullptr));
        _exit(127);
    }
    (void) ::close(target_fd);
    if (launcher < 0) {
        emit("\"event\":\"fork-failed\"");
        (void) ::close(golden_fd); (void) ::close(fixture_fd);
        (void) ::close(root_fd); (void) ::close(parent_fd);
        return finalize_receipt(receipt_parent_fd, receipt_parent_stat,
                                receipt_parent_mount, receipt_name, 2);
    }
    int status = 0;
    if (::waitpid(launcher, &status, 0) != launcher || !WIFSTOPPED(status)) {
        emit("\"event\":\"initial-stop-failed\"");
        (void) ::kill(launcher, SIGKILL); (void) ::waitpid(launcher, nullptr, 0);
        (void) ::close(golden_fd); (void) ::close(fixture_fd);
        (void) ::close(root_fd); (void) ::close(parent_fd);
        return finalize_receipt(receipt_parent_fd, receipt_parent_stat,
                                receipt_parent_mount, receipt_name, 2);
    }
    constexpr long ptrace_options = PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK |
        PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE | PTRACE_O_TRACEEXEC | PTRACE_O_EXITKILL;
    if (::ptrace(PTRACE_SETOPTIONS, launcher, nullptr,
                 reinterpret_cast<void *>(ptrace_options)) != 0 || !resume_syscalls(launcher)) {
        emit("\"event\":\"ptrace-setup-failed\"");
        (void) ::kill(launcher, SIGKILL); (void) ::waitpid(launcher, nullptr, 0);
        (void) ::close(golden_fd); (void) ::close(fixture_fd);
        (void) ::close(root_fd); (void) ::close(parent_fd);

        return finalize_receipt(receipt_parent_fd, receipt_parent_stat,
                                receipt_parent_mount, receipt_name, 2);
    }

    std::unordered_map<pid_t, tracee_state> tracees;
    std::unordered_map<pid_t, bool> selected_at_entry;
    tracees.emplace(launcher, tracee_state {});
    std::array<pinned_directory, 3> dirs {};
    traced_regular marker {};
    std::string published_name;
    pid_t live_child = -1;
    int writer_fd_number = -1, fixture_child_fd_number = -1;
    struct stat writer_identity {};
    std::uint64_t writer_mount = 0;
    bool writer_pinned = false, fixture_child_pinned = false;
    bool fixture_child_lock_acquired = false, writer_child_lock_acquired = false;
    bool injected = false, target_sigkilled = false;
    bool launcher_exited = false, launcher_wifexited = false, launcher_exec_seen = false;
    int launcher_exit = -1;
    bool controller_error = false, bounded_process_cleanup = false;
    bool envelopes_fsync_succeeded = false;
    std::vector<unsigned char> pending_write;
    traced_regular * pending_write_file = nullptr;
    traced_regular * pending_read_file = nullptr;
    std::size_t pending_read_offset = 0, pending_read_size = 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(25);

    while (!tracees.empty() && std::chrono::steady_clock::now() < deadline) {
        const pid_t pid = ::waitpid(-1, &status, __WALL | WNOHANG);
        if (pid == 0) {

            const struct timespec pause { 0, 1000000 };
            (void) ::nanosleep(&pause, nullptr);
            continue;
        }
        if (pid < 0) {
            if (errno == EINTR) continue;
            if (errno == ECHILD) break;
            controller_error = true;
            break;
        }
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            if (pid == live_child && WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL) {
                target_sigkilled = true;
                emit("\"event\":\"target-exit\",\"wifsignaled\":true,\"signal\":9");
            } else if (pid == launcher) {
                launcher_exited = true;
                launcher_wifexited = WIFEXITED(status);
                launcher_exit = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
                emit(std::string("\"event\":\"launcher-exit\",\"wifexited\":") +
                    (launcher_wifexited ? "true" : "false") + ",\"status\":" +
                    std::to_string(launcher_exit));
            }
            tracees.erase(pid);
            selected_at_entry.erase(pid);
            continue;
        }
        if (!WIFSTOPPED(status)) continue;
        const int stop_signal = WSTOPSIG(status);
        const unsigned event = static_cast<unsigned>(status) >> 16;
        if (stop_signal == SIGTRAP && event != 0) {

            if (event == PTRACE_EVENT_FORK || event == PTRACE_EVENT_VFORK ||
                event == PTRACE_EVENT_CLONE) {
                unsigned long child_value = 0;
                if (::ptrace(PTRACE_GETEVENTMSG, pid, nullptr, &child_value) != 0) {
                    controller_error = true;
                    break;
                }
                auto & child_state = tracees[static_cast<pid_t>(child_value)];
                child_state.initial_sigstop_expected = !child_state.initial_sigstop_observed;
                emit(std::string("\"event\":\"descendant\",\"parent\":") +
                     std::to_string(pid) + ",\"pid\":" + std::to_string(child_value));
            } else if (event == PTRACE_EVENT_EXEC && pid == launcher) {
                if (launcher_exec_seen || !same_executable(pid, target_stat)) {
                    controller_error = true;
                    break;
                }
                launcher_exec_seen = true;
            } else if (event == PTRACE_EVENT_EXEC) {
                if (live_child != -1 || !same_executable(pid, target_stat) ||
                    !exact_live_envelope_child_argv(pid)) {
                    controller_error = true;
                    break;
                }
                live_child = pid;
                tracees[pid].live_child = true;
                emit(std::string("\"event\":\"live-child-exec\",\"pid\":") +
                     std::to_string(pid));
            }
            if (!resume_syscalls(pid)) { controller_error = true; break; }
            continue;

        }
        if (stop_signal == (SIGTRAP | 0x80)) {
            struct __ptrace_syscall_info info {};
            const long info_size = ::ptrace(PTRACE_GET_SYSCALL_INFO, pid, sizeof(info), &info);
            if (info_size < 0 || info.arch != AUDIT_ARCH_X86_64) {
                controller_error = true;
                break;
            }
            auto & state = tracees[pid];
            if (info.op == PTRACE_SYSCALL_INFO_ENTRY) {
                state.have_entry = true;
                state.nr = info.entry.nr;
                std::memcpy(state.args, info.entry.args, sizeof(state.args));
                selected_at_entry[pid] = false;
                if (state.live_child) {
                    emit(std::string("\"event\":\"syscall-entry\",\"pid\":") +
                         std::to_string(pid) + ",\"nr\":" + std::to_string(state.nr) +
                         ",\"arg0\":" + std::to_string(state.args[0]) +
                         ",\"arg1\":" + std::to_string(state.args[1]) +
                         ",\"arg2\":" + std::to_string(state.args[2]) +
                         ",\"arg3\":" + std::to_string(state.args[3]) +
                         ",\"arg4\":" + std::to_string(state.args[4]) +
                         ",\"arg5\":" + std::to_string(state.args[5]));
                    if (!envelope.renamed &&
                        envelope.exact_eof_completions == 2 &&
                        name_based_syscall(state.nr)) {
                        std::string expected_publication_name;
                        if (!rename_envelope_entry(pid, state, dirs[2], dirs[0],
                                                   envelope,
                                                   expected_publication_name)) {
                            controller_error = true;
                            break;
                        }
                    }
                    pending_write_file = nullptr;
                    if (pwrite_entry(pid, state, marker)) pending_write_file = &marker;
                    else if (pwrite_entry(pid, state, envelope)) pending_write_file = &envelope;
                    if (pending_write_file != nullptr) {
                        ++pending_write_file->write_occurrences;
                        pending_write.resize(static_cast<std::size_t>(state.args[2]));
                        if (!read_tracee(pid, state.args[1], pending_write.data(),

                                         pending_write.size())) {
                            controller_error = true;
                            break;
                        }
                        if (pending_write_file == &envelope &&
                            (state.args[3] > envelope.authority_bytes.size() ||
                             pending_write.size() >
                                 envelope.authority_bytes.size() -
                                     static_cast<std::size_t>(state.args[3]) ||
                             !std::equal(pending_write.begin(), pending_write.end(),
                                 envelope.authority_bytes.begin() +
                                     static_cast<std::ptrdiff_t>(state.args[3])))) {
                            controller_error = true;
                            break;
                        }
                    }
                    pending_read_file = nullptr;
                    if (state.nr == SYS_pread64) {
                        if (regular_fd_alias(pid, static_cast<int>(state.args[0]), marker))
                            pending_read_file = &marker;
                        else if (regular_fd_alias(pid, static_cast<int>(state.args[0]), envelope))
                            pending_read_file = &envelope;
                        if (pending_read_file != nullptr) {
                            if (state.args[2] > envelope_capacity || state.args[3] > envelope_capacity) {
                                controller_error = true;
                                break;
                            }
                            pending_read_offset = static_cast<std::size_t>(state.args[3]);
                            pending_read_size = static_cast<std::size_t>(state.args[2]);
                            if (pending_read_file == &envelope &&
                                tracee_fd_is_readonly(pid,
                                    static_cast<int>(state.args[0]))) {
                                envelope.readonly_alias_seen = true;
                            }
                        }
                    }
                    const bool selected = selected_envelope_entry(
                        pid, state, input.point, dirs, envelope, published_name,
                        envelopes_fsync_succeeded);
                    selected_at_entry[pid] = selected;
                    if (selected) {
                        emit(std::string("\"event\":\"boundary-entry\",\"name\":\"") +
                             envelope_boundary_name(input.point) +
                             "\",\"occurrence\":" +
                             std::to_string(envelope.write_occurrences));
                        if (input.when == phase::entry) {
                            injected = ::kill(pid, SIGKILL) == 0;

                            emit(std::string("\"event\":\"inject\",\"ok\":") +
                                 (injected ? "true" : "false"));
                        }
                    }
                }
            } else if (info.op == PTRACE_SYSCALL_INFO_EXIT) {
                if (state.live_child && state.have_entry) {
                    emit(std::string("\"event\":\"syscall-exit\",\"pid\":") +
                         std::to_string(pid) + ",\"nr\":" + std::to_string(state.nr) +
                         ",\"rval\":" + std::to_string(info.exit.rval) +
                         ",\"is_error\":" + (info.exit.is_error ? "true" : "false"));
                    if (state.nr == SYS_openat2 && writer_openat2_entry(
                            pid, state, root_stat, root_mount) &&
                        !info.exit.is_error && info.exit.rval >= 0) {
                        writer_fd_number = static_cast<int>(info.exit.rval);
                        if (!tracee_fd_identity(pid, writer_fd_number, writer_identity,
                                               writer_mount) || writer_mount != root_mount) {
                            controller_error = true;
                            break;
                        }
                        writer_pinned = true;
                    }
                    if (state.nr == SYS_openat2 && fixture_child_fd_number < 0 &&
                        fixture_openat2_entry(pid, state, fixture_stat, fixture_mount) &&
                        !info.exit.is_error && info.exit.rval >= 0) {
                        fixture_child_fd_number = static_cast<int>(info.exit.rval);
                        struct stat observed {};
                        std::uint64_t mount = 0;
                        if (!tracee_fd_identity(pid, fixture_child_fd_number, observed, mount) ||
                            !same_object(fixture_lock_stat, observed) || mount != fixture_mount) {

                            controller_error = true;
                            break;
                        }
                        fixture_child_pinned = true;
                    }
                    if (ofd_write_lock_entry(pid, state, fixture_child_fd_number,
                                             fixture_lock_stat, fixture_mount) &&
                        !info.exit.is_error && info.exit.rval == 0) fixture_child_lock_acquired = true;
                    if (ofd_write_lock_entry(pid, state, writer_fd_number,
                                             writer_identity, writer_mount) &&
                        !info.exit.is_error && info.exit.rval == 0) writer_child_lock_acquired = true;
                    if (state.nr == SYS_mkdirat && !info.exit.is_error && info.exit.rval == 0) {
                        std::string name;
                        if (read_tracee_string(pid, state.args[1], name)) {
                            for (int index = 0; index < 3; ++index) {
                                if (name == directory_names[static_cast<std::size_t>(index)] &&
                                    !pin_created_directory(root_fd, index,
                                        static_cast<mode_t>(0700 & ~input.tracee_umask),
                                        dirs[static_cast<std::size_t>(index)])) {
                                    controller_error = true;
                                    break;
                                }
                            }
                        }
                        if (controller_error) break;
                    }
                    if (state.nr == SYS_openat2 && !info.exit.is_error && info.exit.rval >= 0) {
                        if (open_temporary_entry(pid, state, dirs[2], marker_temporary_name) &&
                            !pin_regular_from_tracee(pid, static_cast<int>(info.exit.rval),
                                                    root_mount, marker)) {

                            controller_error = true;
                            break;
                        }
                        if (open_temporary_entry(pid, state, dirs[2], envelope_temporary_name) &&
                            !pin_regular_from_tracee(pid, static_cast<int>(info.exit.rval),
                                                    root_mount, envelope)) {
                            controller_error = true;
                            break;
                        }
                    }
                    if (state.nr == SYS_pwrite64 && pending_write_file != nullptr &&
                        exact_regular_fd(pid, static_cast<int>(state.args[0]), *pending_write_file)) {
                        if (info.exit.is_error || info.exit.rval <= 0 ||
                            static_cast<std::uint64_t>(info.exit.rval) > state.args[2] ||
                            state.args[3] != pending_write_file->bytes.size() ||
                            pending_write.size() != state.args[2]) {
                            controller_error = true;
                            break;
                        }
                        pending_write_file->bytes.insert(pending_write_file->bytes.end(),
                            pending_write.begin(), pending_write.begin() +
                                static_cast<std::ptrdiff_t>(info.exit.rval));
                    }
                    if (state.nr == SYS_pread64 && pending_read_file != nullptr &&
                        regular_fd_alias(pid, static_cast<int>(state.args[0]), *pending_read_file) &&
                        pending_read_offset == state.args[3] && pending_read_size == state.args[2] &&
                        !info.exit.is_error && info.exit.rval >= 0) {
                        if (info.exit.rval == 0) {
                            if (pending_read_offset == pending_read_file->bytes.size()) {
                                pending_read_file->exact_eof_seen = true;
                                ++pending_read_file->exact_eof_completions;
                            }

                        } else {
                            const std::size_t count = static_cast<std::size_t>(info.exit.rval);
                            std::vector<unsigned char> observed(count);
                            if (count > pending_read_size ||
                                pending_read_offset + count > pending_read_file->bytes.size() ||
                                !read_tracee(pid, state.args[1], observed.data(), count) ||
                                !std::equal(observed.begin(), observed.end(),
                                    pending_read_file->bytes.begin() +
                                        static_cast<std::ptrdiff_t>(pending_read_offset))) {
                                controller_error = true;
                                break;
                            }
                        }
                    }
                    if (state.nr == SYS_renameat2 && !info.exit.is_error && info.exit.rval == 0) {
                        if (rename_marker_entry_l05z(pid, state, dirs[2], root_stat, root_mount)) {
                            marker.renamed = true;
                        } else {
                            std::string observed_name;
                            if (rename_envelope_entry(pid, state, dirs[2], dirs[0], envelope,
                                                      observed_name)) {
                                envelope.renamed = true;
                                published_name = observed_name;
                            }
                        }
                    }
                    if (state.nr == SYS_fsync && envelope.renamed &&
                        exact_directory_fd(pid, static_cast<int>(state.args[0]), dirs[0]) &&
                        !info.exit.is_error && info.exit.rval == 0) {
                        envelopes_fsync_succeeded = true;
                    }
                    if (state.nr == SYS_fsync && envelope.renamed &&
                        exact_directory_fd(pid, static_cast<int>(state.args[0]), dirs[2]) &&
                        !envelopes_fsync_succeeded) {
                        controller_error = true;
                        break;
                    }
                    const bool selected = selected_at_entry[pid];
                    bool successful = !info.exit.is_error && info.exit.rval == 0;
                    if (input.point.op == envelope_operation::open_temporary)
                        successful = !info.exit.is_error && info.exit.rval >= 0;

                    if (input.point.op == envelope_operation::pwrite_temporary)
                        successful = !info.exit.is_error && info.exit.rval > 0;
                    if (selected && input.when == phase::exit && successful) {
                        emit(std::string("\"event\":\"boundary-exit\",\"name\":\"") +
                             envelope_boundary_name(input.point) + "\"");
                        injected = ::kill(pid, SIGKILL) == 0;
                        emit(std::string("\"event\":\"inject\",\"ok\":") +
                             (injected ? "true" : "false"));
                    }
                }
                state.have_entry = false;
            } else {
                controller_error = true;
                break;
            }
            if (!resume_syscalls(pid) && !(injected && errno == ESRCH)) controller_error = true;
            continue;
        }
        auto iterator = tracees.find(pid);
        if (stop_signal == SIGSTOP && iterator == tracees.end()) {
            tracee_state newborn {};
            newborn.initial_sigstop_observed = true;
            tracees.emplace(pid, newborn);
            if (!resume_syscalls(pid)) controller_error = true;
            continue;
        }
        if (stop_signal == SIGSTOP && iterator != tracees.end() &&
            iterator->second.initial_sigstop_expected) {
            iterator->second.initial_sigstop_expected = false;
            iterator->second.initial_sigstop_observed = true;

            if (!resume_syscalls(pid)) controller_error = true;
            continue;
        }
        if (stop_signal == SIGSTOP || stop_signal == SIGTRAP) {
            controller_error = true;
            break;
        }
        if (!resume_syscalls(pid, stop_signal) && !(injected && errno == ESRCH)) {
            controller_error = true;
        }
    }
    if (!tracees.empty()) {
        bounded_process_cleanup = true;
        emit("\"event\":\"bounded-process-cleanup\"");
        for (const auto & item : tracees) (void) ::kill(item.first, SIGKILL);
        const auto reap_deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (!tracees.empty() && std::chrono::steady_clock::now() < reap_deadline) {
            const pid_t pid = ::waitpid(-1, &status, __WALL | WNOHANG);
            if (pid > 0) tracees.erase(pid);
            else if (pid == 0) {
                const struct timespec pause { 0, 1000000 };
                (void) ::nanosleep(&pause, nullptr);
            } else if (errno != EINTR) break;
        }
    }

    struct stat root_now {}, fixture_now {};
    std::uint64_t root_mount_now = 0, fixture_mount_now = 0;
    const bool authorities_pinned = ::fstat(root_fd, &root_now) == 0 &&

        fd_mount_id(root_fd, root_mount_now) && ::fstat(fixture_fd, &fixture_now) == 0 &&
        fd_mount_id(fixture_fd, fixture_mount_now) && same_object(root_stat, root_now) &&
        root_mount_now == root_mount && root_now.st_uid == root_stat.st_uid &&
        (root_now.st_mode & 07777) == (root_stat.st_mode & 07777) &&
        same_object(fixture_stat, fixture_now) && fixture_mount_now == fixture_mount &&
        fixture_now.st_uid == fixture_stat.st_uid &&
        (fixture_now.st_mode & 07777) == (fixture_stat.st_mode & 07777);
    const bool golden_pinned = golden_authority_unchanged(
        golden_fd, golden_stat, golden_mount, golden_json);
    const bool inventory_exact = authorities_pinned && golden_pinned &&
        writer_pinned &&
        envelope_root_inventory(root_fd, writer_identity, dirs, input.point, input.when,
                                input.tracee_umask, marker, envelope, published_name);
    const bool fixture_released = prove_ofd_released(fixture_fd, "primitive.lock",
                                                     fixture_lock_stat);
    const bool writer_released = writer_pinned &&
        prove_ofd_released(root_fd, "writer.lock", writer_identity);
    emit(std::string("\"event\":\"post-crash\",\"authorities_pinned\":") +
        (authorities_pinned ? "true" : "false") + ",\"inventory_exact\":" +
        (inventory_exact ? "true" : "false") + ",\"fixture_lock_released\":" +
        (fixture_released ? "true" : "false") + ",\"writer_lock_released\":" +
        (writer_released ? "true" : "false") + ",\"marker_size\":" +
        std::to_string(marker.bytes.size()) + ",\"envelope_size\":" +
        std::to_string(envelope.bytes.size()) +
        ",\"classification\":\"whole-root-discard-only\"");
    emit(std::string("\"event\":\"identity-inventory\",\"root_device\":") +
        std::to_string(static_cast<std::uint64_t>(root_stat.st_dev)) +
        ",\"root_inode\":" + std::to_string(static_cast<std::uint64_t>(root_stat.st_ino)) +
        ",\"root_mount_id\":" + std::to_string(root_mount) +
        ",\"marker_pinned\":" + (marker.pinned ? "true" : "false") +
        ",\"marker_inode\":" + std::to_string(marker.pinned
            ? static_cast<std::uint64_t>(marker.identity.st_ino) : 0) +
        ",\"envelope_pinned\":" + (envelope.pinned ? "true" : "false") +

        ",\"envelope_inode\":" + std::to_string(envelope.pinned
            ? static_cast<std::uint64_t>(envelope.identity.st_ino) : 0) +
        ",\"envelope_mount_id\":" + std::to_string(envelope.mount_id) +
        ",\"envelope_published\":" + (envelope.renamed ? "true" : "false") +
        ",\"envelope_name\":\"" + published_name + "\"" +
        ",\"pwrite_occurrences\":" + std::to_string(envelope.write_occurrences));
    const bool pass = !controller_error && receipt_ok && injected && target_sigkilled &&
        launcher_exec_seen && !bounded_process_cleanup && launcher_exited &&
        launcher_wifexited && launcher_exit != 0 && tracees.empty() &&
        fixture_child_pinned && fixture_child_lock_acquired && writer_child_lock_acquired &&
        inventory_exact && fixture_released && writer_released;
    emit(std::string("\"event\":\"summary\",\"pass\":") +
        (pass ? "true" : "false") + ",\"target_wifsignaled\":" +
        (target_sigkilled ? "true" : "false") + ",\"launcher_status\":" +
        std::to_string(launcher_exit) +
        ",\"classification\":\"whole-root-discard-only\"");
    const bool golden_closed = ::close(golden_fd) == 0;
    const bool fixture_closed = ::close(fixture_fd) == 0;
    const bool root_closed = ::close(root_fd) == 0;
    const bool parent_closed = ::close(parent_fd) == 0;
    const int intended = pass && golden_closed && fixture_closed &&
        root_closed && parent_closed ? 0 : 1;
    return finalize_receipt(receipt_parent_fd, receipt_parent_stat,
                            receipt_parent_mount, receipt_name, intended);
}

} // namespace

int main(int argc, char ** argv) {
    if (!digest_basename_reader_self_test()) {
        std::fprintf(stderr, "L05z digest-basename reader self-test failed\n");
        return 2;
    }
    envelope_options input {};
    if (!parse_envelope(argc, argv, input)) envelope_usage(argv[0]);
    return envelope_controller(input);

}
