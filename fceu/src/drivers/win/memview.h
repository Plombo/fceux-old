void DoMemView();
void UpdateMemoryView(int draw_all);
void UpdateColorTable();
void ChangeMemViewFocus(int newEditingMode, int StartOffset,int EndOffset);

void ApplyPatch(int addr,int size, uint8* data);
void UndoLastPatch();
int GetFileData(uint32 offset);
int WriteFileData(uint32 offset,int data);
int GetRomFileSize();
void FlushUndoBuffer();

extern HWND hMemView;
extern int EditingMode;
