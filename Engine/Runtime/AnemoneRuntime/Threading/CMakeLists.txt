target_sources(AnemoneRuntime
    PRIVATE
        "AutoResetEvent.hxx"
        "CancellationToken.hxx"
        "ConcurrentAccess.hxx"
        "ConditionVariable.hxx"
        "CriticalSection.hxx"
        "CriticalSectionPool.hxx"
        "CurrentThread.hxx"
        "Lock.hxx"
        "ManualResetEvent.hxx"
        "Monitor.hxx"
        "MpmcQueue.hxx"
        "MpscQueue.hxx"
        "ReaderWriterLock.hxx"
        "Runnable.hxx"
        "Semaphore.hxx"
        "SequenceIndex.hxx"
        "SequentialLock.hxx"
        "Spinlock.hxx"
        "SpinWait.hxx"
        "SpscQueue.hxx"
        "SynchronizedSpscQueue.hxx"
        "Thread.hxx"
        "UserAutoResetEvent.hxx"
        "UserCriticalSection.hxx"
        "UserManualResetEvent.hxx"
        "UserReaderWriterLock.hxx"
        "UserSemaphore.hxx"
)

add_subdirectory("Internal")
add_subdirectory("Platform")
