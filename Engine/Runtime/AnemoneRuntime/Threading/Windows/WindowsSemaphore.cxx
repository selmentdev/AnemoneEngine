#include "AnemoneRuntime/Threading/Semaphore.hxx"
#include "AnemoneRuntime/Diagnostics/Assert.hxx"
#include "AnemoneRuntime/Platform/Windows/WindowsInterop.hxx"

namespace Anemone
{
    Semaphore::Semaphore(int32_t initialCount, int32_t maxCount)
    {
        AE_ASSERT(initialCount >= 0);
        AE_ASSERT(maxCount >= 1);

        this->m_native.Inner = CreateSemaphoreW(
            nullptr,
            initialCount,
            maxCount,
            nullptr);
    }

    Semaphore::~Semaphore()
    {
        CloseHandle(this->m_native.Inner);
    }

    void Semaphore::Acquire()
    {
        WaitForSingleObject(
            this->m_native.Inner,
            INFINITE);
    }

    bool Semaphore::TryAcquire(Duration const& timeout)
    {
        return WaitForSingleObject(
                   this->m_native.Inner,
                   Interop::win32_ValidateTimeoutDuration(timeout)) != WAIT_TIMEOUT;
    }

    bool Semaphore::TryAcquire()
    {
        return WaitForSingleObject(
                   this->m_native.Inner,
                   0) != WAIT_TIMEOUT;
    }

    void Semaphore::Release()
    {
        ReleaseSemaphore(
            this->m_native.Inner,
            1,
            nullptr);
    }
}
