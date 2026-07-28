// Note: porting this file to C++ is a work in progress

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#   define NOMINMAX
#endif
#include <windows.h>
#endif

#include "ggml-backend.h"
#include "ggml-backend-impl.h"
#include "ggml-rpc.h"
#include "ggml-alloc.h"
#include "ggml-impl.h"
extern "C" {
#include "sha256/sha256.h"
}

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <chrono>
#include <atomic>
#include <array>
#include <functional>
#include <new>
#include <unordered_map>
#include <unordered_set>
#include <type_traits>
#include <tuple>
#include <mutex>
#include <set>
#include <vector>

#ifdef __APPLE__
#include <sys/types.h>
#include <sys/sysctl.h>
#endif


// backend buffer type

const char * ggml_backend_buft_name(ggml_backend_buffer_type_t buft) {
    GGML_ASSERT(buft);
    return buft->iface.get_name(buft);
}

ggml_backend_buffer_t ggml_backend_buft_alloc_buffer(ggml_backend_buffer_type_t buft, size_t size) {
    GGML_ASSERT(buft);
    if (size == 0) {
        // return a dummy buffer for zero-sized allocations
        return ggml_backend_buffer_init(buft, {}, NULL, 0);
    }
    return buft->iface.alloc_buffer(buft, size);
}

size_t ggml_backend_buft_get_alignment(ggml_backend_buffer_type_t buft) {
    GGML_ASSERT(buft);
    return buft->iface.get_alignment(buft);
}

size_t ggml_backend_buft_get_max_size(ggml_backend_buffer_type_t buft) {
    GGML_ASSERT(buft);
    // get_max_size is optional, defaults to SIZE_MAX
    if (buft->iface.get_max_size) {
        return buft->iface.get_max_size(buft);
    }
    return SIZE_MAX;
}

size_t ggml_backend_buft_get_alloc_size(ggml_backend_buffer_type_t buft, const struct ggml_tensor * tensor) {
    GGML_ASSERT(buft);
    // get_alloc_size is optional, defaults to ggml_nbytes
    if (buft->iface.get_alloc_size) {
        size_t size = buft->iface.get_alloc_size(buft, tensor);
        assert(size >= ggml_nbytes(tensor));
        return size;
    }
    return ggml_nbytes(tensor);
}

bool ggml_backend_buft_is_host(ggml_backend_buffer_type_t buft) {
    GGML_ASSERT(buft);
    if (buft->iface.is_host) {
        return buft->iface.is_host(buft);
    }
    return false;
}

ggml_backend_dev_t ggml_backend_buft_get_device(ggml_backend_buffer_type_t buft) {
    GGML_ASSERT(buft);
    return buft->device;
}

// backend buffer

ggml_backend_buffer_t ggml_backend_buffer_init(
               ggml_backend_buffer_type_t buft,
        struct ggml_backend_buffer_i      iface,
               void *                     context,
               size_t                     size) {
    ggml_backend_buffer_t buffer = new ggml_backend_buffer {
        /* .interface = */ iface,
        /* .buft      = */ buft,
        /* .context   = */ context,
        /* .size      = */ size,
        /* .usage     = */ GGML_BACKEND_BUFFER_USAGE_ANY
    };

    return buffer;
}

const char * ggml_backend_buffer_name(ggml_backend_buffer_t buffer) {
    return ggml_backend_buft_name(ggml_backend_buffer_get_type(buffer));
}

void ggml_backend_buffer_free(ggml_backend_buffer_t buffer) {
    if (buffer == NULL) {
        return;
    }

    if (buffer->iface.free_buffer != NULL) {
        buffer->iface.free_buffer(buffer);
    }
    delete buffer;
}

size_t ggml_backend_buffer_get_size(ggml_backend_buffer_t buffer) {
    GGML_ASSERT(buffer);
    return buffer->size;
}

void * ggml_backend_buffer_get_base(ggml_backend_buffer_t buffer) {
    GGML_ASSERT(buffer);
    // get_base is optional if the buffer is zero-sized
    if (!ggml_backend_buffer_is_meta(buffer) && buffer->size == 0) {
        return NULL;
    }

    // FIXME JG: a multi_buffer has a non-zero size, according to the above comment get_base is not optional,
    //     I don't know whether the above comment is correct
    if (!buffer->iface.get_base) {
        return NULL;
    }

    void * base = buffer->iface.get_base(buffer);

    GGML_ASSERT(base != NULL && "backend buffer base cannot be NULL");

    return base;
}

enum ggml_status ggml_backend_buffer_init_tensor(ggml_backend_buffer_t buffer, struct ggml_tensor * tensor) {
    GGML_ASSERT(buffer);
    // init_tensor is optional
    if (buffer->iface.init_tensor) {
        return buffer->iface.init_tensor(buffer, tensor);
    }
    return GGML_STATUS_SUCCESS;
}

void ggml_backend_buffer_clear(ggml_backend_buffer_t buffer, uint8_t value) {
    GGML_ASSERT(buffer);
    // clear is optional if the buffer is zero-sized
    if (buffer->size == 0) {
        return;
    }

    buffer->iface.clear(buffer, value);
}

size_t ggml_backend_buffer_get_alignment(ggml_backend_buffer_t buffer) {
    return ggml_backend_buft_get_alignment(ggml_backend_buffer_get_type(buffer));
}

size_t ggml_backend_buffer_get_max_size(ggml_backend_buffer_t buffer) {
    return ggml_backend_buft_get_max_size(ggml_backend_buffer_get_type(buffer));
}

size_t ggml_backend_buffer_get_alloc_size(ggml_backend_buffer_t buffer, const struct ggml_tensor * tensor) {
    return ggml_backend_buft_get_alloc_size(ggml_backend_buffer_get_type(buffer), tensor);
}

bool ggml_backend_buffer_is_host(ggml_backend_buffer_t buffer) {
    return ggml_backend_buft_is_host(ggml_backend_buffer_get_type(buffer));
}

void ggml_backend_buffer_set_usage(ggml_backend_buffer_t buffer, enum ggml_backend_buffer_usage usage) {
    GGML_ASSERT(buffer);
    buffer->usage = usage;

    // FIXME: add a generic callback to the buffer interface
    if (ggml_backend_buffer_is_multi_buffer(buffer)) {
        ggml_backend_multi_buffer_set_usage(buffer, usage);
    }
}

enum ggml_backend_buffer_usage ggml_backend_buffer_get_usage(ggml_backend_buffer_t buffer) {
    GGML_ASSERT(buffer);
    return buffer->usage;
}

ggml_backend_buffer_type_t ggml_backend_buffer_get_type(ggml_backend_buffer_t buffer) {
    GGML_ASSERT(buffer);
    return buffer->buft;
}

void ggml_backend_buffer_reset(ggml_backend_buffer_t buffer) {
    GGML_ASSERT(buffer);
    if (buffer->iface.reset) {
        buffer->iface.reset(buffer);
    }
}

bool ggml_backend_buffer_copy_tensor(const struct ggml_tensor * src, struct ggml_tensor * dst) {
    ggml_backend_buffer_t dst_buf = dst->view_src ? dst->view_src->buffer : dst->buffer;
    if (dst_buf->iface.cpy_tensor) {
        return dst_buf->iface.cpy_tensor(dst_buf, src, dst);
    }
    return false;
}

// backend

ggml_guid_t ggml_backend_guid(ggml_backend_t backend) {
    if (backend == NULL) {
        return NULL;
    }
    return backend->guid;
}

const char * ggml_backend_name(ggml_backend_t backend) {
    if (backend == NULL) {
        return "NULL";
    }
    return backend->iface.get_name(backend);
}

void ggml_backend_free(ggml_backend_t backend) {
    if (backend == NULL) {
        return;
    }

    backend->iface.free(backend);
}

ggml_backend_buffer_type_t ggml_backend_get_default_buffer_type(ggml_backend_t backend) {
    GGML_ASSERT(backend);
    return ggml_backend_dev_buffer_type(backend->device);
}

ggml_backend_buffer_t ggml_backend_alloc_buffer(ggml_backend_t backend, size_t size) {
    return ggml_backend_buft_alloc_buffer(ggml_backend_get_default_buffer_type(backend), size);
}

size_t ggml_backend_get_alignment(ggml_backend_t backend) {
    return ggml_backend_buft_get_alignment(ggml_backend_get_default_buffer_type(backend));
}

size_t ggml_backend_get_max_size(ggml_backend_t backend) {
    return ggml_backend_buft_get_max_size(ggml_backend_get_default_buffer_type(backend));
}

void ggml_backend_tensor_set_async(ggml_backend_t backend, struct ggml_tensor * tensor, const void * data, size_t offset, size_t size) {
    GGML_ASSERT(backend);
    GGML_ASSERT(tensor);
    GGML_ASSERT(tensor->data != NULL && "tensor not allocated");
    GGML_ASSERT(offset + size <= ggml_nbytes(tensor) && "tensor write out of bounds");

    if (backend->iface.set_tensor_async == NULL) {
        ggml_backend_synchronize(backend);
        ggml_backend_tensor_set(tensor, data, offset, size);
    } else {
        backend->iface.set_tensor_async(backend, tensor, data, offset, size);
    }
}

void ggml_backend_tensor_get_async(ggml_backend_t backend, const struct ggml_tensor * tensor, void * data, size_t offset, size_t size) {
    GGML_ASSERT(backend);
    GGML_ASSERT(tensor);
    GGML_ASSERT(tensor->data != NULL && "tensor not allocated");
    GGML_ASSERT(offset + size <= ggml_nbytes(tensor) && "tensor read out of bounds");

    if (backend->iface.get_tensor_async == NULL) {
        ggml_backend_synchronize(backend);
        ggml_backend_tensor_get(tensor, data, offset, size);
    } else {
        backend->iface.get_tensor_async(backend, tensor, data, offset, size);
    }
}

void ggml_backend_tensor_set_2d_async(ggml_backend_t backend, struct ggml_tensor * tensor, const void * data, size_t offset, size_t size,
            size_t n_copies, size_t stride_tensor, size_t stride_data) {
    GGML_ASSERT(backend);
    GGML_ASSERT(tensor);
    GGML_ASSERT(tensor->data != NULL && "tensor not allocated");

    if (n_copies <= 1 || backend->iface.set_tensor_2d_async == NULL) {
        for (size_t i = 0; i < n_copies; i++) {
            ggml_backend_tensor_set_async(backend, tensor, (const char *) data + i*stride_data, offset + i*stride_tensor, size);
        }
        return;
    }
    if (size == 0) {
        return;
    }

    GGML_ASSERT(tensor->data != NULL && "tensor not allocated");
    GGML_ASSERT(offset + (n_copies-1)*stride_tensor + size <= ggml_nbytes(tensor) && "tensor write out of bounds");
    backend->iface.set_tensor_2d_async(backend, tensor, data, offset, size, n_copies, stride_tensor, stride_data);
}

void ggml_backend_tensor_get_2d_async(ggml_backend_t backend, const struct ggml_tensor * tensor, void * data, size_t offset, size_t size,
            size_t n_copies, size_t stride_tensor, size_t stride_data) {
    GGML_ASSERT(backend);
    GGML_ASSERT(tensor);
    GGML_ASSERT(tensor->data != NULL && "tensor not allocated");

    if (n_copies <= 1 || backend->iface.set_tensor_2d_async == NULL) {
        for (size_t i = 0; i < n_copies; i++) {
            ggml_backend_tensor_get_async(backend, tensor, (char *) data + i*stride_data, offset + i*stride_tensor, size);
        }
        return;
    }
    if (size == 0) {
        return;
    }

    GGML_ASSERT(tensor->data != NULL && "tensor not allocated");
    GGML_ASSERT(offset + (n_copies-1)*stride_tensor + size <= ggml_nbytes(tensor) && "tensor write out of bounds");
    backend->iface.get_tensor_2d_async(backend, tensor, data, offset, size, n_copies, stride_tensor, stride_data);
}

void ggml_backend_tensor_set(struct ggml_tensor * tensor, const void * data, size_t offset, size_t size) {
    GGML_ASSERT(tensor);
    ggml_backend_buffer_t buf = tensor->view_src ? tensor->view_src->buffer : tensor->buffer;
    GGML_ASSERT(buf != NULL && "tensor buffer not set");

    if (size == 0) {
        return;
    }

    GGML_ASSERT(tensor->data != NULL && "tensor not allocated");
    GGML_ASSERT(offset + size <= ggml_nbytes(tensor) && "tensor write out of bounds");

    buf->iface.set_tensor(buf, tensor, data, offset, size);
}

void ggml_backend_tensor_get(const struct ggml_tensor * tensor, void * data, size_t offset, size_t size) {
    GGML_ASSERT(tensor);
    ggml_backend_buffer_t buf = tensor->view_src ? tensor->view_src->buffer : tensor->buffer;
    GGML_ASSERT(buf != NULL && "tensor buffer not set");

    if (size == 0) {
        return;
    }

    GGML_ASSERT(tensor->data != NULL && "tensor not allocated");
    GGML_ASSERT(offset + size <= ggml_nbytes(tensor) && "tensor read out of bounds");

    buf->iface.get_tensor(buf, tensor, data, offset, size);
}

void ggml_backend_tensor_set_2d(struct ggml_tensor * tensor, const void * data, size_t offset, size_t size,
            size_t n_copies, size_t stride_tensor, size_t stride_data) {
    GGML_ASSERT(tensor);
    ggml_backend_buffer_t buf = tensor->view_src ? tensor->view_src->buffer : tensor->buffer;
    GGML_ASSERT(buf != NULL && "tensor buffer not set");

    if (n_copies <= 1 || buf->iface.set_tensor_2d == NULL) {
        for (size_t i = 0; i < n_copies; i++) {
            ggml_backend_tensor_set(tensor, (const char *) data + i*stride_data, offset + i*stride_tensor, size);
        }
        return;
    }
    if (size == 0) {
        return;
    }

    GGML_ASSERT(tensor->data != NULL && "tensor not allocated");
    GGML_ASSERT(offset + (n_copies-1)*stride_tensor + size <= ggml_nbytes(tensor) && "tensor write out of bounds");

    buf->iface.set_tensor_2d(buf, tensor, data, offset, size, n_copies, stride_tensor, stride_data);
}

void ggml_backend_tensor_get_2d(const struct ggml_tensor * tensor, void * data, size_t offset, size_t size,
            size_t n_copies, size_t stride_tensor, size_t stride_data) {
    GGML_ASSERT(tensor);
    ggml_backend_buffer_t buf = tensor->view_src ? tensor->view_src->buffer : tensor->buffer;
    GGML_ASSERT(buf != NULL && "tensor buffer not set");

    if (n_copies <= 1 || buf->iface.set_tensor_2d == NULL) {
        for (size_t i = 0; i < n_copies; i++) {
            ggml_backend_tensor_get(tensor, (char *) data + i*stride_data, offset + i*stride_tensor, size);
        }
        return;
    }
    if (size == 0) {
        return;
    }

    GGML_ASSERT(tensor->data != NULL && "tensor not allocated");
    GGML_ASSERT(offset + (n_copies-1)*stride_tensor + size <= ggml_nbytes(tensor) && "tensor read out of bounds");

    buf->iface.get_tensor_2d(buf, tensor, data, offset, size, n_copies, stride_tensor, stride_data);
}

void ggml_backend_tensor_memset(struct ggml_tensor * tensor, uint8_t value, size_t offset, size_t size) {
    GGML_ASSERT(tensor);
    ggml_backend_buffer_t buf = tensor->view_src ? tensor->view_src->buffer : tensor->buffer;

    if (size == 0) {
        return;
    }

    GGML_ASSERT(buf != NULL && "tensor buffer not set");
    GGML_ASSERT(tensor->data != NULL && "tensor not allocated");
    GGML_ASSERT(offset + size <= ggml_nbytes(tensor) && "tensor write out of bounds");
    GGML_ASSERT(buf->iface.memset_tensor != NULL && "memset not implemented by backend buffer");

    buf->iface.memset_tensor(buf, tensor, value, offset, size);
}

void ggml_backend_synchronize(ggml_backend_t backend) {
    GGML_ASSERT(backend);
    if (backend->iface.synchronize == NULL) {
        return;
    }

    backend->iface.synchronize(backend);
}

ggml_backend_graph_plan_t ggml_backend_graph_plan_create(ggml_backend_t backend, struct ggml_cgraph * cgraph) {
    GGML_ASSERT(backend);
    GGML_ASSERT(backend->iface.graph_plan_create != NULL);

    return backend->iface.graph_plan_create(backend, cgraph);
}

void ggml_backend_graph_plan_free(ggml_backend_t backend, ggml_backend_graph_plan_t plan) {
    GGML_ASSERT(backend);
    GGML_ASSERT(backend->iface.graph_plan_free != NULL);

    backend->iface.graph_plan_free(backend, plan);
}

enum ggml_status ggml_backend_graph_plan_compute(ggml_backend_t backend, ggml_backend_graph_plan_t plan) {
    GGML_ASSERT(backend);
    GGML_ASSERT(backend->iface.graph_plan_compute != NULL);

    return backend->iface.graph_plan_compute(backend, plan);
}

enum ggml_status ggml_backend_graph_compute(ggml_backend_t backend, struct ggml_cgraph * cgraph) {
    enum ggml_status err = ggml_backend_graph_compute_async(backend, cgraph);
    ggml_backend_synchronize(backend);
    return err;
}

enum ggml_status ggml_backend_graph_compute_async(ggml_backend_t backend, struct ggml_cgraph * cgraph) {
    GGML_ASSERT(backend);
    return backend->iface.graph_compute(backend, cgraph);
}

bool ggml_backend_supports_op(ggml_backend_t backend, const struct ggml_tensor * op) {
    GGML_ASSERT(backend);
    return ggml_backend_dev_supports_op(backend->device, op);
}

bool ggml_backend_supports_buft(ggml_backend_t backend, ggml_backend_buffer_type_t buft) {
    GGML_ASSERT(backend);
    return ggml_backend_dev_supports_buft(backend->device, buft);
}

bool ggml_backend_offload_op(ggml_backend_t backend, const struct ggml_tensor * op) {
    GGML_ASSERT(backend);
    return ggml_backend_dev_offload_op(backend->device, op);
}

ggml_backend_dev_t ggml_backend_get_device(ggml_backend_t backend) {
    GGML_ASSERT(backend);
    return backend->device;
}

// backend copy

void ggml_backend_tensor_copy(const struct ggml_tensor * src, struct ggml_tensor * dst) {
    GGML_ASSERT(ggml_are_same_layout(src, dst) && "cannot copy tensors with different layouts");

    if (src == dst) {
        return;
    }

    if (ggml_backend_buffer_is_host(src->buffer)) {
        ggml_backend_tensor_set(dst, src->data, 0, ggml_nbytes(src));
    } else if (ggml_backend_buffer_is_host(dst->buffer)) {
        ggml_backend_tensor_get(src, dst->data, 0, ggml_nbytes(src));
    } else if (!ggml_backend_buffer_copy_tensor(src, dst)) {
#ifndef NDEBUG
        GGML_LOG_DEBUG("%s: warning: slow copy from %s to %s\n", __func__, ggml_backend_buffer_name(src->buffer), ggml_backend_buffer_name(dst->buffer));
#endif // NDEBUG
        size_t nbytes = ggml_nbytes(src);
        void * data = malloc(nbytes);
        ggml_backend_tensor_get(src, data, 0, nbytes);
        ggml_backend_tensor_set(dst, data, 0, nbytes);
        free(data);
    }
}

void ggml_backend_tensor_copy_async(ggml_backend_t backend_src, ggml_backend_t backend_dst, const struct ggml_tensor * src, struct ggml_tensor * dst) {
    GGML_ASSERT(ggml_are_same_layout(src, dst) && "cannot copy tensors with different layouts");

    if (src == dst) {
        return;
    }

    GGML_ASSERT(backend_dst);
    if (backend_dst->iface.cpy_tensor_async != NULL) {
        if (backend_dst->iface.cpy_tensor_async(backend_src, backend_dst, src, dst)) {
            return;
        }
    }

    // an async copy would normally happen after all the queued operations on both backends are completed
    // to simulate the same behavior, we need to synchronize both backends first, and do a blocking copy
    ggml_backend_synchronize(backend_src);
    ggml_backend_synchronize(backend_dst);
    ggml_backend_tensor_copy(src, dst);
}

// events

ggml_backend_event_t ggml_backend_event_new(ggml_backend_dev_t device) {
    // null device is allowed for the transition period to the device interface
    if (device == NULL || device->iface.event_new == NULL) {
        return NULL;
    }
    return device->iface.event_new(device);
}

void ggml_backend_event_free(ggml_backend_event_t event) {
    if (event == NULL) {
        return;
    }
    event->device->iface.event_free(event->device, event);
}

void ggml_backend_event_record(ggml_backend_event_t event, ggml_backend_t backend) {
    GGML_ASSERT(backend);
    GGML_ASSERT(backend->iface.event_record != NULL);

    backend->iface.event_record(backend, event);
}

void ggml_backend_event_synchronize(ggml_backend_event_t event) {
    GGML_ASSERT(event);
    GGML_ASSERT(event->device->iface.event_synchronize);

    event->device->iface.event_synchronize(event->device, event);
}

void ggml_backend_event_wait(ggml_backend_t backend, ggml_backend_event_t event) {
    GGML_ASSERT(backend);
    GGML_ASSERT(backend->iface.event_wait != NULL);

    backend->iface.event_wait(backend, event);
}

static void ggml_backend_graph_optimize(ggml_backend_t backend, struct ggml_cgraph * cgraph) {
    GGML_ASSERT(backend);
    if (backend->iface.graph_optimize != NULL) {
        backend->iface.graph_optimize(backend, cgraph);
    }
}

// Backend device

const char * ggml_backend_dev_name(ggml_backend_dev_t device) {
    GGML_ASSERT(device);
    return device->iface.get_name(device);
}

const char * ggml_backend_dev_description(ggml_backend_dev_t device) {
    GGML_ASSERT(device);
    return device->iface.get_description(device);
}

void ggml_backend_dev_memory(ggml_backend_dev_t device, size_t * free, size_t * total) {
    GGML_ASSERT(device);
    device->iface.get_memory(device, free, total);
}

enum ggml_backend_dev_type ggml_backend_dev_type(ggml_backend_dev_t device) {
    GGML_ASSERT(device);
    return device->iface.get_type(device);
}

void ggml_backend_dev_get_props(ggml_backend_dev_t device, struct ggml_backend_dev_props * props) {
    GGML_ASSERT(device);
    memset(props, 0, sizeof(*props));
    device->iface.get_props(device, props);
}

ggml_backend_reg_t ggml_backend_dev_backend_reg(ggml_backend_dev_t device) {
    GGML_ASSERT(device);
    return device->reg;
}

ggml_backend_t ggml_backend_dev_init(ggml_backend_dev_t device, const char * params) {
    GGML_ASSERT(device);
    return device->iface.init_backend(device, params);
}

ggml_backend_buffer_type_t ggml_backend_dev_buffer_type(ggml_backend_dev_t device) {
    GGML_ASSERT(device);
    return device->iface.get_buffer_type(device);
}

