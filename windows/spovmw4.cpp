#include <windows.h>
#include <conio.h>
#include <iostream>
#include <vector>
#include <stack>
#include <process.h>

#define MAX_COUNT 10

using namespace std;

unsigned int __stdcall  printString(void* arg);
void CloseLastThread();
void WaitThreads();
void AddThread();

char strings[10][30] = { {"1) First thread"}, {"2) Second thread"}, {"3) Third thread"}, {"4) Fourth thread"}, {"5) Fifth thread"}, {"6) Sixth thread"}, {"7) Seventh thread"}, {"8) Eighth thread"}, {"9) Ninth thread"}, {"10) Tenth thread"} };

CRITICAL_SECTION cs_print;

stack<HANDLE> threads;
stack<HANDLE> closingThreads;
vector<bool*> quitFlags;

struct threadArg
{
	bool* quitFlag;
	int num;
};


void main()
{
	InitializeCriticalSection(&cs_print);
	while (1)
	{
		switch (_getch())
		{
		case '+':
			if (threads.size() < MAX_COUNT) AddThread();
			break;
		case '-':
			if (threads.size() > 0) CloseLastThread();
			break;
		case 'q':
			while (threads.size() > 0)
				CloseLastThread();

			WaitThreads();

			DeleteCriticalSection(&cs_print);
			printf("\n\n");
			system("pause");
			return;
		default:
			break;
		}
	}
}


void CloseLastThread()
{
	closingThreads.push(threads.top());

	*(quitFlags.back()) = true;
	quitFlags.pop_back();

	threads.pop();
}

void WaitThreads()
{
	while (closingThreads.size() > 0)
	{
		WaitForSingleObject(closingThreads.top(), INFINITE);
		closingThreads.pop();
	}
}

void AddThread()//создание потока
{
	quitFlags.push_back(new bool(false));

	threadArg* arg = new threadArg();
	(*arg).num = threads.size();
	(*arg).quitFlag = quitFlags.back();

	HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, printString, (void*)(arg), 0, NULL);
	threads.push(thread);//+

}



unsigned int __stdcall  printString(void* arg)//печатать строку
{


	bool* qFlag = (*(threadArg*)arg).quitFlag;						
	int threadNumber = (*(threadArg*)arg).num;						
	delete arg;																	
																						
	while (1)
	{
		if (*qFlag) break;

		EnterCriticalSection(&cs_print);										//критиеская секция
		for (int i = 0; i < strlen(strings[threadNumber]); i++)
		{

			if (*qFlag) break;

			printf("%c", strings[threadNumber][i]);
			Sleep(100);
		}

		LeaveCriticalSection(&cs_print);										//конец криической секции
		Sleep(1);
	}

	delete qFlag;
	return 0;
}