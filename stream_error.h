#ifndef STREAM_ERROR_H
#define STREAM_ERROR_H

/* Error Codes */
enum {
    STREAM_EOF           = -1,  /* EOF encountered */
    STREAM_ERROR_ALLOC   = -2,  /* Error in allocating stream state */
    STREAM_ERROR_INPUT   = -3,  /* Error in upstream read */
    STREAM_ERROR_FAILURE = -4,  /* Error in stream logic */
    STREAM_ERROR_OUTPUT  = -4,  /* Error in downstream write */
};

#endif

