#include "quick_save_transaction.hpp"
#include <array>
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <stdio.h>
#include <sys/acl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace cm::aot {
namespace {
constexpr const char* staged_name = "save.sav";
struct Descriptor {
    int value = -1;
    Descriptor() = default;
    explicit Descriptor(int fd) : value(fd) {}
    ~Descriptor() { if (value >= 0) ::close(value); }
    Descriptor(const Descriptor&) = delete;
    Descriptor& operator=(const Descriptor&) = delete;
};
QuickSaveTransactionError failure(const char* operation, int code,
                                  const std::filesystem::path& retained = {}) {
    return {std::error_code(code, std::generic_category()), operation, retained};
}
bool same_inode(const struct stat& a, const struct stat& b) {
    return a.st_dev == b.st_dev && a.st_ino == b.st_ino &&
           (a.st_mode & S_IFMT) == (b.st_mode & S_IFMT);
}
bool same_version(const struct stat& a, const struct stat& b) {
    return same_inode(a, b) && a.st_size == b.st_size && a.st_mode == b.st_mode &&
           a.st_uid == b.st_uid && a.st_gid == b.st_gid && a.st_nlink == b.st_nlink &&
           a.st_mtimespec.tv_sec == b.st_mtimespec.tv_sec &&
           a.st_mtimespec.tv_nsec == b.st_mtimespec.tv_nsec &&
           a.st_ctimespec.tv_sec == b.st_ctimespec.tv_sec &&
           a.st_ctimespec.tv_nsec == b.st_ctimespec.tv_nsec;
}
std::string private_name() {
    std::array<unsigned char, 16> random{};
    ::arc4random_buf(random.data(), random.size());
    std::string name = ".cm-quicksave-";
    constexpr char hex[] = "0123456789abcdef";
    for (auto byte : random) { name += hex[byte >> 4]; name += hex[byte & 15]; }
    return name;
}
}

struct QuickSaveTransaction::Impl {
    Descriptor parent, staging_parent, directory, original;
    std::filesystem::path destination, staging, staged;
    std::string leaf, directory_name;
    struct stat parent_identity{}, staging_parent_identity{}, directory_identity{}, original_version{}, backup_version{}, promoted_version{};
    pid_t owner = ::getpid();
    bool original_exists = false;
    bool promotion_attempted = false;
    bool confirmed = false;
    bool backup_exists = false;
    QuickSavePromotionState state = QuickSavePromotionState::not_promoted;

    std::expected<void, QuickSaveTransactionError> check_directory() const {
        if (::getpid() != owner) return std::unexpected(failure("transaction belongs to another process", EPERM, staging));
        struct stat current{};
        if (::lstat(destination.parent_path().c_str(), &current))
            return std::unexpected(failure("inspect destination directory", errno, staging));
        if (!same_inode(current, parent_identity))
            return std::unexpected(failure("destination directory changed", ESTALE, staging));
        if (::lstat(staging.parent_path().c_str(), &current))
            return std::unexpected(failure("inspect staging parent", errno, staging));
        if (!same_inode(current, staging_parent_identity))
            return std::unexpected(failure("staging parent changed", ESTALE, staging));
        if (::fstatat(staging_parent.value, directory_name.c_str(), &current, AT_SYMLINK_NOFOLLOW))
            return std::unexpected(failure("inspect staging directory", errno, staging));
        if (!same_inode(current, directory_identity) || current.st_uid != ::geteuid() ||
            (current.st_mode & 0777) != 0700)
            return std::unexpected(failure("staging directory changed", ESTALE, staging));
        return {};
    }
};

