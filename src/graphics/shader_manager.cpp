#include "shader_manager.h"

#include "command_line/parser.h"
#include "strings/char_types.h"
#include "utils/hash_func.h"
#include "utils/process_queue.h"
#include "utils/terminal.h"

#include <fmt/format.h>
#include <stdexcept>

namespace em::Graphics
{
    void BasicShaderManager::AddShader(Shader &new_shader)
    {
        if (finalized)
            throw std::logic_error("Adding a shader to `ShaderManager` after it already has been finalized.");

        auto [iter, is_new] = shaders.insert(&new_shader);

        // Error on duplicate shader name, but only if the address is different.
        if (!is_new)
        {
            if (*iter != &new_shader)
                throw std::logic_error(fmt::format("Duplicate shader name in `ShaderManager`: `{}`.", new_shader.name));
            return;
        }
    }

    ShaderManager::ShaderManager(Gpu::Device &device)
        : device(&device)
    {}

    ShaderManager::~ShaderManager()
    {
        // Force-destroy shaders.
        // This is needed when they are in static variables, to avoid the static deinit order fiasco, when they are destroyed after the Window and GPU get destroyed.

        for (Shader *shader : shaders)
            shader->shader = {};
    }

    void ShaderManager::Finalize()
    {
        if (finalized)
            throw std::logic_error("Finalizing `ShaderManager` the second time.");

        if (!device)
            throw std::logic_error("Attempt to finalize a null `ShaderManager`.");

        finalized = true;

        struct CompiledShader
        {
            Shader *shader = nullptr;
            std::string filename;
        };

        std::vector<CompiledShader> compiled_shaders;
        std::vector<ProcessQueue::Task> compilation_tasks;

        auto FinalizeShader = [&](Shader &shader, const_byte_view binary)
        {
            try
            {
                shader.shader = Gpu::Shader(*device, shader.name, shader.stage, binary);
            }
            catch (...)
            {
                std::throw_with_nested(std::runtime_error(fmt::format("While loading shader `{}`:", shader.name)));
            }
        };

        phmap::flat_hash_set<std::string> shader_filenames;

        for (Shader *shader : shaders)
        {
            std::uint32_t hash = Hash32(shader->source);

            std::string_view stage_name;
            switch (shader->stage)
            {
              case Gpu::Shader::Stage::vertex:
                stage_name = "vert";
                break;
              case Gpu::Shader::Stage::fragment:
                stage_name = "frag";
                break;
              case Gpu::Shader::Stage::compute:
                stage_name = "comp";
                break;
            }

            // Tweak the shader name a little, to make it look better as a file name.
            std::string fixed_name;
            fixed_name.reserve(shader->name.size());
            for (char ch : shader->name)
            {
                if (Strings::IsIdentifierCharStrict(ch))
                {
                    fixed_name += ch;
                    continue;
                }

                if (!fixed_name.empty() && !fixed_name.ends_with('_'))
                    fixed_name += '_';
            }
            while (fixed_name.ends_with('_'))
                fixed_name.pop_back();

            std::string filename = fmt::format("{}/{}-{:08x}.{}.spv", dir, fixed_name, hash, stage_name);
            shader_filenames.insert(filename);

            bool file_was_loaded = true;
            // If `compile_when_finalized == false`, this just throws.
            Filesystem::LoadedFile file(filename, compile_when_finalized ? &file_was_loaded : nullptr);

            if (file_was_loaded)
            {
                // Load the shader into SDL.
                FinalizeShader(*shader, file);
            }
            else
            {
                // Queue the shader for compilation.
                // This is reachable only after `CompileWhenFinalized()`.

                compiled_shaders.push_back({.shader = shader, .filename = filename});

                std::vector<std::string> command = {"glslc", fmt::format("-fshader-stage={}", stage_name), "-", fmt::format("-o{}", filename)};
                command.append_range(glslc_flags);

                compilation_tasks.push_back({
                    .name = shader->name,
                    .command = std::move(command),
                    .input = shader->source,
                });
            }
        }

        // Compile the missing shaders.
        if (compile_when_finalized)
        {
            // Actually compile the shaders.
            if (!compilation_tasks.empty())
            {
                // Print a simple explanation.
                Terminal::DefaultToConsole(stderr);
                fmt::print(stderr, "### Compiling shaders ###\n");

                // Create the output directory.
                Filesystem::CreateDirectories(dir);

                ProcessQueue queue(std::move(compilation_tasks));
                auto status = queue.WaitUntilFinished();
                if (status.num_failed > 0)
                    throw std::runtime_error("Some shaders failed to compile!");

                // Load the shaders.
                // `LoadedFile(...)` and `FinalizeShader(...)` can throw, we don't mind that.
                for (const auto &elem : compiled_shaders)
                    FinalizeShader(*elem.shader, Filesystem::LoadedFile(elem.filename));
            }

            { // Delete the unwanted files.
                // Just in case, build a vector first and only then delete.
                std::vector<std::string> files_to_delete;
                Filesystem::VisitDirectory(dir, [&](zstring_view name)
                {
                    std::string path = fmt::format("{}/{}", dir, name);

                    if (
                        name.ends_with(".spv") &&
                        !shader_filenames.contains(path) &&
                        Filesystem::GetFileInfo(path)->kind == Filesystem::FileKind::file
                    )
                    {
                        files_to_delete.push_back(std::string(path));
                    }
                    return false;
                });

                if (!files_to_delete.empty())
                {
                    std::fputs("### Deleting stale shaders ###\n", stderr);
                    Terminal::DefaultToConsole(stderr);
                    for (const auto &file_to_delete : files_to_delete)
                    {
                        fmt::print(stderr, "[Deleting] {}\n", file_to_delete);
                        Filesystem::DeleteOne(file_to_delete);
                    }
                }
            }
        }
    }

    void ShaderManager::ProvidedCommandLineFlags(CommandLine::Parser &parser)
    {
        if (finalized)
            throw std::logic_error("Can't call `ProvidedCommandLineFlags()` when `ShaderManager` is already finalized.");

        parser.AddFlag<std::string>(
            "-S,--compile-shaders",
            {},
            "dir",
            "Load shader binaries from `dir` instead of their normal location. Compile any missing binaries and remove unneeded ones.",
            [this](std::string new_dir)
            {
                dir = std::move(new_dir);
                compile_when_finalized = true;
            },
            [this]
            {
                Finalize();
            }
        );
    }
}
