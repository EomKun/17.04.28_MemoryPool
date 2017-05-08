#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <time.h>
#include <stdlib.h>

#include "MemoryPool.h"
#include "MemoryPool_test.h"

/*
테스트 목적
- 넣은 데이터 개수와 뽑은 데이터 개수의 일치 확인
- 데이터를 넣었다가 뽑은 뒤에 이 데이터를 다른이가 메모리를 사용하는지 확인 (2중으로 뽑히는지 확인)
struct st_TEST_DATA
{
volatile LONG64	lData;
volatile LONG64	lCount;
};
//위 구조체의 포인터를 다루는 스택 전역 선언.
//CLockfreeStack<st_TEST_DATA *> g_Stack();
*/

CMemoryPool<st_TEST_DATA *> g_Mempool;

LONG64 lPushTPS = 0;
LONG64 lPopTPS = 0;

LONG64 lPushCounter = 0;
LONG64 lPopCounter = 0;

unsigned __stdcall StackThread(void *pParam);

void main()
{
	HANDLE hThread[dfTHREAD_MAX];
	DWORD dwThreadID;



	for (int iCnt = 0; iCnt < dfTHREAD_MAX; iCnt++)
	{
		hThread[iCnt] = (HANDLE)_beginthreadex(
			NULL,
			0,
			StackThread,
			(LPVOID)1000,
			0,
			(unsigned int *)&dwThreadID
			);
	}

	while (1)
	{
		lPushTPS = lPushCounter;
		lPopTPS = lPopCounter;

		lPushCounter = 0;
		lPopCounter = 0;

		wprintf(L"------------------------------------------------\n");
		wprintf(L"Push TPS : %d\n", lPushTPS);
		wprintf(L"Pop TPS : %d\n", lPopTPS);
		wprintf(L"------------------------------------------------\n\n");

		Sleep(999);
	}
}

/*------------------------------------------------------------------*/
// 0. 각 스레드에서 st_QUEUE_DATA 데이터를 일정 수치 (10000개) 생성		
// 0. 데이터 생성(확보)
// 1. iData = 0x0000000055555555 셋팅
// 1. lCount = 0 셋팅
// 2. 스택에 넣음

// 3. 약간대기  Sleep (0 ~ 3)
// 4. 내가 넣은 데이터 수 만큼 뽑음 
// 4. - 이때 뽑히는건 내가 넣은 데이터일 수도, 다른 스레드가 넣은 데이터일 수도 있음
// 5. 뽑은 전체 데이터가 초기값과 맞는지 확인. (데이터를 누가 사용하는지 확인)
// 6. 뽑은 전체 데이터에 대해 lCount Interlock + 1
// 6. 뽑은 전체 데이터에 대해 iData Interlock + 1
// 7. 약간대기
// 8. + 1 한 데이터가 유효한지 확인 (뽑은 데이터를 누가 사용하는지 확인)
// 9. 데이터 초기화 (0x0000000055555555, 0)
// 10. 뽑은 수 만큼 스택에 다시 넣음
//  3번 으로 반복.
/*------------------------------------------------------------------*/
unsigned __stdcall StackThread(void *pParam)
{
	srand(time(NULL) + (int)pParam);

	int iRand, iCnt;
	st_TEST_DATA *pData;
	st_TEST_DATA *pDataArray[dfTHREAD_ALLOC];

	iRand = rand() % dfTHREAD_ALLOC;

	for (iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
	{
		pDataArray[iCnt] = new st_TEST_DATA;
		pDataArray[iCnt]->lData = 0x0000000055555555;
		pDataArray[iCnt]->lCount = 0;
	}

	for (iCnt = 0; iCnt < iRand; iCnt++)
	{
		g_Mempool.Free(pDataArray[iCnt]);
		InterlockedIncrement64((LONG64 *)&lPushCounter);
	}

	while (1){
		Sleep(0);

		for (int iCnt = 0; iCnt < iRand; iCnt++)
		{
			g_Stack.Pop(&pData);
			InterlockedIncrement64((LONG64 *)&lPopCounter);

			if ((pData->lData != 0x0000000055555555) || (pData->lCount != 0))
				printf("pDataArray[%d] is using in stack\n", iCnt);

			InterlockedIncrement64((LONG64 *)&pData->lCount);
			InterlockedIncrement64((LONG64 *)&pData->lData);

			pDataArray[iCnt] = pData;
		}

		Sleep(0);

		for (int iCnt = 0; iCnt < iRand; iCnt++)
		{
			if ((pDataArray[iCnt]->lCount != 1) || (pDataArray[iCnt]->lData != 0x0000000055555556))
				printf("pDataArray[%d] is using\n", iCnt);
		}

		for (int iCnt = 0; iCnt < iRand; iCnt++)
		{
			InterlockedDecrement64((LONG64 *)&pDataArray[iCnt]->lCount);
			InterlockedDecrement64((LONG64 *)&pDataArray[iCnt]->lData);
		}

		for (int iCnt = 0; iCnt < iRand; iCnt++)
		{
			g_Stack.Push(pDataArray[iCnt]);
			InterlockedIncrement64((LONG64 *)&lPushCounter);
		}
	}
}