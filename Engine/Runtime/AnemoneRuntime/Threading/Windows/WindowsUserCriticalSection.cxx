#include "AnemoneRuntime/Threading/UserCriticalSection.hxx"
#include "AnemoneRuntime/Diagnostics/Assert.hxx"
#include "AnemoneRuntime/Platform/Windows/WindowsInterop.hxx"

namespace Anemone
{
    void UserCriticalSection::Enter()
    {
        while (true)
        {
            if (this->m_Flag.exchange(StateLocked, std::memory_order::acquire) == StateUnlocked)
            {
                return;
            }

            Interop::win32_FutexWait(this->m_Flag, StateLocked);
        }
    }

    bool UserCriticalSection::TryEnter()
    {
        return this->m_Flag.exchange(StateLocked, std::memory_order::acquire) == StateUnlocked;
    }

    void UserCriticalSection::Leave()
    {
        AE_ASSERT(this->m_Flag.load(std::memory_order::relaxed) != StateUnlocked);

        this->m_Flag.store(StateUnlocked, std::memory_order::release);

        WakeByAddressSingle(&this->m_Flag);
    }
}
