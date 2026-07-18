#include "halofpx-context-store.h"

#include <utility>

namespace halofpx {

context_store_candidate::~context_store_candidate() = default;
context_store_generation::~context_store_generation() = default;
context_store_provider::~context_store_provider() = default;

context_store_lookup_result::context_store_lookup_result(
        context_store_lookup_status status,
        std::unique_ptr<const context_store_candidate> candidate) noexcept
    : status_(status), candidate_(std::move(candidate)) {
}

context_store_lookup_result::context_store_lookup_result(context_store_lookup_result &&) noexcept = default;
context_store_lookup_result & context_store_lookup_result::operator=(context_store_lookup_result &&) noexcept = default;
context_store_lookup_result::~context_store_lookup_result() = default;

context_store_lookup_result context_store_lookup_result::miss(context_store_lookup_status status) noexcept {
    if (status == context_store_lookup_status::hit) {
        status = context_store_lookup_status::miss_incomplete;
    }
    return context_store_lookup_result(status, nullptr);
}

context_store_lookup_result context_store_lookup_result::hit(
        std::unique_ptr<const context_store_candidate> candidate) noexcept {
    if (!candidate) {
        return miss(context_store_lookup_status::miss_incomplete);
    }
    return context_store_lookup_result(context_store_lookup_status::hit, std::move(candidate));
}

context_store_lookup_status context_store_lookup_result::status() const noexcept {
    return status_;
}

const context_store_candidate * context_store_lookup_result::candidate() const noexcept {
    return candidate_.get();
}

bool context_store_lookup_result::is_hit() const noexcept {
    return status_ == context_store_lookup_status::hit && candidate_ != nullptr;
}

class disabled_context_store_provider final : public context_store_provider {
public:
    const char * name() const noexcept override {
        return "disabled";
    }

    context_store_capabilities capabilities() const noexcept override {
        return {};
    }

    context_store_lookup_result lookup(const context_store_lookup_request &) const noexcept override {
        return context_store_lookup_result::miss(context_store_lookup_status::miss_disabled);
    }

    context_store_publish_status publish(const context_store_publish_request &) const noexcept override {
        return context_store_publish_status::disabled;
    }
};

std::unique_ptr<context_store_provider> make_disabled_context_store_provider() {
    return std::make_unique<disabled_context_store_provider>();
}

} // namespace halofpx
