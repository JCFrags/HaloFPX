#pragma once

#include "halofpx-context-store-protected-registry-successor.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace halofpx {

constexpr size_t context_store_registry_lab_wire_max_bytes = 4096;
constexpr size_t context_store_registry_lab_credential_max_bytes = 180;

enum class context_store_registry_lab_kind : uint8_t {
    root = 0, head = 1, prepare = 2, close = 3, abort_record = 4, quarantine = 5,
};

enum class context_store_registry_lab_wire_status : uint8_t {
    authenticated_semantic_only,
    structural_rejection,
    invalid_credential,
    authentication_failed,
    semantic_rejection,
};

struct context_store_registry_lab_credential {
    context_store_registry_lab_credential() noexcept = default;
    ~context_store_registry_lab_credential() noexcept;
    context_store_registry_lab_credential(const context_store_registry_lab_credential &) = delete;
    context_store_registry_lab_credential & operator=(const context_store_registry_lab_credential &) = delete;
    context_store_registry_lab_credential(context_store_registry_lab_credential && other) noexcept;
    context_store_registry_lab_credential & operator=(context_store_registry_lab_credential && other) noexcept;
    void clear() noexcept;

    context_store_registered_id key_id;
    uint64_t generation = 0;
    std::array<uint8_t, 32> secret {};
};

struct context_store_registry_lab_expectation {
    context_store_format_digest root_id {};
    context_store_format_digest path_policy_commitment {};
    std::array<uint8_t, 16> store_uuid {};
    std::array<uint8_t, 16> filesystem_uuid {};
    context_store_registered_id registry_id;
    uint64_t registry_epoch = 0;
    uint64_t predecessor_high_water = 0;
    uint64_t predecessor_selector_generation = 0;
    uint64_t successor_selector_generation = 0;
    uint64_t mount_id = 0;
    uint64_t owner_uid = 0;
    uint64_t lock_st_dev = 0;
    uint64_t lock_st_ino = 0;
    context_store_format_digest initial_head_digest {};
    context_store_format_digest successor_head_digest {};
    context_store_format_digest predecessor_envelope_digest {};
    context_store_format_digest successor_envelope_digest {};
    context_store_format_digest prepare_digest {};
    context_store_format_digest operation_commitment {};
    context_store_format_digest attempt_id {};
    context_store_format_digest quarantine_event_id {};
    context_store_format_digest quarantine_previous_record_digest {};
    context_store_format_digest quarantine_head_digest {};
    bool quarantine_attributable = false;
    bool quarantine_has_previous_record = false;
    bool quarantine_has_head = false;
    uint64_t quarantine_reason = 0;
    uint64_t quarantine_phase = 0;
    uint64_t slot = 0;
    const uint8_t * predecessor = nullptr;
    size_t predecessor_size = 0;
    const uint8_t * successor = nullptr;
    size_t successor_size = 0;
};

struct context_store_registry_lab_wire_result {
    context_store_registry_lab_wire_status status = context_store_registry_lab_wire_status::structural_rejection;
    context_store_format_digest content_digest {};
    bool authenticated() const noexcept {
        return status == context_store_registry_lab_wire_status::authenticated_semantic_only;
    }
};

