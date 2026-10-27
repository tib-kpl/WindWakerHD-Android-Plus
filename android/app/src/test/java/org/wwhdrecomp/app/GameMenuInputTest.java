package org.wwhdrecomp.app;

/** Standalone JVM regression checks; no Android runtime or game files required. */
public final class GameMenuInputTest {
    private static int checks;
    private static void check(boolean ok, String message) {
        checks++;
        if (!ok) throw new AssertionError(message);
    }
    public static void main(String[] args) {
        GameMenuInput input = new GameMenuInput(0x0008);
        int[] requests = {0};
        Runnable open = () -> requests[0]++;
        check(input.filter(0x0008, true, GameMenuInput.MAP, open) == 0, "MAP: + selects Items instead of toggling pause");
        check(requests[0] == 1, "MAP: Items requested once");
        check(input.filter(0x0008, true, GameMenuInput.OTHER_MENU, open) == 0, "held + stays suppressed after Items appears");
        check(requests[0] == 1, "held + cannot request repeatedly");
        input.filter(0, true, GameMenuInput.OTHER_MENU, open);
        check(input.filter(0x0008, true, GameMenuInput.OTHER_MENU, open) == 0x0008, "fresh + in Items closes normally");
        input.filter(0, true, GameMenuInput.PLAY, open);
        check(input.filter(0x0008 | 0x8000, true, GameMenuInput.PLAY, open) == 0x8000, "from play: explicitly open Items and preserve other buttons");
        check(requests[0] == 2, "from play: Items requested");
        input.reset();
        check(input.filter(0x0008, true, GameMenuInput.OPENING, open) == 0, "+ during MAP opening changes destination to Items");
        check(requests[0] == 3, "opening: Items requested");
        input.reset();
        check(input.filter(0x0008, true, GameMenuInput.NONE, open) == 0x0008, "title/cutscene/dialogue: + remains native");
        check(requests[0] == 3, "title/cutscene/dialogue: no forced menu");
        input.reset();
        check(input.filter(0x0008, false, GameMenuInput.MAP, open) == 0x0008, "shortcut disabled: native + retained");
        check(requests[0] == 3, "shortcut disabled: no request");
        input.reset();
        check(input.filter(0x0004, true, GameMenuInput.PLAY, open) == 0x0004, "minus and custom mappings unchanged");
        input.filter(0x0008, true, GameMenuInput.MAP, open);
        input.reset();
        check(input.filter(0x8000, true, GameMenuInput.NONE, open) == 0x8000, "focus loss clears suppression without losing other buttons");
        System.out.println("GameMenuInputTest: " + checks + " checks passed");
    }
}
