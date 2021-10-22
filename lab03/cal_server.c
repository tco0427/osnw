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
        struct sockaddr_in sock_addr;
        int listen_sockfd;
        int client_sockfd[3];
	int addr_len[3];
        int i, n, num;
	char buf[MAXBUF];
        float sum, avg;
	struct aggregate result;
	struct sockaddr_in client_addrs[3];

        if((listen_sockfd  = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
                perror("Error ");
                return 1;
        }

        memset((void *)&sock_addr, 0x00, sizeof(sock_addr));

        sock_addr.sin_family = AF_INET;
        sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        sock_addr.sin_port = htons(PORT);

        if(bind(listen_sockfd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) == -1)
        {
                perror("bind error ");
                return 1;
        }

        if(listen(listen_sockfd, 5) == -1)
        {
                perror("Error ");
                return 1;
        }

        while(1)
        {

		memset((void **)&client_addrs, 0x00, (3 * sizeof(client_addrs[0])));

		for(i = 0; i<3; i++)
		{
			addr_len[i] = sizeof(client_addrs[i]);
			client_sockfd[i] = accept(listen_sockfd, (struct sockaddr *)&client_addrs[i], &addr_len[i]);
			
			if(client_sockfd[i] == -1)
			{
				perror("Error ");
				return 1;
			}

			printf("New Client[%d] Connect: %s\n", i, inet_ntoa(client_addrs[i].sin_addr));
		}

		memset((void *)&result, 0x00, sizeof(result));

		result.max = 0;
		result.min = 2147483647;	//int형 최댓값
		sum = 0.0;

              	for(i = 0; i<3; i++)
		{
			memset(&buf, 0x00, sizeof(buf));

			if((n = read(client_sockfd[i], buf, MAXBUF)) <= 0)
			{
				close(client_sockfd[i]);

				perror("Error ");
				return 1;
			}

			num = atoi(buf);

			printf("from client[%d] : %d\n", i, num);
			
			if(result.max < num)
			{
				result.max = num;
				result.max_addr = client_addrs[i];
			}

			if(result.min > num)
			{
				result.min = num;
				result.min_addr = client_addrs[i];
			}

			sum = sum + num;
		}	
		
		avg = (sum /3.0);

		sprintf(result.avg, "%f", avg);

		result.max = htonl(result.max);
		result.min = htonl(result.min);
	
		//port(sin_port)는 2바이트(unsigned short)이므로 htons() 사용	
		result.min_addr.sin_port = htons(result.min_addr.sin_port);
		result.max_addr.sin_port = htons(result.max_addr.sin_port);

		for(i = 0; i<3; i++)
		{
			if (write(client_sockfd[i], (void *)&result, sizeof(result)) <=0)
			{
				perror("write error : ");
				close(client_sockfd[i]);
			}
			close(client_sockfd[i]);
		}
        }
        close(listen_sockfd);

        return 0;
}

