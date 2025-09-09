# foo_mpv Console Spam Analysis - Response to marc2k3

## Thank you marc2k3 for the clarification!

You're absolutely right about foo_mpv's normal behavior:
- ✅ foo_mpv registers as album art fallback using proper SDK APIs
- ✅ Missing artwork should generate console messages (that's expected)
- ✅ Your grid shows 3 missing items = 3 messages (normal behavior)

## However, there's a significant difference in our case

**marc2k3's experience:** 3 missing items = 3 console messages ✅  
**Our user's screenshot:** Dozens of identical `mpv: libav error for open input: Invalid argument` ❌

## The Issue We're Seeing

The problem isn't the normal "artwork not found" messages, but rather:

1. **Repetitive identical errors** - same libav error repeated dozens of times
2. **"libav error for open input: Invalid argument"** - this suggests mpv engine issues, not normal artwork API behavior
3. **Scale difference** - not 1:1 correspondence between missing items and error messages

## Potential Causes for the Discrepancy

1. **Different foo_mpv versions** - behavior may have changed between versions
2. **Configuration differences** - mpv.conf settings or foo_mpv preferences
3. **Request timing/frequency** - our grid component might be making requests differently
4. **Interaction with other components** - something else in the chain causing issues

## Questions to Help Diagnose

1. **What foo_mpv version** are you using vs our user?
2. **Are you seeing specifically "libav error for open input: Invalid argument"** or different messages?
3. **Any special mpv.conf configuration** that might affect this?

## Our Component's Behavior

Our Album Art Grid v10.0.20 implements:
- ✅ **Persistent blacklist** - items without artwork are never re-requested
- ✅ **Zero-retry policy** - each item gets exactly one artwork request
- ✅ **Proper cleanup** - no memory leaks or hanging requests

The component should behave exactly like yours - 1 missing item = 1 console message, no retries.

## Working Theory

The excessive spam might be caused by:
1. **Lower-level mpv engine errors** (libav trying to parse artwork:// URLs)
2. **Specific foo_mpv version bug** that doesn't affect your setup
3. **Edge case in artwork protocol handling** under certain conditions

**We agree the normal foo_mpv behavior should be minimal console output. The question is why our user sees different behavior than yours.**

---

*Would appreciate any insights on version differences or configuration that might explain the discrepancy!*