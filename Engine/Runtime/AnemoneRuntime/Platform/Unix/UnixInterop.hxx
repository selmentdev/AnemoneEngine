#pragma once
#include "AnemoneRuntime/Platform/Base/BaseInterop.hxx"
#include "AnemoneRuntime/Diagnostics/Assert.hxx"
#include "AnemoneRuntime/Duration.hxx"
#include "AnemoneRuntime/DateTime.hxx"
#include "AnemoneRuntime/Platform/Unix/UnixHeaders.hxx"

#include <atomic>

namespace Anemone::Interop
{
    inline constexpr int64_t posix_DateAdjustOffset = 62135596800;
    inline constexpr int64_t posix_NanosecondsInSecond = 1'000'000'000;

    constexpr Duration posix_into_Duration(struct timeval const& tv)
    {
        return Duration{
            .Seconds = tv.tv_sec,
            .Nanoseconds = tv.tv_usec * 1'000,
        };
    }

    constexpr Duration posix_into_Duration(struct timespec const& ts)
    {
        return Duration{
            .Seconds = ts.tv_sec,
            .Nanoseconds = ts.tv_nsec,
        };
    }

    constexpr DateTime posix_into_DateTime(struct timespec const& ts, int64_t bias)
    {
        return DateTime{
            .Inner = {
                .Seconds = ts.tv_sec - bias + posix_DateAdjustOffset,
                .Nanoseconds = ts.tv_nsec,
            },
        };
    }

    constexpr DateTime posix_into_DateTime(struct timespec const& ts)
    {
        return DateTime{
            .Inner = {
                .Seconds = ts.tv_sec + posix_DateAdjustOffset,
                .Nanoseconds = ts.tv_nsec,
            },
        };
    }

    constexpr void posix_Normalize(timespec& self)
    {
        self.tv_sec += (self.tv_nsec / posix_NanosecondsInSecond);

        if ((self.tv_nsec %= posix_NanosecondsInSecond) < 0)
        {
            self.tv_nsec += posix_NanosecondsInSecond;
            --self.tv_sec;
        }
    }

    constexpr void posix_AddDuration(timespec& self, Duration const& value)
    {
        self.tv_sec += value.Seconds;
        self.tv_nsec += value.Nanoseconds;
        posix_Normalize(self);
    }

    constexpr void posix_SubtractDuration(timespec& self, Duration const& value)
    {
        self.tv_sec -= value.Seconds;
        self.tv_nsec -= value.Nanoseconds;
        posix_Normalize(self);
    }

    constexpr void posix_SubtractTimespec(timespec& self, timespec const& value)
    {
        self.tv_sec -= value.tv_sec;
        self.tv_nsec -= value.tv_nsec;
        posix_Normalize(self);
    }

    constexpr timespec posix_TimespecDifference(timespec const& self, timespec const& other)
    {
        timespec result{
            .tv_sec = self.tv_sec - other.tv_sec,
            .tv_nsec = self.tv_nsec - other.tv_nsec,
        };
        posix_Normalize(result);
        return result;
    }

    [[nodiscard]] constexpr bool posix_IsNegative(timespec const& self)
    {
        if (self.tv_sec < 0)
        {
            return self.tv_nsec < 0;
        }

        return self.tv_sec < 0;
    }

    // self < other
    [[nodiscard]] constexpr bool posix_CompareLess(timespec const& self, timespec const& other)
    {
        if (self.tv_sec == other.tv_sec)
        {
            return self.tv_nsec < other.tv_nsec;
        }
        else
        {
            return self.tv_sec < other.tv_sec;
        }
    }

    // self == other
    [[nodiscard]] constexpr bool posix_CompareEqual(timespec const& self, timespec const& other)
    {
        return (self.tv_sec == other.tv_sec) and (self.tv_nsec == other.tv_nsec);
    }

    // self > other
    [[nodiscard]] constexpr bool posix_CompareGreater(timespec const& self, timespec const& other)
    {
        if (self.tv_sec == other.tv_sec)
        {
            return self.tv_nsec > other.tv_nsec;
        }
        else
        {
            return self.tv_sec > other.tv_sec;
        }
    }

    // self >= other
    [[nodiscard]] constexpr bool posix_CompareGreaterEqual(timespec const& self, timespec const& other)
    {
        return not posix_CompareLess(self, other);
    }

    // self <= other
    [[nodiscard]] constexpr bool posix_CompareLessEqual(timespec const& self, timespec const& other)
    {
        return not posix_CompareGreater(self, other);
    }

    inline void posix_ValidateTimeout(timespec& self, Duration const& value)
    {
        posix_AddDuration(self, value);
    }

    inline void posix_ValidateTimeout(timespec& self, int32_t milliseconds)
    {
        posix_AddDuration(self, Duration::FromMilliseconds(milliseconds));
    }

    constexpr timespec posix_FromDuration(Duration const& value)
    {
        return timespec{
            .tv_sec = value.Seconds,
            .tv_nsec = value.Nanoseconds,
        };
    }

    inline pid_t posix_GetThreadId()
    {
        return static_cast<pid_t>(syscall(SYS_gettid));
    }

    inline pid_t posix_GetProcessId()
    {
        return static_cast<pid_t>(syscall(SYS_getpid));
    }

    inline bool posix_IsProcessAlive(pid_t pid)
    {
        return (kill(pid, 0) == 0) or (errno == ESRCH);
    }

