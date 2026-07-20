#include "halofpx-context-store-v1-read-only.h"

#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace halofpx {
namespace {

void wipe_bytes(std::vector<uint8_t> & value) noexcept {
    volatile uint8_t * bytes = value.data();
    for (size_t index = 0; index < value.size(); ++index) bytes[index] = 0;
}

bool registered_id_equal(
        const context_store_registered_id & left,
        const context_store_registered_id & right) noexcept {
    if (left.size != right.size) return false;
    for (size_t index = 0; index < left.size; ++index) {
        if (left.bytes[index] != right.bytes[index]) return false;
    }
    return true;
}

bool object_reference_equal(
        const context_store_object_reference & left,
        const context_store_object_reference & right) noexcept {
    return left.object_id == right.object_id &&
        registered_id_equal(left.stream_type, right.stream_type) &&
        registered_id_equal(left.codec_id, right.codec_id) &&
        left.codec_schema_major == right.codec_schema_major &&
        left.codec_schema_minor == right.codec_schema_minor &&
        left.required == right.required &&
        left.frame_bytes == right.frame_bytes &&
        left.token_sequence_digest == right.token_sequence_digest &&
        left.logical_position == right.logical_position &&
        left.output_boundary == right.output_boundary &&
        left.has_logical_rank == right.has_logical_rank &&
        left.logical_rank == right.logical_rank &&
        left.rank_ownership_digest == right.rank_ownership_digest &&
        left.compatibility_root == right.compatibility_root;
}

bool metadata_equal(
        const context_store_authenticated_manifest_metadata & left,
        const context_store_authenticated_manifest_metadata & right) noexcept {
    if (left.store_uuid != right.store_uuid ||
        left.checkpoint_lineage_id != right.checkpoint_lineage_id ||
        left.generation != right.generation ||
        left.has_predecessor != right.has_predecessor ||
        left.predecessor_manifest_digest != right.predecessor_manifest_digest ||
        left.compatibility_components != right.compatibility_components ||
        left.compatibility_root != right.compatibility_root ||
        left.scope_namespace != right.scope_namespace ||
        left.policy_epoch != right.policy_epoch ||
        !registered_id_equal(left.topology_plan_schema_id, right.topology_plan_schema_id) ||
        !registered_id_equal(left.topology_execution_mode, right.topology_execution_mode) ||
        left.world_size != right.world_size ||
        left.global_plan_digest != right.global_plan_digest ||
        left.topology_epoch != right.topology_epoch ||
        left.rank_count != right.rank_count ||
        !registered_id_equal(left.state_profile_id, right.state_profile_id) ||
        left.producer_identity != right.producer_identity ||
        left.durability_mode != right.durability_mode ||
        left.rank_count > context_store_manifest_max_ranks) {
        return false;
    }
    for (size_t index = 0; index < left.rank_count; ++index) {
        if (left.rank_ownership[index] != right.rank_ownership[index] ||
            left.rank_placements[index] != right.rank_placements[index]) {
            return false;
        }
    }
    return true;
}

context_store_lookup_status authentication_miss(
        context_store_manifest_verify_status status) noexcept {
    switch (status) {
        case context_store_manifest_verify_status::authenticated_unadmitted:
            return context_store_lookup_status::miss_incomplete;
        case context_store_manifest_verify_status::unknown_key:
        case context_store_manifest_verify_status::revoked_key:
        case context_store_manifest_verify_status::read_disabled_key:
        case context_store_manifest_verify_status::key_generation_mismatch:
            return context_store_lookup_status::miss_unauthorized;
        case context_store_manifest_verify_status::authority_mismatch:
        case context_store_manifest_verify_status::replay_mismatch:
            return context_store_lookup_status::miss_replay;
        case context_store_manifest_verify_status::compatibility_corrupt:
        case context_store_manifest_verify_status::compatibility_mismatch:
            return context_store_lookup_status::miss_incompatible;
        case context_store_manifest_verify_status::structural_rejection:
        case context_store_manifest_verify_status::invalid_policy:
        case context_store_manifest_verify_status::authentication_failed:
            return context_store_lookup_status::miss_corrupt;
    }
    return context_store_lookup_status::miss_corrupt;
}

context_store_lookup_status object_miss(context_store_object_verify_status status) noexcept {
    switch (status) {
        case context_store_object_verify_status::object_verified_unadmitted:
            return context_store_lookup_status::miss_incomplete;
        case context_store_object_verify_status::object_index_out_of_range:
        case context_store_object_verify_status::input_empty:
        case context_store_object_verify_status::truncated:
            return context_store_lookup_status::miss_incomplete;
        case context_store_object_verify_status::invalid_limits:
        case context_store_object_verify_status::frame_limit_exceeded:
        case context_store_object_verify_status::payload_limit_exceeded:
            return context_store_lookup_status::miss_unsupported;
        case context_store_object_verify_status::manifest_unadmitted:
        case context_store_object_verify_status::wrong_magic:
        case context_store_object_verify_status::wrong_domain:
        case context_store_object_verify_status::invalid_stream_type:
        case context_store_object_verify_status::payload_length_invalid:
        case context_store_object_verify_status::trailing_data:
        case context_store_object_verify_status::descriptor_mismatch:
        case context_store_object_verify_status::object_digest_mismatch:
            return context_store_lookup_status::miss_corrupt;
    }
    return context_store_lookup_status::miss_corrupt;
}

struct verified_payload {
    size_t offset = 0;
    size_t size = 0;
};

class v1_read_only_candidate final : public context_store_v1_read_only_candidate {
public:
    v1_read_only_candidate(
            const context_store_authenticated_manifest_metadata & metadata,
            const context_store_format_digest & manifest_digest,
            const std::vector<context_store_object_reference> & descriptors,
            const std::vector<std::vector<uint8_t>> & frames,
            const std::vector<verified_payload> & payloads)
        : metadata_(metadata), manifest_digest_(manifest_digest), descriptors_(descriptors) {
        identity_.compatibility_root = metadata.compatibility_root;
        identity_.scope_namespace = metadata.scope_namespace;
        identity_.checkpoint_lineage_id = metadata.checkpoint_lineage_id;
        identity_.policy_epoch = metadata.policy_epoch;
        payloads_.reserve(payloads.size());
        for (size_t index = 0; index < payloads.size(); ++index) {
            const auto & view = payloads[index];
            payloads_.emplace_back(
                frames[index].begin() + static_cast<std::ptrdiff_t>(view.offset),
                frames[index].begin() + static_cast<std::ptrdiff_t>(view.offset + view.size));
        }
    }

