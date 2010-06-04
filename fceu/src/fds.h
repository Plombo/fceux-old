#ifndef FDS_H
#define FDS_H
#include "x6502.h"
extern bool isFDS;
void FDSSoundReset(void);

void FCEU_FDSInsert(void);
//void FCEU_FDSEject(void);
void FCEU_FDSSelect(void);

int FCEU_FDSBiosHook(X6502 *xp);
#endif