template<size_t N> struct context_store_registry_lab_bounded_bytes_v1 { std::array<uint8_t,N> bytes{}; size_t size=0; };
struct context_store_registry_lab_scope_value_v1 { context_store_format_digest root_id{},path_policy_commitment{};context_store_registered_id registry_id;uint64_t registry_epoch=0; };
enum class context_store_registry_lab_root_state_v1:uint8_t { initializing=0,initialized=1 };
enum class context_store_registry_lab_head_selection_v1:uint8_t { predecessor=0,successor=1 };
enum class context_store_registry_lab_terminal_class_v1:uint8_t { normal=0,recovered=1 };
struct context_store_registry_lab_root_value_v1 {context_store_registry_lab_scope_value_v1 scope;std::array<uint8_t,16>store_uuid{},filesystem_uuid{};uint64_t mount_id=0,owner_uid=0,lock_st_dev=0,lock_st_ino=0;context_store_registry_lab_root_state_v1 state=context_store_registry_lab_root_state_v1::initializing;context_store_format_digest initial_head_digest{};};
struct context_store_registry_lab_head_value_v1 {context_store_registry_lab_scope_value_v1 scope;context_store_registry_lab_head_selection_v1 selection=context_store_registry_lab_head_selection_v1::predecessor;context_store_registry_lab_bounded_bytes_v1<1024> selected_envelope{},predecessor_envelope{};context_store_format_digest selected_envelope_digest{},predecessor_envelope_digest{},expected_head_digest{};uint64_t selected_high_water=0,selector_generation=0,predecessor_high_water=0,predecessor_selector_generation=0;};
struct context_store_registry_lab_prepare_value_v1 {context_store_registry_lab_scope_value_v1 scope;context_store_format_digest attempt_id{},operation_commitment{},predecessor_envelope_digest{},successor_envelope_digest{},initial_head_digest{};uint64_t slot=0,predecessor_high_water=0,predecessor_selector_generation=0,successor_selector_generation=0;context_store_registry_lab_bounded_bytes_v1<1024> predecessor{},successor{};};
struct context_store_registry_lab_close_value_v1 {context_store_registry_lab_scope_value_v1 scope;context_store_registry_lab_terminal_class_v1 terminal_class=context_store_registry_lab_terminal_class_v1::normal;context_store_format_digest attempt_id{},operation_commitment{},predecessor_envelope_digest{},successor_envelope_digest{},prepare_digest{},head_digest{};uint64_t slot=0;};
struct context_store_registry_lab_abort_value_v1 {context_store_registry_lab_scope_value_v1 scope;context_store_registry_lab_terminal_class_v1 terminal_class=context_store_registry_lab_terminal_class_v1::normal;context_store_format_digest attempt_id{},operation_commitment{},predecessor_envelope_digest{},successor_envelope_digest{},prepare_digest{},head_digest{};uint64_t slot=0;};
struct context_store_registry_lab_quarantine_value_v1 {context_store_registry_lab_scope_value_v1 scope;context_store_format_digest event_id{},attempt_id{},previous_record_digest{},head_digest{},operation_commitment{};uint64_t slot=0,reason=0,phase=0;bool attributable=false,has_previous_record=false,has_head=false;};
struct context_store_registry_lab_root_evidence_v1 {const uint8_t*authenticated_head=nullptr;size_t authenticated_head_size=0;context_store_registry_lab_bounded_bytes_v1<1024> selected_envelope{};context_store_format_digest selected_envelope_digest{};uint64_t selected_high_water=0,selector_generation=0;};
struct context_store_registry_lab_prepare_evidence_v1 {const uint8_t*predecessor_head=nullptr;size_t predecessor_head_size=0;};
struct context_store_registry_lab_close_evidence_v1 {context_store_registry_lab_prepare_value_v1 transition;const uint8_t*predecessor_head=nullptr;size_t predecessor_head_size=0;const uint8_t*successor_head=nullptr;size_t successor_head_size=0;const uint8_t*prepare=nullptr;size_t prepare_size=0;};
struct context_store_registry_lab_abort_evidence_v1 {context_store_registry_lab_prepare_value_v1 transition;const uint8_t*predecessor_head=nullptr;size_t predecessor_head_size=0;const uint8_t*prepare=nullptr;size_t prepare_size=0;const uint8_t*observed_head=nullptr;size_t observed_head_size=0;const uint8_t*observed_resolved=nullptr;size_t observed_resolved_size=0;};
struct context_store_registry_lab_quarantine_evidence_v1 {const context_store_registry_lab_prepare_value_v1*transition=nullptr;const uint8_t*predecessor_head=nullptr;size_t predecessor_head_size=0;const uint8_t*successor_head=nullptr;size_t successor_head_size=0;const uint8_t*prepare=nullptr;size_t prepare_size=0;};

class context_store_registry_lab_root_witness;
class context_store_registry_lab_head_witness;
class context_store_registry_lab_prepare_witness;
class context_store_registry_lab_close_witness;
class context_store_registry_lab_abort_witness;
class context_store_registry_lab_quarantine_witness;

