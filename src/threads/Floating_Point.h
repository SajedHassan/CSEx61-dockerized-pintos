#ifndef FLOATING_POINT_H
#define FLOATING_POINT_H

#define FF 16384  // 2^14 as a 17.14 fixed-point number representation

typedef int Float;
/*Convert n to fixed point*/
#define CONVERT_INT_TO_FIXEDPOINT(n) (n*FF)
/*Convert x to integer (rounding toward zero)*/
#define CONVERT_FIXEDPOINT_TO_INT(x) (x/FF) 

#define ROUND_FLOATING_TO_NEAREST_INT(x) ((x>=0)?(x+FF/2):(x-FF/2))

#define ADD_TWO_FLOATING_NUMBERS(x,y) (x+y)
/* Subtract two floating values. */
#define SUB_TWO_FLOATING_NUMBERS(x,y) (x-y)
/* Add a floating value x and an integer value n. */
#define ADD_FLOATING_TO_INT(x,n)      (x+n*FF)
/* subtract a floating value x and an integer value n. */
#define SUB_FLOATING_TO_INT(x,n)      (x-n*FF)
/* Multiply a floating value x by an integer value n. */
#define MUL_FLOATING_WITH_INT(x,n)      (x*n)
/* Divide a floating value x by an integer value n. */
#define DIV_FLOATING_WITH_INT(x,n)      (x/n)
/* Multiply two floating values. */
#define MUL_TWO_FLOATING_NUMBERS(x,y)   (((int64_t) x) * y / FF)
/* dIVIDE two floating values. */
#define DIV_TWO_FLOATING_NUMBERS(x,y) (((int64_t) x) * FF / y)


#endif 