ggml_backend_buffer_type_t ggml_backend_dev_host_buffer_type(ggml_backend_dev_t device) {
    GGML_ASSERT(device);
    if (device->iface.get_host_buffer_type == NULL) {
        return NULL;
    }

    return device->iface.get_host_buffer_type(device);
}

ggml_backend_buffer_t ggml_backend_dev_buffer_from_host_ptr(ggml_backend_dev_t device, void * ptr, size_t size, size_t max_tensor_size) {
    GGML_ASSERT(device);
    return device->iface.buffer_from_host_ptr(device, ptr, size, max_tensor_size);
}

bool ggml_backend_dev_supports_op(ggml_backend_dev_t device, const struct ggml_tensor * op) {
    GGML_ASSERT(device);
    return device->iface.supports_op(device, op);
}

bool ggml_backend_dev_supports_buft(ggml_backend_dev_t device, ggml_backend_buffer_type_t buft) {
    GGML_ASSERT(device);
    return device->iface.supports_buft(device, buft);
}

bool ggml_backend_dev_offload_op(ggml_backend_dev_t device, const struct ggml_tensor * op) {
    GGML_ASSERT(device);
    if (device->iface.offload_op != NULL) {
        return device->iface.offload_op(device, op);
    }

    return false;
}

// Backend (reg)

const char * ggml_backend_reg_name(ggml_backend_reg_t reg) {
    GGML_ASSERT(reg);
    return reg->iface.get_name(reg);
}

size_t ggml_backend_reg_dev_count(ggml_backend_reg_t reg) {
    GGML_ASSERT(reg);
    return reg->iface.get_device_count(reg);
}

ggml_backend_dev_t ggml_backend_reg_dev_get(ggml_backend_reg_t reg, size_t index) {
    GGML_ASSERT(reg);
    return reg->iface.get_device(reg, index);
}

void * ggml_backend_reg_get_proc_address(ggml_backend_reg_t reg, const char * name) {
    GGML_ASSERT(reg);
    if (!reg->iface.get_proc_address) {
        return NULL;
    }
    return reg->iface.get_proc_address(reg, name);
}

// multi-buffer buffer

struct ggml_backend_multi_buffer_context {
    ggml_backend_buffer_t * buffers;
    size_t n_buffers;
};

static void ggml_backend_multi_buffer_free_buffer(ggml_backend_buffer_t buffer) {
    GGML_ASSERT(buffer);
    ggml_backend_multi_buffer_context * ctx = (ggml_backend_multi_buffer_context *) buffer->context;
    for (size_t i = 0; i < ctx->n_buffers; i++) {
        ggml_backend_buffer_free(ctx->buffers[i]);
    }

    free(ctx->buffers);
    free(ctx);
}

static void ggml_backend_multi_buffer_clear(ggml_backend_buffer_t buffer, uint8_t value) {
    GGML_ASSERT(buffer);
    ggml_backend_multi_buffer_context * ctx = (ggml_backend_multi_buffer_context *) buffer->context;
    for (size_t i = 0; i < ctx->n_buffers; i++) {
        ggml_backend_buffer_clear(ctx->buffers[i], value);
    }
}

static const struct ggml_backend_buffer_i ggml_backend_multi_buffer_i = {
    /* .free_buffer     = */ ggml_backend_multi_buffer_free_buffer,
    /* .get_base        = */ NULL,
    /* .init_tensor     = */ NULL,
    /* .memset_tensor   = */ NULL,
    /* .set_tensor      = */ NULL,
    /* .get_tensor      = */ NULL,
    /* .set_tensor_2d   = */ NULL,
    /* .get_tensor_2d   = */ NULL,
    /* .cpy_tensor      = */ NULL,
    /* .clear           = */ ggml_backend_multi_buffer_clear,
    /* .reset           = */ NULL,
};

ggml_backend_buffer_t ggml_backend_multi_buffer_alloc_buffer(ggml_backend_buffer_t * buffers, size_t n_buffers) {
    ggml_backend_multi_buffer_context * ctx = (ggml_backend_multi_buffer_context *) malloc(sizeof(struct ggml_backend_multi_buffer_context));
    ctx->n_buffers = n_buffers;
    ctx->buffers = (ggml_backend_buffer_t *) malloc(n_buffers * sizeof(ggml_backend_buffer_t));

    GGML_ASSERT(ctx->buffers != NULL);

    size_t total_size = 0;
    for (size_t i = 0; i < n_buffers; i++) {
        ctx->buffers[i] = buffers[i];
        total_size += ggml_backend_buffer_get_size(buffers[i]);
    }

    return ggml_backend_buffer_init(buffers[0]->buft, ggml_backend_multi_buffer_i, ctx, total_size);
}

bool ggml_backend_buffer_is_multi_buffer(ggml_backend_buffer_t buffer) {
    GGML_ASSERT(buffer);
    return buffer->iface.free_buffer == ggml_backend_multi_buffer_free_buffer;
}

void ggml_backend_multi_buffer_set_usage(ggml_backend_buffer_t buffer, enum ggml_backend_buffer_usage usage) {
    GGML_ASSERT(buffer);
    GGML_ASSERT(ggml_backend_buffer_is_multi_buffer(buffer));
    ggml_backend_multi_buffer_context * ctx = (ggml_backend_multi_buffer_context *) buffer->context;
    for (size_t i = 0; i < ctx->n_buffers; i++) {
        ggml_backend_buffer_set_usage(ctx->buffers[i], usage);
    }
}

// creates a copy of the tensor with the same memory layout
static struct ggml_tensor * ggml_dup_tensor_layout(struct ggml_context * ctx, const struct ggml_tensor * tensor) {
    struct ggml_tensor * dup = ggml_dup_tensor(ctx, tensor);
    for (int i = 0; i < GGML_MAX_DIMS; i++) {
        dup->nb[i] = tensor->nb[i];
    }
    return dup;
}

static bool ggml_is_view_op(enum ggml_op op) {
    return op == GGML_OP_VIEW || op == GGML_OP_RESHAPE || op == GGML_OP_PERMUTE || op == GGML_OP_TRANSPOSE;
}

// scheduler

#ifndef GGML_SCHED_MAX_BACKENDS
#define GGML_SCHED_MAX_BACKENDS 16
#endif

#ifndef GGML_SCHED_MAX_SPLIT_INPUTS
#define GGML_SCHED_MAX_SPLIT_INPUTS 30
#endif

#ifndef GGML_SCHED_MAX_COPIES
#define GGML_SCHED_MAX_COPIES 4
#endif

namespace {

static constexpr uint16_t SCHED_AUTH_MAJOR = 1;
static constexpr uint16_t SCHED_AUTH_MINOR = 0;
static constexpr uint32_t SCHED_AUTH_MAX_EVENTS = 1000000;
static constexpr uint64_t SCHED_AUTH_MAX_TRANSFER = UINT64_C(1) << 30;
static constexpr char SCHED_AUTH_DOMAIN[] = "halofpx.scheduler-execution-authority.v2";
static constexpr uint32_t SCHED_AUTH_MAGIC = 0x32534148; // "HAS2" in canonical LE
static constexpr uint32_t SCHED_AUTH_MAX_EXPORT = UINT32_C(16) << 20;

enum sched_auth_event : uint16_t {
    SCHED_AUTH_GRAPH_TENSOR = 1,
    SCHED_AUTH_NODE_BACKEND = 2,
    SCHED_AUTH_COPY_MAP = 3,
    SCHED_AUTH_SPLIT = 4,
    SCHED_AUTH_COPY_BEFORE = 5,
    SCHED_AUTH_COPY_AFTER = 6,
    SCHED_AUTH_PARTIAL_BEFORE = 7,
    SCHED_AUTH_PARTIAL_AFTER = 8,
    SCHED_AUTH_TRAILER = 0xffff,
};

using sched_digest = std::array<uint8_t, 32>;
struct sched_auth_state {
    struct root_authority {
        uint32_t root_class;
        uint32_t role;
        uint32_t role_ordinal;
    };
    struct admitted_copy {
        uint32_t source_id;
        uint32_t destination_backend;
        uint32_t copy_slot;
        uint32_t root_class;
        uint32_t role;
        uint32_t role_ordinal;
        uint64_t generation;
        ggml_tensor * tensor;
    };
    struct admitted_root {
        uint32_t canonical_id;
        uint32_t backend;
        root_authority authority;
        ggml_tensor * tensor;
    };
    struct admitted_split {
        uint64_t parent_uid;
        uint64_t execution_sequence;
        uint64_t split_uid;
        uint32_t split_ordinal;
        uint32_t backend_ordinal;
    };
    struct copy_range {
        uint32_t source_id;
        uint32_t destination_backend;
        uint32_t copy_slot;
        uint64_t generation;
        uint64_t offset;
        uint64_t size;
        bool partial;
        bool pending;
    };
    ggml_backend_sched_authority_config config {};
    struct ggml_backend_sched_authority_result result {};
    std::unordered_map<const ggml_tensor *, uint32_t> ids;
    std::unordered_map<const ggml_tensor *, root_authority> roots;
    std::unordered_set<uint32_t> rpc_backends;
    std::unordered_map<ggml_backend_buffer_t, uint32_t> buffer_ordinals;
    std::vector<const ggml_tensor *> ordered;
    std::vector<const ggml_tensor *> original_leaves;
    sched_digest chain {};
    std::vector<copy_range> copy_ranges;
    std::vector<admitted_copy> admitted_copies;
    std::vector<admitted_root> admitted_roots;
    std::vector<ggml_backend_sched_authority_census_entry> canonical_census;
    std::vector<admitted_split> admitted_splits;
    sched_digest prepared_root {};
    sched_digest split_mapping_root {};
    struct admission_lifecycle {
        sched_digest object_id {};
        uint32_t state = GGML_BACKEND_SCHED_ADMISSION_PREPARED;
    };
    std::mutex admission_mutex;
    std::unordered_map<uint32_t, admission_lifecycle> admissions;
    uint64_t session_id = 0;
    uint64_t generation = 0;
    uint32_t exported_size = 0;
    bool finalized = false;
    bool prepared = false;
    bool census_resolved = false;
    bool computing = false;
    bool failed = false;
};

static std::atomic<uint64_t> sched_auth_next_session { 1 };

static const sched_auth_state::root_authority * sched_auth_root_for(
        const sched_auth_state * state,
        const ggml_tensor * tensor) {
    for (const ggml_tensor * cur = tensor; cur != nullptr; cur = cur->view_src) {
        const auto found = state->roots.find(cur);
        if (found != state->roots.end()) return &found->second;
    }
    return nullptr;
}

static bool sched_auth_build_canonical_census(sched_auth_state & state) {
    std::vector<ggml_backend_sched_authority_census_entry> candidates;
    candidates.reserve(state.admitted_roots.size() + state.admitted_copies.size());
    for (const auto & root : state.admitted_roots) {
        candidates.push_back({
            root.backend,
            root.canonical_id,
            static_cast<uint32_t>(GGML_BACKEND_SCHED_CENSUS_ROOT),
            static_cast<uint32_t>(
                root.authority.root_class == GGML_BACKEND_SCHED_AUTH_MUTABLE ?
                    GGML_BACKEND_SCHED_CENSUS_REGISTER :
                    GGML_BACKEND_SCHED_CENSUS_EXCLUDE),
            root.authority.root_class,
            root.authority.root_class == GGML_BACKEND_SCHED_AUTH_MUTABLE ?
                root.authority.role :
                static_cast<uint32_t>(
                    root.authority.root_class ==
                            GGML_BACKEND_SCHED_AUTH_IMMUTABLE_WEIGHT ?
                        GGML_RPC_HALOFPX_EXCLUDE_IMMUTABLE_MODEL_WEIGHT :
                        GGML_RPC_HALOFPX_EXCLUDE_LOCAL_STATE_PAYLOAD),
            root.authority.role_ordinal,
            UINT32_MAX,
            0,
            root.tensor,
        });
    }
    for (const auto & copy : state.admitted_copies) {
        const bool scheduler_copy = copy.root_class == 0;
        candidates.push_back({
            copy.destination_backend,
            copy.source_id,
            static_cast<uint32_t>(GGML_BACKEND_SCHED_CENSUS_COPY),
            static_cast<uint32_t>(
                scheduler_copy ||
                        copy.root_class == GGML_BACKEND_SCHED_AUTH_MUTABLE ?
                    GGML_BACKEND_SCHED_CENSUS_REGISTER :
                    GGML_BACKEND_SCHED_CENSUS_EXCLUDE),
            copy.root_class,
            scheduler_copy ?
                static_cast<uint32_t>(GGML_RPC_HALOFPX_MUTABLE_SCHEDULER_COPY) :
            copy.root_class == GGML_BACKEND_SCHED_AUTH_MUTABLE ?
                copy.role :
                static_cast<uint32_t>(
                    copy.root_class ==
                            GGML_BACKEND_SCHED_AUTH_IMMUTABLE_WEIGHT ?
                        GGML_RPC_HALOFPX_EXCLUDE_IMMUTABLE_MODEL_WEIGHT :
                        GGML_RPC_HALOFPX_EXCLUDE_LOCAL_STATE_PAYLOAD),
            copy.role_ordinal,
            copy.copy_slot,
            copy.generation,
            copy.tensor,
        });
    }
    const auto stable_less = [](const auto & a, const auto & b) {
        return std::tie(
            a.destination_backend_ordinal, a.disposition, a.provenance,
            a.stable_tensor_id, a.copy_slot, a.copy_generation, a.root_class,
            a.role, a.role_ordinal) <
            std::tie(
            b.destination_backend_ordinal, b.disposition, b.provenance,
            b.stable_tensor_id, b.copy_slot, b.copy_generation, b.root_class,
            b.role, b.role_ordinal);
    };
    std::sort(candidates.begin(), candidates.end(), stable_less);
    state.canonical_census.clear();
    for (const auto & candidate : candidates) {
        const auto same_stable_identity = [&candidate](const auto & prior) {
            return prior.destination_backend_ordinal ==
                       candidate.destination_backend_ordinal &&
                prior.provenance == candidate.provenance &&
                prior.stable_tensor_id == candidate.stable_tensor_id &&
                prior.copy_slot == candidate.copy_slot &&
                prior.copy_generation == candidate.copy_generation;
        };
        const auto same_runtime_tensor = [&candidate](const auto & prior) {
            return prior.destination_backend_ordinal ==
                       candidate.destination_backend_ordinal &&
                prior.runtime_tensor == candidate.runtime_tensor;
        };
        const auto same_semantics = [&candidate](const auto & prior) {
            return prior.disposition == candidate.disposition &&
                prior.root_class == candidate.root_class &&
                prior.role == candidate.role &&
                prior.role_ordinal == candidate.role_ordinal;
        };
        for (const auto & prior : state.canonical_census) {
            if ((same_stable_identity(prior) || same_runtime_tensor(prior)) &&
                (!same_stable_identity(prior) || !same_runtime_tensor(prior) ||
                 !same_semantics(prior))) {
                state.canonical_census.clear();
                return false;
            }
        }
        const auto duplicate = std::find_if(
            state.canonical_census.begin(), state.canonical_census.end(),
            [&](const auto & prior) {
                return same_stable_identity(prior) &&
                    same_runtime_tensor(prior) && same_semantics(prior);
            });
        if (duplicate == state.canonical_census.end()) {
            state.canonical_census.push_back(candidate);
        }
    }
    return true;
}

bool sched_auth_add_u64(uint64_t a, uint64_t b, uint64_t & out) {
    if (UINT64_MAX - a < b) return false;
    out = a + b;
    return true;
}

bool sched_auth_mul_u64(uint64_t a, uint64_t b, uint64_t & out) {
    if (a != 0 && b > UINT64_MAX / a) return false;
    out = a * b;
    return true;
}

bool sched_auth_ranges_overlap(uint64_t a_offset, uint64_t a_size, uint64_t b_offset, uint64_t b_size) {
    uint64_t a_end = 0, b_end = 0;
    if (!sched_auth_add_u64(a_offset, a_size, a_end) ||
        !sched_auth_add_u64(b_offset, b_size, b_end)) return true;
    return a_offset < b_end && b_offset < a_end;
}

template<typename T>
void sched_auth_le(std::vector<uint8_t> & out, T value) {
    using U = typename std::make_unsigned<T>::type;
    const U v = static_cast<U>(value);
    for (size_t i = 0; i < sizeof(T); ++i) out.push_back(static_cast<uint8_t>(v >> (8*i)));
}

void sched_auth_bytes(std::vector<uint8_t> & out, const void * data, size_t size) {
    const auto * p = static_cast<const uint8_t *>(data);
    out.insert(out.end(), p, p + size);
}

sched_digest sched_auth_sha(const void * data, size_t size) {
    sched_digest result {};
    sha256_t ctx;
    sha256_init(&ctx);
    if (size != 0) sha256_update(&ctx, static_cast<const uint8_t *>(data), size);
    sha256_final(&ctx, result.data());
    memset(&ctx, 0, sizeof(ctx));
    return result;
}

sched_digest sched_auth_hmac(const uint8_t key[32], const void * data, size_t size) {
    std::array<uint8_t, 64> inner {};
    std::array<uint8_t, 64> outer {};
    for (size_t i = 0; i < inner.size(); ++i) {
        const uint8_t byte = i < 32 ? key[i] : 0;
        inner[i] = byte ^ 0x36;
        outer[i] = byte ^ 0x5c;
    }
    sched_digest middle {};
    sched_digest result {};
    sha256_t ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, inner.data(), inner.size());
    sha256_update(&ctx, reinterpret_cast<const uint8_t *>(SCHED_AUTH_DOMAIN), sizeof(SCHED_AUTH_DOMAIN) - 1);
    if (size != 0) sha256_update(&ctx, static_cast<const uint8_t *>(data), size);
    sha256_final(&ctx, middle.data());
    sha256_init(&ctx);
    sha256_update(&ctx, outer.data(), outer.size());
    sha256_update(&ctx, middle.data(), middle.size());
    sha256_final(&ctx, result.data());
    memset(&ctx, 0, sizeof(ctx));
    std::fill(inner.begin(), inner.end(), 0);
    std::fill(outer.begin(), outer.end(), 0);
    std::fill(middle.begin(), middle.end(), 0);
    return result;
}

bool sched_auth_equal(const uint8_t * a, const uint8_t * b, size_t size) {
    uint8_t difference = 0;
    for (size_t i = 0; i < size; ++i) difference |= a[i] ^ b[i];
    return difference == 0;
}

bool sched_auth_zero(const uint8_t * data, size_t size) {
    uint8_t value = 0;
    for (size_t i = 0; i < size; ++i) value |= data[i];
    return value == 0;
}

bool sched_auth_event_record(sched_auth_state * state, sched_auth_event type, const std::vector<uint8_t> & body) {
    if (state == nullptr) return true;
    if (state->failed || body.size() > (UINT32_C(1) << 20) ||
        state->result.event_count >= state->config.max_events) {
        state->failed = true;
        state->result.status = 2;
        return false;
    }
    std::vector<uint8_t> record;
    record.reserve(4 + 2 + 2 + 4 + 8 + 4 + 32 + body.size() + 32);
    sched_auth_le<uint32_t>(record, SCHED_AUTH_MAGIC);
    sched_auth_le<uint16_t>(record, SCHED_AUTH_MAJOR);
    sched_auth_le<uint16_t>(record, static_cast<uint16_t>(type));
    sched_auth_le<uint32_t>(record, static_cast<uint32_t>(body.size()));
    sched_auth_le<uint64_t>(record, state->config.execution_sequence);
    sched_auth_le<uint32_t>(record, state->result.event_count);
    sched_auth_bytes(record, state->chain.data(), state->chain.size());
    sched_auth_bytes(record, body.data(), body.size());
    const auto next = sched_auth_hmac(state->config.key, record.data(), record.size());
    sched_auth_bytes(record, next.data(), next.size());
    if (state->exported_size > state->config.event_buffer_size ||
        record.size() > state->config.event_buffer_size - state->exported_size) {
        state->failed = true;
        state->result.status = 2;
        return false;
    }
    memcpy(state->config.event_buffer + state->exported_size, record.data(), record.size());
    state->exported_size += static_cast<uint32_t>(record.size());
    state->chain = next;
    state->result.event_count++;
    return true;
}

void sched_auth_layout(std::vector<uint8_t> & body, const ggml_tensor * tensor, uint32_t canonical_id) {
    sched_auth_le<uint32_t>(body, canonical_id);
    sched_auth_le<uint32_t>(body, tensor->type);
    for (uint32_t d = 0; d < GGML_MAX_DIMS; ++d) sched_auth_le<int64_t>(body, tensor->ne[d]);
    for (uint32_t d = 0; d < GGML_MAX_DIMS; ++d) sched_auth_le<uint64_t>(body, tensor->nb[d]);
}

bool sched_auth_view_chain(
        const sched_auth_state * state,
        const ggml_tensor * tensor,
        std::vector<uint8_t> & body) {
    std::vector<std::pair<uint32_t, uint64_t>> chain;
    std::unordered_set<const ggml_tensor *> seen;
    for (const ggml_tensor * current = tensor; current != nullptr && current->view_src != nullptr; current = current->view_src) {
        if (!seen.insert(current).second || chain.size() >= 64) return false;
        const auto found = state->ids.find(current->view_src);
        if (found == state->ids.end()) return false;
        chain.emplace_back(found->second, current->view_offs);
    }
    sched_auth_le<uint32_t>(body, static_cast<uint32_t>(chain.size()));
    for (const auto & edge : chain) {
        sched_auth_le<uint32_t>(body, edge.first);
        sched_auth_le<uint64_t>(body, edge.second);
    }
    return true;
}

bool sched_auth_allocation(
        sched_auth_state * state,
        ggml_backend_t backend,
        const ggml_tensor * tensor,
        uint32_t backend_ordinal,
        std::vector<uint8_t> & body) {
    if (backend == nullptr || tensor == nullptr || tensor->buffer == nullptr || tensor->data == nullptr) return false;
    ggml_backend_buffer_t buffer = tensor->buffer;
    const uintptr_t base = reinterpret_cast<uintptr_t>(ggml_backend_buffer_get_base(buffer));
    const uintptr_t data = reinterpret_cast<uintptr_t>(tensor->data);
    const uint64_t buffer_size = ggml_backend_buffer_get_size(buffer);
    const uint64_t tensor_size = ggml_nbytes(tensor);
    if (base == 0 || data < base || static_cast<uint64_t>(data - base) > buffer_size ||
        tensor_size > buffer_size - static_cast<uint64_t>(data - base)) return false;
    auto [it, inserted] = state->buffer_ordinals.emplace(buffer, static_cast<uint32_t>(state->buffer_ordinals.size()));
    GGML_UNUSED(inserted);
    sched_auth_le<uint32_t>(body, backend_ordinal);
    sched_auth_le<uint32_t>(body, it->second);
    sched_auth_le<uint64_t>(body, static_cast<uint64_t>(data - base));
    sched_auth_le<uint64_t>(body, tensor_size);
    sched_auth_le<uint64_t>(body, buffer_size);
    sched_auth_le<uint32_t>(body, ggml_backend_buffer_is_host(buffer) ? 1 : 0);
    sched_auth_le<uint32_t>(body, ggml_backend_buffer_get_usage(buffer));
    return true;
}