class context_store_registry_lab_lifecycle_witness {
private:
    friend class context_store_registry_lab_root_witness;
    friend class context_store_registry_lab_head_witness;
    friend class context_store_registry_lab_prepare_witness;
    friend class context_store_registry_lab_close_witness;
    friend class context_store_registry_lab_abort_witness;
    friend class context_store_registry_lab_quarantine_witness;
    context_store_registry_lab_lifecycle_witness() noexcept = default;
    ~context_store_registry_lab_lifecycle_witness() noexcept;
    context_store_registry_lab_lifecycle_witness(const context_store_registry_lab_lifecycle_witness &) = delete;
    context_store_registry_lab_lifecycle_witness & operator=(const context_store_registry_lab_lifecycle_witness &) = delete;
    context_store_registry_lab_lifecycle_witness(context_store_registry_lab_lifecycle_witness &&) noexcept;
    context_store_registry_lab_lifecycle_witness & operator=(context_store_registry_lab_lifecycle_witness &&) noexcept;

private:
    struct evidence {const uint8_t*predecessor_head=nullptr;size_t predecessor_head_size=0;const uint8_t*successor_head=nullptr;size_t successor_head_size=0;const uint8_t*prepare=nullptr;size_t prepare_size=0;const uint8_t*observed_head=nullptr;size_t observed_head_size=0;const uint8_t*observed_resolved=nullptr;size_t observed_resolved_size=0;};
    static bool admit(const context_store_registry_lab_credential&,const context_store_registry_lab_expectation&,const evidence&,context_store_registry_lab_lifecycle_witness&) noexcept;
    static bool admit_root_initializing(const context_store_registry_lab_credential&,const context_store_registry_lab_expectation&,context_store_registry_lab_lifecycle_witness&) noexcept;
    static bool admit_root_initialized(const context_store_registry_lab_credential&,const context_store_registry_lab_expectation&,const evidence&,context_store_registry_lab_lifecycle_witness&) noexcept;
    static bool admit_head_predecessor(const context_store_registry_lab_credential&,const context_store_registry_lab_expectation&,context_store_registry_lab_lifecycle_witness&) noexcept;
    static bool admit_head_successor(const context_store_registry_lab_credential&,const context_store_registry_lab_expectation&,context_store_registry_lab_lifecycle_witness&) noexcept;
    static bool admit_prepare(const context_store_registry_lab_credential&,const context_store_registry_lab_expectation&,const evidence&,context_store_registry_lab_lifecycle_witness&) noexcept;
    static bool admit_close(const context_store_registry_lab_credential&,const context_store_registry_lab_expectation&,const evidence&,uint8_t,context_store_registry_lab_lifecycle_witness&) noexcept;
    static bool abort_mismatch_evidence(const context_store_registry_lab_credential&,const context_store_registry_lab_expectation&,const evidence&) noexcept;
    static bool admit_abort(const context_store_registry_lab_credential&,const context_store_registry_lab_expectation&,const evidence&,uint8_t,context_store_registry_lab_lifecycle_witness&) noexcept;
    static bool admit_quarantine(const context_store_registry_lab_credential&,const context_store_registry_lab_expectation&,const evidence&,context_store_registry_lab_lifecycle_witness&) noexcept;
    static context_store_registry_lab_wire_result encode(context_store_registry_lab_kind,const context_store_registry_lab_credential&,const context_store_registry_lab_lifecycle_witness&,uint8_t*,size_t,size_t&) noexcept;
    friend bool context_store_registry_lab_admit_root_v1(const context_store_registry_lab_root_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_root_evidence_v1&,context_store_registry_lab_root_witness&) noexcept;
    friend bool context_store_registry_lab_admit_head_v1(const context_store_registry_lab_head_value_v1&,const context_store_registry_lab_credential&,context_store_registry_lab_head_witness&) noexcept;
    friend bool context_store_registry_lab_admit_prepare_v1(const context_store_registry_lab_prepare_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_prepare_evidence_v1&,context_store_registry_lab_prepare_witness&) noexcept;
    friend bool context_store_registry_lab_admit_close_v1(const context_store_registry_lab_close_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_close_evidence_v1&,context_store_registry_lab_close_witness&) noexcept;
    friend bool context_store_registry_lab_admit_abort_v1(const context_store_registry_lab_abort_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_abort_evidence_v1&,context_store_registry_lab_abort_witness&) noexcept;
    friend bool context_store_registry_lab_admit_quarantine_v1(const context_store_registry_lab_quarantine_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_quarantine_evidence_v1&,context_store_registry_lab_quarantine_witness&) noexcept;
    friend context_store_registry_lab_wire_result context_store_registry_lab_encode_root_v1(const context_store_registry_lab_root_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_root_witness&,uint8_t*,size_t,size_t&) noexcept;
    friend context_store_registry_lab_wire_result context_store_registry_lab_encode_head_v1(const context_store_registry_lab_head_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_head_witness&,uint8_t*,size_t,size_t&) noexcept;
    friend context_store_registry_lab_wire_result context_store_registry_lab_encode_prepare_v1(const context_store_registry_lab_prepare_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_prepare_witness&,uint8_t*,size_t,size_t&) noexcept;
    friend context_store_registry_lab_wire_result context_store_registry_lab_encode_close_v1(const context_store_registry_lab_close_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_close_witness&,uint8_t*,size_t,size_t&) noexcept;
    friend context_store_registry_lab_wire_result context_store_registry_lab_encode_abort_v1(const context_store_registry_lab_abort_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_abort_witness&,uint8_t*,size_t,size_t&) noexcept;
    friend context_store_registry_lab_wire_result context_store_registry_lab_encode_quarantine_v1(const context_store_registry_lab_quarantine_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_quarantine_witness&,uint8_t*,size_t,size_t&) noexcept;

