#pragma once

#include <cstdint>
#include <span>

typedef struct SDL_GPUDevice SDL_GPUDevice;
typedef struct SDL_GPUTransferBuffer SDL_GPUTransferBuffer;

namespace em::Gpu
{
    class Buffer;
    class CopyPass;
    class Device;
    class Texture;

    // A texture.
    class TransferBuffer
    {
        struct State
        {
            // At least for `SDL_ReleaseGPUBuffer()`.
            // This isn't a `Device *` because that can be moved around and the SDL handle can't.
            SDL_GPUDevice *device = nullptr;

            SDL_GPUTransferBuffer *buffer = nullptr;
            std::uint32_t size = 0;
        };
        State state;

      public:
        constexpr TransferBuffer() {}

        enum class Usage
        {
            upload,
            download,
        };

        struct Params
        {
            Usage usage = Usage::upload;

            std::uint32_t size = 0;
        };

        TransferBuffer(Device &device, const Params &params);

        TransferBuffer(TransferBuffer &&other) noexcept;
        TransferBuffer &operator=(TransferBuffer other) noexcept;
        ~TransferBuffer();

        [[nodiscard]] explicit operator bool() const {return bool(state.buffer);}
        [[nodiscard]] SDL_GPUTransferBuffer *Handle() {return state.buffer;}
        [[nodiscard]] std::uint32_t Size() const {return state.size;}


        class Mapping
        {
            friend TransferBuffer;

            struct State
            {
                SDL_GPUDevice *device = nullptr;
                SDL_GPUTransferBuffer *buffer = nullptr;
                std::span<unsigned char> mapped_region;
            };
            State state;

          public:
            constexpr Mapping() {}

            // The non-null construction is done by calling `TransferBuffer::Map()`, see below.

            Mapping(Mapping &&other) noexcept;
            Mapping &operator=(Mapping other) noexcept;
            ~Mapping();

            // Returns the mapped region.
            // This is const, because this class is modelled like a pointer.
            [[nodiscard]] std::span<unsigned char> Span() const {return state.mapped_region;}
        };

        // Maps the buffer into memory temporarily. Throws on failure.
        // It gets unmapped when the returned object dies.
        // Call `.Span()` on the result to get the mapped pointer.
        [[nodiscard]] Mapping Map();

        // Upload to a buffer. You can assume the upload finishes immediately.
        void UploadToBuffer(CopyPass &pass, Buffer &target) {UploadToBuffer(pass, 0, target, 0, Size());}
        void UploadToBuffer(CopyPass &pass, std::uint32_t self_offset, Buffer &target, std::uint32_t target_offset, std::uint32_t size);

        // Download from a buffer. You MUST WAIT for the command buffer fence to indicate that it's done before reading this.
        void DownloadFromBuffer(CopyPass &pass, Buffer &target) {DownloadFromBuffer(pass, 0, target, 0, Size());}
        void DownloadFromBuffer(CopyPass &pass, std::uint32_t self_offset, Buffer &target, std::uint32_t target_offset, std::uint32_t size);
    };
}
