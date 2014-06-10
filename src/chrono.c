#ifdef _WIN32
# include <sys/timeb.h>
#else
# include <time.h>
# include <sys/time.h>
#endif

#include "chrono.h"


#ifndef _WIN32

void chrono_reset(chrono_t *c)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	c->base_sec = tv.tv_sec;
	c->base_usec = tv.tv_usec;
}

unsigned long chrono_get_sec(const chrono_t *c)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return tv.tv_sec - c->base_sec;
}

unsigned long chrono_get_msec(const chrono_t *c)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return (tv.tv_sec - c->base_sec)*1000 + (tv.tv_usec - c->base_usec)/1000;
}

unsigned long chrono_get_usec(const chrono_t *c)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return (tv.tv_sec - c->base_sec)*1000000 + tv.tv_usec - c->base_usec;
}

#endif


#ifdef _WIN32

void chrono_reset(chrono_t *c)
{
	struct _timeb tv;
	_ftime(&tv);

	c->base_sec = tv.time;
	c->base_usec = tv.millitm * 1000;
}

unsigned long chrono_get_sec(const chrono_t *c)
{
	struct _timeb tv;
	_ftime(&tv);

	return tv.time - c->base_sec;
}

unsigned long chrono_get_msec(const chrono_t *c)
{
	struct _timeb tv;
	_ftime(&tv);

	return (tv.time - c->base_sec)*1000 + tv.millitm - c->base_usec/1000;
}

unsigned long chrono_get_usec(const chrono_t *c)
{
	struct _timeb tv;
	_ftime(&tv);

	return (tv.time - c->base_sec)*1000000 + tv.millitm*1000 - c->base_usec;
}

#endif
