#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <SDL2/SDL.h>
#include "util.h"

int maxX = 800, maxY=1024;

void __attribute__ ((noinline))
noop() {
    return;
}

void
killed() {
    exit(1);
}

int
isPrime(int p) {
    int i;
    for (i = 2; i < p; i++)
        if (i * (p / i) == p)
            return 0;
    return 1;
}

typedef enum {running=0, stopped=1} munchBlocked;
typedef struct {
    int x, j, dj;
    munchBlocked blocked;
} munchState;

int
cmpMunchState(munchState ms1, munchState ms2) {
    return ms1.x == ms2.x && ms1.j == ms2.j && ms1.dj == ms2.dj; // we ignore blocked. it doesn't matter
}

void
prevPrime(munchState *s) {
    s->x = 0;
    s->j = 0;
    if(s->dj > 1)
        do
            s->dj--;
        while(!isPrime(s->dj));
}

void
nextPrime(munchState *s) {
    s->x = 0;
    s->j = 0;
    do
        s->dj++;
    while(!isPrime(s->dj));
}

typedef enum {forward, reverse} direction;

point
forwardMunch(munchState *s) {
    int *x = &s->x, *j = &s->j, *dj = &s->dj;
    s->blocked = running;

    point result = {*x, (*x ^ *j)%maxY};
    if(++*x >= maxX) {
        *x=0;
        *j+=*dj; 
        if(*j >= *dj * maxY) {
            *j=0;
prime:
            (*dj)++;
            int i;
            for (i = 2; i < *dj; i++)
                if (i * (*dj / i) == *dj)
                    goto prime;
        }
    }
    return result;
}

point
reverseMunch(munchState *s) {
    int *x = &s->x, *j = &s->j, *dj = &s->dj;

    point result = {*x, (*x ^ *j)%maxY};
    if(--*x < 0) {
        *x=maxX-1;
        *j-=*dj; 
        if(*j < 0) {
            switch(*dj) {
            case 1:
                s->blocked = stopped;
                *j=0;
                *x=0;
                return result;
                break;
            case 2:
                s->dj = 1;
                break;
            default:
prime:
                (*dj)--;
                int i;
                for (i = 2; i < *dj; i++)
                    if (i * (*dj / i) == *dj)
                        goto prime;
                break;
            }
            *j=*dj * maxY - *dj;
        }
    }
    return result;
}

void updateLUT(SDL_Surface *s, unsigned char (*pc)[0x10000], Uint32 (*p)[0x10000], Uint32 *mask);

inline void
updateLUT(SDL_Surface *s, unsigned char (*pc)[0x10000], Uint32 (*p)[0x10000], Uint32 *mask) {
    int i;
    for(i=0; i<0x10000; i++)
        (*p)[i] = SDL_MapRGB(s->format, (*pc)[i], (*pc)[i], (*pc)[i]);

    Uint32 masks[4] = {0xFF, 0xFFFF, 0xFFFFFF, 0xFFFFFFFF};
    *mask = masks[s->format->BytesPerPixel-1];
}