bool sched_auth_assign_graph(sched_auth_state * state, const ggml_cgraph * graph) {
    state->ids.clear();
    state->ordered.clear();
    state->original_leaves.clear();
    if (graph->n_nodes <= 0) return false;
    for (int i = 0; i < graph->n_nodes; ++i) {
        if (graph->nodes[i] == nullptr ||
            !state->ids.emplace(graph->nodes[i], static_cast<uint32_t>(state->ordered.size())).second) return false;
        state->ordered.push_back(graph->nodes[i]);
    }
    std::unordered_set<const ggml_tensor *> visiting;
    std::function<bool(const ggml_tensor *)> visit = [&](const ggml_tensor * tensor) {
        if (tensor == nullptr || state->ids.find(tensor) != state->ids.end()) return true;
        if (!visiting.insert(tensor).second) return false;
        for (uint32_t s = 0; s < GGML_MAX_SRC; ++s) {
            if (!visit(tensor->src[s])) return false;
        }
        if (!visit(tensor->view_src)) return false;
        visiting.erase(tensor);
        if (!state->ids.emplace(tensor, static_cast<uint32_t>(state->ordered.size())).second) return false;
        state->ordered.push_back(tensor);
        return true;
    };
    for (int i = 0; i < graph->n_nodes; ++i) {
        const ggml_tensor * node = graph->nodes[i];
        for (uint32_t s = 0; s < GGML_MAX_SRC; ++s) if (!visit(node->src[s])) return false;
        if (!visit(node->view_src)) return false;
    }
    for (uint32_t id = 0; id < state->ordered.size(); ++id) {
        const ggml_tensor * tensor = state->ordered[id];
        bool has_src = false;
        for (const ggml_tensor * src : tensor->src) has_src = has_src || src != nullptr;
        if (!has_src) state->original_leaves.push_back(tensor);
        std::vector<uint8_t> body;
        sched_auth_layout(body, tensor, id);
        sched_auth_le<uint32_t>(body, tensor->op);
        sched_auth_le<int32_t>(body, tensor->flags);
        for (uint32_t p = 0; p < GGML_MAX_OP_PARAMS / sizeof(int32_t); ++p) {
            sched_auth_le<int32_t>(body, tensor->op_params[p]);
        }
        uint32_t null_bitmap = 0;
        for (uint32_t s = 0; s < GGML_MAX_SRC; ++s) {
            if (tensor->src[s] == nullptr) {
                null_bitmap |= UINT32_C(1) << s;
                sched_auth_le<uint32_t>(body, UINT32_MAX);
            } else {
                const auto found = state->ids.find(tensor->src[s]);
                if (found == state->ids.end()) return false;
                sched_auth_le<uint32_t>(body, found->second);
            }
        }
        sched_auth_le<uint32_t>(body, null_bitmap);
        if (tensor->view_src == nullptr) {
            sched_auth_le<uint32_t>(body, UINT32_MAX);
            sched_auth_le<uint64_t>(body, 0);
        } else {
            const auto found = state->ids.find(tensor->view_src);
            if (found == state->ids.end()) return false;
            sched_auth_le<uint32_t>(body, found->second);
            sched_auth_le<uint64_t>(body, tensor->view_offs);
        }
        if (!sched_auth_event_record(state, SCHED_AUTH_GRAPH_TENSOR, body)) return false;
    }
    return true;
}

bool sched_auth_tensor_hash(
        ggml_backend_t backend,
        const ggml_tensor * tensor,
        size_t offset,
        size_t size,
        sched_digest & physical,
        sched_digest & logical,
        uint64_t & logical_bytes,
        uint64_t & padding_bytes) {
    if (tensor == nullptr || tensor->buffer == nullptr || backend == nullptr ||
        size == 0 || size > SCHED_AUTH_MAX_TRANSFER || offset > ggml_nbytes(tensor) ||
        size > ggml_nbytes(tensor) - offset) return false;
    std::vector<uint8_t> bytes(size);
    ggml_backend_tensor_get(tensor, bytes.data(), offset, size);
    physical = sched_auth_sha(bytes.data(), bytes.size());

    const uint64_t block = ggml_blck_size(tensor->type);
    const uint64_t type_size = ggml_type_size(tensor->type);
    if (block == 0 || type_size == 0 || tensor->ne[0] <= 0 ||
        static_cast<uint64_t>(tensor->ne[0]) % block != 0) return false;
    uint64_t row_bytes = 0;
    if (!sched_auth_mul_u64(type_size, static_cast<uint64_t>(tensor->ne[0]) / block, row_bytes)) return false;
    if (row_bytes == 0 || tensor->nb[0] != type_size) return false;
    uint64_t requested_end = 0;
    if (!sched_auth_add_u64(offset, size, requested_end)) return false;

    sha256_t logical_ctx;
    sha256_init(&logical_ctx);
    logical_bytes = 0;
    for (int64_t i3 = 0; i3 < tensor->ne[3]; ++i3) {
        for (int64_t i2 = 0; i2 < tensor->ne[2]; ++i2) {
            for (int64_t i1 = 0; i1 < tensor->ne[1]; ++i1) {
                uint64_t term3 = 0, term2 = 0, term1 = 0, row_offset = 0, row_end = 0;
                if (!sched_auth_mul_u64(static_cast<uint64_t>(i3), tensor->nb[3], term3) ||
                    !sched_auth_mul_u64(static_cast<uint64_t>(i2), tensor->nb[2], term2) ||
                    !sched_auth_mul_u64(static_cast<uint64_t>(i1), tensor->nb[1], term1) ||
                    !sched_auth_add_u64(term3, term2, row_offset) ||
                    !sched_auth_add_u64(row_offset, term1, row_offset) ||
                    !sched_auth_add_u64(row_offset, row_bytes, row_end) ||
                    row_end > ggml_nbytes(tensor)) return false;
                const uint64_t begin = std::max<uint64_t>(row_offset, offset);
                const uint64_t end = std::min<uint64_t>(row_end, requested_end);
                if (begin < end) {
                    sha256_update(&logical_ctx, bytes.data() + (begin - offset), end - begin);
                    if (UINT64_MAX - logical_bytes < end - begin) return false;
                    logical_bytes += end - begin;
                }
            }
        }
    }
    sha256_final(&logical_ctx, logical.data());
    memset(&logical_ctx, 0, sizeof(logical_ctx));
    if (logical_bytes > size) return false;
    padding_bytes = size - logical_bytes;
    return true;
}

bool sched_auth_copy_hash_event(
        sched_auth_state * state,
        sched_auth_event type,
        ggml_backend_t backend,
        uint32_t tensor_backend_ordinal,
        const ggml_tensor * tensor,
        uint32_t source_id,
        uint32_t destination_backend,
        uint32_t copy_slot,
        uint64_t generation,
        size_t offset,
        size_t size,
        size_t transferred_padding,
        sched_digest & physical,
        sched_digest & logical) {
    const bool before = type == SCHED_AUTH_COPY_BEFORE || type == SCHED_AUTH_PARTIAL_BEFORE;
    const bool partial = type == SCHED_AUTH_PARTIAL_BEFORE || type == SCHED_AUTH_PARTIAL_AFTER;
    if (!before && type != SCHED_AUTH_COPY_AFTER && type != SCHED_AUTH_PARTIAL_AFTER) return false;
    uint64_t end = 0;
    if (!sched_auth_add_u64(offset, size, end)) return false;
    if (before) {
        for (const auto & range : state->copy_ranges) {
            if (range.source_id == source_id &&
                range.destination_backend == destination_backend &&
                range.copy_slot == copy_slot &&
                range.generation == generation &&
                sched_auth_ranges_overlap(offset, size, range.offset, range.size)) return false;
        }
        state->copy_ranges.push_back({
            source_id, destination_backend, copy_slot, generation, offset, size, partial, true
        });
    } else {
        auto match = std::find_if(state->copy_ranges.begin(), state->copy_ranges.end(), [&](const sched_auth_state::copy_range & range) {
            return range.pending && range.source_id == source_id &&
                range.destination_backend == destination_backend &&
                range.copy_slot == copy_slot && range.generation == generation &&
                range.offset == offset && range.size == size && range.partial == partial;
        });
        if (match == state->copy_ranges.end()) return false;
        match->pending = false;
    }
    const size_t type_size = ggml_type_size(tensor->type);
    if (transferred_padding >= size || type_size == 0 ||
        offset % type_size != 0 || (size - transferred_padding) % type_size != 0 ||
        transferred_padding % type_size != 0) return false;
    uint64_t ignored_logical_bytes = 0, ignored_padding_bytes = 0;
    sched_digest ignored_logical {};
    if (!sched_auth_tensor_hash(backend, tensor, offset, size, physical, ignored_logical,
                                ignored_logical_bytes, ignored_padding_bytes)) return false;
    sched_digest ignored_physical {};
    uint64_t logical_bytes = 0, structural_padding = 0;
    if (!sched_auth_tensor_hash(backend, tensor, offset, size - transferred_padding,
                                ignored_physical, logical, logical_bytes, structural_padding)) return false;
    uint64_t padding_bytes = 0;
    if (!sched_auth_add_u64(structural_padding, transferred_padding, padding_bytes)) return false;
    std::vector<uint8_t> body;
    sched_auth_le<uint32_t>(body, before ? 0 : 1);
    sched_auth_layout(body, tensor, source_id);
    sched_auth_le<uint32_t>(body, destination_backend);
    sched_auth_le<uint32_t>(body, copy_slot);
    sched_auth_le<uint64_t>(body, generation);
    sched_auth_le<uint64_t>(body, offset);
    sched_auth_le<uint64_t>(body, size);
    sched_auth_le<uint64_t>(body, logical_bytes);
    sched_auth_le<uint64_t>(body, padding_bytes);
    if (!sched_auth_allocation(state, backend, tensor, tensor_backend_ordinal, body)) return false;
    if (before) {
        if (!sched_auth_view_chain(state, tensor, body)) return false;
    } else {
        sched_auth_le<uint32_t>(body, 0);
    }
    sched_auth_bytes(body, physical.data(), physical.size());
    sched_auth_bytes(body, logical.data(), logical.size());
    return sched_auth_event_record(state, type, body);
}

}

struct ggml_backend_sched_split {
    int backend_id;
    int i_start;
    int i_end;
    struct ggml_tensor * inputs[GGML_SCHED_MAX_SPLIT_INPUTS];
    uint32_t input_canonical_ids[GGML_SCHED_MAX_SPLIT_INPUTS];
    int n_inputs;
    // graph view of this split
    struct ggml_cgraph graph;
};

struct ggml_backend_sched {
    bool is_reset; // true if the scheduler has been reset since the last graph split
    bool is_alloc;

    int n_backends;

    ggml_backend_t backends[GGML_SCHED_MAX_BACKENDS];
    ggml_backend_buffer_type_t bufts[GGML_SCHED_MAX_BACKENDS];
    ggml_gallocr_t galloc;

    // hash map of the nodes in the graph
    struct ggml_hash_set  hash_set;
    int                 * hv_tensor_backend_ids; // [hash_set.size]
    struct ggml_tensor ** hv_tensor_copies;      // [hash_set.size][n_backends][n_copies]

    int * node_backend_ids; // [graph_size]
    int * leaf_backend_ids; // [graph_size]

    int * prev_node_backend_ids; // [graph_size]
    int * prev_leaf_backend_ids; // [graph_size]

    // copy of the graph with modified inputs
    struct ggml_cgraph graph;

    // graph splits
    struct ggml_backend_sched_split * splits;
    int n_splits;
    int splits_capacity;

    // pipeline parallelism support
    int n_copies;
    int cur_copy;
    int next_copy;
    ggml_backend_event_t events[GGML_SCHED_MAX_BACKENDS][GGML_SCHED_MAX_COPIES];
    struct ggml_tensor * graph_inputs[GGML_SCHED_MAX_SPLIT_INPUTS];
    int n_graph_inputs;

    struct ggml_context * ctx;

    ggml_backend_sched_eval_callback callback_eval;
    void * callback_eval_user_data;
    sched_auth_state * authority;

    char * context_buffer;
    size_t context_buffer_size;

    bool op_offload;

    int debug;

    // used for debugging graph reallocations [GGML_SCHED_DEBUG_REALLOC]
    // ref: https://github.com/ggml-org/llama.cpp/pull/17617
    int debug_realloc;
    int debug_graph_size;
    int debug_prev_graph_size;
};

#define hash_id(tensor) ggml_hash_find_or_insert(&sched->hash_set, tensor)
#define tensor_backend_id(tensor) sched->hv_tensor_backend_ids[hash_id(tensor)]
#define tensor_id_copy(id, backend_id, copy_id) sched->hv_tensor_copies[(id) * sched->n_backends * sched->n_copies + (backend_id) * sched->n_copies + (copy_id)]
#define tensor_copy(tensor, backend_id, copy_id) tensor_id_copy(hash_id(tensor), backend_id, copy_id)

// returns the priority of the backend, lower id is higher priority
static int ggml_backend_sched_backend_id(ggml_backend_sched_t sched, ggml_backend_t backend) {
    for (int i = 0; i < sched->n_backends; i++) {
        if (sched->backends[i] == backend) {
            return i;
        }
    }
    return -1;
}

static int ggml_backend_sched_backend_from_buffer(ggml_backend_sched_t sched, const struct ggml_tensor * tensor, const struct ggml_tensor * op) {
    ggml_backend_buffer_t buffer = tensor->view_src ? tensor->view_src->buffer : tensor->buffer;
    if (buffer == NULL) {
        return -1;
    }

    // find highest prio backend that supports the buffer type and the op
    for (int i = 0; i < sched->n_backends; i++) {
        if (ggml_backend_supports_buft(sched->backends[i], buffer->buft) &&
            ggml_backend_supports_op(sched->backends[i], op)) {
            return i;
        }
    }

#ifndef NDEBUG
    GGML_LOG_DEBUG("%s: warning: no backend supports op %s with a weight with buffer type %s used in tensor %s, the weight will need to be copied\n",
        __func__, ggml_op_desc(tensor), ggml_backend_buffer_name(buffer), tensor->name);
#endif

    return -1;
}

#if 0
#define GGML_SCHED_MAX_SPLITS_DEBUG 4096
static char causes[GGML_DEFAULT_GRAPH_SIZE*16 + GGML_SCHED_MAX_SPLITS_DEBUG*GGML_SCHED_MAX_SPLIT_INPUTS][128]; // debug only
#define SET_CAUSE(node, ...) sprintf(causes[hash_id(node)], __VA_ARGS__)
#define GET_CAUSE(node) causes[hash_id(node)]
#else
#define SET_CAUSE(node, ...)
#define GET_CAUSE(node) ""
#endif

// returns the backend that should be used for the node based on the current locations
static int ggml_backend_sched_backend_id_from_cur(ggml_backend_sched_t sched, struct ggml_tensor * tensor) {
    // assign pre-allocated nodes to their backend
    int cur_backend_id = ggml_backend_sched_backend_from_buffer(sched, tensor, tensor);
    if (cur_backend_id != -1) {
        SET_CAUSE(tensor, "1.dst");
        return cur_backend_id;
    }

    // view_src
    if (tensor->view_src != NULL) {
        cur_backend_id = ggml_backend_sched_backend_from_buffer(sched, tensor->view_src, tensor);
        if (cur_backend_id != -1) {
            SET_CAUSE(tensor, "1.vsrc");
            return cur_backend_id;
        }
    }

    if (tensor->buffer || (tensor->view_src && tensor->view_src->buffer)) {
        // since the tensor is pre-allocated, it cannot be moved to another backend
        ggml_backend_buffer_t buffer = tensor->view_src ? tensor->view_src->buffer : tensor->buffer;
        GGML_ABORT("pre-allocated tensor (%s) in a buffer (%s) that cannot run the operation (%s)", tensor->name, ggml_backend_buffer_name(buffer), ggml_op_name(tensor->op));
    }

    // graph input
    if (tensor->flags & GGML_TENSOR_FLAG_INPUT) {
        cur_backend_id = sched->n_backends - 1; // last backend (assumed CPU)
        SET_CAUSE(tensor, "1.inp");
        return cur_backend_id;
    }

    // operations with weights are preferably run on the same backend as the weights
    for (int i = 0; i < GGML_MAX_SRC; i++) {
        const struct ggml_tensor * src = tensor->src[i];
        if (src == NULL) {
            continue;
        }
        // skip ROPE since the rope freqs tensor is too small to choose a backend based on it
        // not an ideal solution
        if (tensor->op != GGML_OP_ROPE && src->buffer != NULL && src->buffer->usage == GGML_BACKEND_BUFFER_USAGE_WEIGHTS) {
            int src_backend_id = ggml_backend_sched_backend_from_buffer(sched, src, tensor);
            // check if a backend with higher prio wants to offload the op
            if (sched->op_offload && src_backend_id == sched->n_backends - 1 && ggml_backend_buffer_is_host(src->buffer)) {
                for (int b = 0; b < src_backend_id; b++) {
                    if (ggml_backend_supports_op(sched->backends[b], tensor) && ggml_backend_offload_op(sched->backends[b], tensor)) {
                        SET_CAUSE(tensor, "1.off");
                        return b;
                    }
                }
            }
            SET_CAUSE(tensor, "1.wgt%d", i);
            return src_backend_id;
        }
    }

    return -1;
}

static char * fmt_size(size_t size) {
    static char buffer[128];
    if (size >= 1024*1024) {
        snprintf(buffer, sizeof(buffer), "%zuM", size/1024/1024);
    } else {
        snprintf(buffer, sizeof(buffer), "%zuK", size/1024);
    }
    return buffer;
}

static void ggml_backend_sched_print_assignments(ggml_backend_sched_t sched, struct ggml_cgraph * graph) {
    int cur_split = 0;
    for (int i = 0; i < graph->n_nodes; i++) {
        if (cur_split < sched->n_splits && i == sched->splits[cur_split].i_start) {
            ggml_backend_t split_backend = sched->backends[sched->splits[cur_split].backend_id];
            GGML_LOG_DEBUG("\n## SPLIT #%d: %s # %d inputs", cur_split, ggml_backend_name(split_backend),
                sched->splits[cur_split].n_inputs);
            for (int j = 0; j < sched->splits[cur_split].n_inputs; j++) {
                if (j == 0) {
                    GGML_LOG_DEBUG(": ");
                }
                GGML_LOG_DEBUG("[%s (%5.5s)] ", sched->splits[cur_split].inputs[j]->name,
                    fmt_size(ggml_nbytes(sched->splits[cur_split].inputs[j])));
            }
            GGML_LOG_DEBUG("\n");
            cur_split++;
        }
        struct ggml_tensor * node = graph->nodes[i];
        if (ggml_is_view_op(node->op)) {
            continue;
        }
        if (sched->debug > 1) {
            ggml_backend_t tensor_backend = ggml_backend_sched_get_tensor_backend(sched, node);
            GGML_LOG_DEBUG("node #%3d (%10.10s): %20.20s (%5.5s) [%5.5s %8.8s] use=%d,c=%d:", i, ggml_op_desc(node), node->name,
                fmt_size(ggml_nbytes(node)), tensor_backend ? ggml_backend_name(tensor_backend) : "NULL", GET_CAUSE(node),
                graph->use_counts[ggml_hash_find(&graph->visited_hash_set, node)], node->flags & GGML_TENSOR_FLAG_COMPUTE ? 1 : 0);
            for (int j = 0; j < GGML_MAX_SRC; j++) {
                struct ggml_tensor * src = node->src[j];
                if (src == NULL) {
                    continue;
                }
                ggml_backend_t src_backend = ggml_backend_sched_get_tensor_backend(sched, src);
                GGML_LOG_DEBUG(" %20.20s (%5.5s) [%5.5s %8.8s]", src->name,
                    fmt_size(ggml_nbytes(src)), src_backend ? ggml_backend_name(src_backend) : "NULL", GET_CAUSE(src));
            }
            GGML_LOG_DEBUG("\n");
        }
    }
}

static bool ggml_backend_sched_buffer_supported(ggml_backend_sched_t sched, struct ggml_tensor * t, int backend_id) {
    ggml_backend_buffer_t buf = t->view_src ? t->view_src->buffer : t->buffer;
    ggml_backend_buffer_type_t buft = NULL;

    if (buf) {
        // the tensor is already allocated
        buft = buf->buft;
    } else {
        // see if the tensor already has a backend assigned, and use the buffer type of that backend
        int tensor_backend_id = tensor_backend_id(t);
        if (tensor_backend_id == -1 && t->view_src) {
            tensor_backend_id = tensor_backend_id(t->view_src);
        }
        if (tensor_backend_id != -1) {
            buft = sched->bufts[tensor_backend_id];
        }
    }

    return buft != NULL && ggml_backend_supports_buft(sched->backends[backend_id], buft);
}

static void ggml_backend_sched_set_if_supported(ggml_backend_sched_t sched, struct ggml_tensor * node, int cur_backend_id, int * node_backend_id) {
    if (ggml_backend_supports_op(sched->backends[cur_backend_id], node)) {
        *node_backend_id = cur_backend_id;
        SET_CAUSE(node, "2.sup");
    }
}

