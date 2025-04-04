#include "transfer_buffer.h"

#include "gpu/buffer.h"
#include "gpu/device.h"
#include "gpu/copy_pass.h"

#include <fmt/format.h>
#include <SDL3/SDL_gpu.h>

#include <stdexcept>

namespace em::Gpu
{
    TransferBuffer::TransferBuffer(Device &device, const Params &params)
        : TransferBuffer() // Ensure cleanup on throw.
    {
        state.device = device.Handle(); // Assign this first in case we need to throw below, the destructor needs this value.
        state.size = params.size;

        SDL_GPUTransferBufferCreateInfo sdl_params{
            .usage = params.usage == Usage::download ? SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD : SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = params.size,
            .props = 0, // We don't expose this for now.
        };

        state.buffer = SDL_CreateGPUTransferBuffer(device.Handle(), &sdl_params);
        if (!state.buffer)
            throw std::runtime_error(fmt::format("Unable to create GPU transfer buffer: {}", SDL_GetError()));
    }

    TransferBuffer::TransferBuffer(TransferBuffer &&other) noexcept
        : state(std::move(other.state))
    {
        other.state = {};
    }

    TransferBuffer &TransferBuffer::operator=(TransferBuffer other) noexcept
    {
        std::swap(state, other.state);
        return *this;
    }

    TransferBuffer::~TransferBuffer()
    {
        if (state.buffer)
            SDL_ReleaseGPUTransferBuffer(state.device, state.buffer);
    }

    TransferBuffer::Mapping::Mapping(Mapping &&other) noexcept
        : state(std::move(other.state))
    {
        other.state = {};
    }

    TransferBuffer::Mapping &TransferBuffer::Mapping::operator=(Mapping other) noexcept
    {
        std::swap(state, other.state);
        return *this;
    }

    TransferBuffer::Mapping::~Mapping()
    {
        if (state.buffer)
            SDL_UnmapGPUTransferBuffer(state.device, state.buffer);
    }

    // Maps the buffer into memory temporarily.
    // It gets unmapped when the returned object dies.
    TransferBuffer::Mapping TransferBuffer::Map()
    {
        Mapping ret;
        ret.state.device = state.device;
        ret.state.buffer = state.buffer;

        // Here we always cycle. Not sure why we wouldn't want to.
        void *address = SDL_MapGPUTransferBuffer(state.device, state.buffer, /*cycle:*/true);
        if (!address)
            throw std::runtime_error(fmt::format("Failed to map a GPU transfer buffer: {}", SDL_GetError()));

        ret.state.mapped_region = {reinterpret_cast<unsigned char *>(address), state.size};
        return ret;
    }

    void TransferBuffer::UploadToBuffer(CopyPass &pass, std::uint32_t self_offset, Buffer &target, std::uint32_t target_offset, std::uint32_t size)
    {
        SDL_GPUTransferBufferLocation self_loc{
            .transfer_buffer = state.buffer,
            .offset = self_offset,
        };
        SDL_GPUBufferRegion target_loc{
            .buffer = target.Handle(),
            .offset = target_offset,
            .size = size,
        };

        // Here we always cycle. Not sure why we wouldn't want to.
        // This function can't fail.
        SDL_UploadToGPUBuffer(pass.Handle(), &self_loc, &target_loc, /*cycle=*/true);
    }

    void TransferBuffer::DownloadFromBuffer(CopyPass &pass, std::uint32_t self_offset, Buffer &target, std::uint32_t target_offset, std::uint32_t size)
    {
        SDL_GPUTransferBufferLocation self_loc{
            .transfer_buffer = state.buffer,
            .offset = self_offset,
        };
        SDL_GPUBufferRegion target_loc{
            .buffer = target.Handle(),
            .offset = target_offset,
            .size = size,
        };

        // This function can't fail.
        SDL_DownloadFromGPUBuffer(pass.Handle(), &target_loc, &self_loc);
    }
}
