#include "VersionRev.h"

#define STRAPPLICATIONNAME "QuiteRSS"
#define STRDATE           "2026-09-10\0"
#define STRPRODUCTVER     "0.90.2\0"

#define VERSION           0,90,2
#define PRODUCTVER        VERSION,0
#define FILEVER           VERSION,VCS_REVISION

#define _STRFILE_BUILD(n) #n
#define STRFILE_BUILD(n)  _STRFILE_BUILD(n)
#define STRFILEVER_FULL   STRPRODUCTVER "." STRFILE_BUILD(VCS_REVISION) "\0"
