#include "math_util.h"


int compare(int a, int b)
{
	if (a == b) return 0;
	else return (a < b ? -1 : 1);
}

int min(int a, int b)
{
	return (a < b ? a : b);
}

int max(int a, int b)
{
	return (a > b ? a : b);
}

