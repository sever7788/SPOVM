
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <conio.h>
#include <iostream>
#include <sstream>
#include <string>
#include <locale.h>

using namespace std;

string s_numbers = "Process ";

PROCESS_INFORMATION CreateNewProcess(char command[])				//функция создания процесса
{
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(STARTUPINFO));
	PROCESS_INFORMATION pi;

	char cmd[MAX_PATH] = "C:\\Users\\Иван\\source\\repos\\2\\Debug\\2.exe";
	char cmdline[MAX_PATH + 50];
	strcpy(cmdline, cmd);
	strcat(cmdline, command);

	if(!CreateProcessA(cmd, cmdline, NULL, NULL, TRUE, NULL, NULL, NULL, (LPSTARTUPINFOA)&si, &pi)) {
		printf("%ul", GetLastError());
	}
	return pi;
}

void Create()				//функция родительского процесса с основным циклом и вводом команд
{
	int activeProcess = 0;
	HANDLE hCanWriteEvent = CreateEvent(NULL, FALSE, TRUE, (LPTSTR)"WriteProcess");//главное событие для порядка отработки дочерних процессов
	char buffer[30];
	TCHAR choose;
	PROCESS_INFORMATION mas[10];
	HANDLE hCanClose[30];

	while (choose = _getch())
	{

		if (choose == 'q') break;							//выход

		if (choose == '-')								//завершить последний процесс
			if (activeProcess)
			{
				WaitForSingleObject(hCanWriteEvent, INFINITE);
				TerminateProcess(mas[activeProcess-1].hProcess,0);
				printf("\n*****************\n");
				cout << "Process " << activeProcess << " completed" << endl;
				printf("*****************");
				//SetEvent(hCanClose[activeProcess]);
				SetEvent(hCanWriteEvent);
				activeProcess--;
			}

		if ((choose == '+') && (activeProcess < 10))				//cсоздание процесса и события
		{
			sprintf(buffer, " %d", ++activeProcess);
			hCanClose[activeProcess] = CreateEvent(NULL, FALSE, FALSE, (LPTSTR)buffer);
			mas[activeProcess - 1] = CreateNewProcess(buffer);
		}
		Sleep(300);
	}

	if (activeProcess)										//завершить все процессы перед выходом;
	{
		WaitForSingleObject(hCanWriteEvent, INFINITE);
		while (activeProcess > 0)
		{
			
			TerminateProcess(mas[activeProcess - 1].hProcess, 0);
			//SetEvent(hCanClose[activeProcess]);
			activeProcess--;
		}
		SetEvent(hCanWriteEvent);
	}
}
void Print(int numofProccess)
{
	HANDLE hCanWriteEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, (LPCTSTR)"WriteProcess");
	char buffer[30];
	sprintf(buffer, " %d", numofProccess);
	HANDLE hCanClose = OpenEvent(EVENT_ALL_ACCESS, FALSE, (LPCTSTR)buffer);
	stringstream ss;
	ss << numofProccess;
	string str = ss.str();
	while (1)
	{
		if (WaitForSingleObject(hCanWriteEvent, INFINITE) == WAIT_OBJECT_0)		//ожидание сигнального состаяния главого события
		{
			if (WaitForSingleObject(hCanClose, 100) == WAIT_OBJECT_0)			// индивидульное событие для завершения(однако завершается через terminateProcess)
			{
				SetEvent(hCanWriteEvent);
				return;
			}

			for (auto a : s_numbers) {
				cout << a;
				Sleep(100);
			}
			for (auto a : str) {
				cout << a;
				Sleep(100);
			}

			printf("\n_________________\n");
			SetEvent(hCanWriteEvent);										//установить в сигнальное состояние;
		}
	}
	return;
}

int main(int argc, char* argv[])
{	
	setlocale(LC_ALL, "RUS");
	if (argc == 2)		//для дочернего
	{
		Print(atoi(argv[1]));
	}
	else{				//для родительского
		fflush(stdin);
		cout << "'+' - добавить процесс\n'-' - удалить процесс\n'q' - выход\n" << endl;
		Create();}
	return 0;
}