int
munchMain() {
    int i;

    signal(SIGINT, killed);

    SDL_Window *w;
    SDL_Surface *s;
    if(SDL_Init(SDL_INIT_VIDEO))
        fatal("failed to initialize SDL", -1);
    
    w = SDL_CreateWindow("Munching Squares",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        maxX,
        maxY,
        SDL_WINDOW_RESIZABLE);
    if(!w)
        fatal("failed to create window", -1);
    
    s = SDL_GetWindowSurface(w);
    if(!s)
        fatal("failed to get window surface", -1);

    munchState ms = {0,0,1,running};
    array pa = newArray(maxX, maxY);
    
    int tailEnd = 0;
    unsigned char phosphorCurve[0x10000];
    {
        double n;
        for(i=0, n=0; i<=0xFFFF; i++, n+=1) {
            double n1=0.3, n2 = 0.066, j=0.50;
            double on1 = (255*j)/((n+1/n1)*n1);
            double on2 = (255*(-j+1))/((n+1/n2)*n2);
            phosphorCurve[i] = on1+on2;
            if(!phosphorCurve[i] && !tailEnd)
                tailEnd = i;
        }
    }
    Uint32 format;
    Uint32 phosphor[0x10000];
    Uint32 mask;

    SDL_LockSurface(s);
    updateLUT(s, &phosphorCurve, &phosphor, &mask);
    format = s->format->format;
    SDL_UnlockSurface(s);
    SDL_UpdateWindowSurface(w); // If we don't do this, the second rendered frame is black.

    int n=0, defaultPixelRate=250, actualPixelRate=defaultPixelRate, *pixelRate=&actualPixelRate;
    double t1=0, t2=0;
    double nanoMinFrameDelta = 1000000000/60;
    direction md = forward;
    for(;;) {
        double frameDelta;
        Uint64 start = SDL_GetPerformanceCounter();
        
        SDL_Event e;
        while(SDL_PollEvent(&e))
            switch(e.type) {
            case SDL_KEYDOWN:
                switch(e.key.keysym.sym) {
                case SDLK_HOME:
                    actualPixelRate = 250;
                    break;
                case SDLK_PAGEUP:
                    do
                        actualPixelRate+=100;
                    while(actualPixelRate<=0);
                    break;
                case SDLK_PAGEDOWN:
                    if(actualPixelRate>0)
                        actualPixelRate-=100;
                    break;
                case SDLK_UP:
                    do
                        actualPixelRate+=25;
                    while(actualPixelRate<=0);
                    break;
                case SDLK_DOWN:
                    if(actualPixelRate>0)
                        actualPixelRate-=25;
                    break;
                case SDLK_LEFT:
                    prevPrime(&ms);
                    break;
                case SDLK_RIGHT:
                    nextPrime(&ms);
                    break;
                case SDLK_r:
                    md = reverse;
                    break;
                case SDLK_f:
                    md = forward;
                    break;
                case SDLK_d:
                    printf("tailEnd=%d\n", tailEnd);
                    printf("pixelRate=%d\n", *pixelRate);
                    printf("x=%d j=%d dj=%d blocked=%d\n", ms.x, ms.j, ms.dj, ms.blocked);
                    printf("%f:%f::%f:%f\n", t1, t2, t1/(t1+t2), t2/(t1+t2));
                    break;
                case SDLK_SPACE:
                    if(!pixelRate) {
                        pixelRate = &actualPixelRate;
                    } else {
                        pixelRate = 0;
                    }
                }
                break;
            case SDL_WINDOWEVENT:
                switch(e.window.event) {
                case SDL_WINDOWEVENT_CLOSE:
                    return 0;
                    break;
                case SDL_WINDOWEVENT_RESIZED: {
                        munchState rs = ms;

                        // amount to reverse must be revAmt = ms.x + n*maxX where revAmt>tailEnd*(*pixelRate)
                        // first loop after reverse will have to be <pixelRate unless the stars align
                        //
                        // note: this isn't perfect, we assume that the user hasn't changed pixelRate or md within
                        // the last tailEnd*(*pixelRate) pixels.
                        // The alternative is blanking the screen, which is no fun.
                        int revAmt = 0;
                        switch(md) {
                        case forward:
                            for(;;) {
                                revAmt++;
                                reverseMunch(&rs);
                                if(rs.blocked || (rs.x == 0 && revAmt>tailEnd*(*pixelRate)))
                                    break;
                            }
                            break;
                        case reverse:
                            for(i=0; i<*pixelRate; i++)
                                forwardMunch(&rs);
                            while(!cmpMunchState(rs, ms)) {
                                point p = reverseMunch(&rs);
                                *(ia(pa, p.x, p.y)) = 0;
                            }
                            break;
                        }
                        s = SDL_GetWindowSurface(w);
                        if(!s)
                            fatal("failed to get window surface", -1);
                        free(pa.a);
                        maxX = e.window.data1;
                        maxY = e.window.data2;
                        pa = newArray(maxX, maxY);
                        div_t fwd = div(revAmt, *pixelRate);
                        switch(md) {
                        case forward:
                            for(i=0; i<fwd.rem; i++) {
                                point p = forwardMunch(&rs);
                                *(ia(pa, p.x, p.y)) = fwd.quot;
                            }
                            for(i=fwd.quot-1; i>=0; i--) {
                                int i2;
                                for(i2=0; i2<*pixelRate; i2++) {
                                    point p = forwardMunch(&rs);
                                    *(ia(pa, p.x, p.y)) = i;
                                }
                            }
                            break;
                        case reverse:
                            break;
                        }
                        break;
                    }
                }
                break;
            }

        switch(md) {
        case forward:
            for (i=0; pixelRate && i<*pixelRate; i++) {
                point p = forwardMunch(&ms);
                *(ia(pa, p.x, p.y)) = 0;
            }
            break;
        case reverse:
            for (i=0; !ms.blocked && pixelRate && i<*pixelRate; i++) {
                point p = reverseMunch(&ms);
                *(ia(pa, p.x, p.y)) = 0;
            }
            break;
        }
        unsigned short *pae;
        void *sre;
        for(i=0; i<maxX*maxY; i++) {
            unsigned short r = pa.a[i] + 1;
            if(r<pa.a[i])
                r = 0xFFFF;
            pa.a[i] = r;
        }
        SDL_LockSurface(s);

        if(s->format->format != format) {
            updateLUT(s, &phosphorCurve, &phosphor, &mask);
            format = s->format->format;
        }
        pae = pa.a;
        sre = s->pixels;

        // Almost certainly unnecessary optimization, but
        // I don't want to give special treatment to 32-bit pixels.
        switch(s->format->BytesPerPixel) {
        case 1:
            for(;pae<pa.a+maxX*maxY; pae++, sre+=s->format->BytesPerPixel)
                *(Uint8 *)sre = phosphor[*pae];
            break;
        case 2:
            for(;pae<pa.a+maxX*maxY; pae++, sre+=s->format->BytesPerPixel)
                *(Uint16 *)sre = phosphor[*pae];
            break;
        case 3:
            for(;pae<pa.a+maxX*maxY; pae++, sre+=s->format->BytesPerPixel) {
                Uint8 *vp = (Uint8 *)&phosphor[*pae];
                *(Uint8 *)(sre+0) = *(vp+0);
                *(Uint8 *)(sre+1) = *(vp+1);
                *(Uint8 *)(sre+2) = *(vp+2);
             }
            break;
        case 4:
            for(;pae<pa.a+maxX*maxY; pae++, sre+=4)
                *(Uint32 *)sre = phosphor[*pae];
            break;
        }
        Uint64 bullshit = SDL_GetPerformanceCounter();
        SDL_UnlockSurface(s);
        SDL_UpdateWindowSurface(w);
        Uint64 now = SDL_GetPerformanceCounter();
        frameDelta = (double)((now - start)*1000000000) / SDL_GetPerformanceFrequency();
        if(frameDelta < nanoMinFrameDelta) {
            struct timespec sleepTime = {0, nanoMinFrameDelta-frameDelta};
            nanosleep(&sleepTime, NULL);
            frameDelta = nanoMinFrameDelta;
        }

        {
            double d=2000;
            if(n<d) n++;
            t1 = t1+((double)(bullshit-start)-t1)/(n<d ? n : d);
            t2 = t2+((double)(now-bullshit)-t2)/(n<d ? n : d);
        }
    }
    return 0;
}

