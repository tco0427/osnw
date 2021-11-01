#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

void main()
{
	time_t t;
	struct tm *tm;
	char *str;

	t = time(NULL);
	tm = localtime(&t);
	//asctime()은 시간을 문자 스트링으로 변환하는 함수
	str = asctime(tm);

	str[strlen(str)-1] = '\0';
	
	printf("starting job by crontab at %s: 32180472\n", str); 
}

