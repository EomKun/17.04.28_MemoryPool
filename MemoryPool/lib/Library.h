#ifndef __LIBRARY__H__
#define __LIBRARY__H__

#include <Windows.h>
#include <tchar.h>
#include <locale.h>
#include <process.h>
#include <Strsafe.h>
#include <time.h>
#include <psapi.h>
#include <dbghelp.h>
#include <crtdbg.h>
#include <StrSafe.h>
#include <tlhelp32.h>
#include <strsafe.h>

#pragma comment(lib, "DbgHelp.Lib")
#pragma comment(lib, "ImageHlp")
#pragma comment(lib, "psapi")

#define chINRANGE(low, Num, High) (((low) <= (Num)) && ((Num) <= (High)))

#include "APIHook.h"
#include "CrashDump.h"
#include "SystemLog.h"
#include "Profiler.h"

using namespace Eom;

#endif