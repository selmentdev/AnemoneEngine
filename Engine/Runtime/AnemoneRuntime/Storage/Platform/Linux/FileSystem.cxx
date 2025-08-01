#include "AnemoneRuntime/Storage/Platform/Linux/FileSystem.hxx"
#include "AnemoneRuntime/Storage/Platform/Linux/FileHandle.hxx"
#include "AnemoneRuntime/Base/UninitializedObject.hxx"
#include "AnemoneRuntime/Diagnostics/Trace.hxx"
#include "AnemoneRuntime/Diagnostics/Debug.hxx"
#include "AnemoneRuntime/Platform/FilePath.hxx"

#include "AnemoneRuntime/Interop/Linux/DateTime.hxx"
#include "AnemoneRuntime/Interop/Linux/FileSystem.hxx"

#include <dirent.h>
#include <string_view>
#include <unistd.h>
#include <sys/file.h>


namespace Anemone::Internal
{
    namespace
    {
        UninitializedObject<LinuxFileSystem> GPlatformFileSystem{};
    }

    extern void PlatformInitializeFileSystem()
    {
        GPlatformFileSystem.Create();
    }

    extern void PlatformFinalizeFileSystem()
    {
        GPlatformFileSystem.Destroy();
    }
}

namespace Anemone
{
    FileSystem& FileSystem::GetPlatformFileSystem()
    {
        return *Internal::GPlatformFileSystem;
    }
}
namespace Anemone
{
    namespace
    {
        constexpr auto FileTypeFromSystem(
            dirent const& d)
            -> FileType
        {
            switch (d.d_type)
            {
            default:
            case DT_UNKNOWN:
                return FileType::Unknown;

            case DT_FIFO:
                return FileType::NamedPipe;

            case DT_CHR:
                return FileType::CharacterDevice;

            case DT_DIR:
                return FileType::Directory;

            case DT_BLK:
                return FileType::BlockDevice;

            case DT_REG:
                return FileType::File;

            case DT_LNK:
                return FileType::SymbolicLink;

            case DT_SOCK:
                return FileType::Socket;
            }
        }

        constexpr auto FileTypeFromSystem(
            struct stat64 const& v)
            -> FileType
        {
            switch (v.st_mode & S_IFMT)
            {
            case S_IFSOCK:
                return FileType::Socket;

            case S_IFLNK:
                return FileType::SymbolicLink;

            case S_IFREG:
                return FileType::File;

            case S_IFBLK:
                return FileType::BlockDevice;

            case S_IFDIR:
                return FileType::Directory;

            case S_IFCHR:
                return FileType::CharacterDevice;

            case S_IFIFO:
                return FileType::NamedPipe;

            default:
                return FileType::Unknown;
            }
        }

        void FileInfoFromSystem(
            struct stat64 const& source,
            FileInfo& destination)
        {
            destination.Created = Interop::Linux::ToDateTime(source.st_ctim);
            destination.Modified = Interop::Linux::ToDateTime(source.st_mtim);
            destination.Size = static_cast<int64_t>(source.st_size);
            destination.Type = FileTypeFromSystem(source);

            if (!(source.st_mode & S_IWUSR) and !(source.st_mode & S_IWGRP) and !(source.st_mode & S_IWOTH))
            {
                destination.ReadOnly = true;
            }
            else
            {
                destination.ReadOnly = false;
            }
        }

        constexpr auto TranslateToOpenMode(
            Flags<FileOption> options)
            -> mode_t
        {
            mode_t result = S_IRUSR | S_IWUSR;

            if (options.Any(FileOption::ShareDelete))
            {
                result |= 0;
            }

            if (options.Any(FileOption::ShareRead))
            {
                result |= S_IRGRP | S_IROTH;
            }

            if (options.Any(FileOption::ShareWrite))
            {
                result |= S_IWGRP | S_IWOTH;
            }

            return result;
        }

