#include "AnemoneRuntime/System/Process.hxx"
#include "AnemoneRuntime/Platform/Unix/UnixInterop.hxx"
#include "AnemoneRuntime/Diagnostics/Assert.hxx"

#include <array>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <wordexp.h>
#include <spawn.h>
#include <link.h>

namespace Anemone
{
    std::expected<int32_t, ErrorCode> Process::Wait()
    {
        AE_ASSERT(this->_handle);

#if !ANEMONE_PLATFORM_LINUX
        AE_TRACE(Critical, "Process::Wait not implemented");
        return std::unexpected(ErrorCode::NotImplemented);
#else

        Internal::PlatformProcess const handle = std::exchange(this->_handle, {});

        if (handle)
        {
            int status;
            int rc = Interop::posix_WaitForProcess(handle.Get(), status, 0);

            if (rc != handle.Get())
            {
                return std::unexpected(ErrorCode::InvalidOperation);
            }

            if (WIFEXITED(status))
            {
                return WEXITSTATUS(status);
            }

            return WTERMSIG(status) + 256;
        }

        return std::unexpected(ErrorCode::InvalidHandle);

#endif
    }

    std::expected<int32_t, ErrorCode> Process::TryWait()
    {
        AE_ASSERT(this->_handle);

#if !ANEMONE_PLATFORM_LINUX
        AE_TRACE(Critical, "Process::TryWait not implemented");
        return std::unexpected(ErrorCode::NotImplemented);
#else

        if (this->_handle)
        {
            int status;
            int rc = Interop::posix_WaitForProcess(this->_handle.Get(), status, WNOHANG);

            if (rc == 0)
            {
                // Child process still running.
                return std::unexpected(ErrorCode::OperationInProgress);
            }

            if (rc != this->_handle.Get())
            {
                this->_handle = {};

                // Failed to wait for child process.
                return std::unexpected(ErrorCode::InvalidOperation);
            }

            // Clear handle.
            this->_handle = {};

            // Child process has exited.
            if (WIFEXITED(status))
            {
                return WEXITSTATUS(status);
            }

            return WTERMSIG(status) + 256;
        }

        return std::unexpected(ErrorCode::InvalidHandle);
#endif
    }

    std::expected<void, ErrorCode> Process::Terminate()
    {
        AE_ASSERT(this->_handle);

#if !ANEMONE_PLATFORM_LINUX
        AE_TRACE(Critical, "Process::Terminate not implemented");
        return std::unexpected(ErrorCode::NotImplemented);
#else

        Internal::PlatformProcess const handle = std::exchange(this->_handle, {});

        if (handle)
        {
            if (Interop::posix_TerminateProcess(handle.Get()))
            {
                return {};
            }

            return std::unexpected(ErrorCode::InvalidOperation);
        }

        return std::unexpected(ErrorCode::InvalidHandle);
#endif
    }

    auto Process::Start(
        std::string_view path,
        std::optional<std::string_view> const& params,
        std::optional<std::string_view> const& workingDirectory)
        -> std::expected<Process, ErrorCode>
    {
        return Process::Start(
            path,
            params,
            workingDirectory,
            nullptr,
            nullptr,
            nullptr);
    }

