#ifndef __SYSTEMLOG__H__
#define __SYSTEMLOG__H__

#include <time.h>

namespace Eom{
	namespace LOG{
		enum{
			FILE = 0x1,
			CONSOLE = 0x2,
			LEVEL_SYSTEM = 10,
			LEVEL_ERROR = 20,
			LEVEL_WARNING = 30,
			LEVEL_DEBUG = 40,
		};
	}

	using namespace LOG;

	class CSystemLog
	{
		CSystemLog()
		{
			_iLogCnt = 0;
			_byLogMode = LOG::CONSOLE;

			_byLogLevel = LOG::LEVEL_DEBUG;
			_wcLogDir = "/";

			_wLogBuffer[256] = { 0, };
		}
		/*
	private:
	static CSystemLog *_pSystemLog;

	CSystemLog()
	{
	_iLogCnt = 0;
	_wcLogDir = "/";
	_byLogMode = LOG::FILE;

	}
	virtual ~CSystemLog(){}
	*/
	public:
		/*
		static CSystemLog *GetInstance()
		{
		if (NULL == _pSystemLog)
		{
		_pSystemLog = new CSystemLog();
		atexit(Destroy);
		}

		return _pSystemLog;
		}

		static void Destroy()
		{
		delete _pSystemLog;
		}
		*/

		static bool SetLog(char byLogMode, BYTE byLogLevel){
			_byLogMode = byLogMode;
			_byLogLevel = byLogLevel;
			return true;
		}

		static bool SetLogDirectory(char *wLogDirectory){ _wcLogDir = wLogDirectory; }

		//---------------------------------------------------------------------------------
		// 로그 찍는 함수
		//---------------------------------------------------------------------------------
		static bool Log(WCHAR *wCategory, WCHAR chLogLevel, LPCTSTR szStringFormat, ...)
		{
			WCHAR *wLogLevel = NULL;
			WCHAR szInMessage[1024];
			DWORD dwBytesWritten;

			///////////////////////////////////////////////////////////////////////////////
			// 현재 시간 설정
			///////////////////////////////////////////////////////////////////////////////
			time_t timer;
			tm today;

			time(&timer);

			localtime_s(&today, &timer); // 초 단위의 시간을 분리하여 구조체에 넣기

			///////////////////////////////////////////////////////////////////////////////
			// 가변인자
			///////////////////////////////////////////////////////////////////////////////
			va_list va;
			va_start(va, szStringFormat);
			StringCchVPrintf(szInMessage, 256, szStringFormat, va);
			va_end(va);

			InterlockedIncrement64((LONG64 *)&_iLogCnt);

			if (_byLogLevel >= chLogLevel)
			{
				wsprintf(_wLogBuffer, L"[%s] [%04d-%02d-%02d %02d:%02d:%02d / %s] [%08I64d] %s \r\n",
					wCategory,
					today.tm_year + 1900,
					today.tm_mon + 1,
					today.tm_mday,
					today.tm_hour,
					today.tm_min,
					today.tm_sec,
					wLogLevel,
					_iLogCnt,
					szInMessage);

				////////////////////////////////////////////////////////////////
				// CONSOLE
				////////////////////////////////////////////////////////////////
				if (_byLogMode & 0x1)
				{
					wprintf(L"%s", _wLogBuffer);
				}

				////////////////////////////////////////////////////////////////
				// FILE
				////////////////////////////////////////////////////////////////
				if (_byLogMode & 0x2)
				{
					unsigned short mark = 0xFEFF;
					WCHAR fileName[256];
					wsprintf(fileName, L"%d%02d_%s.txt", today.tm_year + 1900, today.tm_mon + 1, wCategory);

					HANDLE hFile = ::CreateFile(fileName,
						GENERIC_WRITE,
						NULL,
						NULL,
						OPEN_ALWAYS,
						FILE_ATTRIBUTE_NORMAL,
						NULL);
					SetFilePointer(hFile, 0, NULL, FILE_END);

					::WriteFile(hFile, &mark, sizeof(mark), &dwBytesWritten, NULL);
					if (!(::WriteFile(hFile, _wLogBuffer, (DWORD)(wcslen(_wLogBuffer) * sizeof(WCHAR)), &dwBytesWritten, NULL)))
						return false;
					CloseHandle(hFile);

				}
			}
			return true;
		}

		/*
		void PrintToHex();
		void PrintToSessionKey64();
		*/

		static __int64 _iLogCnt;

		static char _byLogMode;

		static char _byLogLevel;
		static char* _wcLogDir;

		static WCHAR _wLogBuffer[256];
	};

#define LOG(chCategory, chLogLevel, szStringFormat, ...)	CSystemLog::Log(chCategory, chLogLevel, szStringFormat, __VA_ARGS__)
#define LOG_SET(byLogMode, byLogLevel)					CSystemLog::SetLog(byLogMode, byLogLevel)
}
#endif