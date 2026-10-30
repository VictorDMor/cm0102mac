#include "quick_save.hpp"
#include <stdexcept>
#include <string>

using cm::aot::QuickSaveDestination;

static void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

int main() {
    QuickSaveDestination destination;
    require(!destination.last(), "An unsaved career must not have a destination");
    destination.begin_load("First:Career.sav");
    destination.finish_load(true);
    require(destination.last() == "First:Career.sav", "A successful load establishes its complete path");
    destination.begin_save("Second:Attempt.sav");
    destination.finish_save(false);
    require(destination.last() == "First:Career.sav", "A failed Save As must not replace the last successful path");
    destination.begin_save("Second:Career.sav");
    destination.finish_save(true);
    require(destination.last() == "Second:Career.sav", "A successful Save As replaces the complete path");
    destination.begin_load("Third:Broken.sav");
    destination.finish_load(false);
    require(destination.last() == "Second:Career.sav", "A failed load must preserve the active career's destination");
    destination.begin_load(std::nullopt);
    destination.finish_load(true);
    require(!destination.last(), "An unreadable successful load must not reuse another career's path");
}