    void clear() noexcept;
    context_store_registry_lab_expectation expectation_ {};
    context_store_format_digest credential_commitment_ {};
    std::array<uint8_t, 1024> predecessor_ {}, successor_ {};
    std::array<uint8_t, 1024> predecessor_head_ {}, successor_head_ {};
    std::array<uint8_t, 1024> observed_head_ {}, observed_resolved_ {};
    std::array<uint8_t, context_store_registry_lab_wire_max_bytes> prepare_ {};
    size_t predecessor_size_ = 0, successor_size_ = 0;
    size_t predecessor_head_size_ = 0, successor_head_size_ = 0, prepare_size_ = 0;
    size_t observed_head_size_ = 0, observed_resolved_size_ = 0;
    bool admitted_ = false, predecessor_ready_ = false, successor_ready_ = false;
    uint8_t kind_tag_ = 0xff, variant_ = 0xff;
};

#define HALOFPX_REGISTRY_TYPED_WITNESS(NAME,VALUE) class NAME { public: NAME() noexcept=default;~NAME() noexcept=default;NAME(const NAME&)=delete;NAME&operator=(const NAME&)=delete;NAME(NAME&&) noexcept=default;NAME&operator=(NAME&&) noexcept=default; private: bool matches(const VALUE&)const noexcept;context_store_registry_lab_lifecycle_witness storage_;VALUE value_{};
HALOFPX_REGISTRY_TYPED_WITNESS(context_store_registry_lab_root_witness,context_store_registry_lab_root_value_v1) friend bool context_store_registry_lab_admit_root_v1(const context_store_registry_lab_root_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_root_evidence_v1&,context_store_registry_lab_root_witness&) noexcept;friend context_store_registry_lab_wire_result context_store_registry_lab_encode_root_v1(const context_store_registry_lab_root_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_root_witness&,uint8_t*,size_t,size_t&) noexcept;};
HALOFPX_REGISTRY_TYPED_WITNESS(context_store_registry_lab_head_witness,context_store_registry_lab_head_value_v1) friend bool context_store_registry_lab_admit_head_v1(const context_store_registry_lab_head_value_v1&,const context_store_registry_lab_credential&,context_store_registry_lab_head_witness&) noexcept;friend context_store_registry_lab_wire_result context_store_registry_lab_encode_head_v1(const context_store_registry_lab_head_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_head_witness&,uint8_t*,size_t,size_t&) noexcept;};
HALOFPX_REGISTRY_TYPED_WITNESS(context_store_registry_lab_prepare_witness,context_store_registry_lab_prepare_value_v1) friend bool context_store_registry_lab_admit_prepare_v1(const context_store_registry_lab_prepare_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_prepare_evidence_v1&,context_store_registry_lab_prepare_witness&) noexcept;friend context_store_registry_lab_wire_result context_store_registry_lab_encode_prepare_v1(const context_store_registry_lab_prepare_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_prepare_witness&,uint8_t*,size_t,size_t&) noexcept;};
HALOFPX_REGISTRY_TYPED_WITNESS(context_store_registry_lab_close_witness,context_store_registry_lab_close_value_v1) friend bool context_store_registry_lab_admit_close_v1(const context_store_registry_lab_close_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_close_evidence_v1&,context_store_registry_lab_close_witness&) noexcept;friend context_store_registry_lab_wire_result context_store_registry_lab_encode_close_v1(const context_store_registry_lab_close_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_close_witness&,uint8_t*,size_t,size_t&) noexcept;};
HALOFPX_REGISTRY_TYPED_WITNESS(context_store_registry_lab_abort_witness,context_store_registry_lab_abort_value_v1) friend bool context_store_registry_lab_admit_abort_v1(const context_store_registry_lab_abort_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_abort_evidence_v1&,context_store_registry_lab_abort_witness&) noexcept;friend context_store_registry_lab_wire_result context_store_registry_lab_encode_abort_v1(const context_store_registry_lab_abort_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_abort_witness&,uint8_t*,size_t,size_t&) noexcept;};
HALOFPX_REGISTRY_TYPED_WITNESS(context_store_registry_lab_quarantine_witness,context_store_registry_lab_quarantine_value_v1) friend bool context_store_registry_lab_admit_quarantine_v1(const context_store_registry_lab_quarantine_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_quarantine_evidence_v1&,context_store_registry_lab_quarantine_witness&) noexcept;friend context_store_registry_lab_wire_result context_store_registry_lab_encode_quarantine_v1(const context_store_registry_lab_quarantine_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_quarantine_witness&,uint8_t*,size_t,size_t&) noexcept;};
#undef HALOFPX_REGISTRY_TYPED_WITNESS

