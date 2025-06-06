#pragma once
#include "AnemoneRuntime/Threading/SpinWait.hxx"
#include "AnemoneRuntime/Threading/Thread.hxx"
#include "AnemoneRuntime/Threading/CurrentThread.hxx"
#include "AnemoneRuntime/Diagnostics/Debug.hxx"
#include "AnemoneRuntime/Threading/Lock.hxx"

#include <atomic>
#include <type_traits>

namespace Anemone
{
    //! Represents a spinlock.
    //!
    //! A spinlock is a lock that uses busy waiting to acquire a lock.
    class Spinlock final
    {
    private:
        std::atomic_flag _flag{};

    public:
        Spinlock() = default;
        Spinlock(Spinlock const&) = delete;
        Spinlock(Spinlock&&) = delete;
        Spinlock& operator=(Spinlock const&) = delete;
        Spinlock& operator=(Spinlock&&) = delete;
        ~Spinlock() = default;

    public:
        //! Acquires the spinlock.
        //!
        //! This function will busy wait until the spinlock is acquired.
        void Enter()
        {
            // test_and_set() returns the previous value of the flag.
            WaitForCompletion([this]
            {
                return !this->_flag.test_and_set(std::memory_order::acquire);
            });
        }

        bool TryEnter()
        {
            return !this->_flag.test_and_set(std::memory_order::acquire);
        }

        //! Releases the spinlock.
        //!
        //! This function will release the spinlock.
        void Leave()
        {
            this->_flag.clear(std::memory_order::release);
        }

        template <typename F>
        auto With(F&& f) -> std::invoke_result_t<F&&>
        {
            UniqueLock scope{*this};
            return std::forward<F>(f)();
        }
    };
}

namespace Anemone
{
    class FastSpinLock final
    {
    private:
        alignas(64) std::atomic_bool _lock{};

    public:
        FastSpinLock() = default;
        FastSpinLock(FastSpinLock const&) = delete;
        FastSpinLock(FastSpinLock&&) = delete;
        FastSpinLock& operator=(FastSpinLock const&) = delete;
        FastSpinLock& operator=(FastSpinLock&&) = delete;
        ~FastSpinLock() = default;

    public:
        void Enter()
        {
            while (true)
            {
                while (this->_lock.load(std::memory_order::acquire))
                {
                    CurrentThread::Pause();
                }

                if (not this->_lock.exchange(true, std::memory_order::acquire))
                {
                    break;
                }
            }
        }

        void Leave()
        {
            this->_lock.store(false, std::memory_order::release);
        }

        template <typename F>
        auto With(F&& f) -> std::invoke_result_t<F&&>
        {
            UniqueLock scope{*this};
            return std::forward<F>(f)();
        }
    };
}

namespace Anemone
{
    class RecursiveSpinlock final
    {
    private:
        std::atomic_size_t m_Count = 0;
        std::atomic<ThreadId> m_Owner{};

    public:
        RecursiveSpinlock() = default;
        RecursiveSpinlock(RecursiveSpinlock const&) = delete;
        RecursiveSpinlock(RecursiveSpinlock&&) = delete;
        RecursiveSpinlock& operator=(RecursiveSpinlock const&) = delete;
        RecursiveSpinlock& operator=(RecursiveSpinlock&&) = delete;
        ~RecursiveSpinlock() = default;

    public:
        void Enter()
        {
            ThreadId const id = CurrentThread::Id();

            if (this->m_Owner.load(std::memory_order::acquire) != id)
            {
                ThreadId owner;
                do
                {
                    // When 'compare_exchange_weak' fails, 'expected' will be updated with the current value of 'm_Owner'.
                    // Reset 'expected' to 'ThreadId::Invalid()' to ensure that the loop will continue until the lock is acquired.
                    owner = ThreadId{};

                    WaitForCompletion([&]
                    {
                        return this->m_Owner.load(std::memory_order::acquire) == ThreadId{};
                    });
                } while (!this->m_Owner.compare_exchange_weak(owner, id, std::memory_order::acquire));
            }

            ++this->m_Count;
        }

        bool TryEnter()
        {
            ThreadId const id = CurrentThread::Id();

            if (ThreadId owner = this->m_Owner.load(std::memory_order::acquire); owner == id)
            {
                // Lock is already owned by the current thread.
                ++this->m_Count;
                return true;
            }

            ThreadId owner = {};

            if (this->m_Owner.compare_exchange_weak(owner, id, std::memory_order::acquire))
            {
                // Successfully acquired the lock by the current thread.
                ++this->m_Count;
                return true;
            }

            // Failed to acquire lock.
            return false;
        }

        void Leave()
        {
            AE_ASSERT(this->m_Owner.load(std::memory_order::relaxed) == CurrentThread::Id(), "Different thread owns this lock.");

            if (--this->m_Count == 0)
            {
                this->m_Owner.store(ThreadId{}, std::memory_order::release);
            }
        }

        template <typename F>
        auto With(F&& f) -> std::invoke_result_t<F&&>
        {
            UniqueLock scope{*this};
            return std::forward<F>(f)();
        }
    };
}