        constexpr auto TranslateToOpenFlags(
            FileMode mode,
            Flags<FileAccess> access,
            Flags<FileOption> options,
            bool failForSymlinks)
            -> int
        {
            int result = O_CLOEXEC;

            if (failForSymlinks)
            {
                result |= O_NOFOLLOW;
            }

            switch (mode)
            {
            case FileMode::OpenExisting:
                break;

            case FileMode::TruncateExisting:
                result |= O_TRUNC;
                break;

            case FileMode::OpenAlways:
                result |= O_CREAT;
                break;

            case FileMode::CreateAlways:
                result |= O_CREAT | O_TRUNC;
                break;

            case FileMode::CreateNew:
                result |= O_CREAT | O_EXCL;
                break;
            }

            switch (access)
            {
            case FileAccess::Read:
                result |= O_RDONLY;
                break;

            case FileAccess::Write:
                result |= O_WRONLY;
                break;

            case FileAccess::ReadWrite:
                result |= O_RDWR;
                break;
            }

            if (options.Any(FileOption::WriteThrough))
            {
                result |= O_SYNC;
            }

            return result;
        }

        [[maybe_unused]]
        auto InternalGetFileStatAndLinkStat(
            Interop::Linux::FilePath const& path,
            struct stat64& outFileStat,
            struct stat64& outLinkStat)
            -> int
        {
            if (not stat64(path.c_str(), &outFileStat))
            {
                if (errno == ENOENT)
                {
                    // Check if it's a dangling symlink.
                    if (not lstat64(path.c_str(), &outFileStat))
                    {
                        return errno;
                    }
                }
            }

            if (not lstat64(path.c_str(), &outLinkStat))
            {
                return errno;
            }

            return 0;
        }

        inline constexpr blksize_t MinimumBlockSize = 8 << 10u;
        inline constexpr blksize_t MaximumBlockSize = 64 << 10u;

        [[maybe_unused]]
        auto InternalCopyFile(
            Interop::Linux::SafeFdHandle const& source,
            Interop::Linux::SafeFdHandle const& destination,
            struct stat64 const& sourceStat)
            -> std::expected<void, ErrorCode>
        {
            // Choose block size.
            size_t const bufferSize = std::min(std::max(sourceStat.st_blksize, MinimumBlockSize), MaximumBlockSize);

            std::unique_ptr<std::byte[]> buffer = std::make_unique_for_overwrite<std::byte[]>(bufferSize);

            ssize_t readBytes;

            while ((readBytes = read(source.Get(), buffer.get(), bufferSize)) > 0)
            {
                std::byte* writeBuffer = buffer.get();
                ssize_t writeBytes = readBytes;

                while (writeBytes > 0)
                {
                    ssize_t const writtenBytes = write(destination.Get(), writeBuffer, writeBytes);

                    if (writtenBytes < 0)
                    {
                        if (errno == EINTR)
                        {
                            continue;
                        }

                        return std::unexpected(ErrorCode::IoError);
                    }

                    writeBytes -= writtenBytes;
                    writeBuffer += writtenBytes;
                }
            }

            if (readBytes < 0)
            {
                return std::unexpected(ErrorCode::IoError);
            }

            AE_ASSERT(readBytes == 0);
            return {};
        }
    }

    LinuxFileSystem::LinuxFileSystem() = default;

    auto LinuxFileSystem::CreatePipe(
        std::unique_ptr<FileHandle>& outReader,
        std::unique_ptr<FileHandle>& outWriter)
        -> std::expected<void, ErrorCode>
    {
        using namespace Interop::Linux;

        int fd[2];

        if (pipe2(fd, O_CLOEXEC | O_NONBLOCK))
        {
            return std::unexpected(ErrorCode::Failure);
        }

        outReader = std::make_unique<LinuxFileHandle>(SafeFdHandle{fd[0]});
        outWriter = std::make_unique<LinuxFileHandle>(SafeFdHandle{fd[1]});

        return {};
    }

    auto LinuxFileSystem::CreateFile(
        std::string_view path,
        FileMode mode,
        Flags<FileAccess> access,
        Flags<FileOption> options)
        -> std::expected<std::unique_ptr<FileHandle>, ErrorCode>
    {
        // FIXME: DeleteOnClose -> remember in LinuxFileHandle that we need unlink file before
        using namespace Interop::Linux;

        int const flags = TranslateToOpenFlags(mode, access, options, false);
        mode_t const fmode = TranslateToOpenMode(options);

        Interop::Linux::FilePath const filePath{path};

        SafeFdHandle handle{open(filePath.c_str(), flags, fmode)};

        if (handle)
        {
            if (flock(handle.Get(), LOCK_EX | LOCK_NB))
            {
                int const error = errno;

                if ((error == EAGAIN) or (error == EWOULDBLOCK))
                {
                    return std::unexpected(ErrorCode::Failure);
                }
            }

            if ((mode == FileMode::TruncateExisting) or (mode == FileMode::CreateAlways))
            {
                if (ftruncate64(handle.Get(), 0))
                {
                    return std::unexpected(ErrorCode::Failure);
                }
            }

            // HAVE_POSIX_FADVISE
            if (options.Has(FileOption::SequentialScan))
            {
                posix_fadvise(handle.Get(), 0, 0, POSIX_FADV_SEQUENTIAL);
            }

            if (options.Has(FileOption::RandomAccess))
            {
                posix_fadvise(handle.Get(), 0, 0, POSIX_FADV_RANDOM);
            }

            return std::make_unique<LinuxFileHandle>(std::move(handle));
        }

        return std::unexpected(ErrorCode::Failure);
    }

