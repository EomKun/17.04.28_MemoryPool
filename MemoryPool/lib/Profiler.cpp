#include <Windows.h>
#include <float.h>
#include <stdio.h>

#include "Profiler.h"

st_SAMPLE_THREAD SAMPLE_TH[SAMPLE_MAX_THREAD];
LARGE_INTEGER		g_IFrequency;
double			g_fMicroFrequency;

SRWLOCK srwSampleThreadLock;

void ProfileInit()
{
	QueryPerformanceFrequency(&g_IFrequency);
	InitializeSRWLock(&srwSampleThreadLock);

	for (int iCnt = 0; iCnt < SAMPLE_MAX_THREAD; iCnt++)
	{
		SAMPLE_TH[iCnt].iThreadID = -1;
	}
}

bool ProfileBegin(WCHAR *pSampleName)
{
	LARGE_INTEGER iStartTime;

	st_SAMPLE *pSample = GetSample(pSampleName);
	QueryPerformanceCounter(&iStartTime);

	// 쓰기...

	if (pSample->iStartTime.QuadPart != 0)
		return false;

	pSample->iStartTime = iStartTime;
}

bool ProfileEnd(WCHAR *pSampleName)
{
	LARGE_INTEGER iEndTime, iSampleTime;
	QueryPerformanceCounter(&iEndTime);
	st_SAMPLE *pSample = GetSample(pSampleName);

	iSampleTime.QuadPart =
		iEndTime.QuadPart - pSample->iStartTime.QuadPart;
	pSample->iTotalSampleTime = iSampleTime.QuadPart;

	if (pSample->dMaxTime[1] < iSampleTime.QuadPart)
	{
		pSample->dMaxTime[0] = pSample->dMaxTime[1];
		pSample->dMaxTime[1] = iSampleTime.QuadPart;
	}

	if (pSample->dMinTime[1] > iSampleTime.QuadPart)
	{
		pSample->dMinTime[0] = pSample->dMinTime[1];
		pSample->dMinTime[1] = iSampleTime.QuadPart;
	}

	pSample->iCallCnt++;

	return true;
}

st_SAMPLE* GetSample(WCHAR *pName)
{
	int iIndex = GetThreadSampleIndex();
	int iSampleIndex;

	/////////////////////////////////////////////////////////
	// 해당 쓰레드 등록이 안되어 있을 때
	/////////////////////////////////////////////////////////
	if (-1 == iIndex)
	{
		AcquireSRWLockExclusive(&srwSampleThreadLock);
		//락걸고 다시 뒤져서 넣기
		for (iIndex = 0; iIndex < SAMPLE_MAX_THREAD; iIndex++)
		{
			if (SAMPLE_TH[iIndex].iThreadID == -1)
			{
				SAMPLE_TH[iIndex].iThreadID = GetCurrentThreadId();
				break;
			}
		}
		ReleaseSRWLockExclusive(&srwSampleThreadLock);
	}

	/////////////////////////////////////////////////////////
	// 샘플 주기
	/////////////////////////////////////////////////////////
	for (iSampleIndex = 0; iSampleIndex < SAMPLE_MAX; iSampleIndex++)
	{
		if (0 == wcscmp(SAMPLE_TH[iIndex].Sample[iSampleIndex].name, pName))
			break;

		/////////////////////////////////////////////////////////
		// 해당 쓰레드 안에 샘플이 없을 때
		/////////////////////////////////////////////////////////
		if (0 == wcscmp(SAMPLE_TH[iIndex].Sample[iSampleIndex].name, L""))
		{
			wcscpy_s(SAMPLE_TH[iIndex].Sample[iSampleIndex].name, pName);
			SAMPLE_TH[iIndex].Sample[iSampleIndex].iStartTime.QuadPart = 0;
			SAMPLE_TH[iIndex].Sample[iSampleIndex].iTotalSampleTime = 0;

			SAMPLE_TH[iIndex].Sample[iSampleIndex].dMaxTime[1] = DBL_MIN;
			SAMPLE_TH[iIndex].Sample[iSampleIndex].dMinTime[1] = DBL_MAX;

			SAMPLE_TH[iIndex].Sample[iSampleIndex].iCallCnt = 0;

			break;
		}
	}

	return &SAMPLE_TH[iIndex].Sample[iSampleIndex];
}

int GetThreadSampleIndex()
{
	int iIndex;
	for (iIndex = 0; iIndex < SAMPLE_MAX_THREAD; iIndex++)
	{
		if (SAMPLE_TH[iIndex].iThreadID == GetCurrentThreadId())
			return iIndex;
	}

	return -1;
}

bool SaveProfile()
{
	SYSTEMTIME stNowTime;
	WCHAR fileName[256];
	DWORD dwBytesWritten;
	WCHAR wBuffer[200];
	HANDLE hFile;

	GetLocalTime(&stNowTime);
	wsprintf(fileName, L"%4d-%02d-%02d %02d.%02d ProFiling.txt", stNowTime.wYear, stNowTime.wMonth,
		stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute);
	hFile = ::CreateFile(fileName,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);

	::WriteFile(hFile, 
		L"﻿ ThreadID |           Name  |     Average  |        Min   |        Max   |      Call |\r\n", 
		89 * sizeof(WCHAR), &dwBytesWritten, NULL);
	::WriteFile(hFile,
		L"--------------------------------------------------------------------------------------\r\n",
		88 * sizeof(WCHAR), &dwBytesWritten, NULL);

	for (int iThCnt = 0; iThCnt < SAMPLE_MAX_THREAD; iThCnt++)
	{
		if (-1 == SAMPLE_TH[iThCnt].iThreadID)
			break;

		for (int iSampleCnt = 0; iSampleCnt < SAMPLE_MAX; iSampleCnt++)
		{
			if (0 == wcscmp(SAMPLE_TH[iThCnt].Sample[iSampleCnt].name, L""))
				break;

			swprintf_s(wBuffer, L" %8d | %15s | %10.4f㎲ | %10.4f㎲ | %10.4f㎲ | %9d |\r\n", 
				SAMPLE_TH[iThCnt].iThreadID,
				SAMPLE_TH[iThCnt].Sample[iSampleCnt].name,
				SAMPLE_TH[iThCnt].Sample[iSampleCnt].iTotalSampleTime / 1000000,
				SAMPLE_TH[iThCnt].Sample[iSampleCnt].dMinTime[1] / 1000000,
				SAMPLE_TH[iThCnt].Sample[iSampleCnt].dMaxTime[1] / 1000000,
				SAMPLE_TH[iThCnt].Sample[iSampleCnt].iCallCnt);
			::WriteFile(hFile, wBuffer, wcslen(wBuffer) * sizeof(WCHAR), &dwBytesWritten, NULL);
		}
		::WriteFile(hFile,
			L"--------------------------------------------------------------------------------------\r\n",
			88 * sizeof(WCHAR), &dwBytesWritten, NULL);
	}

	CloseHandle(hFile);

	return true;
}