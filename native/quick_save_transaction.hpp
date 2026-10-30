#pragma once
#include <expected>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>

namespace cm::aot {
struct QuickSaveTransactionError {
    std::error_code code;
    std::string operation;
    // A failed creation may have left an owned, private directory here.
    std::filesystem::path retained_directory;
};

enum class QuickSavePromotionState {
    not_promoted,
    promoted,
    // The atomic rename happened, but a subsequent identity/durability check
    // failed. Do not retry or claim that the destination is unchanged.
    promoted_unconfirmed
};

struct QuickSavePromotionResult {
    QuickSavePromotionState state = QuickSavePromotionState::not_promoted;
    QuickSaveTransactionError error;
    // Present after an exchange. Retained even on a post-exchange failure.
    std::filesystem::path previous_save;
};

// Owns a private staging directory and directory descriptors, not guest IO.
// The caller must finish/close all staged writers, validate serialization and
// latch their errors before explicitly calling promote(). No guest callbacks
// may write into staging during promotion. Creation does not create save.sav.
//
// Rename preserves the staged inode, including resource fork/Finder metadata.
// Existing destinations are exchanged, never first deleted. No files are
// removed by destruction. An earlier save is retained until explicit discard.
//
// Concurrency boundary: descriptor-relative operations and identity snapshots
// reject observed replacement/modification. macOS has no rename conditional on
// an expected inode: an uncooperative same-user process can change a name after
// its check. A detected post-exchange race retains both files and reports
// promoted_unconfirmed; no unsafe automatic rollback is attempted. The caller
// must exclude competing saves. Directory privacy is not protection from root
// or other processes running as the same user. Sync success reports successful
// system calls, not an unconditional hardware/filesystem crash guarantee.
class QuickSaveTransaction {
public:
    static std::expected<std::unique_ptr<QuickSaveTransaction>, QuickSaveTransactionError>
        create(const std::filesystem::path& destination,
               const std::filesystem::path& preferred_staging_parent = {});
    ~QuickSaveTransaction();
    QuickSaveTransaction(const QuickSaveTransaction&) = delete;
    QuickSaveTransaction& operator=(const QuickSaveTransaction&) = delete;

    const std::filesystem::path& staging_directory() const noexcept;
    const std::filesystem::path& staged_path() const noexcept;
    const std::filesystem::path& destination_path() const noexcept;
    QuickSavePromotionResult promote();

    // Only available after confirmed promotion of an existing destination.
    // Checks both the retained backup and the promoted destination against
    // their captured identity/content metadata and uses
    // its private directory descriptor; never accepts a caller-supplied path.
    // Failure never triggers further cleanup. A sync failure can be reported
    // after unlink has already happened. Requires the same exclusion of other
    // writers as promote(); check/unlink is not an inode-conditional syscall.
    std::expected<void, QuickSaveTransactionError> discard_previous_save();
    // Removes only this transaction's empty staging directory after a
    // confirmed promotion; nonempty or replaced directories are retained.
    std::expected<void, QuickSaveTransactionError> remove_empty_staging_directory();
    // Removes the writer's output and the directory while the destination has
    // not been touched (state not_promoted). Anything else in it is retained.
    std::expected<void, QuickSaveTransactionError> discard_failed_staging();
    // Marks the directory so that remove_stale_quick_save_staging leaves it:
    // for when neither file can be called the obsolete one.
    std::expected<void, QuickSaveTransactionError> keep_for_recovery();

private:
    struct Impl;
    explicit QuickSaveTransaction(std::unique_ptr<Impl>);
    std::unique_ptr<Impl> impl_;
};

// Removes staging directories left in `parent` by earlier sessions or failed
// saves, keeping the newest one holding a save as a recovery copy. The caller
// must have no transaction in progress there. Only directories this code could
// have made are touched: private name, mode and owner, and the writer's two
// file names inside. Returns how many were removed.
std::size_t remove_stale_quick_save_staging(const std::filesystem::path& parent);
}