bool context_store_registry_lab_admit_root_v1(const context_store_registry_lab_root_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_root_evidence_v1&,context_store_registry_lab_root_witness&) noexcept;
bool context_store_registry_lab_admit_head_v1(const context_store_registry_lab_head_value_v1&,const context_store_registry_lab_credential&,context_store_registry_lab_head_witness&) noexcept;
bool context_store_registry_lab_admit_prepare_v1(const context_store_registry_lab_prepare_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_prepare_evidence_v1&,context_store_registry_lab_prepare_witness&) noexcept;
bool context_store_registry_lab_admit_close_v1(const context_store_registry_lab_close_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_close_evidence_v1&,context_store_registry_lab_close_witness&) noexcept;
bool context_store_registry_lab_admit_abort_v1(const context_store_registry_lab_abort_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_abort_evidence_v1&,context_store_registry_lab_abort_witness&) noexcept;
bool context_store_registry_lab_admit_quarantine_v1(const context_store_registry_lab_quarantine_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_quarantine_evidence_v1&,context_store_registry_lab_quarantine_witness&) noexcept;
context_store_registry_lab_wire_result context_store_registry_lab_encode_root_v1(const context_store_registry_lab_root_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_root_witness&,uint8_t*,size_t,size_t&) noexcept;
context_store_registry_lab_wire_result context_store_registry_lab_encode_head_v1(const context_store_registry_lab_head_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_head_witness&,uint8_t*,size_t,size_t&) noexcept;
context_store_registry_lab_wire_result context_store_registry_lab_encode_prepare_v1(const context_store_registry_lab_prepare_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_prepare_witness&,uint8_t*,size_t,size_t&) noexcept;
context_store_registry_lab_wire_result context_store_registry_lab_encode_close_v1(const context_store_registry_lab_close_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_close_witness&,uint8_t*,size_t,size_t&) noexcept;
context_store_registry_lab_wire_result context_store_registry_lab_encode_abort_v1(const context_store_registry_lab_abort_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_abort_witness&,uint8_t*,size_t,size_t&) noexcept;
context_store_registry_lab_wire_result context_store_registry_lab_encode_quarantine_v1(const context_store_registry_lab_quarantine_value_v1&,const context_store_registry_lab_credential&,const context_store_registry_lab_quarantine_witness&,uint8_t*,size_t,size_t&) noexcept;

bool context_store_registry_lab_parse_credential_v1(
    const uint8_t * data, size_t size, context_store_registry_lab_credential & output) noexcept;

bool context_store_registry_lab_path_policy_v1(
    const uint8_t * parent, size_t parent_size,
    const uint8_t * root, size_t root_size,
    const std::array<uint8_t, 16> & filesystem_uuid,
    const std::array<uint8_t, 16> & subvolume_uuid,
    uint64_t mount_id, uint64_t st_dev, uint64_t owner_uid,
    context_store_format_digest & output) noexcept;

context_store_registry_lab_wire_result context_store_registry_lab_verify_v1(
    context_store_registry_lab_kind kind,
    const uint8_t * data, size_t size,
    const context_store_registry_lab_credential & credential,
    const context_store_registry_lab_expectation & expectation) noexcept;

const char * context_store_registry_lab_wire_status_name(context_store_registry_lab_wire_status status) noexcept;

} // namespace halofpx