int
t0() {
    int i;
    munchState s = {0,0,1,running}, s1, s2, s3;
    for(i=0; i<1000000000; i++) {
        s1 = s;
        forwardMunch(&s);
        forwardMunch(&s);
        s2 = s;
        reverseMunch(&s);
        reverseMunch(&s);
        s3 = s;
        if(s1.x != s3.x || s1.j != s3.j || s1.dj != s3.dj) {
            printf("%d\n", i);
            printf("1 x=%d j=%d dj=%d blocked=%d\n", s1.x, s1.j, s1.dj, s1.blocked);
            printf("2 x=%d j=%d dj=%d blocked=%d\n", s2.x, s2.j, s2.dj, s1.blocked);
            printf("3 x=%d j=%d dj=%d blocked=%d\n", s3.x, s3.j, s3.dj, s1.blocked);
            return -1;
        }
        forwardMunch(&s);
    }
    return 0;
}

int
t1() {
    munchState s = {0,0,1,running};
    int x, j, dj, counter;
    for (dj = 1; ; ) {
        j = 0;
        do {
            for (x = 0; x < maxX; x++) {
                int y = (x ^ j) % maxY;
                point p = forwardMunch(&s);
                if(p.x != x || p.y != y) {
                    printf("t1 failed at n=%d\n", counter);
                    printf("original     y=%d x=%d j=%d dj=%d\n", y, x, j, dj);
                    printf("munchForward y=%d x=%d j=%d dj=%d\n", p.y, p.x, s.j, s.dj);
                    return 1;
                }
                if(counter++>1000000000)
                    return 0;
            }
            j += dj;
        } while (j != dj * maxY);
prime:
        dj++;
        for (j = 2; j < dj; j++)
            if (j*(dj/j) == dj)
                goto prime;
    }
}

int
t2() {
    munchState s = {487,190,1,running}, rs = s;
    int nback = 0;
    while(!rs.blocked) {
        reverseMunch(&rs);
        nback++;
        int i;
        munchState ss = rs;
        for(i=0; i<nback; i++)
            forwardMunch(&ss);
        if(!cmpMunchState(ss, s)) {
            printf("t2 failed");
            printf("s  x=%d j=%d dj=%d\n", s.x, s.j, s.dj);
            printf("ss x=%d j=%d dj=%d\n", ss.x, ss.j, ss.dj);
            return 1;
        }
    }
    return 0;
}

int
testMain() {
    int nTests = 2;
    int (*fs[2]) ();
    fs[0] = t0;
    fs[1] = t1;
    fs[2] = t2;

    int i;
    for(i=0; i<nTests; i++) {
        if(!fs[i])
            continue;
        int status = (*fs[i])();
        if(status)
            return status;
    }
    return 0;
}

int
main() {
#ifdef TEST
    return testMain();
#else
    return munchMain();
#endif
}