    const context_store_identity & identity() const noexcept override { return identity_; }
    const context_store_format_digest & manifest_digest() const noexcept override { return manifest_digest_; }
    uint64_t generation() const noexcept override { return metadata_.generation; }
    const context_store_registered_id & state_profile_id() const noexcept override { return metadata_.state_profile_id; }
    uint64_t world_size() const noexcept override { return metadata_.world_size; }
    uint64_t topology_epoch() const noexcept override { return metadata_.topology_epoch; }
    const context_store_format_digest & producer_identity() const noexcept override { return metadata_.producer_identity; }
    uint8_t durability_mode() const noexcept override { return metadata_.durability_mode; }
    size_t object_count() const noexcept override { return descriptors_.size(); }
    const context_store_object_reference * descriptor(size_t index) const noexcept override {
        return index < descriptors_.size() ? &descriptors_[index] : nullptr;
    }
    context_store_v1_payload_view payload(size_t index) const noexcept override {
        if (index >= payloads_.size()) return {};
        return { payloads_[index].data(), payloads_[index].size() };
    }

private:
    context_store_identity identity_;
    context_store_authenticated_manifest_metadata metadata_;
    context_store_format_digest manifest_digest_ {};
    std::vector<context_store_object_reference> descriptors_;
    std::vector<std::vector<uint8_t>> payloads_;
};

class v1_read_only_provider final : public context_store_provider {
public:
    explicit v1_read_only_provider(const context_store_v1_read_only_source & source)
        : manifest_(source.manifest_data, source.manifest_data + source.manifest_size),
          policy_(source.verification_policy),
          admission_metadata_(source.admission.manifest),
          admission_objects_(source.admission.objects,
              source.admission.objects + source.admission.object_count),
          limits_(source.object_limits),
          max_total_frame_bytes_(source.max_total_frame_bytes) {
        frames_.reserve(source.frame_count);
        for (size_t index = 0; index < source.frame_count; ++index) {
            frames_.emplace_back(
                source.frames[index].data,
                source.frames[index].data + source.frames[index].size);
        }
        // Copy secret material only after every other potentially allocating
        // construction step so an allocation failure cannot bypass wiping an
        // already-owned key in a partially constructed provider.
        master_key_.assign(
            source.verification_policy.key.master_key.data,
            source.verification_policy.key.master_key.data + source.verification_policy.key.master_key.size);
        policy_.key.master_key = { master_key_.data(), master_key_.size() };
    }

    ~v1_read_only_provider() override { wipe_bytes(master_key_); }

    const char * name() const noexcept override { return "v1-read-only-synthetic"; }
    context_store_capabilities capabilities() const noexcept override { return {}; }

