/*---------------------------------------------------------------

	procademy MemoryPool.

	메모리 풀 클래스.
	특정 데이타(구조체,클래스,변수)를 일정량 할당 후 나눠쓴다.

	- 사용법.

	procademy::CMemoryPool<DATA> MemPool(300, FALSE);
	DATA *pData = MemPool.Alloc();

	pData 사용

	MemPool.Free(pData);


	!.	아주 자주 사용되어 속도에 영향을 줄 메모리라면 생성자에서
		Lock 플래그를 주어 페이징 파일로 복사를 막을 수 있다.
		아주 중요한 경우가 아닌이상 사용 금지.

		
		
		주의사항 :	단순히 메모리 사이즈로 계산하여 메모리를 할당후 메모리 블록을 리턴하여 준다.
					클래스를 사용하는 경우 클래스의 생성자 호출 및 클래스정보 할당을 받지 못한다.
					클래스의 가상함수, 상속관계가 전혀 이뤄지지 않는다.
					VirtualAlloc 으로 메모리 할당 후 memset 으로 초기화를 하므로 클래스정보는 전혀 없다.
		
				
----------------------------------------------------------------*/
#ifndef  __MEMORYPOOL__H__
#define  __MEMORYPOOL__H__s
#include <assert.h>
#include <new.h>


	template <class DATA>
	class CMemoryPool
	{
	private:

		/* **************************************************************** */
		// 각 블럭 앞에 사용될 노드 구조체.
		/* **************************************************************** */
		struct st_BLOCK_NODE
		{
			st_BLOCK_NODE()
			{
				stpNextBlock = NULL;
			}
			st_BLOCK_NODE *stpNextBlock;
		};

		/* **************************************************************** */
		// 락프리 메모리 풀의 탑 노드
		/* **************************************************************** */
		struct st_TOP_NODE
		{
			st_BLOCK_NODE *pNode;
			__int64 iUniqueNum;
		};

	public:

		//////////////////////////////////////////////////////////////////////////
		// 생성자, 파괴자.
		//
		// Parameters:	(int) 최대 블럭 개수.
		//				(bool) 메모리 Lock 플래그 - 중요하게 속도를 필요로 한다면 Lock.
		// Return:
		//////////////////////////////////////////////////////////////////////////
		CMemoryPool(int iBlockNum, bool bLockFlag = false)
		{
			////////////////////////////////////////////////////////////////
			// TOP 노드 할당
			////////////////////////////////////////////////////////////////
			_pTopNode = (st_TOP_NODE *)_aligned_malloc(sizeof(st_TOP_NODE), 16);
			_pTopNode->pNode = NULL;
			_pTopNode->iUniqueNum = 0;

			_iUniqueNum = 0;

			////////////////////////////////////////////////////////////////
			// 메모리 풀 크기 설정
			////////////////////////////////////////////////////////////////
			m_iBlockCount = iBlockNum;

			if (iBlockNum < 0)	return;

			else if (iBlockNum == 0)
			{
				m_bStoreFlag = true;
				m_stBlockHeader = NULL;
			}

			////////////////////////////////////////////////////////////////
			// DATA * 개수 크기만 큼 메모리 할당
			////////////////////////////////////////////////////////////////
			m_stBlockHeader = new char[(sizeof(DATA) + sizeof(st_BLOCK_NODE)) * m_iBlockCount];

			_pTopNode = (st_BLOCK_NODE *)m_stBlockHeader;
			char *pBlock = (char *)m_stpTop;
			st_BLOCK_NODE *stpNode = m_stpTop;

			////////////////////////////////////////////////////////////////
			// BLOCK 연결
			////////////////////////////////////////////////////////////////
			for (int iCnt = 0; iCnt < m_iBlockCount - 1; iCnt++)
			{
				pBlock += sizeof(DATA) + sizeof(st_BLOCK_NODE);
				stpNode->stpNextBlock = (st_BLOCK_NODE *)pBlock;
				stpNode = stpNode->stpNextBlock;
			}

			stpNode->stpNextBlock = NULL;
		}

		virtual	~CMemoryPool()
		{
			delete []m_stBlockHeader;
		}

		//////////////////////////////////////////////////////////////////////////
		// 블럭 하나를 할당받는다.
		//
		// Parameters: 없음.
		// Return: (DATA *) 데이타 블럭 포인터.
		//////////////////////////////////////////////////////////////////////////
		DATA	*Alloc(bool bPlacementNew = true)
		{
			st_BLOCK_NODE *stpBlock;
			st_TOP_NODE pPreTopNode;
			st_BLOCK_NODE *pNode, *pNewTopNode;

			if (m_iBlockCount <= m_iAllocCount)
			{
				if (m_bStoreFlag)
				{
					stpBlock = (st_BLOCK_NODE *)new char[(sizeof(st_BLOCK_NODE) + sizeof(DATA))];
					InterlockedIncrement64((LONG64 *)&m_iBlockCount);
				}

				else
					return nullptr;
			}

			else
			{
				st_TOP_NODE pPreTopNode;
				st_BLOCK_NODE *pNode;
				__int64 iUniqueNum = InterlockedIncrement64(&_iUniqueNum);

				do
				{
					pPreTopNode.iUniqueNum = _pTop->iUniqueNum;
					pPreTopNode.pTopNode = _pTop->pTopNode;

					pNode = _pTop->pTopNode;
				} while (!InterlockedCompareExchange128((volatile LONG64 *)_pTop, iUniqueNum, (LONG64)_pTop->pTopNode->pNext, (LONG64 *)&pPreTopNode));
				*pOutData = pPreTopNode.pTopNode->Data;
				delete pNode;
		
				stpBlock = m_stpTop;
				m_stpTop = stpBlock->stpNextBlock;
			}

			InterlockedIncrement64((LONG64 *)&m_iAllocCount);
			return (DATA *)(stpBlock + 1);
		}

		//////////////////////////////////////////////////////////////////////////
		// 사용중이던 블럭을 해제한다.
		//
		// Parameters: (DATA *) 블럭 포인터.
		// Return: (BOOL) TRUE, FALSE.
		//////////////////////////////////////////////////////////////////////////
		bool	Free(DATA *pData)
		{
			st_BLOCK_NODE *stpBlock;
			st_TOP_NODE pPreTopNode;

			stpBlock = ((st_BLOCK_NODE *)pData - 1);
			stpBlock->stpNextBlock = _pTopNode;

			__int64 iUniqueNum = InterlockedIncrement64(&_iUniqueNum);

			do {
				pPreTopNode.pTopNode = _pTop->pTopNode;
				pPreTopNode.iUniqueNum = _pTop->iUniqueNum;

				pNode->Data = Data;
				pNode->pNext = _pTop->pTopNode;
			} while (!InterlockedCompareExchange128((volatile LONG64 *)_pTop, iUniqueNum, (LONG64)pNode, (LONG64 *)&pPreTopNode));

			InterlockedDecrement64((LONG64 *)&m_iAllocCount);
			return false;
		}


		//////////////////////////////////////////////////////////////////////////
		// 현재 사용중인 블럭 개수를 얻는다.
		//
		// Parameters: 없음.
		// Return: (int) 사용중인 블럭 개수.
		//////////////////////////////////////////////////////////////////////////
		int		GetAllocCount(void) { return m_iAllocCount; }

	private:
		//////////////////////////////////////////////////////////////////////////
		// 블록 스택의 탑
		//////////////////////////////////////////////////////////////////////////
		st_TOP_NODE *_pTopNode;

		//////////////////////////////////////////////////////////////////////////
		// 탑의 Unique Number
		//////////////////////////////////////////////////////////////////////////
		__int64 _iUniqueNum;

		//////////////////////////////////////////////////////////////////////////
		// 노드 구조체 헤더
		//////////////////////////////////////////////////////////////////////////
		char *m_stBlockHeader;

		//////////////////////////////////////////////////////////////////////////
		// 메모리 Lock 플래그
		//////////////////////////////////////////////////////////////////////////
		bool m_bLockFlag;

		//////////////////////////////////////////////////////////////////////////
		// 메모리 동적 플래그, true면 없으면 동적할당 함
		//////////////////////////////////////////////////////////////////////////
		bool m_bStoreFlag;

		//////////////////////////////////////////////////////////////////////////
		// 현재 사용중인 블럭 개수
		//////////////////////////////////////////////////////////////////////////
		int m_iAllocCount;

		//////////////////////////////////////////////////////////////////////////
		// 전체 블럭 개수
		//////////////////////////////////////////////////////////////////////////
		int m_iBlockCount;
	};

#endif