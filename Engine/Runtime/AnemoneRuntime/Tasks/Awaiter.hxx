#pragma once
#include "AnemoneRuntime/Interop/Headers.hxx"
#include "AnemoneRuntime/Base/Reference.hxx"
#include "AnemoneRuntime/Threading/Spinlock.hxx"
#include "AnemoneRuntime/Base/Intrusive.hxx"

#include <atomic>


namespace Anemone
{
    class Awaiter;
    class Task;
}

namespace Anemone
{
    class RUNTIME_API Awaiter final
    {
        friend class Reference<Awaiter>;

    private:
        std::atomic<uint32_t> m_Value{};
        std::atomic<uint32_t> m_ReferenceCount{};
        Spinlock m_Lock{};
        IntrusiveList<Task, Task> m_WaitList{};

    public:
        bool IsCompleted() const
        {
            return this->m_Value.load(std::memory_order_acquire) == 0;
        }

        void AddDependency()
        {
            ++this->m_Value;
        }

        void AddWaitingTask(Task& task);

        bool NotifyCompleted()
        {
            return --this->m_Value == 0;
        }

        void FlushWaitList(IntrusiveList<Task, Task>& list)
        {
            AE_ASSERT(list.IsEmpty());

            UniqueLock scope{this->m_Lock};

            if (this->IsCompleted())
            {
                list.Splice(this->m_WaitList);
            }
        }

    private:
        uint32_t AcquireReference()
        {
            return this->m_ReferenceCount.fetch_add(1, std::memory_order::relaxed);
        }

        uint32_t ReleaseReference()
        {
            uint32_t const result = this->m_ReferenceCount.fetch_sub(1, std::memory_order::acq_rel) - 1;

            if (result == 0)
            {
                delete this;
                return 0;
            }

            return result;
        }
    };

    using AwaiterHandle = Reference<Awaiter>;
}

namespace Anemone
{
    class [[nodiscard]] AwaiterCompletionDeferral final
    {
    private:
        AwaiterHandle& m_Awaiter;

    public:
        AwaiterCompletionDeferral() = delete;
        AwaiterCompletionDeferral(AwaiterCompletionDeferral const&) = delete;
        AwaiterCompletionDeferral(AwaiterCompletionDeferral&&) = delete;
        AwaiterCompletionDeferral& operator=(AwaiterCompletionDeferral const&) = delete;
        AwaiterCompletionDeferral& operator=(AwaiterCompletionDeferral&&) = delete;

        explicit AwaiterCompletionDeferral(AwaiterHandle& awaiter)
            : m_Awaiter{awaiter}
        {
            if (this->m_Awaiter)
            {
                this->m_Awaiter->AddDependency();
            }
        }

        ~AwaiterCompletionDeferral()
        {
            if (this->m_Awaiter)
            {
                this->m_Awaiter->NotifyCompleted();
            }
        }
    };
}
