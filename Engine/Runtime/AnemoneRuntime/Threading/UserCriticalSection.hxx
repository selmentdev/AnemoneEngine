#pragma once
#include "AnemoneRuntime/Interop/Headers.hxx"

#if ANEMONE_FEATURE_FUTEX

#if ANEMONE_PLATFORM_WINDOWS
#include "AnemoneRuntime/Threading/Platform/Windows/WindowsThreading.hxx"
#elif ANEMONE_PLATFORM_ANDROID || ANEMONE_PLATFORM_LINUX
#include "AnemoneRuntime/Threading/Platform/Unix/UnixThreading.hxx"
#else
#error Not implemented
#endif

#include "AnemoneRuntime/Threading/Lock.hxx"
#include "AnemoneRuntime/Diagnostics/Debug.hxx"

#include <atomic>

namespace Anemone
{
    class UserCriticalSection final
    {
    private:
        std::atomic<int32_t> m_Flag{};

        static constexpr int32_t StateLocked = 1;
        static constexpr int32_t StateUnlocked = 0;

    public:
        UserCriticalSection() = default;

        UserCriticalSection(UserCriticalSection const&) = delete;

        UserCriticalSection(UserCriticalSection&&) = delete;

        UserCriticalSection& operator=(UserCriticalSection const&) = delete;

        UserCriticalSection& operator=(UserCriticalSection&&) = delete;

        ~UserCriticalSection() = default;

        void Enter()
        {
            while (true)
            {
                if (this->m_Flag.exchange(StateLocked, std::memory_order::acquire) == StateUnlocked)
                {
                    return;
                }

                Internal::Futex::Wait(this->m_Flag, StateLocked);
            }
        }

        bool TryEnter()
        {
            return this->m_Flag.exchange(StateLocked, std::memory_order::acquire) == StateUnlocked;
        }

        void Leave()
        {
            AE_ASSERT(this->m_Flag.load(std::memory_order::relaxed) != StateUnlocked);

            this->m_Flag.store(StateUnlocked, std::memory_order::release);

            Internal::Futex::WakeOne(this->m_Flag);
        }

        template <typename F>
        auto With(F&& f) -> std::invoke_result_t<F&&>
        {
            UniqueLock scope{*this};
            return std::forward<F>(f)();
        }
    };
}

#else

#include "AnemoneRuntime/Threading/CriticalSection.hxx"

namespace Anemone
{
    using UserCriticalSection = CriticalSection;
}

#endif
