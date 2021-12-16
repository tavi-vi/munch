#include <stdio.h>
#include <stdlib.h>
#include "util.h"

void
fatal(char *s, int i) {
    fputs(s, stderr);
    exit(i);
}

extern inline unsigned short *
ia(array a, int x, int y);

array
newArray(int x, int y) {
    array a = {
        .x = x,
        .y = y,
        .a = malloc(sizeof(unsigned short)*x*y)
    };
    int i;
    for(i=0; i<x*y; i++) {
        a.a[i] = ~0;
    }
    return a;
}
