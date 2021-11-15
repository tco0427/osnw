#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdbool.h>

#define MAXLINE 1024
#define PORTNUM 3600

struct data {
	char str[MAXLINE];
	int num;
};

union semun {
	int val;
};

int main(int argc, char **argv)
{
	int listen_fd, client_fd;
	pid_t pid, pid_pnc;
	socklen_t addrlen;
	int readn,i;
	struct data buf;
	struct sockaddr_in client_addr, server_addr;
	char temp;

	void *shared_memory = NULL;
        int shmid;
        struct data *shm_buf;
	int shmid_check;
	void *shared_memory_check = NULL;
        int semid;
        union semun sem_union;
        struct sembuf semopen = {0, -1, SEM_UNDO};
        struct sembuf semclose = {0, 1, SEM_UNDO};

	bool *input_check;

	int count = 427;

	if( (listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		return 1;
	}

	memset((void *)&server_addr, 0x00, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORTNUM);

	if(bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==-1)
	{
		perror("bind error");
		return 1;
	}
	if(listen(listen_fd, 5) == -1)
	{
		perror("listen error");
		return 1;
	}

	//signal(SIG_CHLD, SIG_IGN)을 설정하면 child 프로세스가 종료되었을 때 zombi가 되는 것을 방지
	signal(SIGCHLD, SIG_IGN);
	
	while(1)
	{
		memset(&buf, 0x00, sizeof(buf));
		
		addrlen = sizeof(client_addr);

		client_fd = accept(listen_fd, (struct sockaddr *) &client_addr, &addrlen);
		if(client_fd == -1)
		{
			printf("accept error\n");
			break;
		}
		
		pid = fork();

		if(pid == 0){
			close(listen_fd);

			//소비자 프로세스에서 공유 메모리와 세마포 생성
			shmid = shmget((key_t)count, sizeof(struct data), 0666 | IPC_CREAT);
                        shared_memory = shmat(shmid, NULL, 0);
                        shm_buf = (struct data *)shared_memory;

                        shmid_check = shmget((key_t)(count+1), 1, 0666 | IPC_CREAT);
                        shared_memory_check = shmat(shmid_check, NULL, 0);
                        input_check = (bool *) shared_memory_check;

                        *input_check = false;

                        semid = semget((key_t)(count+2), 1, 0666 | IPC_CREAT);
                        sem_union.val = 1;
                        semctl(semid, 0, SETVAL, sem_union);

			pid_pnc = fork();
			
			if(pid_pnc == 0){	//생산자 프로세스
				memset(&buf, 0x00, sizeof(buf));

				readn = read(client_fd, &buf, sizeof(buf));

				buf.num = ntohl(buf.num);

				printf("Read Data %s (%d) : %s and %d\n",
						//inet_ntoa() : 이진 인터넷 주소를 점표기 방식으로 바꿈
						inet_ntoa(client_addr.sin_addr), client_addr.sin_port, buf.str, buf.num);
					
				semop(semid, &semopen, 1);	//P()

				strcpy(shm_buf->str, buf.str);
				shm_buf->num = buf.num;

				*input_check = true;

				semop(semid, &semclose, 1);	//V()

				while(1) {
					semop(semid, &semopen, 1);	//P()
	
					memset(&buf, 0x00, sizeof(buf));

					strcpy(buf.str, shm_buf->str);

					for(i = 0; i <= strlen(buf.str); i++) {
						if(i == 0){
							temp = buf.str[0];
						}
						if(buf.str[i] != '\0') {
							buf.str[i-1] = buf.str[i];
						}else{
							buf.str[i-1] = temp;
						}
					}

					buf.num = shm_buf->num +1;

					strcpy(shm_buf->str, buf.str);
					shm_buf->num = buf.num;
					
					sleep(1);	//sleep(1)없으면 너무 빠르게 출력되기 때문에 추가

					semop(semid, &semclose, 1);	//V()
				}
			}else if(pid_pnc > 0) {	//소비자 프로세스
				
				//client 입력 받기 이전에 흐름이 넘어가는 것을 방지
				while((*input_check) == false){}

				while(1) {
					semop(semid, &semopen, 1);	//P()

					buf = *shm_buf;

					buf.num = htonl(buf.num);

					write(client_fd, &buf, sizeof(buf));

					sleep(1);

					semop(semid, &semclose, 1);	//V()
				}	
			}
		}else if(pid > 0) {
			count = count + 3;
			close(client_fd);	
		}
	}
	return 0;
}

