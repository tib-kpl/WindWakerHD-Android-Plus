package org.wwhdrecomp.app;

/** Route + to Items after the Select shortcut has chosen Map, without also toggling pause. */
final class GameMenuInput {
    // Published by map_shortcut::context() on the game thread. Title, events and dialogue use NONE.
    static final int NONE = 0, PLAY = 1, OPENING = 2, MAP = 3, OTHER_MENU = 4;
    private final int plus;
    private boolean plusDown, suppressPlus;

    GameMenuInput(int plus) { this.plus = plus; }

    int filter(int buttons, boolean enabled, int context, Runnable openItems) {
        boolean down = (buttons & plus) != 0;
        if (down && !plusDown) {
            suppressPlus = enabled && (context == PLAY || context == OPENING || context == MAP);
            if (suppressPlus) openItems.run();
        }
        plusDown = down;
        if (!down) suppressPlus = false;
        return suppressPlus ? buttons & ~plus : buttons;
    }

    void reset() { plusDown = suppressPlus = false; }
}
