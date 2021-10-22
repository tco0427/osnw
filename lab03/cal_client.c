#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define PORT 3600
#define IP "127.0.0.1"
#define MAXBUF 1024

struct aggregate
{
	int max;
	struct sockaddr_in max_addr;
	int min;
	struct sockaddr_in min_addr;
	char avg[MAXBUF];
};

int main(int argc, char **argv)
{
	struct sockaddr_in addr;
	int server_sockfd;
	int len;
	float avg;
	char buf[MAXBUF];
	struct aggregate data;

    	if ((server_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		perror("error: ");
		return 1;
	}
   
    	addr.sin_family = AF_INET;
    	addr.sin_port = htons(PORT);
    	addr.sin_addr.s_addr = inet_addr(IP);

    	if (connect(server_sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1 )
    	{
		perror("connect error :");
   	 	close(server_sockfd);
   	 	return 1;
	}

	//입력 받은 값에 대한 처리를 client단에서 하는 것이 아니고 server단에서 처리해주기 때문에
	//read()로 읽은 문자열에 대해서 atoi()로 정수로 변환하지 않고 문자열 그대로를 전달해줍니다.
	memset(buf, 0x00, MAXBUF);
	read(0, buf, MAXBUF);
			
	if (write(server_sockfd, buf, MAXBUF) <= 0)
	{
		perror("write error : ");
		return 1;
	}

	memset((void *)&data, 0x00, sizeof(data));

	if (read(server_sockfd, (void *)&data, sizeof(data)) <= 0)
	{
		perror("read error : ");
		return 1;
	}

	avg = atof(data.avg);

	data.min = ntohl(data.min);
	data.max = ntohl(data.max);

	//port(sin_port)는 2바이트(unsgined short)이므로 ntohs() 사용
	data.min_addr.sin_port = ntohs(data.min_addr.sin_port);
	data.max_addr.sin_port = ntohs(data.max_addr.sin_port);

	printf("min = %d from %s:%d\n", data.min, inet_ntoa(data.min_addr.sin_addr), data.min_addr.sin_port);
	printf("max = %d from %s:%d\n", data.max, inet_ntoa(data.max_addr.sin_addr), data.max_addr.sin_port);
	printf("avg = %.2f\n", avg);

    	close(server_sockfd);
    	return 0;
}
