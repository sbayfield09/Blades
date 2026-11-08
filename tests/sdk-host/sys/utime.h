/* newlib provides this include; Linux puts the definition in <utime.h>.
   Only needed to compile unused SDK path functions in the host launch test. */
#ifndef BLADES_HOST_SYS_UTIME_H
#define BLADES_HOST_SYS_UTIME_H
#include <time.h>
struct utimbuf { time_t actime,modtime; };
#endif
