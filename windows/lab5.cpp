// Mylab5.cpp : Defines the entry point for the console application.
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <string>

using namespace std;

DWORD WINAPI WriterThread(PVOID pvParam);
DWORD WINAPI ReaderThread(PVOID argv);

#define read_finished_event 0
#define exit_event 1
#define write_finished_event 2

struct OperationInfo
{
    HANDLE hFile;             // Дескриптор обрабатываемого в данный момент файла
	DWORD NumberOfBytes;	  // Количество байт для чтения/записи
    CHAR  buf[100];			  // Буфер чтения/записи
	DWORD  pos_in_file;		  // Позиция в файле для чтения
	DWORD  pos_out_file;	  // Позиция в выходном файле
	OVERLAPPED Overlapped;
} info;

HINSTANCE library;
HANDLE events[3];


int main(int argc, char *argv[])
{
	if(argc!=3)
	{
		printf("arguments error");
		return 0;
	}

	HANDLE hEvent;        // дескриптор события для OVERLAPPED
	HANDLE hThreads[2];   // Дескрипторы потока-писателя и потока-читателя

	hEvent = CreateEvent (NULL, FALSE, TRUE, TEXT("Event_lab5"));  // Сигнальное, автосброс

	events[write_finished_event] = CreateEvent (NULL, FALSE, TRUE, NULL);   // Автосброс, начальное состояние - сигнальное
	events[read_finished_event] = CreateEvent (NULL, FALSE, FALSE, NULL);  // Автосброс, начальное состояние - несигнальное
	events[exit_event] = CreateEvent (NULL, TRUE, FALSE, NULL);            // Ручной сброс, начальное состояние - несигнальное
	 
	info.Overlapped.Offset = 0;
	info.Overlapped.OffsetHigh = 0;
	info.Overlapped.hEvent = hEvent;
	info.pos_out_file = 0;
	info.NumberOfBytes = sizeof(info.buf);
    
	library = LoadLibrary("library.dll");
  
	hThreads[0] = CreateThread(NULL, 0, WriterThread, (LPVOID)argv[2], 0, NULL); // Поток-писатель (аргумент - путь к выходному файлу)
	hThreads[1] = CreateThread(NULL, 0, ReaderThread, (LPVOID)argv[1], 0, NULL); // Поток-читатель (аргумент - путь к папке с текстовыми файлами)

	WaitForMultipleObjects(2, hThreads, TRUE, INFINITE);

	CloseHandle(hThreads[0]);
	CloseHandle(hThreads[1]);

	CloseHandle(events[write_finished_event]);
	CloseHandle(events[read_finished_event]);
	CloseHandle(events[exit_event]);
	CloseHandle(hEvent);
	FreeLibrary(library);
	printf("\n\n");
	return 0;
}


DWORD WINAPI ReaderThread(PVOID folderPATH)
{
	string folder(((const char*)folderPATH));               // Путь к папке с файлами
	folder.append("\\");
	string fileMask = folder + "*.txt";   // Тип файлов для поиска
	char ReadFilePATH[MAX_PATH];

	WIN32_FIND_DATA FindFileData; // информация о найденных файлах
	HANDLE find_Handle,  // Дескриптор поиска
           hReadFile;    // дескриптор файла для чтения

	BOOL readResult = false;

	BOOL (*Read)(OperationInfo*) = (BOOL (*)(OperationInfo*))GetProcAddress(library,"read"); // Функция чтения

	find_Handle = FindFirstFile(fileMask.c_str(), &FindFileData);

	if (find_Handle == INVALID_HANDLE_VALUE) 
    {
        printf (" Error: %d\n", GetLastError ());
		return 0;
    } 

	while(1)
	{   
		WaitForSingleObject(events[write_finished_event], INFINITE);
		if(readResult == false)
		{
			info.pos_in_file = 0;
			strcpy(ReadFilePATH, folder.c_str());
			strcat(ReadFilePATH, FindFileData.cFileName);  // Получаем путь к текущему читаемому файлу
			hReadFile = CreateFile(ReadFilePATH, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
		}
		info.hFile = hReadFile;
//		printf("\nStart read from %s", ReadFilePATH);
		readResult = (Read)(&info);	// Прочитать из файла

		if(readResult == false && GetLastError() == ERROR_HANDLE_EOF) // Если достигнут конец файла
		{
			if(FindNextFile(find_Handle, &FindFileData)) 
			{
				printf("\nRead from %s", ReadFilePATH);
				CloseHandle(hReadFile);
				SetEvent(events[write_finished_event]);
				continue;
			}
			else break;
		}
		
		SetEvent(events[read_finished_event]);
	}
	FindClose(find_Handle);
	CloseHandle(hReadFile);
	SetEvent(events[exit_event]);
	return 0;
}

DWORD WINAPI WriterThread(PVOID outFilePath)
{
	HANDLE hOutputFile = CreateFile((const char*)outFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
	BOOL (*Write)(OperationInfo*) = (BOOL (*)(OperationInfo*))GetProcAddress(library,"write"); // Функция записи
	HANDLE events_[2] = {events[read_finished_event], events[exit_event]  };
	while(1)
	{   
		int event = WaitForMultipleObjects(2, events_, FALSE, INFINITE) - WAIT_OBJECT_0;
		if(event == exit_event)
			break;
//		printf("\nStart write to %s", (const char*)outFilePath);
		info.hFile = hOutputFile;
        (Write)(&info);
        SetEvent(events[write_finished_event]);
	}
	CloseHandle(hOutputFile);
	return 0;
}