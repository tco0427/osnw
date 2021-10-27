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

#define MAXLINE 1024
#define PORTNUM 3600

int main(int argc, char **argv)
{
	int listen_fd;
	pid_t pid;
	socklen_t addrlen;
	int readn,i,count;
	char buf[MAXLINE];
	char result[MAXLINE];
	int client_fd[3], fdp[2], fdc[2];
	struct sockaddr_in client_addr, server_addr;

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

	pipe(fdc);
	pipe(fdp);
	
	while(1)
	{
		memset(result, 0x00, MAXLINE);
		for(i = 0; i < 3; i++)
		{
			addrlen = sizeof(client_addr);

			client_fd[i] = accept(listen_fd,(struct sockaddr *)&client_addr, &addrlen);
			if(client_fd[i] == -1)
			{	
				printf("accept error\n");
				break;
			}
			printf("New Client[%d] Connect: %s\n", i, inet_ntoa(client_addr.sin_addr));
			

			pid = fork();

			if(pid == 0)
			{
				close( listen_fd );

				memset(buf, 0x00, MAXLINE);

				while((readn = read(client_fd[i], buf, MAXLINE)) > 0)
				{
					printf("from client[%d] (%s:%d) : %s\n",
							//inet_ntoa() : 이진 인터넷 주소를 점표기 방식으로 바꿈
							i, inet_ntoa(client_addr.sin_addr), client_addr.sin_port,buf);
				
					write(fdc[1], buf, sizeof(buf));
					read(fdp[0], buf, sizeof(buf));
					write(client_fd[i], buf, sizeof(buf));
				}
				
				memset(buf, 0x00, MAXLINE);
				close(client_fd[i]);

				while(1){}	//죽지않고 대기
			}else if(pid > 0 ){
				close(client_fd[i]);
			}
		}
		if(pid>0) {
			for(i = 0; i<3; i++){
				read(fdc[0], buf, sizeof(buf));
				buf[strlen(buf)-1] = ' ';
				strcat(result, buf);
			}
			strcat(result, "\n");
			for(i=0;i<3;i++){
				write(fdp[1], result, sizeof(result));
			}
		}
	}
	return 0;
}
