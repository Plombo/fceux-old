#include <assert.h>

void DrawTextLineBG(uint8 *dest);
void DrawMessage(bool beforeMovie);
void FCEU_DrawRecordingStatus(uint8* XBuf);
void FCEU_DrawNumberRow(uint8 *XBuf, int *nstatus, int cur);
void DrawTextTrans(uint8 *dest, uint32 width, uint8 *textmsg, uint8 fgcolor);