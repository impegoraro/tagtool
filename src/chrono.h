#ifndef CHRONO_H
#define CHRONO_H


/*
 * Chronograph. Calculates elapsed time.
 */
typedef struct { 
	unsigned long base_sec;
	int base_usec;
} chrono_t;


/*
 * Marks the base time
 */
void chrono_reset(chrono_t *c);

/*
 * Gets elapsed time in seconds
 */
unsigned long chrono_get_sec(const chrono_t *c);

/*
 * Gets elapsed time in milliseconds
 */
unsigned long chrono_get_msec(const chrono_t *c);

/*
 * Gets elapsed time in microseconds
 */
unsigned long chrono_get_usec(const chrono_t *c);



#endif
