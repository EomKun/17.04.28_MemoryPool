#ifndef __PROFILER__H__
#define __PROFILER__H__

enum
{
	NAME_MAX = 20,
	SAMPLE_MAX = 100,
	SAMPLE_MAX_THREAD = 100
};

struct st_SAMPLE
{
	WCHAR			name[NAME_MAX];

	LARGE_INTEGER		iStartTime;
	double			iTotalSampleTime;

	double			dMaxTime[2];
	double			dMinTime[2];

	int				iCallCnt;
};

struct st_SAMPLE_THREAD
{
	int				iThreadID;
	st_SAMPLE		Sample[SAMPLE_MAX];
};

void					ProfileInit();

bool					ProfileBegin(WCHAR *pSampleName);
bool					ProfileEnd(WCHAR *pSampleName);

st_SAMPLE*			GetSample(WCHAR *pName);
int					GetThreadSampleIndex();

bool					SaveProfile();

#endif

#define PROFILE_CHECK

#ifdef PROFILE_CHECK
#define PRO_BEGIN(X) ProfileBegin(X)
#define PRO_END(X) ProfileEnd(X)
#else
#define PRO_BEGIN(X)
#define PRO_END(X)
#endif