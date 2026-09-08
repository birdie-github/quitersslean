#include "VersionRev.h"

#define STRAPPLICATIONNAME "QuiteRss"
#define STRDATE           "2026-09-08\0"
#define STRPRODUCTVER     "0.90.1\0"

#define VERSION           0,90,1
#define PRODUCTVER        VERSION,0
#define FILEVER           VERSION,VCS_REVISION

#define _STRFILE_BUILD(n) #n
#define STRFILE_BUILD(n)  _STRFILE_BUILD(n)
#define STRFILEVER_FULL   STRPRODUCTVER "." STRFILE_BUILD(VCS_REVISION) "\0"
