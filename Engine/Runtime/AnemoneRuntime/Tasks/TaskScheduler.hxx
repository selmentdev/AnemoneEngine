#pragma once
#include "AnemoneRuntime/Diagnostics/Assert.hxx"
#include "AnemoneRuntime/Threading/Thread.hxx"
#include "AnemoneRuntime/Threading/CancellationToken.hxx"
#include "AnemoneRuntime/Threading/UserSemaphore.hxx"
#include "AnemoneRuntime/Threading/Semaphore.hxx"
#include "AnemoneRuntime/Tasks/TaskQueue.hxx"
#include "AnemoneRuntime/Tasks/TaskWorker.hxx"
#include "AnemoneRuntime/UninitializedObject.hxx"

#include <atomic>
#include <vector>

// BUG: Semaphore is used in wrong way.
//      Release/Acquire are unbalanced when reentrant Dispatch calls are made.
namespace Anemone::Tasks
{
    class TaskWorker;

    struct TaskSchedulerOptions final
    {
        uint32_t WorkerThreadsCount{};
    };

    class RUNTIME_API TaskScheduler final
    {
    private:
        std::atomic_uint32_t m_IdGenerator{};

    public:
        uint32_t GenerateTaskId();

    public:
        explicit TaskScheduler(TaskSchedulerOptions const& options);
        TaskScheduler(TaskScheduler const&) = delete;
        TaskScheduler(TaskScheduler&&) = delete;
        TaskScheduler& operator=(TaskScheduler const&) = delete;
        TaskScheduler& operator=(TaskScheduler&&) = delete;
        ~TaskScheduler();

    public:
        TaskQueue m_Queue{};
        std::vector<Thread> m_Threads{};
        std::vector<std::unique_ptr<TaskWorker>> m_Workers{};
        CancellationToken m_CancellationToken{};
        UserSemaphore m_Semaphore{0};
        uint32_t m_WorkerThreadsCount{};

    public:
        void Dispatch(
            Task& task,
            AwaiterHandle const& awaiter,
            AwaiterHandle const& dependency,
            TaskPriority priority);

        void Dispatch(
            Task& task,
            AwaiterHandle const& awaiter,
            AwaiterHandle const& dependency)
        {
            this->Dispatch(task, awaiter, dependency, TaskPriority::Normal);
        }

        void Execute(Task& task);

        void Wait(AwaiterHandle& awaiter);

        bool TryWait(AwaiterHandle& awaiter, Duration const& timeout);

        void Delay(Duration const& timeout);

        uint32_t GetWorkerCount() const
        {
            return this->m_WorkerThreadsCount;
        }
    };
}

namespace Anemone::Tasks
{
    RUNTIME_API extern UninitializedObject<TaskScheduler> GTaskScheduler;
}