// assigns backends to ops and splits the graph into subgraphs that can be computed on the same backend
void ggml_backend_sched_split_graph(ggml_backend_sched_t sched, struct ggml_cgraph * graph) {
    // reset splits
    sched->n_splits = 0;
    sched->n_graph_inputs = 0;
    sched->is_reset = false;
    if (sched->authority != nullptr) {
        sched->authority->generation++;
        if (sched->authority->generation != 1 ||
            !sched_auth_assign_graph(sched->authority, graph)) {
            sched->authority->failed = true;
            sched->authority->result.status = 2;
            return;
        }
    }

    struct ggml_init_params params = {
        /* .mem_size =   */ sched->context_buffer_size,
        /* .mem_buffer = */ sched->context_buffer,
        /* .no_alloc =   */ true
    };

    ggml_free(sched->ctx);

    sched->ctx = ggml_init(params);
    if (sched->ctx == NULL) {
        GGML_ABORT("%s: failed to initialize context\n", __func__);
    }

    graph->uid = ggml_graph_next_uid();

    // pass 1: assign backends to ops with pre-allocated inputs
    for (int i = 0; i < graph->n_leafs; i++) {
        struct ggml_tensor * leaf = graph->leafs[i];
        int * leaf_backend_id = &tensor_backend_id(leaf);
        // do not overwrite user assignments
        if (*leaf_backend_id == -1) {
            *leaf_backend_id = ggml_backend_sched_backend_id_from_cur(sched, leaf);
        }
    }

    for (int i = 0; i < graph->n_nodes; i++) {
        struct ggml_tensor * node = graph->nodes[i];
        int * node_backend_id = &tensor_backend_id(node);
        // do not overwrite user assignments
        if (*node_backend_id == -1) {
            *node_backend_id = ggml_backend_sched_backend_id_from_cur(sched, node);

#if 0
            // src
            if (node->op == GGML_OP_NONE) {
                continue;
            }

            for (int j = 0; j < GGML_MAX_SRC; j++) {
                struct ggml_tensor * src = node->src[j];
                if (src == NULL) {
                    continue;
                }
                int * src_backend_id = &tensor_backend_id(src);
                if (*src_backend_id == -1) {
                    *src_backend_id = ggml_backend_sched_backend_id_from_cur(sched, src);
                }
            }
#endif
        }
    }

    // pass 2: expand current backend assignments
    // assign the same backend to adjacent nodes
    // expand gpu backends (i.e. non last prio) up and down, ignoring cpu (the lowest priority backend)
    // thus, cpu will never be used unless weights are on cpu, or there are no gpu ops between cpu ops
    // ops unsupported by the backend being expanded will be left unassigned so that they can be assigned later when the locations of its inputs are known
    // expand gpu down
    {
        int cur_backend_id = -1;
        for (int i = 0; i < graph->n_nodes; i++) {
            struct ggml_tensor * node = graph->nodes[i];
            if (ggml_is_view_op(node->op)) {
                continue;
            }
            int * node_backend_id = &tensor_backend_id(node);
            if (*node_backend_id != -1) {
                if (*node_backend_id == sched->n_backends - 1) {
                    // skip cpu (lowest prio backend)
                    cur_backend_id = -1;
                } else {
                    cur_backend_id = *node_backend_id;
                }
            } else if (cur_backend_id != -1) {
                ggml_backend_sched_set_if_supported(sched, node, cur_backend_id, node_backend_id);
            }
        }
    }
    // expand gpu up
    {
        int cur_backend_id = -1;
        for (int i = graph->n_nodes - 1; i >= 0; i--) {
            struct ggml_tensor * node = graph->nodes[i];
            if (ggml_is_view_op(node->op)) {
                continue;
            }
            int * node_backend_id = &tensor_backend_id(node);
            if (*node_backend_id != -1) {
                if (*node_backend_id == sched->n_backends - 1) {
                    // skip cpu (lowest prio backend)
                    cur_backend_id = -1;
                } else {
                    cur_backend_id = *node_backend_id;
                }
            } else if (cur_backend_id != -1) {
                ggml_backend_sched_set_if_supported(sched, node, cur_backend_id, node_backend_id);
            }
        }
    }
    // expand rest down
    {
        int cur_backend_id = -1;
        for (int i = 0; i < graph->n_nodes; i++) {
            struct ggml_tensor * node = graph->nodes[i];
            if (ggml_is_view_op(node->op)) {
                continue;
            }
            int * node_backend_id = &tensor_backend_id(node);
            if (*node_backend_id != -1) {
                cur_backend_id = *node_backend_id;
            } else if (cur_backend_id != -1) {
                ggml_backend_sched_set_if_supported(sched, node, cur_backend_id, node_backend_id);
            }
        }
    }
    // expand rest up
    {
        int cur_backend_id = -1;
        for (int i = graph->n_nodes - 1; i >= 0; i--) {
            struct ggml_tensor * node = graph->nodes[i];
            if (ggml_is_view_op(node->op)) {
                continue;
            }
            int * node_backend_id = &tensor_backend_id(node);
            if (*node_backend_id != -1) {
                cur_backend_id = *node_backend_id;
            } else if (cur_backend_id != -1) {
                ggml_backend_sched_set_if_supported(sched, node, cur_backend_id, node_backend_id);
            }
        }
    }

    // pass 3: upgrade nodes to higher prio backends with compatible buffer types
    // if the tensor is already in the same buffer type (*) as another higher priority backend, we should move it there
    // however, we also need to verify that the sources are in compatible buffer types
    // (*) the actual requirement is more relaxed, the buffer type of the backend should be supported by all the users of this tensor further down the graph
    // however, this is slow to verify, so we have a more strict requirement that the buffer type is the same
    // this is not uncommon since multiple backends can use host memory, with the same buffer type (eg. BLAS and CPU)
    // additionally, set remaining unassigned nodes to the backend with the most supported inputs
    // only nodes that could not be assigned during expansion due to the backend not supporting the op should be unassigned at this point
    for (int i = 0; i < graph->n_nodes; i++) {
        struct ggml_tensor * node = graph->nodes[i];
        if (ggml_is_view_op(node->op)) {
            continue;
        }
        int * node_backend_id = &tensor_backend_id(node);
        if (*node_backend_id == -1) {
            // unassigned node: find the backend with the most supported inputs
            int n_supported_best = -1;
            for (int b = 0; b < sched->n_backends; b++) {
                if (ggml_backend_supports_op(sched->backends[b], node)) {
                    int n_supported = 0;
                    for (int j = 0; j < GGML_MAX_SRC; j++) {
                        struct ggml_tensor * src = node->src[j];
                        if (src == NULL) {
                            continue;
                        }
                        if ((tensor_backend_id(src) != -1 || tensor_backend_id(src->view_src) != -1) && ggml_backend_sched_buffer_supported(sched, src, b)) {
                            n_supported++;
                        }
                    }
                    if (n_supported > n_supported_best) {
                        n_supported_best = n_supported;
                        *node_backend_id = b;
                        SET_CAUSE(node, "3.best");
                    }
                }
            }
        } else {
            // assigned node: upgrade to higher prio backend if possible
            for (int b = 0; b < *node_backend_id; b++) {
                if (sched->bufts[b] == sched->bufts[*node_backend_id] && ggml_backend_supports_op(sched->backends[b], node)) {
                    bool supported = true;
                    for (int j = 0; j < GGML_MAX_SRC; j++) {
                        struct ggml_tensor * src = node->src[j];
                        if (src == NULL) {
                            continue;
                        }
                        if (!ggml_backend_sched_buffer_supported(sched, src, b)) {
                            supported = false;
                            break;
                        }
                    }
                    if (supported) {
                        *node_backend_id = b;
                        SET_CAUSE(node, "3.upg");
                        break;
                    }
                }
            }
        }
    }

    // pass 4: assign backends to remaining src from dst and view_src
    for (int i = 0; i < graph->n_nodes; i++) {
        struct ggml_tensor * node = graph->nodes[i];
        int * cur_backend_id = &tensor_backend_id(node);
        if (node->view_src != NULL && *cur_backend_id == -1) {
            *cur_backend_id = tensor_backend_id(node->view_src);
            SET_CAUSE(node, "4.vsrc");
        }
        for (int j = 0; j < GGML_MAX_SRC; j++) {
            struct ggml_tensor * src = node->src[j];
            if (src == NULL) {
                continue;
            }
            int * src_backend_id = &tensor_backend_id(src);
            if (*src_backend_id == -1) {
                if (src->view_src != NULL) {
                    // views are always on the same backend as the source
                    *src_backend_id = tensor_backend_id(src->view_src);
                    SET_CAUSE(src, "4.vsrc");
                } else {
                    *src_backend_id = *cur_backend_id;
                    SET_CAUSE(src, "4.cur");
                }
            }
        }
        // if the node is still unassigned, assign it to the first backend that supports it
        for (int b = 0; b < sched->n_backends && *cur_backend_id == -1; b++) {
            ggml_backend_sched_set_if_supported(sched, node, b, cur_backend_id);
        }
        GGML_ASSERT(*cur_backend_id != -1);
    }

    // pass 5: split graph, find tensors that need to be copied
    if (sched->authority != nullptr) {
        for (int i = 0; i < graph->n_nodes; ++i) {
            const auto found = sched->authority->ids.find(graph->nodes[i]);
            if (found == sched->authority->ids.end()) {
                sched->authority->failed = true;
                return;
            }
            std::vector<uint8_t> body;
            sched_auth_le<uint32_t>(body, found->second);
            sched_auth_le<uint32_t>(body, tensor_backend_id(graph->nodes[i]));
            if (!sched_auth_event_record(sched->authority, SCHED_AUTH_NODE_BACKEND, body)) return;
        }
    }

    // pass 5: split graph, find tensors that need to be copied
    {
        int i_split = 0;
        struct ggml_backend_sched_split * split = &sched->splits[0];
        // find the backend of the first split, skipping view ops
        int i = 0;
        for (; i < graph->n_nodes; i++) {
            struct ggml_tensor * node = graph->nodes[i];
            if (!ggml_is_view_op(node->op)) {
                split->backend_id = tensor_backend_id(node);
                break;
            }
        }
        split->i_start = 0;
        split->n_inputs = 0;
        int cur_backend_id = split->backend_id;
        for (; i < graph->n_nodes; i++) {
            struct ggml_tensor * node = graph->nodes[i];

            if (ggml_is_view_op(node->op)) {
                continue;
            }

            const int node_backend_id = tensor_backend_id(node);

            GGML_ASSERT(node_backend_id != -1); // all nodes should be assigned by now, this can happen if there is no CPU fallback

            // check if we should start a new split based on the sources of the current node
            bool need_new_split = false;
            if (node_backend_id == cur_backend_id && split->n_inputs > 0) {
                for (int j = 0; j < GGML_MAX_SRC; j++) {
                    struct ggml_tensor * src = node->src[j];
                    if (src == NULL) {
                        continue;
                    }
                    // check if a weight is on a different and incompatible backend
                    // by starting a new split, the memory of the previously offloaded weights can be reused
                    if (src->buffer != NULL && src->buffer->usage == GGML_BACKEND_BUFFER_USAGE_WEIGHTS) {
                        int src_backend_id = tensor_backend_id(src);
                        if (src_backend_id != cur_backend_id && !ggml_backend_sched_buffer_supported(sched, src, cur_backend_id)) {
                            need_new_split = true;
                            break;
                        }
                    }
                    // check if the split has too many inputs
                    // FIXME: count the number of inputs instead of only checking when full
                    if (split->n_inputs == GGML_SCHED_MAX_SPLIT_INPUTS) {
                        const size_t id = hash_id(src);
                        int src_backend_id = sched->hv_tensor_backend_ids[id];
                        bool supported = ggml_backend_sched_buffer_supported(sched, src, cur_backend_id);
                        if (src_backend_id != cur_backend_id && tensor_id_copy(id, cur_backend_id, 0) == NULL && !supported) {
                            need_new_split = true;
                            break;
                        }
                    }
                }
            }

            if (node_backend_id != cur_backend_id || need_new_split) {
                split->i_end = i;
                i_split++;
                if (i_split >= sched->splits_capacity) {
                    sched->splits_capacity *= 2;
                    sched->splits = (ggml_backend_sched_split *)
                        realloc(sched->splits, sched->splits_capacity * sizeof(struct ggml_backend_sched_split));
                    GGML_ASSERT(sched->splits != NULL);
                }
                split = &sched->splits[i_split];
                split->backend_id = node_backend_id;
                split->i_start = i;
                split->n_inputs = 0;
                cur_backend_id = node_backend_id;
            }

            // find inputs that are not on the same backend
            for (int j = 0; j < GGML_MAX_SRC; j++) {
                struct ggml_tensor * src = node->src[j];
                if (src == NULL) {
                    continue;
                }

                size_t src_id = hash_id(src);
                const int src_backend_id = sched->hv_tensor_backend_ids[src_id];
                GGML_ASSERT(src_backend_id != -1); // all inputs should be assigned by now

                if (src->flags & GGML_TENSOR_FLAG_INPUT && sched->n_copies > 1) {
                    if (tensor_id_copy(src_id, src_backend_id, 0) == NULL) {
                        ggml_backend_t backend = sched->backends[src_backend_id];
                        for (int c = 0; c < sched->n_copies; c++) {
                            struct ggml_tensor * tensor_copy;
                            if (c == sched->cur_copy) {
                                tensor_copy = src; // use the original tensor as the current copy
                            } else {
                                tensor_copy = ggml_dup_tensor_layout(sched->ctx, src);
                                ggml_format_name(tensor_copy, "%s#%s#%d", ggml_backend_name(backend), src->name, c);
                            }
                            ggml_set_input(tensor_copy);
                            ggml_set_output(tensor_copy); // prevent ggml-alloc from overwriting the tensor
                            tensor_id_copy(src_id, src_backend_id, c) = tensor_copy;
                            SET_CAUSE(tensor_copy, "4.cpy");
                        }
                        int n_graph_inputs = sched->n_graph_inputs++;
                        GGML_ASSERT(n_graph_inputs < GGML_SCHED_MAX_SPLIT_INPUTS);
                        sched->graph_inputs[n_graph_inputs] = src;
                    }
                }

                if (src_backend_id != cur_backend_id && !ggml_backend_sched_buffer_supported(sched, src, cur_backend_id)) {
                    // create a copy of the input in the split's backend
                    bool inserted_copy = false;
                    if (tensor_id_copy(src_id, cur_backend_id, 0) == NULL) {
                        inserted_copy = true;
                        ggml_backend_t backend = sched->backends[cur_backend_id];
                        for (int c = 0; c < sched->n_copies; c++) {
                            struct ggml_tensor * tensor_copy = ggml_dup_tensor_layout(sched->ctx, src);
                            ggml_format_name(tensor_copy, "%s#%s#%d", ggml_backend_name(backend), src->name, c);
                            if (sched->n_copies > 1) {
                                ggml_set_input(tensor_copy);
                                ggml_set_output(tensor_copy); // prevent ggml-alloc from overwriting the tensor
                            }
                            tensor_id_copy(src_id, cur_backend_id, c) = tensor_copy;
                            SET_CAUSE(tensor_copy, "4.cpy");
                        }
                        int n_inputs = split->n_inputs++;
                        GGML_ASSERT(n_inputs < GGML_SCHED_MAX_SPLIT_INPUTS);
                        split->inputs[n_inputs] = src;
                        if (sched->authority != nullptr) {
                            const auto source = sched->authority->ids.find(src);
                            if (source == sched->authority->ids.end()) {
                                sched->authority->failed = true;
                                return;
                            }
                            split->input_canonical_ids[n_inputs] = source->second;
                        }
                    }
                    if (sched->authority != nullptr) {
                        const auto source = sched->authority->ids.find(src);
                        const auto consumer = sched->authority->ids.find(node);
                        if (source == sched->authority->ids.end() ||
                            consumer == sched->authority->ids.end()) {
                            sched->authority->failed = true;
                            return;
                        }
                        for (int c = 0; c < sched->n_copies; ++c) {
                            std::vector<uint8_t> body;
                            sched_auth_le<uint32_t>(body, source->second);
                            sched_auth_le<uint32_t>(body, cur_backend_id);
                            sched_auth_le<uint32_t>(body, c);
                            sched_auth_le<uint64_t>(body, sched->authority->generation);
                            sched_auth_le<uint32_t>(body, consumer->second);
                            sched_auth_le<uint32_t>(body, j);
                            sched_auth_le<uint32_t>(body, inserted_copy ? 1 : 0);
                            sched_auth_layout(body, src, source->second);
                            if (!sched_auth_view_chain(sched->authority, src, body)) {
                                sched->authority->failed = true;
                                return;
                            }
                            const ggml_tensor * destination = tensor_id_copy(src_id, cur_backend_id, c);
                            if (destination == nullptr) {
                                sched->authority->failed = true;
                                return;
                            }
                            sched_auth_layout(body, destination, source->second);
                            sched_auth_le<uint32_t>(body, 0); // generated copy has no graph view chain
                            if (!sched_auth_event_record(sched->authority, SCHED_AUTH_COPY_MAP, body)) return;
                            sched->authority->result.copy_map_count++;
                            const auto * root = sched_auth_root_for(sched->authority, src);
                            sched->authority->admitted_copies.push_back({
                                source->second,
                                static_cast<uint32_t>(cur_backend_id),
                                static_cast<uint32_t>(c),
                                root ? root->root_class : 0,
                                root ? root->role : 0,
                                root ? root->role_ordinal : source->second,
                                sched->authority->generation,
                                const_cast<ggml_tensor *>(destination),
                            });
                        }
                    }
                    node->src[j] = tensor_id_copy(src_id, cur_backend_id, sched->cur_copy);
                }
            }
        }
        split->i_end = graph->n_nodes;
        sched->n_splits = i_split + 1;
    }

    if (sched->authority != nullptr) {
        for (int i = 0; i < sched->n_splits; ++i) {
            const auto & split = sched->splits[i];
            std::vector<uint8_t> body;
            sched_auth_le<uint32_t>(body, i);
            sched_auth_le<uint32_t>(body, split.backend_id);
            sched_auth_le<uint32_t>(body, split.i_start);
            sched_auth_le<uint32_t>(body, split.i_end);
            sched_auth_le<uint32_t>(body, split.n_inputs);
            for (int j = 0; j < split.n_inputs; ++j) {
                sched_auth_le<uint32_t>(body, split.input_canonical_ids[j]);
            }
            if (!sched_auth_event_record(sched->authority, SCHED_AUTH_SPLIT, body)) return;
            sched->authority->result.split_count++;
        }
    }

    if (sched->debug) {
        ggml_backend_sched_print_assignments(sched, graph);
    }

    // swap node_backend_ids and leaf _backend_ids with prevs
    {
        int * tmp = sched->node_backend_ids;
        sched->node_backend_ids = sched->prev_node_backend_ids;
        sched->prev_node_backend_ids = tmp;

        tmp = sched->leaf_backend_ids;
        sched->leaf_backend_ids = sched->prev_leaf_backend_ids;
        sched->prev_leaf_backend_ids = tmp;
    }

    int graph_size = std::max(graph->n_nodes, graph->n_leafs) + sched->n_splits*GGML_SCHED_MAX_SPLIT_INPUTS*2*sched->n_copies;

    // remember the actual graph_size for performing reallocation checks later [GGML_SCHED_DEBUG_REALLOC]
    sched->debug_prev_graph_size = sched->debug_graph_size;
    sched->debug_graph_size = graph_size;

    if (sched->graph.size < graph_size) {
        sched->graph.size = graph_size;
        sched->graph.nodes = (ggml_tensor **) realloc(sched->graph.nodes, graph_size * sizeof(struct ggml_tensor *));
        sched->graph.leafs = (ggml_tensor **) realloc(sched->graph.leafs, graph_size * sizeof(struct ggml_tensor *));
        GGML_ASSERT(sched->graph.nodes != NULL);
        GGML_ASSERT(sched->graph.leafs != NULL);
    }
    sched->graph.n_nodes = 0;
    sched->graph.n_leafs = 0;

    struct ggml_cgraph * graph_copy = &sched->graph;

    for (int i = 0; i < sched->n_splits; i++) {
        struct ggml_backend_sched_split * split = &sched->splits[i];
        split->graph = ggml_graph_view(graph, split->i_start, split->i_end);

        // Optimize this split of the graph. This needs to happen before we make graph_copy,
        // so they are in sync.
        ggml_backend_graph_optimize(sched->backends[split->backend_id], &split->graph);

        // add inputs to the graph copy so that they are allocated by ggml-alloc at the start of the split
        for (int j = 0; j < split->n_inputs; j++) {
            assert(graph_copy->size > (graph_copy->n_nodes + 1));

            struct ggml_tensor * input = split->inputs[j];
            const size_t input_id = hash_id(input);
            struct ggml_tensor * input_cpy = tensor_id_copy(input_id, split->backend_id, sched->cur_copy);

            // add a dependency to the input source so that it is not freed before the copy is done
            struct ggml_tensor * input_dep = ggml_view_tensor(sched->ctx, input);
            input_dep->src[0] = input;
            sched->node_backend_ids[graph_copy->n_nodes] = sched->hv_tensor_backend_ids[input_id];
            graph_copy->nodes[graph_copy->n_nodes++] = input_dep;

            // add a dependency to the input copy so that it is allocated at the start of the split
            sched->node_backend_ids[graph_copy->n_nodes] = split->backend_id;
            graph_copy->nodes[graph_copy->n_nodes++] = input_cpy;
        }

        for (int j = split->i_start; j < split->i_end; j++) {
            assert(graph_copy->size > graph_copy->n_nodes);
            sched->node_backend_ids[graph_copy->n_nodes] = tensor_backend_id(graph->nodes[j]);
            graph_copy->nodes[graph_copy->n_nodes++] = graph->nodes[j];
        }
    }

    if (sched->n_copies > 1) {
        // add input copies as leafs so that they are allocated first
        for (int i = 0; i < sched->n_graph_inputs; i++) {
            struct ggml_tensor * input = sched->graph_inputs[i];
            size_t id = hash_id(input);
            int backend_id = tensor_backend_id(input);
            for (int c = 0; c < sched->n_copies; c++) {
                struct ggml_tensor * input_cpy = tensor_id_copy(id, backend_id, c);
                sched->leaf_backend_ids[graph_copy->n_leafs] = backend_id;
                assert(graph_copy->size > graph_copy->n_leafs);
                graph_copy->leafs[graph_copy->n_leafs++] = input_cpy;
            }
        }

        for (int i = 0; i < sched->n_splits; i++) {
            struct ggml_backend_sched_split * split = &sched->splits[i];
            int backend_id = split->backend_id;
            for (int j = 0; j < split->n_inputs; j++) {
                struct ggml_tensor * input = split->inputs[j];
                size_t id = hash_id(input);
                for (int c = 0; c < sched->n_copies; c++) {
                    struct ggml_tensor * input_cpy = tensor_id_copy(id, backend_id, c);
                    sched->leaf_backend_ids[graph_copy->n_leafs] = backend_id;
                    assert(graph_copy->size > graph_copy->n_leafs);
                    graph_copy->leafs[graph_copy->n_leafs++] = input_cpy;
                }
            }
        }
    }

    // add leafs from the original graph
    for (int i = 0; i < graph->n_leafs; i++) {
        struct ggml_tensor * leaf = graph->leafs[i];
        sched->leaf_backend_ids[graph_copy->n_leafs] = tensor_backend_id(leaf);
        assert(graph_copy->size > graph_copy->n_leafs);
        graph_copy->leafs[graph_copy->n_leafs++] = leaf;
    }

    // set ids for all splits
    for (int i = 0; i < sched->n_splits; ++i) {
        sched->splits[i].graph.uid = ggml_graph_next_uid();
    }
}

static bool ggml_backend_sched_alloc_splits(ggml_backend_sched_t sched) {
    bool backend_ids_changed = false;
    for (int i = 0; i < sched->graph.n_nodes; i++) {
        if (sched->node_backend_ids[i] != sched->prev_node_backend_ids[i] &&
            sched->bufts[sched->node_backend_ids[i]] != sched->bufts[sched->prev_node_backend_ids[i]]) {
            backend_ids_changed = true;
            break;
        }
    }
    if (!backend_ids_changed) {
        for (int i = 0; i < sched->graph.n_leafs; i++) {
            if (sched->leaf_backend_ids[i] != sched->prev_leaf_backend_ids[i] &&
                sched->bufts[sched->leaf_backend_ids[i]] != sched->bufts[sched->prev_leaf_backend_ids[i]]) {
                backend_ids_changed = true;
                break;
            }
        }
    }

    // allocate graph
    if (backend_ids_changed || !ggml_gallocr_alloc_graph(sched->galloc, &sched->graph)) {
#ifndef NDEBUG
        GGML_LOG_DEBUG("%s: failed to allocate graph, reserving (backend_ids_changed = %d)\n", __func__, backend_ids_changed);
#endif

        if (sched->debug_realloc > 0) {
            // we are interested only in situations where the graph was reallocated even though its size remained the same [GGML_SCHED_DEBUG_REALLOC]
            // example: https://github.com/ggml-org/llama.cpp/pull/17143
            const bool unexpected = !backend_ids_changed && sched->debug_prev_graph_size == sched->debug_graph_size;

            if (unexpected || sched->debug_realloc > 1) {
                GGML_ABORT("%s: unexpected graph reallocation (graph size = %d, nodes = %d, leafs = %d), debug_realloc = %d\n", __func__,
                        sched->debug_graph_size, sched->graph.n_nodes, sched->graph.n_leafs, sched->debug_realloc);
            }
        }

        // the re-allocation may cause the split inputs to be moved to a different address
        // synchronize without ggml_backend_sched_synchronize to avoid changing cur_copy
        for (int i = 0; i < sched->n_backends; i++) {
            ggml_backend_synchronize(sched->backends[i]);
        }

        ggml_gallocr_reserve_n(sched->galloc, &sched->graph, sched->node_backend_ids, sched->leaf_backend_ids);
        if (!ggml_gallocr_alloc_graph(sched->galloc, &sched->graph)) {
            GGML_LOG_ERROR("%s: failed to allocate graph\n", __func__);
            return false;
        }
    }

    return true;
}

