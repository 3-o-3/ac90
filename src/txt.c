#include "ac90.h"

int txt__ends_with(char *end, char *str)
{
	int le = strlen(end);
	int ls = strlen(str);
	if (le > ls) return 0;
	return !strcmp(end, str + ls - le);
}
