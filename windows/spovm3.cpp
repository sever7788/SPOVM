#include<stdio.h>
#include<windows.h>
#include<conio.h>
#include<time.h>
#include<iostream>
#include<string>

using namespace std;

void Server(char* path);
void Client();



int main(int argc, char* argv[])
{
	switch (argc)
	{
	case 1:
		Server(argv[0]);
		break;

	default:
		Client();
		break;
	}
}



void Server(char* path)
{
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	PROCESS_INFORMATION childProcessInfo;
	ZeroMemory(&childProcessInfo, sizeof(childProcessInfo));

	HANDLE hMyPipe;
	HANDLE Semaphores[3];

	char buffer[20];
	int bufferSize = sizeof(buffer);

	string message;


	Semaphores[0] = CreateSemaphoreA(NULL, 0, 1, "SEMAPHORE_lab3");				//создание семафоров
	Semaphores[1] = CreateSemaphoreA(NULL, 0, 1, "SEMAPHORE_end_lab3");
	Semaphores[2] = CreateSemaphoreA(NULL, 0, 1, "SEMAPHORE_EXIT_lab3");

	cout << "Server process\n\n";
    //создаем именнованный канал
	hMyPipe = CreateNamedPipeA("\\\\.\\pipe\\MyPipe", PIPE_ACCESS_OUTBOUND, PIPE_TYPE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, 0, 0, INFINITE, (LPSECURITY_ATTRIBUTES)NULL);
	//создание дочернего процесса
	CreateProcessA(path, (LPSTR)" 2", NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, (LPSTARTUPINFOA)&si, &childProcessInfo);

	if (!ConnectNamedPipe(hMyPipe, (LPOVERLAPPED)NULL)) //подключение к каналу
		cout << "Connection failure\n";

	while (1)//основной цикл сервера
	{
		DWORD NumberOfBytesWritten;

		cout << "\nEnter message:\n";
		cin.clear();
		getline(cin, message);//считываем сообщение


		if (message == "quit")
		{
			ReleaseSemaphore(Semaphores[2], 1, NULL);//установить семафор в несигнальное состояние
			WaitForSingleObject(childProcessInfo.hProcess, INFINITE);
			break;
		}

		ReleaseSemaphore(Semaphores[0], 1, NULL);

		cout << "w1\n";
		int NumberOfBlocks = message.size() / bufferSize + 1;
		WriteFile(hMyPipe, &NumberOfBlocks, sizeof(NumberOfBlocks), &NumberOfBytesWritten, (LPOVERLAPPED)NULL);

		cout << "w2\n";
		int size = message.size();
		WriteFile(hMyPipe, &size, sizeof(size), &NumberOfBytesWritten, (LPOVERLAPPED)NULL);

		cout << "w_blocks\n";
		for (int i = 0; i < NumberOfBlocks; i++)
		{
			message.copy(buffer, bufferSize, i * bufferSize);
			if (!WriteFile(hMyPipe, buffer, bufferSize, &NumberOfBytesWritten, (LPOVERLAPPED)NULL)) cout << "Write Error\n";
		}

		WaitForSingleObject(Semaphores[1], INFINITE);
	}

	CloseHandle(hMyPipe);
	CloseHandle(Semaphores[0]);
	CloseHandle(Semaphores[1]);
	cout << "\n\n";
	system("pause");
	return;
}

void Client()		
{
	HANDLE hMyPipe;
	HANDLE Semaphores[3];

	char buffer[20];
	int bufferSize = sizeof(buffer);

	string message;

	bool successFlag;
	Semaphores[0] = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, TRUE, "SEMAPHORE_lab3");
	Semaphores[1] = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, TRUE, "SEMAPHORE_end_lab3");
	Semaphores[2] = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, TRUE, "SEMAPHORE_EXIT_lab3");

	cout << "Child process\n\n";

	hMyPipe = CreateFileA("\\\\.\\pipe\\MyPipe", GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);


	while (1)	//основной цикл клиента
	{
		successFlag = TRUE;
		DWORD NumberOfBytesRead;
		message.clear();

		int index = WaitForMultipleObjects(3, Semaphores, FALSE, INFINITE) - WAIT_OBJECT_0;
		if (index == 2)
			break;

		int NumberOfBlocks;
		if (!ReadFile(hMyPipe, &NumberOfBlocks, sizeof(NumberOfBlocks), &NumberOfBytesRead, NULL)) break;	//считывание файла

		int size;
		if (!ReadFile(hMyPipe, &size, sizeof(size), &NumberOfBytesRead, NULL)) break;

		for (int i = 0; i < NumberOfBlocks; i++)
		{
			successFlag = ReadFile(hMyPipe, buffer, bufferSize, &NumberOfBytesRead, NULL);
			if (!successFlag) break;

			message.append(buffer, bufferSize);
		}
		if (!successFlag) break;

		message.resize(size);

		for (int i = 0; i < size; i++)
		{
			cout << message[i];
			Sleep(100);
		}
		cout << endl;

		ReleaseSemaphore(Semaphores[1], 1, NULL);		//установка семафора в несигнальное состояние
	}
	CloseHandle(hMyPipe);
	CloseHandle(Semaphores[0]);
	CloseHandle(Semaphores[1]);
	return;
}