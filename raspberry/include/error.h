#ifndef WIFEEDER_ERROR_H
#define WIFEEDER_ERROR_H

#pragma once

#include <cstdint>

#define EUNKNOWN   ((int32_t)-23)
#define ECORRUPT   ((int32_t)-22)
#undef EACCES
#define EACCES     ((int32_t)-21)
#define EFORMAT    ((int32_t)-20)
#define EDUP       ((int32_t)-19)
#define ESQLEX     ((int32_t)-18)
#define ENULLPTR   ((int32_t)-17)
#define ENETWORK   ((int32_t)-16)
#define ENOTFOUND  ((int32_t)-15)
#define EUNDERFLOW ((int32_t)-14)
#undef EOVERFLOW
#define EOVERFLOW  ((int32_t)-13)
#define EBADRES    ((int32_t)-12)
#define ENETREG    ((int32_t)-11)
#define ETIMEOUT   ((int32_t)-10)
#define EMISMATCH  ((int32_t)-9)
#define EHW        ((int32_t)-8)
#define EARG       ((int32_t)-6)
#undef EBADF
#define EBADF      ((int32_t)-5)
#define EACK       ((int32_t)-3)
#define ENOTRDY    ((int32_t)-2)
#undef EWOULDBLOCK
#define EWOULDBLOCK ((int32_t)-1)
#define ENOERR     ((int32_t)0)

void error_critical(void);

#endif /* WIFEEDER_ERROR_H */
