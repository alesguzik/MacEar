typedef struct
{
    int         ref;            /* reference adc pitch point */
    short       size;           /* number of adc points in this point */
    short       dist;           /* distance to last frame in adc points */
    short       syncflag;       /* flag saying if frame is pitch sync */
    short       framesize;      /* spectral size of frame */
    short       *frame;         /* usual frame info entire spectrum */
    short       enersize;       /* number of energy points */
    short	padding;	/* Padding to make things work on Sun-4 */
    short       *ener;          /* usual energy info and extra space */
} SYNC;
