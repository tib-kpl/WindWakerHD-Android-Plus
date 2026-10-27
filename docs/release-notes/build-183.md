Fix HDMI display ownership in TV-only mode.

- TV-only layout and Pro Controller no longer open a GamePad window on a secondary display, which could replace Android's mirrored game image with a blank GamePad image.
- Apply this rule when the layout or controller mode changes, and recheck display availability when Android reports a display change.
- Never put the GamePad window over the display already hosting the main activity.
- Clarify the second-display setting in French, English, Spanish and Brazilian Portuguese; document the fix in the fork README.

Java sources and Android resources compile successfully against API 36. A physical HDMI/TV test remains to be done.

The APK contains no game code or data.
