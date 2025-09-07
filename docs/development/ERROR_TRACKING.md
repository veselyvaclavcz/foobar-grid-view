# Error Tracking Database

## Purpose
Track all errors, crashes, and issues encountered during development and testing.

## Error Entry Template

```markdown
### ERROR-[ID]: [Short Description]
**Date Found**: [Date]  
**Severity**: [Critical/High/Medium/Low]  
**Status**: [Open/In Progress/Fixed/Won't Fix]  
**Module**: [Affected module]  
**Version**: [Version where found]  

#### Description
[Detailed description of the error]

#### Reproduction Steps
1. [Step 1]
2. [Step 2]
3. [Expected result]
4. [Actual result]

#### Stack Trace / Error Message
```
[Error output]
```

#### Root Cause
[Analysis of why it happens]

#### Fix Applied
[Description of fix]
[Link to commit/code]

#### Verification
- [ ] Fix tested
- [ ] Regression test added
- [ ] Documentation updated
```

---

## Known Issues from Previous Versions

### ERROR-001: Shutdown Crash at 0x1208Ch
**Date Found**: Multiple occurrences in v10.x  
**Severity**: Critical  
**Status**: To Be Fixed in v11  
**Module**: LifecycleManager / CallbackManager  
**Version**: v10.0.17-v10.0.20  

#### Description
Component crashes when foobar2000 shuts down, particularly when UI element is active.

#### Root Cause
- Double-delete from service_impl_t inheritance
- Callbacks firing on destroyed objects
- GDI+ shutdown timing issues

#### Planned Fix for v11
- Implement weak pointer pattern
- Proper shutdown sequence
- Object validation before all operations

---

### ERROR-002: Memory Leak with Large Libraries
**Date Found**: v10.0.x  
**Severity**: High  
**Status**: To Be Fixed in v11  
**Module**: ThumbnailCache  

#### Description
Memory usage grows unbounded with large libraries (10,000+ albums).

#### Planned Fix for v11
- Implement LRU cache with strict limits
- Auto-detection based on available RAM
- Configurable cache sizes

---