// SDK fixes for compilation
#pragma once

// Fix missing timeGetTime
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// Define interface keyword for COM compatibility
#define interface struct

// Ensure COM types are defined
#include <unknwn.h>
#include <objidl.h>