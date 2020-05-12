#include <iostream>
#include <stdio.h>
#include <termios.h> 
#include <unistd.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <wait.h>
#include <semaphore.h>
#include <string.h>
#include <math.h>

using namespace std;

union semun 
{
	int              val;    
	struct semid_ds *buf;    
	unsigned short  *array;  
	struct seminfo  *__buf; 

} semunion;

void WaitSemaphore(int sem_id, int num);
void ReleaseSemaphore(int sem_id, int num);


int main(int argc, char *argv[])
{  
	int bufferSize = 20; 
	int shm_id = shmget(IPC_PRIVATE, bufferSize, IPC_CREAT|0666); 

	if (shm_id < 0) 
	{
		printf("shmget error\n");
		return(0);
	} 

	int NumberOfBlocks; 
	int length;	      

	system("clear");
	string message;

	
	int sem_id = semget(IPC_PRIVATE, 4, IPC_CREAT|0666);
	semctl(sem_id, 0, SETALL, 0);
	if(sem_id < 0)
	{
		printf("Semaphores is not created.");
		return 0;
	}

	int pid = fork();
	switch(pid)
	{
	case -1:
		printf("New process is not created\n");
		return 0;
		
	case 0: 
		{
			void *buffer = shmat(shm_id, NULL, 0);
			while(1)
			{
				message.clear();
				WaitSemaphore(sem_id, 2); 
				
				WaitSemaphore(sem_id, 0);
				length = *(int*)buffer;	
				ReleaseSemaphore(sem_id, 1);	
				
				if(length == -1) break;  
				
				NumberOfBlocks = ceil((double)length / (double)bufferSize); 
				for( int i=0; i < NumberOfBlocks; i++)
				{
					WaitSemaphore(sem_id, 0);					
					message.append((char*)buffer, bufferSize); 
					ReleaseSemaphore(sem_id, 1);
				}
				
				message.resize(length);
				cout << "\nChild process:\n";

				for(int i = 0; i < length; i++)
				{
					putchar(message[i]);
					fflush(stdout);
					usleep(100000);
				}
				cout<<"\n\n\n";
				
				ReleaseSemaphore(sem_id, 3);		  
			}
			
			
		}
		break;
		
	default: 
		{
			void *buffer = shmat(shm_id, NULL, 0);
			while(1)
			{
				message.clear();
				cout<<"Parent process:\n";
				system("stty echo");
				tcflush(STDIN_FILENO, TCIFLUSH);
				getline(cin, message);
				system("stty -echo");
				
				ReleaseSemaphore(sem_id, 2);	
				
				if(message == "quit")
				{
					length = -1;	
					*(int*)buffer = length;
					ReleaseSemaphore(sem_id, 0);
					WaitSemaphore(sem_id, 1);    				
					waitpid(pid, NULL, 0);
					break;
				}			
				
				length = message.size();
				*(int*)buffer = length;
				
				ReleaseSemaphore(sem_id, 0);	
				WaitSemaphore(sem_id, 1);    
				
				NumberOfBlocks = ceil((double)length / (double)bufferSize); 
				for(int i = 0; i < NumberOfBlocks; i++)
				{					
					message.copy((char*)buffer, bufferSize, i*bufferSize);	
					
					ReleaseSemaphore(sem_id, 0);	
					WaitSemaphore(sem_id, 1);    
				}	
				
				WaitSemaphore(sem_id, 3);  
				
			}
			
			semctl(sem_id, 0, IPC_RMID, semunion); 
		}
		break;
	}

	system("clear"); 
	system("stty echo");
	return 0;
}

void WaitSemaphore(int sem_id, int num)
{
	struct sembuf buf;
	buf.sem_op = -1;
	buf.sem_flg = SEM_UNDO;
	buf.sem_num = num;
	semop(sem_id, &buf, 1);	
}

void ReleaseSemaphore(int sem_id, int num)
{
	struct sembuf buf;
	buf.sem_op = 1;
	buf.sem_flg = SEM_UNDO;
	buf.sem_num = num;
	semop(sem_id, &buf, 1); 
}