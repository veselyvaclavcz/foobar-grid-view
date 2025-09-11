# Console Spam Issue - Excessive Logging When Artwork Missing

## Problem Description
foo_mpv generates excessive console output when artwork requests fail or artwork is missing, causing console spam especially with components that make frequent artwork requests (like Album Art Grid).

User report: *"Still not fixed. As long as there is one track which has no art to show, you will see this."*

## Root Cause Analysis
After analyzing the source code, I identified specific logging statements that generate spam:

**In `artwork_protocol.cpp`:**
```cpp
FB2K_console_formatter() << "mpv: Artwork request " << newid;          // Line ~90
FB2K_console_formatter() << "mpv: Can't open artwork - missing";       // Line ~150
FB2K_console_formatter() << "mpv: Stale artwork reference " << action;  // Line ~180
```

**In `mpv_player.cpp`:**
```cpp
FB2K_console_formatter() << "mpv: Error loading artwork";  // Line ~320
FB2K_console_formatter() << "mpv: Ignoring loaded artwork"; // Line ~340
```

## Proposed Solution
Make all artwork-related logging conditional based on the logging preference:

```cpp
// BEFORE (causes spam):
FB2K_console_formatter() << "mpv: Artwork request " << newid;

// AFTER (conditional logging):
if (cfg_logging) {
    FB2K_console_formatter() << "mpv: Artwork request " << newid;
}
```

## Impact
- **High**: Users with components making frequent artwork requests see constant console spam
- **Workaround**: Disable foo_mpv logging in preferences, but loses all diagnostic info

## Test Case
1. Install Album Art Grid component or similar artwork-heavy component
2. Load playlist with tracks missing artwork
3. Observe excessive console output from foo_mpv
4. Apply fix and verify spam eliminated while preserving diagnostic capability

## Files to Modify
- `src/artwork_protocol.cpp` - wrap lines ~90, ~150, ~180 in `if (cfg_logging)`
- `src/mpv_player.cpp` - wrap lines ~320, ~340 in `if (cfg_logging)`

This change would respect user's logging preference and eliminate unwanted spam while preserving diagnostic capability when logging is explicitly enabled.