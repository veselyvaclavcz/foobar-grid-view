// stdafx.h : include file for standard system include files
#pragma once

#define FOOBAR2000_TARGET_VERSION 80
#define _WIN32_WINNT 0x0600

// Windows headers
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX  // Prevent Windows min/max macros
#include <windows.h>
#include <objbase.h>
#include <shobjidl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <windowsx.h>
#include <uxtheme.h>
#include <gdiplus.h>
#include <shellapi.h>  // For ShellExecute

// Windows multimedia for timeGetTime
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// foobar2000 SDK
#include "SDK-2025-03-07/foobar2000/SDK/foobar2000.h"
#include "SDK-2025-03-07/foobar2000/SDK/coreDarkMode.h"
#include "SDK-2025-03-07/foobar2000/helpers/DarkMode.h"

// Use std::min and std::max
#include <algorithm>
using std::min;
using std::max;