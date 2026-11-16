#pragma once
namespace cm::aot {
int run_probe(int argc, char** argv);
// Shown after a hardware bounds fault has been reported, before the process
// exits (the app's "stopped" alert); none by default.
using RunStopPresenter = void (*)();
void set_run_stop_presenter(RunStopPresenter presenter);
}