    context_store_lookup_result lookup(const context_store_lookup_request & request) const noexcept override {
        try {
            const auto verified = context_store_verify_manifest_v1(
                manifest_.data(), manifest_.size(), policy_);
            if (verified.status != context_store_manifest_verify_status::authenticated_unadmitted) {
                return context_store_lookup_result::miss(authentication_miss(verified.status));
            }
            const auto * metadata = verified.authenticated_manifest_metadata();
            if (metadata == nullptr || verified.authenticated_object_count() != admission_objects_.size() ||
                frames_.size() != admission_objects_.size()) {
                return context_store_lookup_result::miss(context_store_lookup_status::miss_incomplete);
            }
            if (request.identity.scope_namespace != metadata->scope_namespace) {
                return context_store_lookup_result::miss(context_store_lookup_status::miss_unauthorized);
            }
            if (request.identity.compatibility_root != metadata->compatibility_root) {
                return context_store_lookup_result::miss(context_store_lookup_status::miss_incompatible);
            }
            if (request.identity.checkpoint_lineage_id != metadata->checkpoint_lineage_id ||
                request.identity.policy_epoch != metadata->policy_epoch) {
                return context_store_lookup_result::miss(context_store_lookup_status::miss_replay);
            }
            if (!metadata_equal(*metadata, admission_metadata_)) {
                return context_store_lookup_result::miss(context_store_lookup_status::miss_unsupported);
            }

            std::vector<context_store_object_reference> descriptors;
            std::vector<verified_payload> payloads;
            descriptors.reserve(admission_objects_.size());
            payloads.reserve(admission_objects_.size());
            uint64_t aggregate_frame_bytes = 0;
            for (size_t index = 0; index < admission_objects_.size(); ++index) {
                const auto * descriptor = verified.authenticated_object_reference(index);
                if (descriptor == nullptr || !object_reference_equal(*descriptor, admission_objects_[index])) {
                    return context_store_lookup_result::miss(context_store_lookup_status::miss_unsupported);
                }
                if (frames_[index].size() > max_total_frame_bytes_ - aggregate_frame_bytes) {
                    return context_store_lookup_result::miss(context_store_lookup_status::miss_unsupported);
                }
                aggregate_frame_bytes += frames_[index].size();
                const auto object = context_store_verify_object_frame_v1(
                    frames_[index].data(), frames_[index].size(), limits_, verified, index);
                if (object.status != context_store_object_verify_status::object_verified_unadmitted) {
                    return context_store_lookup_result::miss(object_miss(object.status));
                }
                descriptors.push_back(*descriptor);
                payloads.push_back({ object.payload_offset, object.payload_size });
            }
            return context_store_lookup_result::hit(std::make_unique<v1_read_only_candidate>(
                *metadata, verified.manifest_digest, descriptors, frames_, payloads));
        } catch (...) {
            return context_store_lookup_result::miss(context_store_lookup_status::miss_storage);
        }
    }

    context_store_publish_status publish(const context_store_publish_request &) const noexcept override {
        return context_store_publish_status::disabled;
    }

private:
    std::vector<uint8_t> manifest_;
    std::vector<uint8_t> master_key_;
    context_store_manifest_verification_policy policy_;
    context_store_authenticated_manifest_metadata admission_metadata_;
    std::vector<context_store_object_reference> admission_objects_;
    std::vector<std::vector<uint8_t>> frames_;
    context_store_object_limits limits_;
    uint64_t max_total_frame_bytes_ = 0;
};

bool valid_source(const context_store_v1_read_only_source & source) noexcept {
    if (source.manifest_data == nullptr || source.manifest_size == 0 ||
        source.manifest_size > context_store_manifest_max_bytes ||
        source.verification_policy.key.master_key.data == nullptr ||
        source.verification_policy.key.master_key.size == 0 ||
        source.verification_policy.key.master_key.size > context_store_master_key_max_bytes ||
        source.frames == nullptr || source.frame_count == 0 ||
        source.frame_count > context_store_manifest_max_objects ||
        source.admission.objects == nullptr || source.admission.object_count == 0 ||
        source.admission.object_count != source.frame_count ||
        source.object_limits.max_frame_bytes == 0 ||
        source.object_limits.max_payload_bytes == 0 ||
        source.max_total_frame_bytes == 0) {
        return false;
    }
    uint64_t total = 0;
    for (size_t index = 0; index < source.frame_count; ++index) {
        const auto & frame = source.frames[index];
        if (frame.data == nullptr || frame.size == 0 ||
            frame.size > source.max_total_frame_bytes - total) {
            return false;
        }
        total += frame.size;
    }
    return true;
}

} // namespace

context_store_v1_read_only_candidate::~context_store_v1_read_only_candidate() = default;

std::unique_ptr<context_store_provider> make_context_store_v1_read_only_provider(
        const context_store_v1_read_only_source & source) {
    if (!valid_source(source)) {
        throw std::invalid_argument("invalid synthetic full-v1 read-only source");
    }
    return std::make_unique<v1_read_only_provider>(source);
}

} // namespace halofpx
