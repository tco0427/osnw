#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXBUF 1024

int main(int argc, char **argv)
{
    	int server_sockfd;
   	int client_sockfd[3];
	int client_len, n, loop;
	char buf[MAXBUF], result[MAXBUF];
	struct sockaddr_in clientaddr, serveraddr;
    
	client_len = sizeof(clientaddr);
    
	if ((server_sockfd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP )) == -1)
	{
		perror("socket error : ");
		exit(0);
	}
    
	memset(&serveraddr, 0x00, sizeof(serveraddr));
    
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(atoi(argv[1]));

	if(bind (server_sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1)
	{
		perror("bind error : ");
		exit(0);
	}
    
	listen(server_sockfd, 5);

	while(1)
	{
        	for(loop = 0; loop < 3; loop++){
			client_sockfd[loop] = accept(server_sockfd, (struct sockaddr *)&clientaddr, &client_len);
            		printf("New Client[%d] Connect: %s\n", loop, inet_ntoa(clientaddr.sin_addr));
		}
        
        	memset(result, 0x00, MAXBUF);
        
        	for(loop = 0; loop<3; loop++){
            		memset(buf, 0x00, MAXBUF);
            
            		if ((n = read(client_sockfd[loop], buf, MAXBUF)) <= 0)
            		{
                		close(client_sockfd[loop]);
                		continue;
            		}
            		printf("from clinet[%d] : %s\n", loop, buf);

                    buf[strlen(buf)-1] = ' ';
            		strcat(result,buf);
        	}
        	for(loop = 0; loop<3; loop++){
        		if (write(client_sockfd[loop], result, MAXBUF) <=0)
        		{
           			 perror("write error : ");
				 close(client_sockfd[loop]);
        		}
			close(client_sockfd[loop]);
		}
	}

	close(server_sockfd);
    
	return 0;
}

