#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

void main()
{
	int i=0;
	time_t t;
	struct tm *tm;

	while(i<100)
	{
		sleep(1);	//wait상태로 빠짐 이후 ready 갔다가 running
		i++;

		//현재 컴퓨터의 시각을 출력하는 명령
		t = time(NULL);
		tm = localtime(&t);
		printf("%d from background job at %s\n", i, asctime(tm)); 
	}
}