    auto Process::Start(
        std::string_view path,
        std::optional<std::string_view> const& params,
        std::optional<std::string_view> const& workingDirectory,
        FileHandle* input,
        FileHandle* output,
        FileHandle* error)
        -> std::expected<Process, ErrorCode>
    {
#if !ANEMONE_PLATFORM_LINUX
        (void)path;
        (void)params;
        (void)workingDirectory;
        (void)input;
        (void)output;
        (void)error;
        return {};
        return std::unexpected(ErrorCode::NotImplemented);
#else

        bool const redirectInput = input != nullptr;
        bool const redirectOutput = output != nullptr;
        bool const redirectError = error != nullptr;

        std::string fullPath{path};

        std::string fullParams = fmt::format("\"{}\"", fullPath);

        if (params)
        {
            fullParams += " ";
            fullParams += *params;
        }

        wordexp_t wordexpStatus{};

        int exp_result = wordexp(fullParams.c_str(), &wordexpStatus, 0);

        if (exp_result != 0)
        {
            return std::unexpected(ErrorCode::InvalidArgument);
        }

        pid_t process_id{};

        posix_spawnattr_t spawn_attributes{};
        posix_spawnattr_init(&spawn_attributes);

        short spawn_flags{};

        sigset_t empty_sigset{};
        sigemptyset(&empty_sigset);
        posix_spawnattr_setsigmask(&spawn_attributes, &empty_sigset);

        spawn_flags |= POSIX_SPAWN_SETSIGMASK;

        sigset_t default_sigset{};
        sigemptyset(&default_sigset);

        for (int i = SIGRTMIN; i < SIGRTMAX; ++i)
        {
            sigaddset(&default_sigset, i);
        }

        posix_spawnattr_setsigdefault(&spawn_attributes, &default_sigset);

        spawn_flags |= POSIX_SPAWN_SETSIGDEF;

        // TODO: Determine if we can do this in normal FileHandle::CreatePipe() function.
        Interop::UnixSafeFdHandle fhWriteInput{};
        Interop::UnixSafeFdHandle fhReadInput{};
        Interop::UnixSafeFdHandle fhWriteOutput{};
        Interop::UnixSafeFdHandle fhReadOutput{};
        Interop::UnixSafeFdHandle fhWriteError{};
        Interop::UnixSafeFdHandle fhReadError{};

        posix_spawn_file_actions_t files{};
        posix_spawn_file_actions_init(&files);

        if (workingDirectory)
        {
            posix_spawn_file_actions_addchdir_np(&files, std::string{*workingDirectory}.c_str());
        }

        if (redirectInput or redirectOutput or redirectError)
        {
            if (redirectInput)
            {
                int fd[2];

                if (pipe2(fd, O_CLOEXEC | O_NONBLOCK) != 0)
                {
                    return std::unexpected(ErrorCode::InvalidOperation);
                }

                // Create file handle for parent process.
                fhWriteInput = Interop::UnixSafeFdHandle{fd[1]};
                fhReadInput = Interop::UnixSafeFdHandle{fd[0]};

                // Prepare file actions for child process.
                posix_spawn_file_actions_adddup2(&files, fd[0], STDIN_FILENO);
            }

            if (redirectOutput)
            {
                int fd[2];

                if (pipe2(fd, O_CLOEXEC | O_NONBLOCK) != 0)
                {
                    return std::unexpected(ErrorCode::InvalidOperation);
                }

                // Create file handle for parent process.
                fhWriteOutput = Interop::UnixSafeFdHandle{fd[1]};
                fhReadOutput = Interop::UnixSafeFdHandle{fd[0]};

                // Prepare file actions for child process.
                posix_spawn_file_actions_adddup2(&files, fd[1], STDOUT_FILENO);
            }

            if (redirectError)
            {
                int fd[2];

                if (pipe2(fd, O_CLOEXEC | O_NONBLOCK) != 0)
                {
                    return std::unexpected(ErrorCode::InvalidOperation);
                }

                // Create file handle for parent process.
                fhWriteError = Interop::UnixSafeFdHandle{fd[1]};
                fhReadError = Interop::UnixSafeFdHandle{fd[0]};

                // Prepare file actions for child process.
                posix_spawn_file_actions_adddup2(&files, fd[1], STDERR_FILENO);
            }
        }

        int spawn_result = posix_spawn(
            &process_id,
            fullPath.c_str(),
            &files,
            &spawn_attributes,
            wordexpStatus.we_wordv,
            environ);

        posix_spawn_file_actions_destroy(&files);

        wordfree(&wordexpStatus);

        if (spawn_result == 0)
        {
            if (input != nullptr)
            {
                *input = FileHandle{std::move(fhWriteInput)};
            }

            if (output != nullptr)
            {
                *output = FileHandle{std::move(fhReadOutput)};
            }

            if (error != nullptr)
            {
                *error = FileHandle{std::move(fhReadError)};
            }

            return Process{Internal::PlatformProcess{process_id}};
        }
        else
        {
            return std::unexpected(ErrorCode::InvalidOperation);
        }
#endif
    }

    auto Process::Execute(
        std::string_view path,
        std::optional<std::string_view> const& params,
        std::optional<std::string_view> const& workingDirectory)
        -> std::expected<int32_t, ErrorCode>
    {
        return Process::Start(path, params, workingDirectory, nullptr, nullptr, nullptr)
            .and_then([](Process process)
        {
            return process.Wait();
        });
    }

    auto Process::Execute(
        std::string_view path,
        std::optional<std::string_view> const& params,
        std::optional<std::string_view> const& workingDirectory,
        FunctionRef<void(std::string_view)> output,
        FunctionRef<void(std::string_view)> error)
        -> std::expected<int32_t, ErrorCode>
    {
        FileHandle pipeOutput{};
        FileHandle pipeError{};

        if (auto process = Process::Start(path, params, workingDirectory, nullptr, &pipeOutput, &pipeError))
        {
            std::array<char, 512> buffer;
            std::span view = std::as_writable_bytes(std::span{buffer});

            while (true)
            {
                auto rc = process->TryWait();

                while (auto processed = pipeOutput.Read(view))
                {
                    if (*processed == 0)
                    {
                        break;
                    }

                    output(std::string_view{buffer.data(), *processed});
                }

                while (auto processed = pipeError.Read(view))
                {
                    if (*processed == 0)
                    {
                        break;
                    }

                    error(std::string_view{buffer.data(), *processed});
                }

                if (rc)
                {
                    return *rc;
                }

                if (rc.error() != ErrorCode::OperationInProgress)
                {
                    return std::unexpected(rc.error());
                }
            }
        }
        else
        {
            return std::unexpected(process.error());
        }
    }
}
