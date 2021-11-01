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
	str = asctime(tm);

	str[strlen(str)-1] = '\0';

	printf("starting job by crontab2 at %s: 김동규\n", str); 
}