    inline bool posix_WaitForProcess(pid_t pid)
    {
        int status;
        return waitpid(pid, &status, 0) >= 0;
    }

    inline pid_t posix_WaitForProcess(pid_t pid, int& status, int flags)
    {
        pid_t rc;

        do
        {
            rc = waitpid(pid, &status, flags);
        } while ((rc < 0) and (errno == EINTR));

        return rc;
    }

    inline bool posix_TerminateProcess(pid_t pid)
    {
        bool result = true;
        if (kill(pid, SIGKILL))
        {
            result = false;
        }

        // Wait without WNOHANG to avoid zombie processes.
        int status;
        waitpid(pid, &status, 0);

        return result;
    }

    //
    // On Linux, read() (and similar system calls) will transfer at most 0x7ffff000 (2,147,479,552) bytes,
    // returning the number of bytes actually transferred.
    //

    inline constexpr size_t unix_MaxIoRequestLength = 0x7ffff000uz;

    [[nodiscard]] constexpr int unix_ValidateIoRequestLength(size_t value)
    {
        return static_cast<int>(std::min(value, unix_MaxIoRequestLength));
    }

    template <size_t StaticCapacityT>
    inline bool unix_LoadFile(string_buffer<char, StaticCapacityT>& buffer, const char* path)
    {
        bool result = false;

        int const fd = open(path, O_RDONLY);

        if (fd != -1)
        {
            struct stat fs;
            if (fstat(fd, &fs) == 0)
            {
                buffer.resize(fs.st_size + 1u);

                ssize_t const processed = read(fd, buffer.data(), buffer.size());

                if (processed >= 0)
                {
                    buffer.trim(static_cast<size_t>(processed));
                    result = true;
                }
                else
                {
                    buffer.trim(0);
                }
            }

            close(fd);
        }

        return result;
    }
}

// https://www.remlab.net/op/futex-condvar.shtml
// https://www.remlab.net/op/futex-misc.shtml

namespace Anemone::Interop
{
    anemone_forceinline void posix_FutexWait(std::atomic<int>& futex, int expected, timespec const* timeout) noexcept
    {
        int const rc = syscall(SYS_futex, &futex, FUTEX_WAIT_PRIVATE, expected, timeout, nullptr, 0);

        if (rc == -1)
        {
            AE_PANIC("FutexWait (rc: {}, '{}')", errno, strerror(errno));
        }
    }

    anemone_forceinline void posix_FutexWakeOne(std::atomic<int>& futex)
    {
        int const rc = syscall(SYS_futex, &futex, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, 1, nullptr, nullptr, 0);

        if (rc == -1)
        {
            AE_PANIC("FutexWakeOne (rc: {}, '{}')", errno, strerror(errno));
        }
    }

    anemone_forceinline void posix_FutexWakeAll(std::atomic<int>& futex)
    {
        int const rc = syscall(SYS_futex, &futex, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, INT32_MAX, nullptr, nullptr, 0);

        if (rc == -1)
        {
            AE_PANIC("FutexWakeAll (rc: {}, '{}')", errno, strerror(errno));
        }
    }

    anemone_forceinline void posix_FutexWait(std::atomic<int>& futex, int expected)
    {
        while (true)
        {
            int const rc = syscall(SYS_futex, &futex, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, expected, nullptr, nullptr, 0);

            if (rc == -1)
            {
                if (errno == EAGAIN)
                {
                    return;
                }
                else
                {
                    AE_PANIC("FutexWait (rc: {}, '{}')", errno, strerror(errno));
                }
            }
            else if (rc == 0)
            {
                if (futex.load() != expected)
                {
                    return;
                }
            }
        }
    }

    template <typename PredicateT>
    anemone_forceinline void posix_FutexWaitUntil(std::atomic<int>& futex, PredicateT&& predicate)
    {
        int value = futex.load();

        while (not std::forward<PredicateT>(predicate)(value))
        {
            posix_FutexWait(futex, value);
            value = futex.load();
        }
    }

    anemone_forceinline bool posix_FutexWaitTimeout(std::atomic<int>& futex, int expected, Duration const& timeout)
    {
        timespec tsTimeout = Interop::posix_FromDuration(timeout);
        timespec tsStart{};
        clock_gettime(CLOCK_MONOTONIC, &tsStart);

        timespec tsElapsed{};

        while (true)
        {
            if (not Interop::posix_CompareGreaterEqual(tsElapsed, tsTimeout))
            {
                return false;
            }

            timespec tsPartialTimeout = Interop::posix_TimespecDifference(tsTimeout, tsElapsed);

            int const rc = syscall(SYS_futex, &futex, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, expected, &tsPartialTimeout, nullptr, 0);

            if (rc != 0)
            {
                int const error = errno;

                if (error == ETIMEDOUT)
                {
                    return false;
                }

                if (error != EAGAIN)
                {
                    AE_PANIC("FutexWaitTimeout (rc: {}, '{}')", error, strerror(error));
                }
            }

            timespec tsCurrent{};
            clock_gettime(CLOCK_MONOTONIC, &tsCurrent);

            tsElapsed = Interop::posix_TimespecDifference(tsCurrent, tsStart);

            if (futex.load() != expected)
            {
                return true;
            }
        }
    }
}

namespace Anemone::Interop
{
    // Note: PATH_MAX is typically defined as 4 KiB.
    using unix_Path = string_buffer<char, 256>;
}