static enum ggml_status ggml_backend_sched_compute_splits(ggml_backend_sched_t sched) {
    GGML_ASSERT(sched);
    if (sched->authority != nullptr && sched->authority->result.status != 0) {
        sched->authority->failed = true;
        sched->authority->result.status = 2;
        return GGML_STATUS_ABORTED;
    }
    struct ggml_backend_sched_split * splits = sched->splits;

    ggml_tensor * prev_ids_tensor = nullptr;
    std::vector<int32_t> ids;
    std::vector<ggml_bitset_t> used_ids;

    for (int split_id = 0; split_id < sched->n_splits; split_id++) {
        struct ggml_backend_sched_split * split = &splits[split_id];
        int split_backend_id = split->backend_id;
        ggml_backend_t split_backend = sched->backends[split_backend_id];

        // copy the input tensors to the split backend
        for (int input_id = 0; input_id < split->n_inputs; input_id++) {
            ggml_backend_t input_backend = ggml_backend_sched_get_tensor_backend(sched, split->inputs[input_id]);
            const int input_backend_id = ggml_backend_sched_backend_id(sched, input_backend);
            if (sched->authority != nullptr && input_backend_id < 0) {
                sched->authority->failed = true;
                return GGML_STATUS_ABORTED;
            }
            struct ggml_tensor * input = split->inputs[input_id];
            struct ggml_tensor * input_cpy = tensor_copy(input, split_backend_id, sched->cur_copy);
            sched_digest source_physical {};
            sched_digest source_logical {};

            if (input->flags & GGML_TENSOR_FLAG_INPUT) {
                // inputs from the user must be copied immediately to prevent the user overwriting the data before the copy is done
                if (sched->events[split_backend_id][sched->cur_copy] != NULL) {
                    ggml_backend_event_synchronize(sched->events[split_backend_id][sched->cur_copy]);
                } else {
                    ggml_backend_synchronize(split_backend);
                }
                if (sched->authority != nullptr) {
                    ggml_backend_synchronize(input_backend);
                    if (!sched_auth_copy_hash_event(
                            sched->authority, SCHED_AUTH_COPY_BEFORE, input_backend, input_backend_id, input,
                            split->input_canonical_ids[input_id], split_backend_id, sched->cur_copy,
                            sched->authority->generation, 0, ggml_nbytes(input),
                            0,
                            source_physical, source_logical)) {
                        sched->authority->failed = true;
                        return GGML_STATUS_ABORTED;
                    }
                }
                ggml_backend_tensor_copy(input, input_cpy);
                if (sched->authority != nullptr) {
                    ggml_backend_synchronize(split_backend);
                    sched_digest destination_physical {};
                    sched_digest destination_logical {};
                    if (!sched_auth_copy_hash_event(
                            sched->authority, SCHED_AUTH_COPY_AFTER, split_backend, split_backend_id, input_cpy,
                            split->input_canonical_ids[input_id], split_backend_id, sched->cur_copy,
                            sched->authority->generation, 0, ggml_nbytes(input_cpy),
                            0,
                            destination_physical, destination_logical) ||
                        !sched_auth_equal(source_physical.data(), destination_physical.data(), 32) ||
                        !sched_auth_equal(source_logical.data(), destination_logical.data(), 32)) {
                        sched->authority->failed = true;
                        return GGML_STATUS_ABORTED;
                    }
                    sched->authority->result.verified_copy_count++;
                }
            } else {
                // wait for the split backend to finish using the input before overwriting it
                if (sched->events[split_backend_id][sched->cur_copy] != NULL) {
                    ggml_backend_event_wait(split_backend, sched->events[split_backend_id][sched->cur_copy]);
                } else {
                    ggml_backend_synchronize(split_backend);
                }

                // when offloading MoE weights, we can reduce the amount of data copied by copying only the experts that are used
                ggml_tensor * node = split->graph.nodes[0];
                if (split->graph.n_nodes > 0 &&
                    ggml_backend_buffer_get_usage(input->buffer) == GGML_BACKEND_BUFFER_USAGE_WEIGHTS &&
                    ggml_backend_buffer_is_host(input->buffer) && (
                    (node->src[0] == input_cpy && node->op == GGML_OP_MUL_MAT_ID)
                    //|| (node->src[1] == input_cpy && node->op == GGML_OP_ADD_ID) /* GGML_OP_ADD_ID weights are small and not worth splitting */
                    )) {

                    const int64_t n_expert   = node->op == GGML_OP_MUL_MAT_ID ? input->ne[2] : input->ne[1];
                    const size_t expert_size = node->op == GGML_OP_MUL_MAT_ID ? input->nb[2] : input->nb[1];

                    ggml_backend_synchronize(input_backend);

                    // get the ids
                    ggml_tensor * ids_tensor = node->src[2];
                    ggml_backend_t ids_backend = split_backend;

                    // if the ids tensor is also an input of the split, it may not have been copied yet to the split backend
                    // in that case, we use the original ids tensor
                    for (int i = input_id + 1; i < split->n_inputs; i++) {
                        if (ids_tensor == tensor_copy(split->inputs[i], split_backend_id, sched->cur_copy)) {
                            ids_tensor = split->inputs[i];
                            ids_backend = ggml_backend_sched_get_tensor_backend(sched, split->inputs[i]);
                            break;
                        }
                    }

                    if (ids_tensor != prev_ids_tensor) {
                        ids.resize(ggml_nbytes(ids_tensor) / sizeof(int32_t));
                        ggml_backend_tensor_get_async(ids_backend, ids_tensor, ids.data(), 0, ggml_nbytes(ids_tensor));
                        ggml_backend_synchronize(ids_backend);

                        // find the used experts
                        used_ids.clear();
                        used_ids.resize(ggml_bitset_size(n_expert));
                        for (int64_t i1 = 0; i1 < ids_tensor->ne[1]; i1++) {
                            for (int64_t i0 = 0; i0 < ids_tensor->ne[0]; i0++) {
                                int32_t id = ids[i1 * ids_tensor->nb[1]/sizeof(int32_t) + i0 * ids_tensor->nb[0]/sizeof(int32_t)];
                                GGML_ASSERT(id >= 0 && id < n_expert);
                                ggml_bitset_set(used_ids.data(), id);
                            }
                        }

                        prev_ids_tensor = ids_tensor;
                    }

                    // group consecutive experts and copy them together
                    bool authority_copy_ok = true;
                    auto copy_experts = [&](int32_t first_id, int32_t last_id) {
                        const size_t expert_offset = first_id * expert_size;
                        const size_t expert_size_copy =  (last_id - first_id + 1) * expert_size;
                        const size_t padding = std::min<size_t>(expert_size, 512);
                        const size_t padding_end = last_id < n_expert - 1 ? padding : 0;
                        sched_digest partial_source_physical {};
                        sched_digest partial_source_logical {};
                        if (sched->authority != nullptr &&
                            !sched_auth_copy_hash_event(
                                sched->authority, SCHED_AUTH_PARTIAL_BEFORE, input_backend, input_backend_id, input,
                                split->input_canonical_ids[input_id], split_backend_id, sched->cur_copy,
                                sched->authority->generation, expert_offset,
                                expert_size_copy + padding_end,
                                padding_end,
                                partial_source_physical, partial_source_logical)) {
                            authority_copy_ok = false;
                            return;
                        }

                        ggml_backend_tensor_set_async(split_backend,
                            input_cpy,
                            (const uint8_t *)input->data + expert_offset, expert_offset,
                            // copy a bit extra at the to ensure there are no NaNs in the padding of the last expert
                            // this is necessary for MMQ in the CUDA backend
                            expert_size_copy + padding_end);
                        if (sched->authority != nullptr) {
                            ggml_backend_synchronize(split_backend);
                            sched_digest partial_destination_physical {};
                            sched_digest partial_destination_logical {};
                            if (!sched_auth_copy_hash_event(
                                    sched->authority, SCHED_AUTH_PARTIAL_AFTER, split_backend, split_backend_id, input_cpy,
                                    split->input_canonical_ids[input_id], split_backend_id, sched->cur_copy,
                                    sched->authority->generation, expert_offset,
                                    expert_size_copy + padding_end,
                                    padding_end,
                                    partial_destination_physical, partial_destination_logical) ||
                                !sched_auth_equal(partial_source_physical.data(), partial_destination_physical.data(), 32) ||
                                !sched_auth_equal(partial_source_logical.data(), partial_destination_logical.data(), 32)) {
                                authority_copy_ok = false;
                                return;
                            }
                            sched->authority->result.verified_partial_count++;
                        }
                    };

                    int id = 0;
                    while (!ggml_bitset_get(used_ids.data(), id)) {
                        id++;
                    }
                    int32_t first_id = id;
                    int32_t last_id = first_id;

                    for (++id; id < n_expert; ++id) {
                        if (!ggml_bitset_get(used_ids.data(), id)) {
                            continue;
                        }

                        if (id == last_id + 1) {
                            last_id = id;
                            continue;
                        }

                        copy_experts(first_id, last_id);

                        first_id = id;
                        last_id = id;
                    }
                    copy_experts(first_id, last_id);
                    if (!authority_copy_ok) {
                        sched->authority->failed = true;
                        return GGML_STATUS_ABORTED;
                    }
                } else {
                    // try async copy, but if not possible, we can still use a sync copy without synchronizing the dst backend, since we handle the synchronization here with multiple copies and events
                    // TODO: add public function to facilitate this, since applications do not have direct access to the backend interface
                    if (sched->authority != nullptr) {
                        ggml_backend_synchronize(input_backend);
                        ggml_backend_synchronize(split_backend);
                        if (!sched_auth_copy_hash_event(
                                sched->authority, SCHED_AUTH_COPY_BEFORE, input_backend, input_backend_id, input,
                                split->input_canonical_ids[input_id], split_backend_id, sched->cur_copy,
                                sched->authority->generation, 0, ggml_nbytes(input),
                                0,
                                source_physical, source_logical)) {
                            sched->authority->failed = true;
                            return GGML_STATUS_ABORTED;
                        }
                        if (!split_backend->iface.cpy_tensor_async ||
                            !split_backend->iface.cpy_tensor_async(input_backend, split_backend, input, input_cpy)) {
                            ggml_backend_tensor_copy(input, input_cpy);
                        }
                        ggml_backend_synchronize(split_backend);
                        sched_digest destination_physical {};
                        sched_digest destination_logical {};
                        if (!sched_auth_copy_hash_event(
                                sched->authority, SCHED_AUTH_COPY_AFTER, split_backend, split_backend_id, input_cpy,
                                split->input_canonical_ids[input_id], split_backend_id, sched->cur_copy,
                                sched->authority->generation, 0, ggml_nbytes(input_cpy),
                                0,
                                destination_physical, destination_logical) ||
                            !sched_auth_equal(source_physical.data(), destination_physical.data(), 32) ||
                            !sched_auth_equal(source_logical.data(), destination_logical.data(), 32)) {
                            sched->authority->failed = true;
                            return GGML_STATUS_ABORTED;
                        }
                        sched->authority->result.verified_copy_count++;
                    } else {
                        if (!split_backend->iface.cpy_tensor_async || !split_backend->iface.cpy_tensor_async(input_backend, split_backend, input, input_cpy)) {
                            ggml_backend_synchronize(input_backend);
                            if (sched->events[split_backend_id][sched->cur_copy] != NULL) {
                                ggml_backend_event_synchronize(sched->events[split_backend_id][sched->cur_copy]);
                            } else {
                                ggml_backend_synchronize(split_backend);
                            }
                            ggml_backend_tensor_copy(input, input_cpy);
                        }
                    }
                }
            }
        }

        if (!sched->callback_eval) {
            enum ggml_status ec = ggml_backend_graph_compute_async(split_backend, &split->graph);
            if (ec != GGML_STATUS_SUCCESS) {
                return ec;
            }
        } else {
            // similar to ggml_backend_compare_graph_backend
            for (int j0 = 0; j0 < split->graph.n_nodes; j0++) {
                struct ggml_tensor * t = split->graph.nodes[j0];

                // check if the user needs data from this node
                bool need = sched->callback_eval(t, true, sched->callback_eval_user_data);

                int j1 = j0;

                // determine the range [j0, j1] of nodes that can be computed together
                while (!need && j1 < split->graph.n_nodes - 1) {
                    t = split->graph.nodes[++j1];
                    need = sched->callback_eval(t, true, sched->callback_eval_user_data);
                }

                struct ggml_cgraph gv = ggml_graph_view(&split->graph, j0, j1 + 1);

                enum ggml_status ec = ggml_backend_graph_compute_async(split_backend, &gv);
                if (ec != GGML_STATUS_SUCCESS) {
                    return ec;
                }

                // TODO: pass backend to the callback, then the user can decide if they want to synchronize
                ggml_backend_synchronize(split_backend);

                if (need && !sched->callback_eval(t, false, sched->callback_eval_user_data)) {
                    break;
                }

                j0 = j1;
            }
        }

        // record the event of this copy
        if (split->n_inputs > 0) {
            if (sched->events[split_backend_id][sched->cur_copy] != NULL) {
                ggml_backend_event_record(sched->events[split_backend_id][sched->cur_copy], split_backend);
            }
        }
    }

    if (sched->authority != nullptr) {
        if (sched->authority->failed) return GGML_STATUS_ABORTED;
        sched->authority->result.status = 1;
    }
    return GGML_STATUS_SUCCESS;
}

ggml_backend_sched_t ggml_backend_sched_new(
        ggml_backend_t * backends,
        ggml_backend_buffer_type_t * bufts,
        int n_backends,
        size_t graph_size,
        bool parallel,
        bool op_offload) {
    GGML_ASSERT(n_backends > 0);
    GGML_ASSERT(n_backends <= GGML_SCHED_MAX_BACKENDS);
    GGML_ASSERT(ggml_backend_dev_type(ggml_backend_get_device(backends[n_backends - 1])) == GGML_BACKEND_DEVICE_TYPE_CPU);

    struct ggml_backend_sched * sched = (ggml_backend_sched *) calloc(1, sizeof(struct ggml_backend_sched));

    const char * GGML_SCHED_DEBUG = getenv("GGML_SCHED_DEBUG");
    sched->debug = GGML_SCHED_DEBUG ? atoi(GGML_SCHED_DEBUG) : 0;

    sched->debug_realloc = 0;
#ifdef GGML_SCHED_NO_REALLOC
    sched->debug_realloc = 1;
#endif
    const char * GGML_SCHED_DEBUG_REALLOC = getenv("GGML_SCHED_DEBUG_REALLOC");
    sched->debug_realloc = GGML_SCHED_DEBUG_REALLOC ? atoi(GGML_SCHED_DEBUG_REALLOC) : sched->debug_realloc;

    sched->n_backends = n_backends;
    sched->n_copies = parallel ? GGML_SCHED_MAX_COPIES : 1;

    // initialize hash table
    // FIXME: needs to be size*2 to account for leafs (do it in graph_split instead)
    sched->hash_set    = ggml_hash_set_new(graph_size);
    sched->hv_tensor_backend_ids = (int *) malloc(sched->hash_set.size * sizeof(sched->hv_tensor_backend_ids[0]));
    sched->hv_tensor_copies      = (ggml_tensor **) malloc(sched->hash_set.size * sched->n_backends * sched->n_copies * sizeof(struct ggml_tensor *));

    const size_t ggml_sched_max_splits = graph_size; // at most there is one split for each node in the graph
    const size_t nodes_size = graph_size + ggml_sched_max_splits*GGML_SCHED_MAX_SPLIT_INPUTS*2;
    sched->node_backend_ids = (int *) calloc(nodes_size, sizeof(sched->node_backend_ids[0]));
    sched->leaf_backend_ids = (int *) calloc(nodes_size, sizeof(sched->leaf_backend_ids[0]));
    sched->prev_node_backend_ids = (int *) calloc(nodes_size, sizeof(sched->prev_node_backend_ids[0]));
    sched->prev_leaf_backend_ids = (int *) calloc(nodes_size, sizeof(sched->prev_leaf_backend_ids[0]));

    sched->debug_graph_size = 0;
    sched->debug_prev_graph_size = 0;

    sched->context_buffer_size = ggml_sched_max_splits*GGML_SCHED_MAX_SPLIT_INPUTS*2*sizeof(struct ggml_tensor) + ggml_graph_overhead_custom(graph_size, false);
    sched->context_buffer = (char *) malloc(sched->context_buffer_size);

    const int initial_splits_capacity = 16;
    sched->splits = (ggml_backend_sched_split *) calloc(initial_splits_capacity, sizeof(sched->splits[0]));
    sched->splits_capacity = initial_splits_capacity;

    for (int b = 0; b < n_backends; b++) {
        sched->backends[b] = backends[b];
        sched->bufts[b] = bufts ? bufts[b] : ggml_backend_get_default_buffer_type(backends[b]);
        GGML_ASSERT(ggml_backend_supports_buft(backends[b], sched->bufts[b]));

        if (sched->n_copies > 1) {
            for (int c = 0; c < sched->n_copies; c++) {
                sched->events[b][c] = ggml_backend_event_new(backends[b]->device);
            }
        }
    }

    sched->galloc = ggml_gallocr_new_n(sched->bufts, n_backends);
    sched->op_offload = op_offload;

    ggml_backend_sched_reset(sched);

    return sched;
}

void ggml_backend_sched_free(ggml_backend_sched_t sched) {
    if (sched == NULL) {
        return;
    }
    for (int b = 0; b < sched->n_backends; b++) {
        for (int c = 0; c < sched->n_copies; c++) {
            ggml_backend_event_free(sched->events[b][c]);
        }
    }
    ggml_gallocr_free(sched->galloc);
    ggml_free(sched->ctx);
    ggml_hash_set_free(&sched->hash_set);
    free(sched->splits);
    free(sched->hv_tensor_backend_ids);
    free(sched->hv_tensor_copies);
    free(sched->node_backend_ids);
    free(sched->leaf_backend_ids);
    free(sched->prev_node_backend_ids);
    free(sched->prev_leaf_backend_ids);
    free(sched->context_buffer);
    free(sched->graph.nodes);
    free(sched->graph.leafs);
    if (sched->authority != nullptr) {
        memset(&sched->authority->config, 0, sizeof(sched->authority->config));
        memset(sched->authority->chain.data(), 0, sched->authority->chain.size());
        delete sched->authority;
    }
    free(sched);
}

bool ggml_backend_sched_authority_enable(
        ggml_backend_sched_t sched,
        const struct ggml_backend_sched_authority_config * config) {
    if (sched == nullptr || config == nullptr || sched->authority != nullptr ||
        config->major != SCHED_AUTH_MAJOR || config->minor != SCHED_AUTH_MINOR ||
        config->encoded_size != sizeof(*config) || config->execution_sequence == 0 ||
        config->max_events == 0 || config->max_events > SCHED_AUTH_MAX_EVENTS ||
        config->event_buffer == nullptr || config->event_buffer_size < 256 ||
        config->event_buffer_size > SCHED_AUTH_MAX_EXPORT ||
        sched_auth_zero(config->attempt_nonce, sizeof(config->attempt_nonce)) ||
        sched_auth_zero(config->key, sizeof(config->key))) return false;
    sched->authority = new (std::nothrow) sched_auth_state();
    if (sched->authority == nullptr) return false;
    sched->authority->config = *config;
    sched->authority->result.major = SCHED_AUTH_MAJOR;
    sched->authority->result.minor = SCHED_AUTH_MINOR;
    sched->authority->result.encoded_size = sizeof(sched->authority->result);
    sched->authority->result.execution_sequence = config->execution_sequence;
    memcpy(sched->authority->result.attempt_nonce, config->attempt_nonce, 32);
    memset(config->event_buffer, 0, config->event_buffer_size);
    std::vector<uint8_t> seed;
    sched_auth_bytes(seed, SCHED_AUTH_DOMAIN, sizeof(SCHED_AUTH_DOMAIN) - 1);
    sched_auth_bytes(seed, config->attempt_nonce, 32);
    sched_auth_le<uint64_t>(seed, config->execution_sequence);
    sched->authority->chain = sched_auth_hmac(config->key, seed.data(), seed.size());
    return true;
}

static bool sched_auth_handle_matches(
        ggml_backend_sched_t sched,
        const ggml_backend_sched_authority_handle * handle) {
    return sched != nullptr && handle != nullptr && sched->authority != nullptr &&
        handle->major == SCHED_AUTH_MAJOR && handle->minor == SCHED_AUTH_MINOR &&
        handle->encoded_size == sizeof(*handle) &&
        handle->session_id != 0 && handle->session_id == sched->authority->session_id &&
        handle->generation == 1 &&
        handle->execution_sequence == sched->authority->config.execution_sequence &&
        sched_auth_equal(handle->attempt_nonce, sched->authority->config.attempt_nonce, 32);
}

bool ggml_backend_sched_authority_arm(
        ggml_backend_sched_t sched,
        const ggml_backend_sched_authority_config * config,
        ggml_backend_sched_authority_handle * handle) {
    if (sched == nullptr || config == nullptr || handle == nullptr || sched->authority != nullptr ||
        !ggml_backend_sched_authority_enable(sched, config)) return false;
    auto & state = *sched->authority;
    state.session_id = sched_auth_next_session.fetch_add(1, std::memory_order_relaxed);
    if (state.session_id == 0) {
        state.session_id = sched_auth_next_session.fetch_add(1, std::memory_order_relaxed);
    }
    memset(handle, 0, sizeof(*handle));
    handle->major = SCHED_AUTH_MAJOR;
    handle->minor = SCHED_AUTH_MINOR;
    handle->encoded_size = sizeof(*handle);
    handle->session_id = state.session_id;
    handle->generation = 1;
    handle->execution_sequence = config->execution_sequence;
    memcpy(handle->attempt_nonce, config->attempt_nonce, 32);
    return true;
}

bool ggml_backend_sched_authority_register_root(
        ggml_backend_sched_t sched,
        const ggml_backend_sched_authority_handle * handle,
        ggml_tensor * tensor,
        ggml_backend_sched_authority_root_class root_class,
        uint32_t role,
        uint32_t role_ordinal) {
    if (!sched_auth_handle_matches(sched, handle) || tensor == nullptr ||
        sched->authority->prepared || sched->authority->computing ||
        root_class < GGML_BACKEND_SCHED_AUTH_MUTABLE ||
        root_class > GGML_BACKEND_SCHED_AUTH_STATE_PAYLOAD ||
        role == 0) return false;
    const sched_auth_state::root_authority value {
        static_cast<uint32_t>(root_class), role, role_ordinal
    };
    auto [it, inserted] = sched->authority->roots.emplace(tensor, value);
    return inserted ||
        (it->second.root_class == value.root_class &&
         it->second.role == value.role &&
         it->second.role_ordinal == value.role_ordinal);
}