    auto LinuxFileSystem::GetPathInfo(
        std::string_view path)
        -> std::expected<FileInfo, ErrorCode>
    {
        using namespace Interop::Linux;

        Interop::Linux::FilePath nativePath{path};

        struct stat64 st{};

        if (stat64(nativePath.c_str(), &st) < 0)
        {
            return std::unexpected(ErrorCode::Failure);
        }

        FileInfo fileInfo;
        FileInfoFromSystem(st, fileInfo);
        return fileInfo;
    }

    auto LinuxFileSystem::Exists(
        std::string_view path)
        -> std::expected<bool, ErrorCode>
    {
        using namespace Interop::Linux;

        Interop::Linux::FilePath nativePath{path};

        struct stat64 st{};

        if (stat64(nativePath.c_str(), &st) < 0)
        {
            // Check if file exists
            return std::unexpected(ErrorCode::Failure);
        }

        return S_ISREG(st.st_mode) or S_ISDIR(st.st_mode);
    }

    auto LinuxFileSystem::FileExists(
        std::string_view path)
        -> std::expected<bool, ErrorCode>
    {
        using namespace Interop::Linux;

        Interop::Linux::FilePath nativePath{path};

        struct stat64 st{};

        if (stat64(nativePath.c_str(), &st) < 0)
        {
            return std::unexpected(ErrorCode::Failure);
        }

        return S_ISREG(st.st_mode);
    }

    auto LinuxFileSystem::FileDelete(
        std::string_view path)
        -> std::expected<void, ErrorCode>
    {
        using namespace Interop::Linux;

        Interop::Linux::FilePath nativePath{path};

        if (unlink(nativePath.c_str()) < 0)
        {
            return std::unexpected(ErrorCode::Failure);
        }

        return {};
    }

    auto LinuxFileSystem::FileCopy(
        std::string_view source,
        std::string_view destination,
        NameCollisionResolve nameCollisionResolve)
        -> std::expected<void, ErrorCode>
    {
        using namespace Interop::Linux;

        Interop::Linux::FilePath nativeSource{source};
        Interop::Linux::FilePath nativeDestination{destination};

        SafeFdHandle handleSource{open(nativeSource.c_str(), O_RDONLY, 0)};

        if (not handleSource)
        {
            return std::unexpected(ErrorCode::Failure);
        }

        struct stat64 st;
        if (fstat64(handleSource.Get(), &st) < 0)
        {
            return std::unexpected(ErrorCode::Failure);
        }

        SafeFdHandle handleDestination{};

        if (nameCollisionResolve == NameCollisionResolve::Fail)
        {
            handleDestination = SafeFdHandle{open(nativeDestination.c_str(), O_WRONLY | O_CREAT | O_EXCL, st.st_mode)};
        }
        else
        {
            handleDestination = SafeFdHandle{open(nativeDestination.c_str(), O_WRONLY | O_TRUNC, st.st_mode)};

            if (not handleDestination)
            {
                handleDestination = SafeFdHandle{open(nativeDestination.c_str(), O_WRONLY | O_CREAT | O_TRUNC, st.st_mode)};
            }
        }

        if (not handleDestination)
        {
            return std::unexpected(ErrorCode::Failure);
        }

        return InternalCopyFile(handleSource, handleDestination, st);
    }

