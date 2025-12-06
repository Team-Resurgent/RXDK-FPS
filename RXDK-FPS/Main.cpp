#include "FPS.h"
#include "Defines.h"
#include "Undocumented.h"

#include <xtl.h>

//-----------------------------------------------------------------------------
// Name: Process()
// Desc: Simple thread process to display message every 5 seconds
//-----------------------------------------------------------------------------

static void Process(PKSTART_ROUTINE StartRoutine, PVOID StartContext)
{
    unsigned char* FrameBuffer = (unsigned char*)StartContext;
    while (true)
    {
        FPS::UpdateFramebuffer(FrameBuffer);
        Sleep(1000);
    }
}

//-----------------------------------------------------------------------------
// Name: DxtEntry()
// Desc: The Plugin's entry point
//-----------------------------------------------------------------------------

void NTAPI DxtEntry(PULONG pfUnload)
{
    OutputDebugStringA("DxtEntry: Creating Thread\n");

    PVOID FrameBuffer = MmAllocateContiguousMemory(PITCH * HEIGHT);

    HANDLE handle;
    DWORD status = PsCreateSystemThreadEx(&handle, 0, 8192, 0, NULL, NULL, FrameBuffer, FALSE, FALSE, Process);

    //Do not unload if thread created ok
    *pfUnload = status >= 0 ? FALSE : TRUE;
}