bool ggml_backend_sched_authority_mark_rpc_backend(
        ggml_backend_sched_t sched,
        const ggml_backend_sched_authority_handle * handle,
        uint32_t backend_ordinal) {
    if (!sched_auth_handle_matches(sched, handle) || sched->authority->prepared ||
        backend_ordinal >= static_cast<uint32_t>(sched->n_backends)) return false;
    return sched->authority->rpc_backends.insert(backend_ordinal).second;
}

bool ggml_backend_sched_authority_prepare(
        ggml_backend_sched_t sched,
        const ggml_backend_sched_authority_handle * handle,
        ggml_cgraph * graph,
        ggml_backend_sched_authority_prepared * prepared) {
    if (!sched_auth_handle_matches(sched, handle) || graph == nullptr || prepared == nullptr ||
        sched->authority->prepared || sched->authority->failed ||
        sched->authority->generation != 1 || !sched->is_alloc) return false;
    auto & state = *sched->authority;
    uint32_t local_count = 0;
    uint32_t rpc_count = 0;
    std::vector<uint8_t> census;
    state.admitted_roots.clear();
    for (const ggml_tensor * leaf : state.original_leaves) {
        const auto * root = sched_auth_root_for(&state, leaf);
        if (root == nullptr) {
            GGML_LOG_ERROR(
                "%s: refusing unclassified graph leaf canonical_id=%u type=%d "
                "view=%u\n",
                __func__,
                state.ids.count(leaf) ? state.ids.at(leaf) : UINT32_MAX,
                static_cast<int>(leaf->type),
                leaf->view_src != nullptr ? 1u : 0u);
            state.failed = true;
            state.result.status = 2;
            return false;
        }
        const int backend = tensor_backend_id(const_cast<ggml_tensor *>(leaf));
        if (backend < 0) {
            state.failed = true;
            state.result.status = 2;
            return false;
        }
        const bool rpc = state.rpc_backends.count(static_cast<uint32_t>(backend)) != 0;
        rpc ? ++rpc_count : ++local_count;
        const auto id = state.ids.find(leaf);
        if (id == state.ids.end()) {
            state.failed = true;
            state.result.status = 2;
            return false;
        }
        state.admitted_roots.push_back({
            id->second,
            static_cast<uint32_t>(backend),
            *root,
            const_cast<ggml_tensor *>(leaf),
        });
        sched_auth_le<uint32_t>(census, id->second);
        sched_auth_le<uint32_t>(census, root->root_class);
        sched_auth_le<uint32_t>(census, root->role);
        sched_auth_le<uint32_t>(census, root->role_ordinal);
    }
    for (const auto & copy : state.admitted_copies) {
        if (state.rpc_backends.count(copy.destination_backend) != 0) ++rpc_count;
    }
    if (!sched_auth_build_canonical_census(state)) {
        state.failed = true;
        state.result.status = 2;
        return false;
    }
    if (graph->uid == 0 || sched->n_splits <= 0 ||
        static_cast<uint32_t>(sched->n_splits) != state.result.split_count) {
        state.failed = true;
        state.result.status = 2;
        return false;
    }
    state.admitted_splits.clear();
    std::unordered_set<uint64_t> split_uids;
    std::vector<uint8_t> split_mapping;
    sched_auth_le<uint64_t>(split_mapping, graph->uid);
    sched_auth_le<uint64_t>(split_mapping, state.config.execution_sequence);
    sched_auth_le<uint32_t>(split_mapping, static_cast<uint32_t>(sched->n_splits));
    for (int i = 0; i < sched->n_splits; ++i) {
        const auto & split = sched->splits[i];
        if (split.graph.uid == 0 || split.backend_id < 0 ||
            split.backend_id >= sched->n_backends ||
            !split_uids.insert(split.graph.uid).second) {
            state.failed = true;
            state.result.status = 2;
            return false;
        }
        const sched_auth_state::admitted_split value {
            graph->uid,
            state.config.execution_sequence,
            split.graph.uid,
            static_cast<uint32_t>(i),
            static_cast<uint32_t>(split.backend_id),
        };
        state.admitted_splits.push_back(value);
        sched_auth_le<uint64_t>(split_mapping, value.parent_uid);
        sched_auth_le<uint64_t>(split_mapping, value.execution_sequence);
        sched_auth_le<uint32_t>(split_mapping, value.split_ordinal);
        sched_auth_le<uint32_t>(split_mapping, value.backend_ordinal);
        sched_auth_le<uint64_t>(split_mapping, value.split_uid);
    }
    state.split_mapping_root =
        sched_auth_hmac(state.config.key, split_mapping.data(), split_mapping.size());
    const auto census_root = sched_auth_hmac(state.config.key, census.data(), census.size());
    std::vector<uint8_t> canonical;
    sched_auth_le<uint64_t>(canonical, graph->uid);
    sched_auth_le<uint64_t>(canonical, state.config.execution_sequence);
    sched_auth_le<uint32_t>(canonical, state.ordered.size());
    sched_auth_le<uint32_t>(canonical, state.result.split_count);
    sched_auth_le<uint32_t>(canonical, state.result.copy_map_count);
    sched_auth_le<uint32_t>(canonical, local_count);
    sched_auth_le<uint32_t>(canonical, rpc_count);
    sched_auth_bytes(canonical, state.chain.data(), state.chain.size());
    sched_auth_bytes(canonical, census_root.data(), census_root.size());
    sched_auth_bytes(canonical, state.split_mapping_root.data(), state.split_mapping_root.size());
    const auto root = sched_auth_hmac(state.config.key, canonical.data(), canonical.size());
    state.prepared_root = root;
    memset(prepared, 0, sizeof(*prepared));
    prepared->major = SCHED_AUTH_MAJOR;
    prepared->minor = SCHED_AUTH_MINOR;
    prepared->encoded_size = sizeof(*prepared);
    prepared->status = 1;
    prepared->graph_entry_count = state.ordered.size();
    prepared->split_count = state.result.split_count;
    prepared->copy_count = state.admitted_copies.size();
    prepared->local_count = local_count;
    prepared->rpc_count = rpc_count;
    prepared->graph_uid = graph->uid;
    prepared->execution_sequence = state.config.execution_sequence;
    memcpy(prepared->attempt_nonce, state.config.attempt_nonce, 32);
    memcpy(prepared->prepared_root, root.data(), 32);
    memcpy(prepared->split_mapping_root, state.split_mapping_root.data(), 32);
    std::vector<uint8_t> tagged(
        reinterpret_cast<const uint8_t *>(prepared),
        reinterpret_cast<const uint8_t *>(prepared) + offsetof(ggml_backend_sched_authority_prepared, tag));
    const auto tag = sched_auth_hmac(state.config.key, tagged.data(), tagged.size());
    memcpy(prepared->tag, tag.data(), 32);
    state.prepared = true;
    return true;
}

enum sched_auth_resolved_insert {
    SCHED_AUTH_RESOLVED_INSERTED,
    SCHED_AUTH_RESOLVED_ALIAS,
    SCHED_AUTH_RESOLVED_CONFLICT,
};

static sched_auth_resolved_insert sched_auth_insert_resolved_census(
        std::vector<ggml_backend_sched_authority_census_entry> & resolved,
        const ggml_backend_sched_authority_census_entry & entry) {
    const auto same_storage = [&entry](const auto & prior) {
        return prior.destination_backend_ordinal ==
                   entry.destination_backend_ordinal &&
            memcmp(prior.storage_tensor_identity,
                   entry.storage_tensor_identity, 32) == 0;
    };
    const auto same_semantics = [&entry](const auto & prior) {
        return prior.disposition == entry.disposition &&
            prior.root_class == entry.root_class &&
            prior.role == entry.role &&
            prior.role_ordinal == entry.role_ordinal &&
            prior.rpc_connection_epoch == entry.rpc_connection_epoch &&
            prior.rpc_device == entry.rpc_device &&
            memcmp(prior.runtime_semantic_identity,
                   entry.runtime_semantic_identity, 32) == 0 &&
            memcmp(prior.rpc_endpoint_identity,
                   entry.rpc_endpoint_identity, 32) == 0;
    };
    const auto existing = std::find_if(
        resolved.begin(), resolved.end(), same_storage);
    if (existing == resolved.end()) {
        resolved.push_back(entry);
        return SCHED_AUTH_RESOLVED_INSERTED;
    }
    return same_semantics(*existing) ?
        SCHED_AUTH_RESOLVED_ALIAS : SCHED_AUTH_RESOLVED_CONFLICT;
}

bool ggml_backend_sched_authority_resolve_census(
        ggml_backend_sched_t sched,
        const ggml_backend_sched_authority_handle * handle,
        ggml_backend_sched_authority_storage_resolver resolver,
        void * user_data) {
    if (!sched_auth_handle_matches(sched, handle) ||
        !sched->authority->prepared || sched->authority->failed ||
        sched->authority->census_resolved ||
        !sched->authority->admissions.empty() ||
        resolver == nullptr) {
        return false;
    }
    auto & state = *sched->authority;
    std::vector<ggml_backend_sched_authority_census_entry> resolved;
    resolved.reserve(state.canonical_census.size());
    for (auto entry : state.canonical_census) {
        if (entry.destination_backend_ordinal >=
                static_cast<uint32_t>(sched->n_backends) ||
            state.rpc_backends.count(entry.destination_backend_ordinal) == 0) {
            state.failed = true;
            return false;
        }
        ggml_backend_sched_authority_storage_resolution storage {};
        if (!resolver(
                user_data, entry.destination_backend_ordinal,
                entry.runtime_tensor, &storage) ||
            storage.connection_epoch == 0 ||
            sched_auth_zero(storage.endpoint_identity, 32) ||
            sched_auth_zero(storage.storage_identity, 32) ||
            sched_auth_zero(storage.runtime_semantic_identity, 32)) {
            state.failed = true;
            return false;
        }
        std::vector<uint8_t> logical;
        sched_auth_le<uint32_t>(logical, entry.destination_backend_ordinal);
        sched_auth_le<uint32_t>(logical, entry.stable_tensor_id);
        sched_auth_le<uint32_t>(logical, entry.provenance);
        sched_auth_le<uint32_t>(logical, entry.copy_slot);
        sched_auth_le<uint64_t>(logical, entry.copy_generation);
        const auto logical_digest =
            sched_auth_sha(logical.data(), logical.size());
        memcpy(entry.logical_tensor_identity, logical_digest.data(), 32);
        memcpy(entry.storage_tensor_identity, storage.storage_identity, 32);
        memcpy(
            entry.runtime_semantic_identity,
            storage.runtime_semantic_identity, 32);
        memcpy(entry.rpc_endpoint_identity, storage.endpoint_identity, 32);
        entry.rpc_connection_epoch = storage.connection_epoch;
        entry.rpc_device = storage.device;
        entry.resolved = 1;

        const auto inserted =
            sched_auth_insert_resolved_census(resolved, entry);
        if (inserted == SCHED_AUTH_RESOLVED_CONFLICT) {
            state.failed = true;
            state.canonical_census.clear();
            return false;
        }
    }
    std::set<std::pair<uint32_t, uint32_t>> role_keys;
    for (const auto & entry : resolved) {
        const uint32_t encoded_role =
            entry.disposition == GGML_BACKEND_SCHED_CENSUS_REGISTER ?
                entry.role : UINT32_C(0x80000000) + entry.role;
        if (!role_keys.emplace(encoded_role, entry.role_ordinal).second) {
            state.failed = true;
            state.canonical_census.clear();
            return false;
        }
    }
    state.canonical_census = std::move(resolved);
    state.census_resolved = true;
    return true;
}

static bool sched_auth_build_prepared_admission(
        ggml_backend_sched_t sched,
        const ggml_backend_sched_authority_handle * handle,
        const ggml_backend_sched_authority_admission_expectation * expectation,
        struct ggml_backend_sched_authority_prepared_admission * admission,
        bool seal_and_register) {
    if (!sched_auth_handle_matches(sched, handle) || expectation == nullptr ||
        admission == nullptr ||
        !sched->authority->prepared || sched->authority->failed ||
        !sched->authority->census_resolved ||
        sched->authority->admitted_splits.empty() ||
        std::any_of(
            sched->authority->canonical_census.begin(),
            sched->authority->canonical_census.end(),
            [](const auto & entry) { return entry.resolved != 1; }) ||
        expectation->major != 1 || expectation->minor != 0 ||
        expectation->encoded_size != sizeof(*expectation) ||
        expectation->allowed_operation <
            GGML_BACKEND_SCHED_OPERATION_AUTHENTICATED_EXECUTE ||
        expectation->allowed_operation >
            GGML_BACKEND_SCHED_OPERATION_AUTHENTICATED_RECOMPUTE ||
        expectation->backend_ordinal == UINT32_MAX ||
        expectation->key_generation == 0 ||
        expectation->client_connection_epoch == 0 ||
        expectation->server_connection_epoch == 0 ||
        expectation->allocation_topology_epoch == 0 ||
        expectation->issued_unix_ns == 0 ||
        expectation->expires_unix_ns <= expectation->issued_unix_ns ||
        expectation->expires_unix_ns - expectation->issued_unix_ns !=
            UINT64_C(30000000000)) {
        return false;
    }
    auto & state = *sched->authority;
    const uint64_t parent_uid = state.admitted_splits.front().parent_uid;
    if (parent_uid == 0 ||
        std::any_of(state.admitted_splits.begin(), state.admitted_splits.end(),
                    [parent_uid, &state](const auto & split) {
                        return split.parent_uid != parent_uid ||
                            split.execution_sequence != state.config.execution_sequence;
                    })) {
        return false;
    }
    memset(admission, 0, sizeof(*admission));
    admission->major = 3;
    admission->minor = 0;
    admission->encoded_size = sizeof(*admission);
    admission->capabilities = UINT64_C(0x7ff);
    admission->state = GGML_BACKEND_SCHED_ADMISSION_PREPARED;
    admission->allowed_operation = expectation->allowed_operation;
    admission->key_generation = expectation->key_generation;
    admission->scheduler_session_id = handle->session_id;
    admission->scheduler_generation = handle->generation;
    admission->execution_sequence = state.config.execution_sequence;
    admission->parent_graph_uid = parent_uid;
    admission->client_connection_epoch = expectation->client_connection_epoch;
    admission->server_connection_epoch = expectation->server_connection_epoch;
    admission->allocation_topology_epoch = expectation->allocation_topology_epoch;
    admission->split_count = static_cast<uint32_t>(state.admitted_splits.size());
    admission->backend_ordinal = expectation->backend_ordinal;
    if (admission->split_count > 64) return false;
    for (size_t i = 0; i < state.admitted_splits.size(); ++i) {
        const auto & split = state.admitted_splits[i];
        admission->ordered_splits[i].split_graph_uid = split.split_uid;
        admission->ordered_splits[i].split_ordinal = split.split_ordinal;
        admission->ordered_splits[i].backend_ordinal = split.backend_ordinal;
    }
    std::vector<std::pair<uint32_t, uint32_t>> logical_mutable;
    std::vector<std::pair<uint32_t, uint32_t>> logical_excluded;
    const auto first = std::lower_bound(
        state.canonical_census.begin(), state.canonical_census.end(),
        expectation->backend_ordinal,
        [](const auto & entry, uint32_t backend) {
            return entry.destination_backend_ordinal < backend;
        });
    const auto last = std::upper_bound(
        first, state.canonical_census.end(), expectation->backend_ordinal,
        [](uint32_t backend, const auto & entry) {
            return backend < entry.destination_backend_ordinal;
        });
    std::vector<const ggml_backend_sched_authority_census_entry *>
        logical_projection;
    for (auto it = first; it != last; ++it) logical_projection.push_back(&*it);
    for (const auto * entry_ptr : logical_projection) {
        const auto & entry = *entry_ptr;
        auto & target =
            entry.disposition == GGML_BACKEND_SCHED_CENSUS_REGISTER ?
                logical_mutable : logical_excluded;
        target.emplace_back(
            entry.disposition == GGML_BACKEND_SCHED_CENSUS_REGISTER ?
                entry.role : UINT32_C(0x80000000) + entry.role,
            entry.role_ordinal);
    }
    std::sort(logical_mutable.begin(), logical_mutable.end());
    std::sort(logical_excluded.begin(), logical_excluded.end());
    std::vector<uint8_t> logical_plan;
    sched_auth_le<uint32_t>(logical_plan, logical_mutable.size());
    sched_auth_le<uint32_t>(logical_plan, logical_excluded.size());
    for (const auto & value : logical_mutable) {
        sched_auth_le<uint32_t>(logical_plan, value.first);
        sched_auth_le<uint32_t>(logical_plan, value.second);
    }
    for (const auto & value : logical_excluded) {
        sched_auth_le<uint32_t>(logical_plan, value.first);
        sched_auth_le<uint32_t>(logical_plan, value.second);
    }
    for (const auto * entry_ptr : logical_projection) {
        const auto & entry = *entry_ptr;
        sched_auth_bytes(logical_plan, entry.logical_tensor_identity, 32);
        sched_auth_bytes(logical_plan, entry.storage_tensor_identity, 32);
        sched_auth_bytes(logical_plan, entry.runtime_semantic_identity, 32);
        sched_auth_le<uint32_t>(logical_plan, entry.disposition);
        sched_auth_le<uint32_t>(logical_plan, entry.role);
        sched_auth_le<uint32_t>(logical_plan, entry.role_ordinal);
        sched_auth_le<uint32_t>(
            logical_plan, entry.destination_backend_ordinal);
        sched_auth_bytes(logical_plan, entry.rpc_endpoint_identity, 32);
        sched_auth_le<uint32_t>(logical_plan, entry.rpc_device);
        sched_auth_le<uint64_t>(logical_plan, entry.rpc_connection_epoch);
    }
    const auto logical_root = sched_auth_sha(
        logical_plan.data(), logical_plan.size());
    admission->logical_expected_mutable_count = logical_mutable.size();
    admission->logical_expected_exclusion_count = logical_excluded.size();
    admission->issued_unix_ns = expectation->issued_unix_ns;
    admission->expires_unix_ns = expectation->expires_unix_ns;
    memcpy(admission->attempt_nonce, state.config.attempt_nonce, 32);
    memcpy(admission->prepared_graph_digest, state.prepared_root.data(), 32);
    memcpy(admission->prepared_root, state.prepared_root.data(), 32);
    memcpy(admission->split_mapping_root, state.split_mapping_root.data(), 32);
    memcpy(admission->scheduler_chain_root, state.chain.data(), 32);
    memcpy(admission->logical_expected_census_root, logical_root.data(), 32);
    std::vector<uint8_t> object_material;
    sched_auth_le<uint64_t>(object_material, admission->scheduler_session_id);
    sched_auth_le<uint64_t>(object_material, admission->scheduler_generation);
    sched_auth_le<uint64_t>(object_material, admission->execution_sequence);
    sched_auth_le<uint32_t>(object_material, admission->backend_ordinal);
    sched_auth_bytes(object_material, admission->attempt_nonce, 32);
    sched_auth_bytes(object_material, admission->prepared_root, 32);
    const auto object_id = sched_auth_hmac(
        state.config.key, object_material.data(), object_material.size());
    memcpy(admission->object_id, object_id.data(), 32);
    std::vector<uint8_t> canonical(
        reinterpret_cast<const uint8_t *>(admission),
        reinterpret_cast<const uint8_t *>(admission) +
            offsetof(ggml_backend_sched_authority_prepared_admission_wire, tag));
    if (seal_and_register) {
        const auto tag = sched_auth_hmac(state.config.key, canonical.data(), canonical.size());
        memcpy(admission->tag, tag.data(), tag.size());
        std::lock_guard<std::mutex> lock(state.admission_mutex);
        auto [it, inserted] = state.admissions.emplace(
            expectation->backend_ordinal,
            sched_auth_state::admission_lifecycle { object_id,
                GGML_BACKEND_SCHED_ADMISSION_PREPARED });
        if (!inserted) return false;
    }
    return true;
}

bool ggml_backend_sched_authority_expected_prepared_admission(
        ggml_backend_sched_t sched,
        const ggml_backend_sched_authority_handle * handle,
        const ggml_backend_sched_authority_admission_expectation * expectation,
        struct ggml_backend_sched_authority_prepared_admission * expected) {
    return sched_auth_build_prepared_admission(
        sched, handle, expectation, expected, false);
}

bool ggml_backend_sched_authority_prepared_admission(
        ggml_backend_sched_t sched,
        const ggml_backend_sched_authority_handle * handle,
        const ggml_backend_sched_authority_admission_expectation * expectation,
        struct ggml_backend_sched_authority_prepared_admission * admission) {
    return sched_auth_build_prepared_admission(
        sched, handle, expectation, admission, true);
}

bool ggml_backend_sched_authority_verify_prepared_admission(
        const struct ggml_backend_sched_authority_prepared_admission * admission,
        const uint8_t key[32],
        const struct ggml_backend_sched_authority_prepared_admission * expected) {
    if (admission == nullptr || key == nullptr || expected == nullptr ||
        !sched_auth_zero(expected->tag, sizeof(expected->tag)) ||
        admission->major != 3 || admission->minor != 0 ||
        admission->encoded_size != sizeof(*admission) ||
        admission->capabilities != UINT64_C(0x7ff) ||
        admission->state != GGML_BACKEND_SCHED_ADMISSION_PREPARED ||
        admission->allowed_operation <
            GGML_BACKEND_SCHED_OPERATION_AUTHENTICATED_EXECUTE ||
        admission->allowed_operation >
            GGML_BACKEND_SCHED_OPERATION_AUTHENTICATED_RECOMPUTE ||
        admission->key_generation == 0 ||
        admission->scheduler_session_id == 0 ||
        admission->scheduler_generation == 0 ||
        admission->execution_sequence == 0 ||
        admission->parent_graph_uid == 0 ||
        admission->client_connection_epoch == 0 ||
        admission->server_connection_epoch == 0 ||
        admission->allocation_topology_epoch == 0 ||
        admission->split_count == 0 || admission->split_count > 64 ||
        admission->backend_ordinal == UINT32_MAX ||
        admission->issued_unix_ns == 0 ||
        admission->expires_unix_ns <= admission->issued_unix_ns ||
        admission->expires_unix_ns - admission->issued_unix_ns != UINT64_C(30000000000) ||
        sched_auth_zero(admission->attempt_nonce, 32) ||
        sched_auth_zero(admission->object_id, 32) ||
        sched_auth_zero(admission->prepared_graph_digest, 32) ||
        sched_auth_zero(admission->prepared_root, 32) ||
        sched_auth_zero(admission->split_mapping_root, 32) ||
        sched_auth_zero(admission->scheduler_chain_root, 32) ||
        sched_auth_zero(admission->logical_expected_census_root, 32)) {
        return false;
    }
    const uint64_t now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    constexpr uint64_t max_clock_skew_ns = UINT64_C(5000000000);
    if (admission->issued_unix_ns > now_ns + max_clock_skew_ns ||
        now_ns > admission->expires_unix_ns) return false;
    std::vector<uint8_t> canonical(
        reinterpret_cast<const uint8_t *>(admission),
        reinterpret_cast<const uint8_t *>(admission) +
            offsetof(ggml_backend_sched_authority_prepared_admission_wire, tag));
    const auto tag = sched_auth_hmac(key, canonical.data(), canonical.size());
    return memcmp(admission->tag, tag.data(), tag.size()) == 0 &&
        memcmp(admission, expected, offsetof(ggml_backend_sched_authority_prepared_admission_wire, tag)) == 0;
}

