#ifndef MATH_UTIL_H
#define MATH_UTIL_H


/* 
 * Compares two integers in 'strcmp' style:
 * if a<b, returns -1 
 * if a=b, returns 0
 * if a>b, returns 1
 */
int compare(int a, int b);

/* 
 * Minimum of two signed integers
 */
int min(int a, int b);

/* 
 * Maximum of two signed integers
 */
int max(int a, int b);


#endif