QuickSaveTransaction::QuickSaveTransaction(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
QuickSaveTransaction::~QuickSaveTransaction() = default;

std::expected<std::unique_ptr<QuickSaveTransaction>, QuickSaveTransactionError>
QuickSaveTransaction::create(const std::filesystem::path& destination,
                             const std::filesystem::path& preferred_staging_parent) {
    if (destination.empty() || destination.filename().empty() || destination.filename() == "." ||
        destination.filename() == ".." || destination.native().find('\0') != std::string::npos)
        return std::unexpected(failure("invalid destination", EINVAL));
    std::error_code error;
    const auto parent_path = std::filesystem::canonical(
        destination.has_parent_path() ? destination.parent_path() : std::filesystem::path("."), error);
    if (error) return std::unexpected(QuickSaveTransactionError{error, "resolve destination directory", {}});
    auto impl = std::make_unique<Impl>();
    impl->leaf = destination.filename().string();
    impl->destination = parent_path / impl->leaf;
    impl->parent.value = ::open(parent_path.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (impl->parent.value < 0) return std::unexpected(failure("open destination directory", errno));
    if (::fstat(impl->parent.value, &impl->parent_identity))
        return std::unexpected(failure("inspect destination directory", errno));

    // Use the writable-volume root when it shares the destination device: the
    // original game has a fixed-size path buffer, and nested save folders can
    // otherwise make a private path longer than the original destination.
    auto staging_parent_path = parent_path;
    if (!preferred_staging_parent.empty()) {
        const auto preferred = std::filesystem::canonical(preferred_staging_parent, error);
        if (!error) {
            Descriptor candidate(::open(preferred.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
            struct stat candidate_identity{};
            if (candidate.value >= 0 && !::fstat(candidate.value, &candidate_identity) &&
                candidate_identity.st_dev == impl->parent_identity.st_dev) {
                staging_parent_path = preferred;
                impl->staging_parent.value = candidate.value;
                candidate.value = -1;
                impl->staging_parent_identity = candidate_identity;
            }
        }
    }
    if (impl->staging_parent.value < 0) {
        impl->staging_parent.value = ::open(parent_path.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (impl->staging_parent.value < 0) return std::unexpected(failure("open staging parent", errno));
        if (::fstat(impl->staging_parent.value, &impl->staging_parent_identity))
            return std::unexpected(failure("inspect staging parent", errno));
        if (!same_inode(impl->staging_parent_identity, impl->parent_identity))
            return std::unexpected(failure("destination directory changed during creation", ESTALE));
    }

    struct stat current{};
    if (::fstatat(impl->parent.value, impl->leaf.c_str(), &current, AT_SYMLINK_NOFOLLOW) == 0) {
        if (!S_ISREG(current.st_mode)) return std::unexpected(failure("destination is not a regular file", EINVAL));
        impl->original.value = ::openat(impl->parent.value, impl->leaf.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
        if (impl->original.value < 0) return std::unexpected(failure("open original save", errno));
        if (::fstat(impl->original.value, &impl->original_version))
            return std::unexpected(failure("inspect original save", errno));
        if (!same_version(current, impl->original_version))
            return std::unexpected(failure("destination changed during creation", ESTALE));
        impl->original_exists = true;
    } else if (errno != ENOENT) return std::unexpected(failure("inspect destination", errno));

    for (unsigned attempt = 0; attempt != 32; ++attempt) {
        impl->directory_name = private_name();
        if (::mkdirat(impl->staging_parent.value, impl->directory_name.c_str(), 0700) == 0) break;
        if (errno != EEXIST || attempt == 31)
            return std::unexpected(failure("create private staging directory", errno));
    }
    impl->staging = staging_parent_path / impl->directory_name;
    impl->staged = impl->staging / staged_name;
    impl->directory.value = ::openat(impl->staging_parent.value, impl->directory_name.c_str(),
                                    O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (impl->directory.value < 0)
        return std::unexpected(failure("open private staging directory", errno, impl->staging));
    if (::fstat(impl->directory.value, &impl->directory_identity))
        return std::unexpected(failure("inspect private staging directory", errno, impl->staging));
    if (impl->directory_identity.st_dev != impl->parent_identity.st_dev ||
        impl->directory_identity.st_uid != ::geteuid())
        return std::unexpected(failure("staging directory is not private on the destination volume", EPERM, impl->staging));
    // Inherited macOS ACLs can grant access despite a 0700 POSIX mode. Remove
    // them on this newly owned descriptor before exposing the staging path.
    acl_t empty_acl = ::acl_init(0);
    if (!empty_acl) return std::unexpected(failure("allocate private staging ACL", errno, impl->staging));
    const int acl_result = ::acl_set_fd(impl->directory.value, empty_acl);
    const int acl_error = errno;
    ::acl_free(empty_acl);
    if (acl_result) return std::unexpected(failure("clear inherited staging ACL", acl_error, impl->staging));
    if (::fchmod(impl->directory.value, 0700))
        return std::unexpected(failure("set private staging permissions", errno, impl->staging));
    if (auto checked = impl->check_directory(); !checked) return std::unexpected(checked.error());
    return std::unique_ptr<QuickSaveTransaction>(new QuickSaveTransaction(std::move(impl)));
}

const std::filesystem::path& QuickSaveTransaction::staging_directory() const noexcept { return impl_->staging; }
const std::filesystem::path& QuickSaveTransaction::staged_path() const noexcept { return impl_->staged; }
const std::filesystem::path& QuickSaveTransaction::destination_path() const noexcept { return impl_->destination; }

QuickSavePromotionResult QuickSaveTransaction::promote() {
    auto& t = *impl_;
    QuickSavePromotionResult result;
    result.state = t.state;
    if (t.backup_exists) result.previous_save = t.staged;
    auto reject = [&](const char* operation, int code) {
        result.error = failure(operation, code, t.staging);
        return result;
    };
    if (t.promotion_attempted) return reject("promotion was already attempted", EALREADY);
    t.promotion_attempted = true;
    if (auto checked = t.check_directory(); !checked) { result.error = checked.error(); return result; }
    Descriptor staged(::openat(t.directory.value, staged_name, O_RDONLY | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK));
    if (staged.value < 0) return reject("open staged save", errno);
    struct stat staged_version{}, current{};
    if (::fstat(staged.value, &staged_version)) return reject("inspect staged save", errno);
    if (!S_ISREG(staged_version.st_mode) || staged_version.st_uid != ::geteuid() ||
        staged_version.st_nlink != 1 || staged_version.st_dev != t.parent_identity.st_dev)
        return reject("staged save is not a private regular file on the destination volume", EINVAL);
    if (::fcntl(staged.value, F_FULLFSYNC) == -1) return reject("sync staged save", errno);
    if (::fsync(t.directory.value)) return reject("sync staging directory before promotion", errno);
    if (::fsync(t.staging_parent.value)) return reject("sync staging parent before promotion", errno);
    if (::fsync(t.parent.value)) return reject("sync destination directory before promotion", errno);
    if (auto checked = t.check_directory(); !checked) { result.error = checked.error(); return result; }
    if (::fstatat(t.directory.value, staged_name, &current, AT_SYMLINK_NOFOLLOW))
        return reject("recheck staged save", errno);
    if (!same_version(current, staged_version)) return reject("staged save changed", ESTALE);
    if (::fstatat(t.parent.value, t.leaf.c_str(), &current, AT_SYMLINK_NOFOLLOW) == 0) {
        if (!t.original_exists || !same_version(current, t.original_version))
            return reject("destination changed before promotion", ESTALE);
    } else {
        const int code = errno;
        if (code != ENOENT || t.original_exists) return reject("destination disappeared or cannot be inspected", code);
    }

    // No delete-first fallback: unsupported atomic operations are failures.
    const unsigned flags = (t.original_exists ? RENAME_SWAP : RENAME_EXCL) | RENAME_NOFOLLOW_ANY;
    if (::renameatx_np(t.directory.value, staged_name, t.parent.value, t.leaf.c_str(), flags))
        return reject("atomically promote staged save", errno);
    result.state = t.state = QuickSavePromotionState::promoted_unconfirmed;
    if (t.original_exists) {
        result.previous_save = t.staged;
        t.backup_exists = true;
        if (::fstatat(t.directory.value, staged_name, &t.backup_version, AT_SYMLINK_NOFOLLOW))
            return reject("inspect exchanged previous save", errno);
        if (!same_inode(t.backup_version, t.original_version) ||
            t.backup_version.st_size != t.original_version.st_size ||
            t.backup_version.st_mtimespec.tv_sec != t.original_version.st_mtimespec.tv_sec ||
            t.backup_version.st_mtimespec.tv_nsec != t.original_version.st_mtimespec.tv_nsec)
            return reject("destination changed during exchange; retain both files", ESTALE);
    }
    if (::fstatat(t.parent.value, t.leaf.c_str(), &current, AT_SYMLINK_NOFOLLOW))
        return reject("inspect promoted save", errno);
    if (!same_inode(current, staged_version) || current.st_size != staged_version.st_size ||
        current.st_mtimespec.tv_sec != staged_version.st_mtimespec.tv_sec ||
        current.st_mtimespec.tv_nsec != staged_version.st_mtimespec.tv_nsec)
        return reject("promoted save changed; retain both files", ESTALE);
    if (::fsync(t.directory.value)) return reject("sync staging directory after promotion", errno);
    if (::fsync(t.staging_parent.value)) return reject("sync staging parent after promotion", errno);
    if (::fsync(t.parent.value)) return reject("sync destination directory after promotion", errno);
    if (::fcntl(staged.value, F_FULLFSYNC) == -1) return reject("sync promoted save", errno);
    if (auto checked = t.check_directory(); !checked) { result.error = checked.error(); return result; }
    if (::fstatat(t.parent.value, t.leaf.c_str(), &t.promoted_version, AT_SYMLINK_NOFOLLOW))
        return reject("recheck promoted save", errno);
    if (!same_version(current, t.promoted_version))
        return reject("promoted save changed during sync; retain both files", ESTALE);
    // Explicit close exposes errors instead of silently using the RAII fallback.
    const int staged_fd = staged.value;
    staged.value = -1;
    if (::close(staged_fd)) return reject("close promoted save descriptor", errno);
    t.confirmed = true;
    result.state = t.state = QuickSavePromotionState::promoted;
    return result;
}

std::expected<void, QuickSaveTransactionError> QuickSaveTransaction::discard_previous_save() {
    auto& t = *impl_;
    if (!t.confirmed || !t.backup_exists)
        return std::unexpected(failure("no confirmed previous-save backup to discard", EINVAL, t.staging));
    if (auto checked = t.check_directory(); !checked) return checked;
    struct stat current{};
    if (::fstatat(t.parent.value, t.leaf.c_str(), &current, AT_SYMLINK_NOFOLLOW))
        return std::unexpected(failure("inspect promoted save before backup discard", errno, t.staging));
    if (!same_version(current, t.promoted_version))
        return std::unexpected(failure("promoted save changed; retain previous save", ESTALE, t.staging));
    if (::fstatat(t.directory.value, staged_name, &current, AT_SYMLINK_NOFOLLOW))
        return std::unexpected(failure("inspect previous-save backup", errno, t.staging));
    if (!same_version(current, t.backup_version))
        return std::unexpected(failure("previous-save backup changed; retain it", ESTALE, t.staging));
    if (::unlinkat(t.directory.value, staged_name, 0))
        return std::unexpected(failure("discard previous-save backup", errno, t.staging));
    t.backup_exists = false;
    if (::fsync(t.directory.value))
        return std::unexpected(failure("sync directory after backup discard", errno, t.staging));
    return {};
}
std::expected<void, QuickSaveTransactionError> QuickSaveTransaction::remove_empty_staging_directory() {
    auto& t = *impl_;
    if (!t.confirmed || t.backup_exists)
        return std::unexpected(failure("staging is not empty after confirmed promotion", EINVAL, t.staging));
    if (auto checked = t.check_directory(); !checked) return checked;
    if (::unlinkat(t.staging_parent.value, t.directory_name.c_str(), AT_REMOVEDIR))
        return std::unexpected(failure("remove empty staging directory", errno, t.staging));
    if (::fsync(t.staging_parent.value))
        return std::unexpected(failure("sync staging parent after cleanup", errno, t.staging));
    return {};
}
}