bool ggml_backend_sched_authority_consume_prepared_admission(
        ggml_backend_sched_t sched,
        const ggml_backend_sched_authority_handle * handle,
        const struct ggml_backend_sched_authority_prepared_admission * admission) {
    if (!sched_auth_handle_matches(sched, handle) || admission == nullptr) return false;
    auto & state = *sched->authority;
    std::lock_guard<std::mutex> lock(state.admission_mutex);
    auto found = state.admissions.find(admission->backend_ordinal);
    const uint64_t now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    if (found == state.admissions.end() ||
        memcmp(found->second.object_id.data(), admission->object_id, 32) != 0 ||
        found->second.state != GGML_BACKEND_SCHED_ADMISSION_PREPARED) return false;
    if (now_ns > admission->expires_unix_ns) {
        found->second.state = GGML_BACKEND_SCHED_ADMISSION_EXPIRED;
        return false;
    }
    const auto expected_tag = sched_auth_hmac(
        state.config.key, admission,
        offsetof(ggml_backend_sched_authority_prepared_admission_wire, tag));
    if (memcmp(expected_tag.data(), admission->tag, 32) != 0) return false;
    found->second.state = GGML_BACKEND_SCHED_ADMISSION_CONSUMED;
    return true;
}

bool ggml_backend_sched_authority_abort_prepared_admission(
        ggml_backend_sched_t sched,
        const ggml_backend_sched_authority_handle * handle) {
    if (!sched_auth_handle_matches(sched, handle)) return false;
    auto & state = *sched->authority;
    std::lock_guard<std::mutex> lock(state.admission_mutex);
    bool changed = false;
    for (auto & item : state.admissions) {
        if (item.second.state == GGML_BACKEND_SCHED_ADMISSION_PREPARED) {
            item.second.state = GGML_BACKEND_SCHED_ADMISSION_ABORTED;
            changed = true;
        }
    }
    return changed;
}

size_t ggml_backend_sched_authority_split_count(
        ggml_backend_sched_t sched,
        const ggml_backend_sched_authority_handle * handle) {
    return sched_auth_handle_matches(sched, handle) && sched->authority->prepared ?
        sched->authority->admitted_splits.size() : 0;
}

bool ggml_backend_sched_authority_split_at(
        ggml_backend_sched_t sched,
        const ggml_backend_sched_authority_handle * handle,
        size_t index,
        ggml_backend_sched_authority_split * split) {
    if (!sched_auth_handle_matches(sched, handle) || split == nullptr ||
        !sched->authority->prepared ||
        index >= sched->authority->admitted_splits.size()) return false;
    const auto & value = sched->authority->admitted_splits[index];
    memset(split, 0, sizeof(*split));
    split->parent_graph_uid = value.parent_uid;
    split->execution_sequence = value.execution_sequence;
    split->split_graph_uid = value.split_uid;
    split->split_ordinal = value.split_ordinal;
    split->backend_ordinal = value.backend_ordinal;
    memcpy(split->mapping_root, sched->authority->split_mapping_root.data(), 32);
    return true;
}

size_t ggml_backend_sched_authority_copy_count(
        ggml_backend_sched_t sched,
        const ggml_backend_sched_authority_handle * handle) {
    return sched_auth_handle_matches(sched, handle) && sched->authority->prepared ?
        sched->authority->admitted_copies.size() : 0;
}

bool ggml_backend_sched_authority_copy_at(
        ggml_backend_sched_t sched,
        const ggml_backend_sched_authority_handle * handle,
        size_t index,
        ggml_backend_sched_authority_copy * copy) {
    if (!sched_auth_handle_matches(sched, handle) || copy == nullptr ||
        !sched->authority->prepared || index >= sched->authority->admitted_copies.size()) return false;
    const auto & value = sched->authority->admitted_copies[index];
    memset(copy, 0, sizeof(*copy));
    copy->source_canonical_id = value.source_id;
    copy->destination_backend_ordinal = value.destination_backend;
    copy->copy_slot = value.copy_slot;
    copy->root_class = value.root_class;
    copy->role = value.role;
    copy->role_ordinal = value.role_ordinal;
    copy->copy_generation = value.generation;
    copy->runtime_tensor = value.tensor;
    return true;
}

size_t ggml_backend_sched_authority_root_count(
        ggml_backend_sched_t sched,
        const ggml_backend_sched_authority_handle * handle) {
    return sched_auth_handle_matches(sched, handle) && sched->authority->prepared ?
        sched->authority->admitted_roots.size() : 0;
}

bool ggml_backend_sched_authority_root_at(
        ggml_backend_sched_t sched,
        const ggml_backend_sched_authority_handle * handle,
        size_t index,
        ggml_backend_sched_authority_root * root) {
    if (!sched_auth_handle_matches(sched, handle) || root == nullptr ||
        !sched->authority->prepared || index >= sched->authority->admitted_roots.size()) return false;
    const auto & value = sched->authority->admitted_roots[index];
    memset(root, 0, sizeof(*root));
    root->canonical_id = value.canonical_id;
    root->backend_ordinal = value.backend;
    root->root_class = value.authority.root_class;
    root->role = value.authority.role;
    root->role_ordinal = value.authority.role_ordinal;
    root->runtime_tensor = value.tensor;
    return true;
}

size_t ggml_backend_sched_authority_census_count(
        ggml_backend_sched_t sched,
        const ggml_backend_sched_authority_handle * handle,
        uint32_t backend_ordinal) {
    if (!sched_auth_handle_matches(sched, handle) ||
        !sched->authority->prepared) {
        return 0;
    }
    return static_cast<size_t>(std::count_if(
        sched->authority->canonical_census.begin(),
        sched->authority->canonical_census.end(),
        [backend_ordinal](const auto & entry) {
            return entry.destination_backend_ordinal == backend_ordinal;
        }));
}

bool ggml_backend_sched_authority_census_at(
        ggml_backend_sched_t sched,
        const ggml_backend_sched_authority_handle * handle,
        uint32_t backend_ordinal,
        size_t index,
        ggml_backend_sched_authority_census_entry * entry) {
    if (!sched_auth_handle_matches(sched, handle) || entry == nullptr ||
        !sched->authority->prepared) {
        return false;
    }
    size_t current = 0;
    for (const auto & candidate : sched->authority->canonical_census) {
        if (candidate.destination_backend_ordinal != backend_ordinal) continue;
        if (current++ == index) {
            *entry = candidate;
            return true;
        }
    }
    return false;
}

bool ggml_backend_sched_authority_finalize_execution(
        ggml_backend_sched_t sched,
        ggml_backend_sched_authority_handle * handle,
        struct ggml_backend_sched_authority_result * result) {
    if (!sched_auth_handle_matches(sched, handle) || result == nullptr ||
        !sched->authority->prepared) return false;
    sched->authority->computing = true;
    ggml_backend_sched_synchronize(sched);
    const bool ok = ggml_backend_sched_authority_result(sched, result);
    if (!ok) return false;
    memset(&sched->authority->config, 0, sizeof(sched->authority->config));
    memset(sched->authority->chain.data(), 0, sched->authority->chain.size());
    memset(sched->authority->prepared_root.data(), 0, sched->authority->prepared_root.size());
    memset(sched->authority->split_mapping_root.data(), 0, sched->authority->split_mapping_root.size());
    delete sched->authority;
    sched->authority = nullptr;
    memset(handle, 0, sizeof(*handle));
    return true;
}

bool ggml_backend_sched_authority_abort_execution(
        ggml_backend_sched_t sched,
        ggml_backend_sched_authority_handle * handle) {
    if (!sched_auth_handle_matches(sched, handle)) return false;
    memset(&sched->authority->config, 0, sizeof(sched->authority->config));
    memset(sched->authority->chain.data(), 0, sched->authority->chain.size());
    memset(sched->authority->prepared_root.data(), 0, sched->authority->prepared_root.size());
    memset(sched->authority->split_mapping_root.data(), 0, sched->authority->split_mapping_root.size());
    delete sched->authority;
    sched->authority = nullptr;
    memset(handle, 0, sizeof(*handle));
    return true;
}

bool ggml_backend_sched_authority_result(
        ggml_backend_sched_t sched,
        struct ggml_backend_sched_authority_result * result) {
    if (sched == nullptr || result == nullptr || sched->authority == nullptr) return false;
    auto & state = *sched->authority;
    if (std::any_of(state.copy_ranges.begin(), state.copy_ranges.end(),
                    [](const sched_auth_state::copy_range & range) { return range.pending; })) {
        state.failed = true;
        state.result.status = 2;
    }
    if (!state.finalized && !state.failed && state.result.status == 1) {
        std::vector<uint8_t> trailer;
        sched_auth_le<uint32_t>(trailer, state.result.split_count);
        sched_auth_le<uint32_t>(trailer, state.result.copy_map_count);
        sched_auth_le<uint32_t>(trailer, state.result.verified_copy_count);
        sched_auth_le<uint32_t>(trailer, state.result.verified_partial_count);
        sched_auth_le<uint32_t>(trailer, state.result.event_count);
        sched_auth_bytes(trailer, state.chain.data(), state.chain.size());
        state.result.trailer_offset = state.exported_size;
        if (!sched_auth_event_record(&state, SCHED_AUTH_TRAILER, trailer)) {
            state.failed = true;
            state.result.status = 2;
        } else {
            state.finalized = true;
        }
    }
    state.result.exported_size = state.exported_size;
    memcpy(state.result.chain_root, state.chain.data(), state.chain.size());
    memset(state.result.tag, 0, sizeof(state.result.tag));
    std::vector<uint8_t> canonical;
    canonical.reserve(sizeof(state.result));
    sched_auth_le<uint16_t>(canonical, state.result.major);
    sched_auth_le<uint16_t>(canonical, state.result.minor);
    sched_auth_le<uint32_t>(canonical, state.result.encoded_size);
    sched_auth_le<uint32_t>(canonical, state.result.status);
    sched_auth_le<uint32_t>(canonical, state.result.event_count);
    sched_auth_le<uint32_t>(canonical, state.result.split_count);
    sched_auth_le<uint32_t>(canonical, state.result.copy_map_count);
    sched_auth_le<uint32_t>(canonical, state.result.verified_copy_count);
    sched_auth_le<uint32_t>(canonical, state.result.verified_partial_count);
    sched_auth_le<uint32_t>(canonical, state.result.exported_size);
    sched_auth_le<uint32_t>(canonical, state.result.trailer_offset);
    sched_auth_le<uint64_t>(canonical, state.result.execution_sequence);
    sched_auth_bytes(canonical, state.result.attempt_nonce, 32);
    sched_auth_bytes(canonical, state.result.chain_root, 32);
    canonical.resize(canonical.size() + 32, 0);
    const auto tag = sched_auth_hmac(state.config.key, canonical.data(), canonical.size());
    memcpy(state.result.tag, tag.data(), tag.size());
    *result = state.result;
    return !state.failed && state.result.status == 1;
}

bool ggml_backend_sched_authority_admission(
        ggml_backend_sched_t sched,
        struct ggml_backend_sched_authority_admission * admission) {
    if (sched == nullptr || admission == nullptr || sched->authority == nullptr ||
        sched->authority->failed) return false;
    memset(admission, 0, sizeof(*admission));
    admission->major = SCHED_AUTH_MAJOR;
    admission->minor = SCHED_AUTH_MINOR;
    admission->encoded_size = sizeof(*admission);
    admission->execution_sequence = sched->authority->config.execution_sequence;
    memcpy(admission->attempt_nonce, sched->authority->config.attempt_nonce, 32);
    memcpy(admission->admission_root,
        sched->authority->prepared ?
            sched->authority->prepared_root.data() :
            sched->authority->chain.data(),
        32);
    return admission->execution_sequence != 0 &&
        !sched_auth_zero(admission->attempt_nonce, 32) &&
        !sched_auth_zero(admission->admission_root, 32);
}

uint32_t ggml_backend_sched_authority_self_test(void) {
    uint32_t passed = 0;
    uint64_t value = 0;
    if (sched_auth_add_u64(1, 2, value) && value == 3) passed |= 1u << 0;
    if (!sched_auth_add_u64(UINT64_MAX, 1, value)) passed |= 1u << 1;
    if (sched_auth_mul_u64(7, 9, value) && value == 63) passed |= 1u << 2;
    if (!sched_auth_mul_u64(UINT64_MAX, 2, value)) passed |= 1u << 3;

    std::array<uint8_t, 32> key {};
    std::array<uint8_t, 32> nonce {};
    for (uint32_t i = 0; i < 32; ++i) {
        key[i] = static_cast<uint8_t>(i + 1);
        nonce[i] = static_cast<uint8_t>(0xa0 + i);
    }
    const std::array<uint8_t, 4> message {{ 1, 2, 3, 4 }};
    const auto first = sched_auth_hmac(key.data(), message.data(), message.size());
    auto changed = message;
    changed[3] ^= 1;
    const auto second = sched_auth_hmac(key.data(), changed.data(), changed.size());
    if (!sched_auth_equal(first.data(), second.data(), first.size())) passed |= 1u << 4;
    if (sched_auth_equal(first.data(), first.data(), first.size())) passed |= 1u << 5;
    if (!sched_auth_zero(key.data(), key.size()) && !sched_auth_zero(nonce.data(), nonce.size())) passed |= 1u << 6;

    sched_auth_state state {};
    state.config.major = SCHED_AUTH_MAJOR;
    state.config.minor = SCHED_AUTH_MINOR;
    state.config.encoded_size = sizeof(state.config);
    state.config.max_events = 1;
    std::array<uint8_t, 512> export_buffer {};
    state.config.event_buffer = export_buffer.data();
    state.config.event_buffer_size = export_buffer.size();
    state.config.execution_sequence = 9;
    memcpy(state.config.key, key.data(), key.size());
    memcpy(state.config.attempt_nonce, nonce.data(), nonce.size());
    state.chain = first;
    std::vector<uint8_t> body;
    sched_auth_le<uint32_t>(body, 0x12345678);
    if (sched_auth_event_record(&state, SCHED_AUTH_SPLIT, body) && state.result.event_count == 1) passed |= 1u << 7;
    if (!sched_auth_event_record(&state, SCHED_AUTH_SPLIT, body) && state.failed && state.result.status == 2) passed |= 1u << 8;

    std::vector<uint8_t> little;
    sched_auth_le<uint32_t>(little, 0x12345678);
    if (little == std::vector<uint8_t>({ 0x78, 0x56, 0x34, 0x12 })) passed |= 1u << 9;
    if (sched_auth_ranges_overlap(8, 8, 12, 4) &&
        !sched_auth_ranges_overlap(8, 4, 12, 4)) passed |= 1u << 10;
    if (sched_auth_ranges_overlap(UINT64_MAX - 1, 4, 0, 1)) passed |= 1u << 11;
    ggml_tensor base {};
    ggml_tensor first_view {};
    ggml_tensor nested_view {};
    first_view.view_src = &base;
    first_view.view_offs = 64;
    nested_view.view_src = &first_view;
    nested_view.view_offs = 32;
    sched_auth_state view_state {};
    view_state.ids.emplace(&base, 7);
    view_state.ids.emplace(&first_view, 8);
    std::vector<uint8_t> view_body;
    if (sched_auth_view_chain(&view_state, &nested_view, view_body) &&
        view_body.size() == 28 && view_body[0] == 2 &&
        view_body[1] == 0 && view_body[2] == 0 && view_body[3] == 0) passed |= 1u << 12;
    sched_auth_state missing_view_state {};
    std::vector<uint8_t> missing_body;
    if (!sched_auth_view_chain(&missing_view_state, &nested_view, missing_body)) passed |= 1u << 13;
    base.view_src = &nested_view;
    view_state.ids.emplace(&nested_view, 9);
    std::vector<uint8_t> cycle_body;
    if (!sched_auth_view_chain(&view_state, &nested_view, cycle_body)) passed |= 1u << 14;
    const size_t q8_size = ggml_type_size(GGML_TYPE_Q8_0);
    if (ggml_blck_size(GGML_TYPE_Q8_0) == 32 && q8_size != 0 &&
        (q8_size * 2) % q8_size == 0 && (q8_size + 1) % q8_size != 0) passed |= 1u << 15;
    ggml_tensor strided {};
    strided.type = GGML_TYPE_F32;
    strided.ne[0] = 4; strided.ne[1] = 2; strided.ne[2] = 1; strided.ne[3] = 1;
    strided.nb[0] = 4; strided.nb[1] = 32; strided.nb[2] = 64; strided.nb[3] = 64;
    std::vector<uint8_t> strided_body;
    sched_auth_layout(strided_body, &strided, 11);
    if (strided_body.size() == 72 && strided_body[0] == 11 &&
        strided_body[48] == 32) passed |= 1u << 16;
    ggml_tensor repeated_copy_tensor {};
    sched_auth_state repeated_copy_state {};
    repeated_copy_state.admitted_copies.push_back({
        21, 0, 0, 0, 0, 21, 1, &repeated_copy_tensor
    });
    repeated_copy_state.admitted_copies.push_back(
        repeated_copy_state.admitted_copies.front());
    if (sched_auth_build_canonical_census(repeated_copy_state) &&
        repeated_copy_state.canonical_census.size() == 1 &&
        repeated_copy_state.canonical_census[0].provenance ==
            GGML_BACKEND_SCHED_CENSUS_COPY &&
        repeated_copy_state.canonical_census[0].role ==
            GGML_RPC_HALOFPX_MUTABLE_SCHEDULER_COPY) {
        passed |= 1u << 17;
    }
    sched_auth_state conflicting_copy_state {};
    conflicting_copy_state.admitted_copies =
        repeated_copy_state.admitted_copies;
    conflicting_copy_state.admitted_copies[1].role_ordinal = 22;
    if (!sched_auth_build_canonical_census(conflicting_copy_state) &&
        conflicting_copy_state.canonical_census.empty()) {
        passed |= 1u << 18;
    }
    ggml_tensor interleaved_register {};
    ggml_tensor interleaved_exclude {};
    sched_auth_state interleaved_state {};
    interleaved_state.admitted_roots.push_back({
        1, 0,
        { GGML_BACKEND_SCHED_AUTH_IMMUTABLE_WEIGHT, 1, 1 },
        &interleaved_exclude,
    });
    interleaved_state.admitted_roots.push_back({
        2, 0,
        { GGML_BACKEND_SCHED_AUTH_MUTABLE, 1, 2 },
        &interleaved_register,
    });
    if (sched_auth_build_canonical_census(interleaved_state) &&
        interleaved_state.canonical_census.size() == 2 &&
        interleaved_state.canonical_census[0].disposition ==
            GGML_BACKEND_SCHED_CENSUS_REGISTER &&
        interleaved_state.canonical_census[1].disposition ==
            GGML_BACKEND_SCHED_CENSUS_EXCLUDE) {
        passed |= 1u << 19;
    }
    ggml_backend_sched_authority_census_entry resolved_base {};
    resolved_base.destination_backend_ordinal = 0;
    resolved_base.disposition = GGML_BACKEND_SCHED_CENSUS_REGISTER;
    resolved_base.root_class = GGML_BACKEND_SCHED_AUTH_MUTABLE;
    resolved_base.role = GGML_RPC_HALOFPX_MUTABLE_TOKEN;
    resolved_base.role_ordinal = 3;
    resolved_base.rpc_connection_epoch = 9;
    resolved_base.rpc_device = 0;
    memset(resolved_base.storage_tensor_identity, 0x51, 32);
    memset(resolved_base.rpc_endpoint_identity, 0x61, 32);
    std::vector<ggml_backend_sched_authority_census_entry> resolved_entries;
    const auto base_insert =
        sched_auth_insert_resolved_census(resolved_entries, resolved_base);
    auto resolved_alias = resolved_base;
    memset(resolved_alias.logical_tensor_identity, 0x71, 32);
    const auto alias_insert =
        sched_auth_insert_resolved_census(resolved_entries, resolved_alias);
    auto resolved_conflict = resolved_base;
    resolved_conflict.disposition = GGML_BACKEND_SCHED_CENSUS_EXCLUDE;
    resolved_conflict.root_class = GGML_BACKEND_SCHED_AUTH_IMMUTABLE_WEIGHT;
    resolved_conflict.role = GGML_RPC_HALOFPX_EXCLUDE_IMMUTABLE_MODEL_WEIGHT;
    const auto conflict_insert =
        sched_auth_insert_resolved_census(resolved_entries, resolved_conflict);
    if (base_insert == SCHED_AUTH_RESOLVED_INSERTED &&
        alias_insert == SCHED_AUTH_RESOLVED_ALIAS &&
        conflict_insert == SCHED_AUTH_RESOLVED_CONFLICT &&
        resolved_entries.size() == 1) {
        passed |= 1u << 20;
    }
    memset(state.config.key, 0, sizeof(state.config.key));
    memset(state.config.attempt_nonce, 0, sizeof(state.config.attempt_nonce));
    std::fill(key.begin(), key.end(), 0);
    std::fill(nonce.begin(), nonce.end(), 0);
    return passed;
}

bool ggml_backend_sched_authority_hash_probe(
        ggml_backend_t backend,
        const struct ggml_tensor * tensor,
        size_t offset,
        size_t size,
        size_t transferred_padding,
        struct ggml_backend_sched_authority_hash_probe * result) {
    if (result == nullptr || tensor == nullptr || transferred_padding >= size) return false;
    memset(result, 0, sizeof(*result));
    const size_t type_size = ggml_type_size(tensor->type);
    if (type_size == 0 || offset % type_size != 0 ||
        (size - transferred_padding) % type_size != 0 ||
        transferred_padding % type_size != 0) return false;
    sched_digest physical {}, ignored_logical {}, ignored_physical {}, logical {};
    uint64_t ignored_logical_bytes = 0, ignored_padding_bytes = 0;
    if (!sched_auth_tensor_hash(backend, tensor, offset, size, physical, ignored_logical,
                                ignored_logical_bytes, ignored_padding_bytes)) return false;
    uint64_t structural_padding = 0;
    if (!sched_auth_tensor_hash(backend, tensor, offset, size - transferred_padding,
                                ignored_physical, logical, result->logical_bytes, structural_padding) ||
        !sched_auth_add_u64(structural_padding, transferred_padding, result->padding_bytes)) return false;
    memcpy(result->physical_digest, physical.data(), physical.size());
    memcpy(result->logical_digest, logical.data(), logical.size());
    return true;
}

