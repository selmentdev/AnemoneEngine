#include "AnemoneRuntime/Threading/Unix/UnixThread.hxx"
#include "AnemoneRuntime/Diagnostics/Assert.hxx"
#include "AnemoneRuntime/Platform/Unix/UnixInterop.hxx"

#include <cmath>
#include <utility>

namespace Anemone::Private
{
    constexpr int ConvertThreadPriority(ThreadPriority priority) noexcept
    {
        switch (priority)
        {
        case ThreadPriority::TimeCritical:
        case ThreadPriority::Highest:
            return 30;
        case ThreadPriority::AboveNormal:
            return 25;
        case ThreadPriority::Normal:
            return 15;
        case ThreadPriority::BelowNormal:
            return 5;
        case ThreadPriority::Lower:
            return 14;
        case ThreadPriority::Lowest:
            return 1;
        }

        return 5;
    }

    static void* ThreadEntryPoint(void* context)
    {
        if (context == nullptr)
        {
            AE_PANIC("Thread started without context.");
        }

        Runnable* const runnable = static_cast<Runnable*>(context);
        runnable->Run();

        pthread_exit(nullptr);
        return nullptr;
    }

}

namespace Anemone
{
    UnixThread::UnixThread(ThreadStart const& start)
    {
        if (start.Callback == nullptr)
        {
            AE_PANIC("Thread started without callback.");
        }

        pthread_attr_t attr{};

        if (int const rc = pthread_attr_init(&attr); rc != 0)
        {
            AE_PANIC("pthread_attr_init (rc: {}, `{}`)", rc, strerror(rc));
        }

        if (start.StackSize)
        {
            if (int const rc = pthread_attr_setstacksize(&attr, *start.StackSize); rc != 0)
            {
                AE_PANIC("pthread_attr_init (rc: {}, `{}`)", rc, strerror(rc));
            }
        }

        if (int const rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); rc != 0)
        {
            AE_PANIC("pthread_attr_setdetachstate (rc: {}, `{}`)", rc, strerror(rc));
        }

        if (int const rc = pthread_create(this->_handle.GetAddressOf(), &attr, Private::ThreadEntryPoint, start.Callback); rc != 0)
        {
            AE_PANIC("pthread_create (rc: {}, `{}`)", rc, strerror(rc));
        }

        if (int const rc = pthread_attr_destroy(&attr); rc != 0)
        {
            AE_PANIC("pthread_attr_destroy (rc: {}, `{}`)", rc, strerror(rc));
        }

        if (not this->_handle)
        {
            AE_PANIC("Failed to start thread");
        }

        this->_id = ThreadId{Interop::posix_GetThreadId()};


        //
        // FIXME:
        //     Thread is started immediately in pthread_create. It is possible that the thread will
        //     finish before we set the name and priority.
        //
        //     In that case, either ignore errors in reported by further pthread calls or notify
        //     thread callback that we finished initialization process so it can continue.
        //
        //     This is not a problem in Windows implementation because thread is started in suspended
        //     state.
        //
        //     Alternatively we can set these properties in the thread callback.
        //

        if (start.Name)
        {
            std::string const name{*start.Name};
            pthread_setname_np(this->_handle.Get(), name.c_str());
        }

        if (start.Priority)
        {
            sched_param sched{};
            int32_t policy = SCHED_RR;

            if (pthread_getschedparam(this->_handle.Get(), &policy, &sched) == 0)
            {
                sched.sched_priority = Private::ConvertThreadPriority(*start.Priority);

                pthread_setschedparam(this->_handle.Get(), policy, &sched);
            }
        }
    }

    UnixThread::UnixThread(UnixThread&& other) noexcept
        : _handle{std::exchange(other._handle, {})}
        , _id{std::exchange(other._id, {})}
    {
    }

    UnixThread& UnixThread::operator=(UnixThread&& other) noexcept
    {
        if (this != std::addressof(other))
        {
            if (this->IsJoinable())
            {
                this->Join();
            }

            this->_handle = std::exchange(other._handle, {});
            this->_id = std::exchange(other._id, {});
        }

        return *this;
    }

    UnixThread::~UnixThread()
    {
        if (this->IsJoinable())
        {
            this->Join();
        }
    }

    void UnixThread::Join()
    {
        if (not this->_handle)
        {
            AE_PANIC("Cannot join non-started thread.");
        }

        if (this->_handle.Get() == pthread_self())
        {
            AE_PANIC("Joining thread from itself");
        }

        if (int const rc = pthread_join(this->_handle.Get(), nullptr); rc != 0)
        {
            AE_PANIC("pthread_join (rc: {}, `{}`)", rc, strerror(rc));
        }

        this->_handle = {};
        this->_id = {};
    }

    [[nodiscard]] bool UnixThread::IsJoinable() const
    {
        return this->_handle.IsValid();
    }

    void UnixThread::Detach()
    {
        if (not this->_handle)
        {
            AE_PANIC("Failed to detach from thread");
        }

        if (int const rc = pthread_detach(this->_handle.Get()); rc != 0)
        {
            AE_PANIC("pthread_detach (rc: {}, `{}`)", rc, strerror(rc));
        }

        this->_handle = {};
        this->_id = {};
    }
}
