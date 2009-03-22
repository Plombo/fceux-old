#ifndef DEBUGGER_H
#define DEBUGGER_H

//#define GetMem(x) (((x < 0x2000) || (x >= 0x4020))?ARead[x](x):0xFF)
#include <windows.h>
//#include "debug.h"

// TODO: Maybe change breakpoint array to std::vector
// Maximum number of breakpoints supported
#define MAXIMUM_NUMBER_OF_BREAKPOINTS 64

// Return values for AddBreak
#define TOO_MANY_BREAKPOINTS 1
#define INVALID_BREAKPOINT_CONDITION 3

//extern volatile int userpause; //mbg merge 7/18/06 removed for merging
extern int scanline; //current scanline! :D
extern HWND hDebug;

extern int childwnd,numWPs; //mbg merge 7/18/06 had to make extern

void CenterWindow(HWND hwndDlg);
void DoPatcher(int address,HWND hParent);
void UpdatePatcher(HWND hwndDlg);
int GetEditHex(HWND hwndDlg, int id);

extern void AddBreakList();

void UpdateDebugger();
void DoDebug(uint8 halt);

extern bool inDebugger;

extern class DebugSystem {
public:
	DebugSystem();
	~DebugSystem();
	
	HFONT hFixedFont;
	static const int fixedFontWidth = 8;
	static const int fixedFontHeight = 14;
} *debugSystem;


#endif