void ggml_backend_sched_reset(ggml_backend_sched_t sched) {
    GGML_ASSERT(sched);
    // reset state for the next run
    if (!sched->is_reset) {
        ggml_hash_set_reset(&sched->hash_set);
        memset(sched->hv_tensor_backend_ids, -1, sched->hash_set.size * sizeof(sched->hv_tensor_backend_ids[0]));
        memset(sched->hv_tensor_copies,       0, sched->hash_set.size * sched->n_backends * sched->n_copies * sizeof(struct ggml_tensor *));
        sched->is_reset = true;
    }
    sched->is_alloc = false;
}

void ggml_backend_sched_reserve_size(ggml_backend_sched_t sched, struct ggml_cgraph * measure_graph, size_t * sizes) {
    GGML_ASSERT(sched);
    GGML_ASSERT((int)sched->hash_set.size >= measure_graph->n_nodes + measure_graph->n_leafs);
    GGML_ASSERT(sizes);

    ggml_backend_sched_reset(sched);

    ggml_backend_sched_synchronize(sched);

    ggml_backend_sched_split_graph(sched, measure_graph);
    if (sched->authority != nullptr && sched->authority->failed) return;

    ggml_gallocr_reserve_n_size(sched->galloc, &sched->graph, sched->node_backend_ids, sched->leaf_backend_ids, sizes);
}

bool ggml_backend_sched_reserve(ggml_backend_sched_t sched, struct ggml_cgraph * measure_graph) {
    GGML_ASSERT(sched);
    GGML_ASSERT((int)sched->hash_set.size >= measure_graph->n_nodes + measure_graph->n_leafs);

    ggml_backend_sched_synchronize(sched);

    ggml_backend_sched_split_graph(sched, measure_graph);
    if (sched->authority != nullptr && sched->authority->failed) return false;

    if (!ggml_gallocr_reserve_n(sched->galloc, &sched->graph, sched->node_backend_ids, sched->leaf_backend_ids)) {
        return false;
    }

    ggml_backend_sched_reset(sched);

    return true;
}

bool ggml_backend_sched_alloc_graph(ggml_backend_sched_t sched, struct ggml_cgraph * graph) {
    GGML_ASSERT(sched);
    GGML_ASSERT((int)sched->hash_set.size >= graph->n_nodes + graph->n_leafs);
    GGML_ASSERT(!sched->is_alloc);

    sched->cur_copy = sched->next_copy;
    sched->next_copy = (sched->next_copy + 1) % sched->n_copies;

    ggml_backend_sched_split_graph(sched, graph);
    if (sched->authority != nullptr && sched->authority->failed) return false;

    if (!ggml_backend_sched_alloc_splits(sched)) {
        return false;
    }

    sched->is_alloc = true;

    return true;
}

enum ggml_status ggml_backend_sched_graph_compute(ggml_backend_sched_t sched, struct ggml_cgraph * graph) {
    enum ggml_status err = ggml_backend_sched_graph_compute_async(sched, graph);
    ggml_backend_sched_synchronize(sched);
    return err;
}

enum ggml_status ggml_backend_sched_graph_compute_async(ggml_backend_sched_t sched, struct ggml_cgraph * graph) {
    GGML_ASSERT(sched);
    if (!sched->is_reset && !sched->is_alloc) {
        ggml_backend_sched_reset(sched);
    }

    if (!sched->is_alloc) {
        if (!ggml_backend_sched_alloc_graph(sched, graph)) {
            return GGML_STATUS_ALLOC_FAILED;
        }
    }

    return ggml_backend_sched_compute_splits(sched);
}

void ggml_backend_sched_synchronize(ggml_backend_sched_t sched) {
    GGML_ASSERT(sched);
    for (int i = 0; i < sched->n_backends; i++) {
        ggml_backend_synchronize(sched->backends[i]);
    }
    if (!sched->is_alloc) {
        // if the graph is not already allocated, always use copy 0 after a synchronization
        // this ensures that during generation the same copy is used every time,
        // which avoids changes in the graph that could cause CUDA or other graphs to be disabled
        sched->next_copy = 0;
    }
}

void ggml_backend_sched_set_eval_callback(ggml_backend_sched_t sched, ggml_backend_sched_eval_callback callback, void * user_data) {
    GGML_ASSERT(sched);
    sched->callback_eval = callback;
    sched->callback_eval_user_data = user_data;
}

int ggml_backend_sched_get_n_splits(ggml_backend_sched_t sched) {
    GGML_ASSERT(sched);
    return sched->n_splits;
}

int ggml_backend_sched_get_n_copies(ggml_backend_sched_t sched) {
    GGML_ASSERT(sched);
    return sched->n_copies;
}

int ggml_backend_sched_get_n_backends(ggml_backend_sched_t sched) {
    GGML_ASSERT(sched);
    return sched->n_backends;
}

ggml_backend_t ggml_backend_sched_get_backend(ggml_backend_sched_t sched, int i) {
    GGML_ASSERT(sched);
    GGML_ASSERT(i >= 0 && i < sched->n_backends);
    return sched->backends[i];
}

ggml_backend_buffer_type_t ggml_backend_sched_get_buffer_type(ggml_backend_sched_t sched, ggml_backend_t backend) {
    GGML_ASSERT(sched);
    int backend_index = ggml_backend_sched_backend_id(sched, backend);
    GGML_ASSERT(backend_index >= 0 && backend_index < sched->n_backends);

    return sched->bufts[backend_index];
}

size_t ggml_backend_sched_get_buffer_size(ggml_backend_sched_t sched, ggml_backend_t backend) {
    GGML_ASSERT(sched);
    int backend_index = ggml_backend_sched_backend_id(sched, backend);
    GGML_ASSERT(backend_index >= 0 && backend_index < sched->n_backends);

    return ggml_gallocr_get_buffer_size(sched->galloc, backend_index);
}

void ggml_backend_sched_set_tensor_backend(ggml_backend_sched_t sched, struct ggml_tensor * node, ggml_backend_t backend) {
    GGML_ASSERT(sched);
    int backend_index = ggml_backend_sched_backend_id(sched, backend);
    GGML_ASSERT(backend_index >= 0 && backend_index < sched->n_backends);
    tensor_backend_id(node) = backend_index;
    SET_CAUSE(node, "usr");
    sched->is_reset = false;
}

ggml_backend_t ggml_backend_sched_get_tensor_backend(ggml_backend_sched_t sched, struct ggml_tensor * node) {
    GGML_ASSERT(sched);
    int backend_index = tensor_backend_id(node);
    if (backend_index == -1) {
        return NULL;
    }
    return sched->backends[backend_index];
}

// utils

enum ggml_status ggml_backend_view_init(struct ggml_tensor * tensor) {
    GGML_ASSERT(tensor);
    GGML_ASSERT(tensor->buffer == NULL);
    GGML_ASSERT(tensor->view_src != NULL);
    GGML_ASSERT(tensor->view_src->buffer != NULL);
    GGML_ASSERT(tensor->view_src->data != NULL);

    tensor->buffer = tensor->view_src->buffer;
    tensor->data = (char *)tensor->view_src->data + tensor->view_offs;
    return ggml_backend_buffer_init_tensor(tensor->buffer, tensor);
}

enum ggml_status ggml_backend_tensor_alloc(ggml_backend_buffer_t buffer, struct ggml_tensor * tensor, void * addr) {
    GGML_ASSERT(tensor);
    GGML_ASSERT(tensor->buffer == NULL);
    GGML_ASSERT(tensor->data == NULL);
    GGML_ASSERT(tensor->view_src == NULL);
    GGML_ASSERT(addr >= ggml_backend_buffer_get_base(buffer));
    GGML_ASSERT(ggml_backend_buffer_is_meta(buffer) ||
        (char *) addr + ggml_backend_buffer_get_alloc_size(buffer, tensor) <=
        (char *) ggml_backend_buffer_get_base(buffer) + ggml_backend_buffer_get_size(buffer));

    tensor->buffer = buffer;
    tensor->data = addr;
    return ggml_backend_buffer_init_tensor(buffer, tensor);
}

static struct ggml_tensor * graph_copy_dup_tensor(struct ggml_hash_set hash_set, struct ggml_tensor ** node_copies,
    struct ggml_context * ctx_allocated, struct ggml_context * ctx_unallocated, struct ggml_tensor * src) {

    GGML_ASSERT(src != NULL);
    GGML_ASSERT(src->data && "graph must be allocated");

    size_t id = ggml_hash_insert(&hash_set, src);
    if (id == GGML_HASHSET_ALREADY_EXISTS) {
        return node_copies[ggml_hash_find(&hash_set, src)];
    }

    struct ggml_tensor * dst = ggml_dup_tensor_layout(src->data && !src->view_src ? ctx_allocated : ctx_unallocated, src);
    if (src->view_src != NULL) {
        dst->view_src = graph_copy_dup_tensor(hash_set, node_copies, ctx_allocated, ctx_unallocated, src->view_src);
        dst->view_offs = src->view_offs;
    }
    dst->op = src->op;
    dst->flags = src->flags;
    memcpy(dst->op_params, src->op_params, sizeof(dst->op_params));
    ggml_set_name(dst, src->name);

    // copy src
    for (int i = 0; i < GGML_MAX_SRC; i++) {
        struct ggml_tensor * s = src->src[i];
        if (s == NULL) {
            continue;
        }
        dst->src[i] = graph_copy_dup_tensor(hash_set, node_copies, ctx_allocated, ctx_unallocated, s);
    }

    node_copies[id] = dst;
    return dst;
}

static void graph_copy_init_tensor(struct ggml_hash_set * hash_set, struct ggml_tensor ** node_copies, bool * node_init, struct ggml_tensor * src) {
    size_t id = ggml_hash_find(hash_set, src);
    if (node_init[id]) {
        return;
    }
    node_init[id] = true;

    struct ggml_tensor * dst = node_copies[id];
    if (dst->view_src != NULL) {
        graph_copy_init_tensor(hash_set, node_copies, node_init, src->view_src);
        enum ggml_status status = ggml_backend_view_init(dst);
        GGML_ASSERT(status == GGML_STATUS_SUCCESS);
    }
    else {
        ggml_backend_tensor_copy(src, dst);
    }

    // init src
    for (int i = 0; i < GGML_MAX_SRC; i++) {
        struct ggml_tensor * s = src->src[i];
        if (s == NULL) {
            continue;
        }
        graph_copy_init_tensor(hash_set, node_copies, node_init, s);
    }
}

struct ggml_backend_graph_copy ggml_backend_graph_copy(ggml_backend_t backend, struct ggml_cgraph * graph) {
    GGML_ASSERT(graph);
    struct ggml_hash_set hash_set = ggml_hash_set_new(graph->visited_hash_set.size);
    struct ggml_tensor ** node_copies = (ggml_tensor **) calloc(hash_set.size, sizeof(node_copies[0])); // NOLINT
    bool * node_init = (bool *) calloc(hash_set.size, sizeof(node_init[0]));

    struct ggml_init_params params = {
        /* .mem_size   = */ ggml_tensor_overhead()*hash_set.size + ggml_graph_overhead_custom(graph->size, false),
        /* .mem_buffer = */ NULL,
        /* .no_alloc   = */ true
    };

    struct ggml_context * ctx_allocated = ggml_init(params);
    struct ggml_context * ctx_unallocated = ggml_init(params);

    if (ctx_allocated == NULL || ctx_unallocated == NULL) {
        GGML_LOG_ERROR("%s: failed to allocate context for graph copy\n", __func__);
        ggml_hash_set_free(&hash_set);
        free(node_copies);
        free(node_init);
        ggml_free(ctx_allocated);
        ggml_free(ctx_unallocated);
        return {
            /* .buffer           = */ NULL,
            /* .ctx_allocated    = */ NULL,
            /* .ctx_unallocated  = */ NULL,
            /* .graph            = */ NULL,
        };
    }

    // dup nodes
    for (int i = 0; i < graph->n_nodes; i++) {
        struct ggml_tensor * node = graph->nodes[i];
        graph_copy_dup_tensor(hash_set, node_copies, ctx_allocated, ctx_unallocated, node);
    }

    // allocate nodes
    ggml_backend_buffer_t buffer = ggml_backend_alloc_ctx_tensors(ctx_allocated, backend);
    if (buffer == NULL) {
        GGML_LOG_ERROR("%s: failed to allocate buffer for graph copy\n", __func__);
        ggml_hash_set_free(&hash_set);
        free(node_copies);
        free(node_init);
        ggml_free(ctx_allocated);
        ggml_free(ctx_unallocated);
        return {
            /* .buffer           = */ NULL,
            /* .ctx_allocated    = */ NULL,
            /* .ctx_unallocated  = */ NULL,
            /* .graph            = */ NULL,
        };
    }

    //printf("copy buffer size: %zu MB\n", ggml_backend_buffer_get_size(buffer) / 1024 / 1024);

    // copy data and init views
    for (int i = 0; i < graph->n_nodes; i++) {
        struct ggml_tensor * node = graph->nodes[i];
        graph_copy_init_tensor(&hash_set, node_copies, node_init, node);
    }

    // build graph copy
    struct ggml_cgraph * graph_copy = ggml_new_graph_custom(ctx_allocated, graph->size, false);
    for (int i = 0; i < graph->n_nodes; i++) {
        struct ggml_tensor * node = graph->nodes[i];
        struct ggml_tensor * node_copy = node_copies[ggml_hash_find(&hash_set, node)];
        graph_copy->nodes[i] = node_copy;
    }
    graph_copy->n_nodes = graph->n_nodes;

    ggml_hash_set_free(&hash_set);
    free(node_copies);
    free(node_init);

    return {
        /* .buffer           = */ buffer,
        /* .ctx_allocated    = */ ctx_allocated,
        /* .ctx_unallocated  = */ ctx_unallocated,
        /* .graph            = */ graph_copy,
    };
}

void ggml_backend_graph_copy_free(struct ggml_backend_graph_copy copy) {
    ggml_backend_buffer_free(copy.buffer);
    ggml_free(copy.ctx_allocated);
    ggml_free(copy.ctx_unallocated);
}

bool ggml_backend_compare_graph_backend(ggml_backend_t backend1, ggml_backend_t backend2, struct ggml_cgraph * graph, ggml_backend_eval_callback callback, void * user_data, struct ggml_tensor const * const * test_nodes, size_t num_test_nodes) {
    struct ggml_backend_graph_copy copy = ggml_backend_graph_copy(backend2, graph);
    if (copy.buffer == NULL) {
        return false;
    }

    struct ggml_cgraph * g1 = graph;
    struct ggml_cgraph * g2 = copy.graph;

    assert(g1->n_nodes == g2->n_nodes);

    if (num_test_nodes != 0) {
        GGML_ASSERT(test_nodes);
        // Compute the whole graph and only test the output for specific tensors
        ggml_backend_graph_compute(backend1, g1);
        ggml_backend_graph_compute(backend2, g2);

        bool verified = false;
        for (int i = 0; i < g1->n_nodes; i++) {
            for (size_t j = 0; j < num_test_nodes; ++j) {
                if (g1->nodes[i] == test_nodes[j]) {
                    callback(i, g1->nodes[i], g2->nodes[i], user_data);
                    verified = true;
                }
            }
        }
        GGML_ASSERT(verified);
    } else {
        for (int i = 0; i < g1->n_nodes; i++) {
            struct ggml_tensor * t1 = g1->nodes[i];
            struct ggml_tensor * t2 = g2->nodes[i];

            assert(t1->op == t2->op && ggml_are_same_layout(t1, t2));

            struct ggml_cgraph g1v = ggml_graph_view(g1, i, i + 1);
            struct ggml_cgraph g2v = ggml_graph_view(g2, i, i + 1);

            ggml_backend_graph_compute(backend1, &g1v);
            ggml_backend_graph_compute(backend2, &g2v);

            if (ggml_is_view_op(t1->op)) {
                continue;
            }

            // compare results, calculate rms etc
            if (!callback(i, t1, t2, user_data)) {
                break;
            }
        }
    }
    ggml_backend_graph_copy_free(copy);

    return true;
}

// CPU backend - buffer

static void * ggml_backend_cpu_buffer_get_base(ggml_backend_buffer_t buffer) {
    GGML_ASSERT(buffer);
    uintptr_t data = (uintptr_t)buffer->context;

    // align the buffer
    if (data % TENSOR_ALIGNMENT != 0) {
        data = GGML_PAD(data, TENSOR_ALIGNMENT);
    }

    return (void *)data;
}

static void ggml_backend_cpu_buffer_free_buffer(ggml_backend_buffer_t buffer) {
    GGML_ASSERT(buffer);
    ggml_aligned_free(buffer->context, buffer->size);
}

static void ggml_backend_cpu_buffer_memset_tensor(ggml_backend_buffer_t buffer, struct ggml_tensor * tensor, uint8_t value, size_t offset, size_t size) {
    GGML_ASSERT(tensor);
    memset((char *)tensor->data + offset, value, size);

    GGML_UNUSED(buffer);
}

static void ggml_backend_cpu_buffer_set_tensor(ggml_backend_buffer_t buffer, struct ggml_tensor * tensor, const void * data, size_t offset, size_t size) {
    GGML_ASSERT(tensor);
    memcpy((char *)tensor->data + offset, data, size);

    GGML_UNUSED(buffer);
}

static void ggml_backend_cpu_buffer_get_tensor(ggml_backend_buffer_t buffer, const struct ggml_tensor * tensor, void * data, size_t offset, size_t size) {
    GGML_ASSERT(tensor);
    memcpy(data, (const char *)tensor->data + offset, size);

    GGML_UNUSED(buffer);
}

static bool ggml_backend_cpu_buffer_cpy_tensor(ggml_backend_buffer_t buffer, const struct ggml_tensor * src, struct ggml_tensor * dst) {
    GGML_ASSERT(src);
    if (ggml_backend_buffer_is_host(src->buffer)) {
        memcpy(dst->data, src->data, ggml_nbytes(src));
        return true;
    }
    return false;

    GGML_UNUSED(buffer);
}

static void ggml_backend_cpu_buffer_clear(ggml_backend_buffer_t buffer, uint8_t value) {
    GGML_ASSERT(buffer);
    memset(buffer->context, value, buffer->size);
}

static const struct ggml_backend_buffer_i ggml_backend_cpu_buffer_i = {
    /* .free_buffer     = */ ggml_backend_cpu_buffer_free_buffer,
    /* .get_base        = */ ggml_backend_cpu_buffer_get_base,
    /* .init_tensor     = */ NULL, // no initialization required
    /* .memset_tensor   = */ ggml_backend_cpu_buffer_memset_tensor,
    /* .set_tensor      = */ ggml_backend_cpu_buffer_set_tensor,
    /* .get_tensor      = */ ggml_backend_cpu_buffer_get_tensor,
    /* .set_tensor_2d   = */ NULL,
    /* .get_tensor_2d   = */ NULL,
    /* .cpy_tensor      = */ ggml_backend_cpu_buffer_cpy_tensor,
    /* .clear           = */ ggml_backend_cpu_buffer_clear,
    /* .reset           = */ NULL,
};

static const struct ggml_backend_buffer_i ggml_backend_cpu_buffer_from_ptr_i = {
    /* .free_buffer     = */ NULL, // ptr is not owned by the buffer, so it does not need to be freed
    /* .get_base        = */ ggml_backend_cpu_buffer_get_base,
    /* .init_tensor     = */ NULL, // no initialization required
    /* .memset_tensor   = */ ggml_backend_cpu_buffer_memset_tensor,
    /* .set_tensor      = */ ggml_backend_cpu_buffer_set_tensor,
    /* .get_tensor      = */ ggml_backend_cpu_buffer_get_tensor,
    /* .set_tensor_2d   = */ NULL,
    /* .get_tensor_2d   = */ NULL,
    /* .cpy_tensor      = */ ggml_backend_cpu_buffer_cpy_tensor,
    /* .clear           = */ ggml_backend_cpu_buffer_clear,
    /* .reset           = */ NULL,
};

// CPU backend buffer type

// this buffer type is defined here to make it available to all backends

static const char * ggml_backend_cpu_buffer_type_get_name(ggml_backend_buffer_type_t buft) {
    return "CPU";

    GGML_UNUSED(buft);
}

static ggml_backend_buffer_t ggml_backend_cpu_buffer_type_alloc_buffer(ggml_backend_buffer_type_t buft, size_t size) {
    void * data = ggml_aligned_malloc(size);

    if (data == NULL) {
        GGML_LOG_ERROR("%s: failed to allocate buffer of size %zu\n", __func__, size);
        return NULL;
    }

    return ggml_backend_buffer_init(buft, ggml_backend_cpu_buffer_i, data, size);
}

static size_t ggml_backend_cpu_buffer_type_get_alignment(ggml_backend_buffer_type_t buft) {
    return TENSOR_ALIGNMENT;

    GGML_UNUSED(buft);
}

static bool ggml_backend_cpu_buffer_type_is_host(ggml_backend_buffer_type_t buft) {
    return true;

    GGML_UNUSED(buft);
}

ggml_backend_buffer_type_t ggml_backend_cpu_buffer_type(void) {
    static struct ggml_backend_buffer_type ggml_backend_cpu_buffer_type = {
        /* .iface   = */ {
            /* .get_name         = */ ggml_backend_cpu_buffer_type_get_name,
            /* .alloc_buffer     = */ ggml_backend_cpu_buffer_type_alloc_buffer,
            /* .get_alignment    = */ ggml_backend_cpu_buffer_type_get_alignment,
            /* .get_max_size     = */ NULL, // defaults to SIZE_MAX
            /* .get_alloc_size   = */ NULL, // defaults to ggml_nbytes
            /* .is_host          = */ ggml_backend_cpu_buffer_type_is_host,
        },
        /* .device  = */ NULL, // FIXME ggml_backend_reg_dev_get(ggml_backend_cpu_reg(), 0),
        /* .context = */ NULL,
    };

    return &ggml_backend_cpu_buffer_type;
}

static const char * ggml_backend_cpu_buffer_from_ptr_type_get_name(ggml_backend_buffer_type_t buft) {
    return "CPU_Mapped";

    GGML_UNUSED(buft);
}

static ggml_backend_buffer_type_t ggml_backend_cpu_buffer_from_ptr_type(void) {
    static struct ggml_backend_buffer_type ggml_backend_cpu_buffer_type = {
        /* .iface   = */ {
            /* .get_name         = */ ggml_backend_cpu_buffer_from_ptr_type_get_name,
            /* .alloc_buffer     = */ ggml_backend_cpu_buffer_type_alloc_buffer,
            /* .get_alignment    = */ ggml_backend_cpu_buffer_type_get_alignment,
            /* .get_max_size     = */ NULL, // defaults to SIZE_MAX
            /* .get_alloc_size   = */ NULL, // defaults to ggml_nbytes
            /* .is_host          = */ ggml_backend_cpu_buffer_type_is_host,
        },
        /* .device  = */ NULL, // FIXME ggml_backend_reg_dev_get(ggml_backend_cpu_reg(), 0),
        /* .context = */ NULL,
    };

    return &ggml_backend_cpu_buffer_type;
}

ggml_backend_buffer_t ggml_backend_cpu_buffer_from_ptr(void * ptr, size_t size) {
    GGML_ASSERT((uintptr_t)ptr % TENSOR_ALIGNMENT == 0 && "buffer pointer must be aligned");
    return ggml_backend_buffer_init(ggml_backend_cpu_buffer_from_ptr_type(), ggml_backend_cpu_buffer_from_ptr_i, ptr, size);
}
