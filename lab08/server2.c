#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <pthread.h>

#define MAXLINE 1024
#define PORTNUM 3600
#define LIMITCLIENT 1024

struct data {
    char str[MAXLINE];
    int num;
};

struct thread_arg {
	struct data *buf;
	int client_fd;
	struct sockaddr_in client_addr;
	int count;
};

//client 각각을 위한 mutex와cond
pthread_mutex_t lock[LIMITCLIENT];
pthread_cond_t cond[LIMITCLIENT];

int client_count = 0;



void * producer_func(void *param)
{
	struct thread_arg *arg = (struct thread_arg *)param;

	int client_fd = arg->client_fd;
	struct data *buf = arg->buf;
	struct sockaddr_in client_addr = arg->client_addr;
	int count = arg->count;
	int i,readn;
	char temp;

	readn = read(client_fd, buf, sizeof(struct data));

	buf->num = ntohl(buf->num);

	printf("Read Data %s (%d) : %s and %d\n",
			//inet_ntoa() : 이진 인터넷 주소를 점표기 방식으로 바꿈
			inet_ntoa(client_addr.sin_addr), client_addr.sin_port, buf->str, buf->num);
	
	while(1)
	{
		pthread_mutex_lock(&lock[count]);

		for(i = 0; i <= strlen(buf->str); i++)
		{
			if(i == 0)
			{
				temp = buf->str[0];
			}
			if(buf->str[i] != '\0'){
				buf->str[i-1] = buf->str[i];
			}else{
				buf->str[i-1] = temp;
			}
		}

		buf->num = buf->num + 1;

		pthread_mutex_unlock(&lock[count]);
		pthread_cond_signal(&cond[count]);
		sleep(1);

	}
}

void * consumer_func(void *param)
{
	struct thread_arg *arg = (struct thread_arg *)param;

        int client_fd = arg->client_fd;
        struct data *buf = arg->buf;
        struct sockaddr_in client_addr = arg->client_addr;
        int count = arg->count;

	while(1)
	{
		pthread_mutex_lock(&lock[count]);
		pthread_cond_wait(&cond[count], &lock[count]);

		buf->num = htonl(buf->num);

		write(client_fd, buf, sizeof(struct data));

		buf->num = ntohl(buf->num);

		pthread_mutex_unlock(&lock[count]);
	}
}

int main(int argc, char **argv)
{
	int listen_fd, client_fd;
	socklen_t addrlen;
	int readn;
	struct data *buf;
	struct thread_arg *targ[LIMITCLIENT];
	struct sockaddr_in client_addr, server_addr;
	pthread_t thread_id[2];

	if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket error");
		return 1;
	}

	memset((void *)&server_addr, 0x00, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORTNUM);

	if(bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
	{
		perror("bind error");
		return 1;
	}

	if(listen(listen_fd, 5) == -1)
	{
		perror("listen error");
		return 1;
	}

	//소켓 연결이 종료(client강제종료) 되었는데 send하면 errno에는 EPIPE로 설정된다.
	//여기에 더해 SIGPIPE시그널이 발생하므로 이 시그널을 무시하도록 signal()함수로 설정
	//(SIGPEPE는 기본 동작이 프로세스의 종료)
	signal(SIGPIPE, SIG_IGN);

	while(1)
	{
		if(client_count >= LIMITCLIENT)
		{
			printf("최대 클라이언트 수를 초과하였습니다. 더 이상 클라이언트 접속이 불가합니다.\n");
			while(1){}
		}

		addrlen = sizeof(client_addr);

		//새로운 client 요청이 있을 때까지 block
                client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addrlen);

		if(client_fd == -1)
		{
			printf("accept error\n");
			break;
		}
		
		buf = malloc(sizeof(struct data));
		
		targ[client_count] = malloc(sizeof(struct thread_arg));

		targ[client_count]->buf = buf;
		targ[client_count]->client_fd = client_fd;

		targ[client_count]->client_addr = client_addr;
		targ[client_count]->count = client_count;

		//If successful, pthread_mutex_init() returns zero
		if(pthread_mutex_init(&lock[client_count], 0) != 0)
		{
			perror("Mutex Init failure");
			return 1;
		}

		//If successful, pthread_cond_init() returns zero
		if(pthread_cond_init(&cond[client_count], 0) != 0)
		{
			perror("Cond Init failure");
			return 1;
		}

		//If successful, pthread_create() returns zero
               	if(pthread_create(&thread_id[0], NULL, producer_func, (void *)targ[client_count]) != 0)
		{
			perror("pthread_create failure");
			return 1;
		}
		if(pthread_detach(thread_id[0]) != 0)
		{
			perror("pthread_detach failure");
			return 1;
		}

		if(pthread_create(&thread_id[1], NULL, consumer_func, (void *)targ[client_count]) != 0)
		{
			perror("pthread_create failure");
			return 1;
		}
		if(pthread_detach(thread_id[1]) != 0)
		{
			perror("pthread_detach failure");
			return 1;
		}

		client_count++;
	}

}