    auto LinuxFileSystem::FileMove(
        std::string_view source,
        std::string_view destination,
        NameCollisionResolve nameCollisionResolve)
        -> std::expected<void, ErrorCode>
    {
        using namespace Interop::Linux;

        Interop::Linux::FilePath nativeSource{source};
        Interop::Linux::FilePath nativeDestination{destination};

        // TODO: name collision resolve strategy
        (void)nameCollisionResolve;

        if (rename(nativeSource.c_str(), nativeDestination.c_str()) < 0)
        {
            return std::unexpected(ErrorCode::Failure);
        }

        return {};
    }

    auto LinuxFileSystem::DirectoryExists(
        std::string_view path)
        -> std::expected<bool, ErrorCode>
    {
        using namespace Interop::Linux;

        Interop::Linux::FilePath nativePath{path};

        struct stat64 st{};

        if (stat64(nativePath.c_str(), &st) < 0)
        {
            return std::unexpected(ErrorCode::Failure);
        }

        return S_ISDIR(st.st_mode);
    }

    auto LinuxFileSystem::DirectoryDelete(
        std::string_view path)
        -> std::expected<void, ErrorCode>
    {
        using namespace Interop::Linux;

        Interop::Linux::FilePath nativePath{path};

        if (rmdir(nativePath.c_str()) < 0)
        {
            return std::unexpected(ErrorCode::Failure);
        }

        return {};
    }

    auto LinuxFileSystem::DirectoryDeleteRecursive(
        std::string_view path)
        -> std::expected<void, ErrorCode>
    {
        // not implemented
        (void)path;
        return std::unexpected(ErrorCode::Failure);
    }

    auto LinuxFileSystem::DirectoryCreate(
        std::string_view path)
        -> std::expected<void, ErrorCode>
    {
        using namespace Interop::Linux;

        Interop::Linux::FilePath nativePath{path};

        // or 0755?
        if (mkdir(nativePath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) < 0)
        {
            return std::unexpected(ErrorCode::Failure);
        }

        return {};
    }

    auto LinuxFileSystem::DirectoryCreateRecursive(
        std::string_view path)
        -> std::expected<void, ErrorCode>
    {
        (void)path;
        // not implemented
        return std::unexpected(ErrorCode::Failure);
    }

    auto LinuxFileSystem::DirectoryEnumerate(
        std::string_view path,
        FileSystemVisitor& visitor)
        -> std::expected<void, ErrorCode>
    {
        using namespace std::literals;

        std::string root{path};

        auto& error = errno;

        if (Interop::Linux::SafeDirHandle handle{opendir(root.c_str())})
        {
            while (true)
            {
                error = 0;

                dirent* current = readdir(handle.Get());

                if (current == nullptr)
                {
                    if (error != 0)
                    {
                        return std::unexpected(ErrorCode::Failure);
                    }

                    // End of directory.
                    return {};
                }

                FileType const fileType = FileTypeFromSystem(*current);
                std::string_view const name{current->d_name};

                if (fileType == FileType::Directory)
                {
                    if ((name == "."sv) || (name == ".."sv))
                    {
                        continue;
                    }
                }

                FilePath::PushFragment(root, name);

                FileInfo fileInfo{};
                struct stat64 st{};

                if (stat64(root.c_str(), &st) == 0)
                {
                    FileInfoFromSystem(st, fileInfo);
                }
                else
                {
                    AE_TRACE(Warning, "Failed to access file info for {}", root);
                }

                FilePath::PopFragment(root);

                visitor.Visit(root, name, fileInfo);
            }
        }

        return std::unexpected(ErrorCode::Failure);
    }

    auto LinuxFileSystem::DirectoryEnumerateRecursive(
        std::string_view path,
        FileSystemVisitor& visitor)
        -> std::expected<void, ErrorCode>
    {
        class RecursiveVisitor final : public FileSystemVisitor
        {
        private:
            LinuxFileSystem* _owner{};
            FileSystemVisitor* _visitor{};

        public:
            RecursiveVisitor(
                LinuxFileSystem* owner,
                FileSystemVisitor& visitor)
                : _owner{owner}
                , _visitor{&visitor}
            {
            }

            void Visit(std::string_view path, std::string_view name, FileInfo const& info) override
            {
                this->_visitor->Visit(path, name, info);

                if (info.Type == FileType::Directory)
                {
                    (void)this->_owner->DirectoryEnumerate(path, *this->_visitor);
                }
            }
        };

        RecursiveVisitor recursiveVisitor{this, visitor};
        return this->DirectoryEnumerate(path, recursiveVisitor);
    }
}
