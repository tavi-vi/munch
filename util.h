void fatal(char *s, int i);

typedef struct {
    int x, y;
    unsigned short *a;
} array;

typedef struct{ int x, y; } point;

inline unsigned short *
ia(array a, int x, int y) {
    return &a.a[x+y*a.x];
}

array newArray(int x, int y);
