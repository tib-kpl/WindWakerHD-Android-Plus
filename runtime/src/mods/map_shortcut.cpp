// Select opens the map. The game's − (Off-TV play on the Wii U) does nothing in this port; a short
// press of a controller's Select asks for the menu on its MAP tab instead (MainActivity, Controls >
// "Select opens the map").
//
// The in-game UI (hdIngameUi_c, sIngameUi) runs a state machine at +0x10 (hdStateMachine); the pause
// menu opens through StateID_ToMenu, as the player's + does (hdIngameChangeState), and the menu then
// shows the tab of mTransitionTab (+0x228) while no tab button is pressed (changeActiveScreen): 1 is
// the MAP tab, 0 is Items. + explicitly returns to Items instead of keeping MAP selected.
// Addresses from ZeldaWWHDRecomp (controller_reapply.cpp, mods/story_chart.cpp).
// From play it opens the menu there; with the menu already open it goes to the MAP tab. Elsewhere
// (an event, a dialogue, the title) the press is ignored: the game couldn't open its menu either.
#include "map_shortcut.h"

#include <atomic>

#include "../release.h"
#include "../runtime.h"

namespace map_shortcut {
namespace {
const release::Data kIngameUi{0x101F8344};     // sIngameUi (hdIngameUi_c*, 0 outside gameplay)
const release::Data kPlayState{0x1049EC00};    // l_ingamePlayState
const release::Data kToPlayState{0x1049EBD4};  // normal menu closing transition
const release::Data kToMenuState{0x1049EC84};  // l_ingameToMenuState (the menu opening)
const release::Data kMenuState{0x1049ECB0};    // l_ingameMenuState
const release::Code kCurrentState{0x02006478}; // the state machine's current state id
const release::Code kChangeState{0x020063C0};  // hdIngameChangeState(machine, id)
constexpr uint32_t kUiState = 0x10, kUiTransitionTab = 0x228, kTabItems = 0, kTabMap = 1;
constexpr int kOpenFrames = 120;  // the menu is open within this many frames

std::atomic<int> g_request{-1};  // no request, Items (0), or Map (1)
std::atomic<int> g_context{0};
int g_opening = 0;  // frames left for the menu that was asked for to open
uint32_t g_opening_tab = kTabItems;

bool object(uint32_t p) { return p >= mem::kMem2Start && p < mem::kMem2End - 0x400; }

uint32_t state_key(Cpu* c, uint32_t id) {
    uint32_t vt = ld32(id + 8);
    return object(vt) ? guest_call(c, ld32(vt + 0x14), {id}) : 0;
}

// the in-game UI's current StateID key (0: none)
uint32_t ui_state(Cpu* c, uint32_t ui) {
    uint32_t current = guest_call(c, kCurrentState, {ui + kUiState});
    return object(current) ? state_key(c, current) : 0;
}

void step(Cpu* c, uint32_t ui) {
    const uint32_t state = ui_state(c, ui);
    const bool play = state != 0 && state == state_key(c, kPlayState);
    const bool menu = state != 0 && state == state_key(c, kMenuState);
    const bool opening = state != 0 && state == state_key(c, kToMenuState);
    if (g_opening > 0) {
        // The opening animation may reset the tab. Reapply the requested destination once open.
        if (menu) {
            st32(ui + kUiTransitionTab, g_opening_tab);
            g_opening = 0;
        } else if (--g_opening == 0) {
            LOG("[map] the menu did not open");
        }
    }
    g_context.store(play ? 1 : opening ? 2 : menu ? (ld32(ui + kUiTransitionTab) == kTabMap ? 3 : 4) : 0,
                    std::memory_order_relaxed);
    const int requested = g_request.exchange(-1);
    if (requested < 0) return;
    const uint32_t tab = static_cast<uint32_t>(requested);
    if (play) {
        guest_call(c, kChangeState, {ui + kUiState, kToMenuState});
        st32(ui + kUiTransitionTab, tab);
        g_opening_tab = tab;
        g_opening = kOpenFrames;
        LOG("[map] menu opens on %s", tab == kTabMap ? "MAP" : "Items");
    } else if (menu && tab == kTabMap && ld32(ui + kUiTransitionTab) == kTabMap) {
        // Select is a toggle. Close through the game's transition, and forget the forced MAP
        // destination so a subsequent native + cannot reopen the stale MAP tab either.
        guest_call(c, kChangeState, {ui + kUiState, kToPlayState});
        st32(ui + kUiTransitionTab, kTabItems);
        g_opening = 0;
        g_context.store(0, std::memory_order_relaxed);
        LOG("[map] Select closes MAP");
    } else if (menu || opening) {
        st32(ui + kUiTransitionTab, tab);
        if (opening) { g_opening_tab = tab; g_opening = kOpenFrames; }
        // Publish the change now: a second + in Items should close the menu normally.
        if (menu) g_context.store(tab == kTabMap ? 3 : 4, std::memory_order_relaxed);
        LOG("[map] menu switches to %s", tab == kTabMap ? "MAP" : "Items");
    }
}
}  // namespace

void request() { g_request = kTabMap; }
void request_items() { g_request = kTabItems; }
int context() { return g_context.load(std::memory_order_relaxed); }

void service(Cpu* c) {
    const uint32_t ui = ld32(kIngameUi);
    if (!object(ui)) {
        g_request = -1;
        g_context = 0;
        g_opening = 0;
        return;
    }
    // the guest calls leave the registers as the game left them for this frame
    Cpu saved = *c;
    step(c, ui);
    uint32_t core = c->core;
    void* th = c->thread;
    *c = saved;
    c->core = core;
    c->thread = th;
}
}  // namespace map_shortcut
