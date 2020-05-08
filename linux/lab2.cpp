#include<iostream>
#include<unistd.h>
#include<stdlib.h>
#include<vector>
#include<sys/sem.h>
#include<signal.h> 
#include<termios.h>

using namespace std;

int killAllProcesses(vector<pid_t>);//завершить все дочерине процессыю
int createProcess(pid_t);	//создать дочерний процесс
void handler(int);		//прототип обработчика сигнала SIGUSR1 для родительского процесса

vector<pid_t> pids;		//вектор для хранения PID-ов доч. процессов
pid_t pid;			//переменная для хранения созданного/удаляемого процесса
int count = 0;			//индекс работающего процесса
bool fl;			//флаг занятости дочернего процесса(у каждого свой)

int main(){
	signal(SIGUSR1, handler);//привязка сигнала к обработчику
	char command;

	std::cout << "\n'+' - создать процесс\n'-' - удалить процесс\n'q' - выйти из программы" << endl; 	
		
	struct termios ts, ots;		//для считывания нажатой клавиши без эха
	tcgetattr(STDIN_FILENO, &ts);
	ots = ts;
	while(true){
		ts.c_lflag  = ~ECHO;
		ts.c_lflag |= ECHONL;
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &ts);
		cin>>command;	
		tcsetattr(STDIN_FILENO, TCSANOW, &ots);	//возвращает эхо
		switch(command){
			case '+':
				if(pids.size() == 0)	//если процесс первый, то он преступае к работет
					fl = true;
				else 	fl = false;
				createProcess(pid = fork()); //создание процесса
				pids.push_back(pid); break; //добавить в вектор
			case '-':
				if(pids.size() == 0){
						std::cout << "\n0 дочерних процессов" << endl;
						break;
				}
				
				pid = pids.back();
				pids.pop_back();			
				kill(pid, SIGKILL);  
				     break;
			case 'q':
			killAllProcesses(pids); //убить всех перед выходом
			exit(0); 				 break;
			default: std::cout << "Неверная команда!" << endl;
		}
	}
	
	return 0;
}
void handler(int sig){		//обработчик SIGUSR1
	count++;
		if(count>=pids.size())
			count = 0;
		kill(pids[count], SIGUSR2);
	return;
}

void handler2(int sig){		//обработчик SIGUSR2
	fl = true;
	return;
}

int killAllProcesses(vector<pid_t> pids){
	for(auto &a: pids){
		kill(a, SIGKILL);
	}
	return 0;
}

int createProcess(pid_t pid){
	switch(pid){
		case -1: perror("fork"); exit(0); break;
		case  0: 
			signal(SIGUSR2, handler2);
			string str = "PID: ";
			str+=to_string(getpid());
			//std::cout << "Создан новый процесс" << endl;
			while(true){
				if(fl == 1){
					for(auto &a: str){
						std::cout << a << flush;
						usleep(100000);
					}
					std::cout<<endl;
					fl = false;			//блокировка исполнения процесса
					kill(getppid(), SIGUSR1);	//сообщение родителю об отработке цикла
				}
				sleep(100);
			}
			break;
	}
	return 0;
}