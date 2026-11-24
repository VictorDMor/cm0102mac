#include "guest_worker.hpp"
#include "aot_runtime.hpp"
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

    explicit Impl(State& state) : main(state) {
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
    }
    void loop() {
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
        // A stray access under hardware bounds stops the process on the faulting thread with the game thread's
        // reporter; until the worker has its own, it runs only where bounds are exact (checkers and replays).
        if (!main.memory.exact_bounds()) throw std::logic_error("The guest worker does not run under hardware bounds yet");
        const auto mapped = main.memory.mapped_ranges();
        worker.memory.share(main.memory);
        // One call at a time, the caller waiting: main's observers see the worker's accesses as their own.
        worker.memory.observer = main.memory.observer;
        worker.memory.date_observer = main.memory.date_observer;
        worker.memory.read_trace = main.memory.read_trace;
        worker.registers = main.registers;
        worker.pc = main.pc;
        worker.executed = main.executed; worker.budget = main.budget; worker.native_calls = main.native_calls;
        worker.allocated = main.allocated; worker.available = main.available; worker.heap_end = main.heap_end;
        worker.memory_error = main.memory_error;
        worker.platform = main.platform;
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
        main.pc = worker.pc;
        main.executed = worker.executed; main.native_calls = worker.native_calls; main.memory_error = worker.memory_error;
        if (failure) std::rethrow_exception(failure);
        if (worker.allocated != main.allocated || worker.available != main.available || worker.heap_end != main.heap_end)
            throw std::runtime_error("A call on the guest worker changed the host's allocation records");
        // The bounds tables are shared, the list of ranges is not: a mapping made on the worker would leave main's stale.
        if (worker.memory.mapped_ranges() != mapped) throw std::runtime_error("A call on the guest worker mapped guest memory");
    }
};

GuestWorker::GuestWorker(State& main) : impl(std::make_unique<Impl>(main)) {}
GuestWorker::~GuestWorker() = default;
void GuestWorker::call(std::uint32_t return_pc) { impl->call(return_pc); }
std::uint64_t GuestWorker::calls() const { return impl->calls; }
}
