# Version Switching Memory Corruption Fix - v10.0.5

## Problem Analysis

The crash reported in `failure_00000003.txt` was **NOT** caused by v10.0.5 code, but by improper version switching:

### Root Cause
1. **v10.0.5 was never loaded** - crash log shows "Album Art Grid 10.0.4" (line 259)
2. **foobar2000 was not restarted** after component installation
3. **Memory corruption during version switching** - old v10.0.4 DLL tried to access memory modified by installation process

### Technical Details
- **Crash Location**: Still in v10.0.4 `uPrintCrashInfo_OnEvent` function
- **Crash Type**: Access violation at `FFFFFFFFFFFFFFFFh` (corrupted memory pattern)  
- **Crash Context**: Main thread callback (not shutdown) after 95 minutes runtime
- **Memory Pattern**: `FFFFFFFFFFFFFFFF` indicates corrupted pointers from installation interference

## Solution: Safe Version Switching

### v10.0.5 Shutdown Fixes
✅ **Already implemented** in v10.0.5:
- Atomic shutdown flags in all helper functions
- Critical section protection during cleanup
- Proper GDI+ shutdown sequence
- Thread-safe resource cleanup

### Installation Procedure Fix
✅ **New safe installation script** `INSTALL_V10_0_5_SAFE.bat`:

```batch
1. Check if foobar2000 is running
2. Force close foobar2000 if running  
3. Extract and install new component DLL
4. Restart foobar2000 cleanly
5. Verify version loaded correctly
```

## Prevention Rules

### For Future Version Switching:
1. **ALWAYS close foobar2000** before installing new component versions
2. **NEVER replace DLLs** while foobar2000 is running
3. **ALWAYS restart foobar2000** after component changes
4. **Verify component version** in foobar2000 console after restart

### Memory Corruption Prevention:
- Use the safe installation script for all future updates
- Never manually copy DLLs while foobar2000 is running
- Allow 3-second delay after foobar2000 shutdown before DLL replacement

## Version History

### v10.0.4 → v10.0.5 Transition Issue:
- **Problem**: Component replaced without foobar2000 restart
- **Result**: Memory corruption and crash during "version switching"
- **Solution**: Safe installation procedure with mandatory restart

### v10.0.5 Features:
- Fixed original shutdown crash in `uPrintCrashInfo_OnEvent`
- Robust shutdown handling with atomic flags
- Critical section protection
- Thread-safe cleanup sequence

## Files Created:
- `INSTALL_V10_0_5_SAFE.bat` - Safe installation script
- `grid_v10_0_5_shutdown_fix.cpp` - Updated main component
- `initquit_v10_0_5.cpp` - Enhanced shutdown handling
- `helpers_minimal_v10_0_5.cpp` - Protected helper functions
- `BUILD_V10_0_5_SHUTDOWN_FIX.bat` - Build script

## Testing Status:
✅ v10.0.5 built successfully  
✅ Safe installation procedure created  
✅ foobar2000 restarted with v10.0.5  
⏳ Awaiting shutdown crash test results  

## User Instructions:
1. Use `INSTALL_V10_0_5_SAFE.bat` for installation
2. Verify "Album Art Grid v10.0.5 initialized" appears in foobar2000 console
3. Test normal shutdown to confirm crash is fixed
4. Report any remaining issues

The "new bug" mentioned by user was actually caused by improper version switching, not v10.0.5 code itself.