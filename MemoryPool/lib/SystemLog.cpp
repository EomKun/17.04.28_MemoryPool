#include "Library.h"

__int64 CSystemLog::_iLogCnt = 0;
char CSystemLog::_byLogMode = LOG::CONSOLE;
WCHAR CSystemLog::_wLogBuffer[256] = { 0, };
char CSystemLog::_byLogLevel = LOG::LEVEL_DEBUG;