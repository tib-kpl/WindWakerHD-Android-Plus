#include "runtime.h"
#include "release.h"
#include "mods/map_shortcut.h"
#include <sys/mman.h>
#include <cassert>
#include <cstdio>

static constexpr uint32_t UI = 0x14500000, CURRENT = 0x14501000;
static constexpr uint32_t TO_PLAY = 0x1049EBD4;
static constexpr uint32_t PLAY = 0x1049EC00, OPENING = 0x1049EC84, MENU = 0x1049ECB0;
static uint32_t state = PLAY;
static int changes, checks;
namespace release {
uint32_t code(uint32_t p) { return p; }
uint32_t data(uint32_t p) { return p; }
}
void log_msg(const char*, ...) {}
uint32_t guest_call(Cpu* c, uint32_t fn, std::initializer_list<uint32_t> args) {
    c->r[3] = 0x12345678; c->lr = 0x87654321;
    if (fn == 0x02006478) return CURRENT;
    if (fn == 0x020063C0) { state = *(args.begin() + 1); ++changes; return 0; }
    assert(fn == 0x02000040);
    uint32_t id = *args.begin();
    return id == CURRENT ? state : id;
}
static void check(bool ok, const char* msg) {
    ++checks;
    if (!ok) { fprintf(stderr, "FAIL: %s\n", msg); abort(); }
}
static void tick(Cpu& cpu) {
    Cpu before = cpu;
    map_shortcut::service(&cpu);
    check(memcmp(&cpu, &before, sizeof(Cpu)) == 0, "guest CPU registers preserved");
}
int main() {
    for (auto [base, size] : {std::pair<uint32_t, uint32_t>{0x101F0000, 0x10000}, {0x10490000, 0x20000}, {UI, 0x20000}}) {
        void* p = mmap(ppc_ptr(base), size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
        assert(p == ppc_ptr(base));
    }
    st32(0x101F8344, UI);
    for (uint32_t id : {PLAY, TO_PLAY, OPENING, MENU, CURRENT}) st32(id + 8, 0x104A0000);
    st32(0x104A0014, 0x02000040);
    Cpu cpu{}; cpu.r[3] = 13; cpu.lr = 42;
    tick(cpu);
    check(map_shortcut::context() == 1, "play context is published");
    map_shortcut::request(); tick(cpu);
    check(state == OPENING && changes == 1 && ld32(UI + 0x228) == 1, "Select opens MAP");
    tick(cpu); check(map_shortcut::context() == 2, "opening context");
    state = MENU; st32(UI + 0x228, 0); tick(cpu);
    check(ld32(UI + 0x228) == 1 && map_shortcut::context() == 3, "MAP survives the opening animation reset");
    map_shortcut::request_items(); tick(cpu);
    check(state == MENU && changes == 1 && ld32(UI + 0x228) == 0, "Start from MAP selects Items without closing");
    check(map_shortcut::context() == 4, "Items context allows the next Start to close normally");
    map_shortcut::request(); tick(cpu);
    check(ld32(UI + 0x228) == 1, "Select from Items returns to MAP");
    map_shortcut::request_items(); tick(cpu);
    check(ld32(UI + 0x228) == 0, "Start returns to Items repeatedly");
    state = PLAY; tick(cpu); map_shortcut::request_items(); tick(cpu);
    check(state == OPENING && ld32(UI + 0x228) == 0, "Start from play opens Items explicitly");
    map_shortcut::request(); tick(cpu);
    check(ld32(UI + 0x228) == 1, "Select during opening changes destination to MAP");
    map_shortcut::request_items(); tick(cpu);
    state = MENU; st32(UI + 0x228, 1); tick(cpu);
    check(ld32(UI + 0x228) == 0 && map_shortcut::context() == 4, "Start during MAP opening chooses Items");
    state = 42; tick(cpu); int previous = changes;
    map_shortcut::request_items(); tick(cpu);
    check(changes == previous && map_shortcut::context() == 0, "dialogue/events cannot force open a menu");
    st32(0x101F8344, 0); map_shortcut::request(); tick(cpu);
    check(map_shortcut::context() == 0 && changes == previous, "title/no game safely rejects a request");
    st32(0x101F8344, UI); state = MENU; st32(UI + 0x228, 1); tick(cpu);
    previous = changes; map_shortcut::request(); tick(cpu);
    check(state == TO_PLAY && changes == previous + 1, "second Select closes MAP via normal transition");
    check(ld32(UI + 0x228) == 0 && map_shortcut::context() == 0, "closing MAP clears remembered destination");
    state = PLAY; tick(cpu); map_shortcut::request_items(); tick(cpu);
    check(state == OPENING && ld32(UI + 0x228) == 0, "Start after closing MAP opens Items");
    printf("native_menu_check: %d checks passed\n", checks);
}
