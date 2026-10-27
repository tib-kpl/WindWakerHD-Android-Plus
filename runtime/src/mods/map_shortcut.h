#pragma once
#include "ppc.h"

// Select opens the game's menu on its MAP tab (map_shortcut.cpp)
namespace map_shortcut {
void request();          // any thread: Select was pressed (MainActivity, "open_map")
void request_items();    // any thread: + opens Items / returns there from MAP
// Published on the game thread: none=0, play=1, opening=2, MAP=3, another open menu=4.
// The host must leave + alone on the title, during events/dialogue and when closing Items.
int context();
void service(Cpu* c);    // game main thread, frame start (interp.cpp)
}  // namespace map_shortcut
