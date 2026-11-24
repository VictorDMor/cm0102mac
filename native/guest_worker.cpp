#include "guest_worker.hpp"
#include "aot_runtime.hpp"
#include "guest_scratch.hpp"
#include <algorithm>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <pthread.h>
#include <stdexcept>
#include <system_error>

namespace cm::aot {
struct GuestWorker::Impl {
    State& main;
    State worker;
    pthread_t thread{};
    std::mutex mutex, call_mutex;
    std::condition_variable wake;
    enum class Job { none, run, done, quit } job = Job::none;
    std::uint32_t return_pc = 0;
    std::exception_ptr failure;
    std::uint64_t calls = 0;
    // The part of the caller's frame a callee may touch (linkage and parameter area), carried to the own
    // stack and back; and where the own stack's first frame starts, 0 on the caller's stack.
    static constexpr std::uint32_t caller_frame = 256;
    std::uint32_t own_sp = 0;
    bool scratch = false;

    Impl(State& state, std::optional<unsigned> own_stack, bool own_heap) : main(state) {
        if (own_stack) {
            // Stacks end where the scratch heaps start.
            static_assert(GuestWorker::stack_start(GuestScratch::limit) <= GuestScratch::start(0));
            if (*own_stack >= GuestScratch::limit) throw std::out_of_range("Guest worker stack index");
            const auto start = GuestWorker::stack_start(*own_stack);
            bool mapped = false;
            for (const auto& [address, size] : main.memory.mapped_ranges())
                mapped = mapped || (address <= start && start + std::uint64_t{GuestWorker::stack_size} <= address + size);
            if (!mapped) main.memory.map(start, GuestWorker::stack_size);
            own_sp = (start + GuestWorker::stack_size - 0x100 - caller_frame) & ~15u;
            if (own_heap) { GuestScratch::enter(main, worker, *own_stack); scratch = true; }
        }
        try { start_thread(); } catch (...) { GuestScratch::leave(worker); throw; }
    }
    void start_thread() {
        pthread_attr_t attributes;
        if (const auto error = pthread_attr_init(&attributes)) throw std::system_error(error, std::generic_category(), "Guest worker attributes");
        // As the game thread: the nested native call chain needs far more than a secondary thread's default stack.
        const auto stack = std::max<std::size_t>(8 * 1024 * 1024, pthread_get_stacksize_np(pthread_self()));
        auto error = pthread_attr_setstacksize(&attributes, stack);
        if (!error) error = pthread_create(&thread, &attributes, [](void* self) -> void* {
            pthread_setname_np("CM match worker");
            static_cast<Impl*>(self)->loop();
            return nullptr;
        }, this);
        pthread_attr_destroy(&attributes);
        if (error) throw std::system_error(error, std::generic_category(), "Guest worker thread");
    }
    ~Impl() {
        { std::lock_guard lock(mutex); job = Job::quit; }
        wake.notify_all();
        pthread_join(thread, nullptr);
        GuestScratch::leave(worker);
    }
    void loop() {
        set_hardware_fault_view(&worker.memory, &worker);
        std::unique_lock lock(mutex);
        for (;;) {
            wake.wait(lock, [&] { return job == Job::run || job == Job::quit; });
            if (job == Job::quit) return;
            try { run_until(worker, return_pc); } catch (...) { failure = std::current_exception(); }
            job = Job::done;
            wake.notify_all();
        }
    }
    void call(std::uint32_t until) {
        std::lock_guard one_call(call_mutex);
        // The original allocator's routines take the game's pool as given: with them selected the scratch heap is bypassed.
        if (scratch && (main.allocator_original || std::ranges::contains(main.allocator_entry_original, true)))
            throw std::logic_error("A guest worker with a scratch heap needs the rewritten allocator");
        // Each call: what main selects now. Observers are shared, which is sound only one call at a time.
        copy_configuration(worker, main);
        const auto mapped = main.memory.mapped_ranges();
        worker.memory.share(main.memory);
        // One call at a time, the caller waiting: main's observers see the worker's accesses as their own.
        worker.memory.observer = main.memory.observer;
        worker.memory.date_observer = main.memory.date_observer;
        worker.memory.read_trace = main.memory.read_trace;
        worker.registers = main.registers;
        worker.pc = main.pc;
        worker.executed = main.executed; worker.budget = main.budget; worker.native_calls = main.native_calls;
        // On the game's heap the worker carries main's block records and may not change them; with a scratch
        // heap it has its own.
        if (!scratch) { worker.allocated = main.allocated; worker.available = main.available; worker.heap_end = main.heap_end; }
        worker.memory_error = main.memory_error;
        worker.platform = main.platform;
        const auto main_sp = std::uint32_t(main.r<4>(4));
        if (own_sp) {
            main.memory.copy_in(own_sp, main.memory.copy_out(main_sp, caller_frame));
            worker.wr<4>(4, own_sp);
        }
        {
            std::unique_lock lock(mutex);
            return_pc = until; failure = nullptr; job = Job::run;
            wake.notify_all();
            wake.wait(lock, [&] { return job == Job::done; });
            job = Job::none;
        }
        ++calls;
        // As in line: whatever the routine did, main stands where it stopped, also when it threw.
        main.registers = worker.registers;
        if (own_sp) main.wr<4>(4, main_sp);
        main.pc = worker.pc;
        main.executed = worker.executed; main.native_calls = worker.native_calls; main.memory_error = worker.memory_error;
        if (own_sp) main.memory.copy_in(main_sp, main.memory.copy_out(own_sp, caller_frame));
        if (failure) std::rethrow_exception(failure);
        if (own_sp && worker.r<4>(4) != own_sp) throw std::runtime_error("A call on the guest worker returned with another stack pointer");
        if (!scratch && (worker.allocated != main.allocated || worker.available != main.available || worker.heap_end != main.heap_end))
            throw std::runtime_error("A call on the guest worker changed the host's allocation records");
        // The bounds tables are shared, the list of ranges is not: a mapping made on the worker would leave main's stale.
        if (worker.memory.mapped_ranges() != mapped) throw std::runtime_error("A call on the guest worker mapped guest memory");
    }
};

GuestWorker::GuestWorker(State& main, std::optional<unsigned> own, bool scratch) : impl(std::make_unique<Impl>(main, own, scratch)) {}
GuestWorker::~GuestWorker() = default;
void GuestWorker::call(std::uint32_t return_pc) { impl->call(return_pc); }
std::uint64_t GuestWorker::calls() const { return impl->calls; }
std::uint32_t GuestWorker::scratch_in_use() const { return impl->scratch ? GuestScratch::in_use(impl->worker) : 0; }
}
