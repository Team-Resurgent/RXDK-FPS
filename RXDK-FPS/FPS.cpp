#include "FPS.h"
#include "Defines.h"
#include "Undocumented.h"

#include <xtl.h>
#include "font.h"
#include <xbdm.h>
#include <stdio.h>

namespace
{
    LONGLONG lastFrame = 0;
}

void FPS::UpdateFramebuffer(unsigned char* framebuffer)
{
    HANDLE h;
    HRESULT hr = DmOpenPerformanceCounter("frames", &h);
    if(!SUCCEEDED(hr)) {
        return;
    }
    
    // Read counter
    DM_COUNTDATA count_data;
    hr = DmQueryPerformanceCounterHandle(h, 0x11, &count_data);
    DmClosePerformanceCounter(h);

    // Get frame info, but avoid frame 0
    LONGLONG frame = count_data.CountValue.QuadPart;
    if (frame == 0) {
        return;
    }

    // Bootstrap counter
    if (lastFrame == 0) {
        lastFrame = frame;
        return;
    }

    // Calculate number of frames since last frame
    LONGLONG delta = frame - lastFrame;
    lastFrame = frame;

    for (int i = 0; i < WIDTH * HEIGHT; i++) 
    {
        framebuffer[i * 2 + 0] = 0x00;
        framebuffer[i * 2 + 1] = 0x7f;
    }

    MEMORYSTATUS memStatus;
	memset(&memStatus, 0, sizeof(memStatus));
	GlobalMemoryStatus(&memStatus);

    char message[256]; 
    if (delta > 99999) {
        sprintf(message, "RXDK FPS");
    }
    else 
    {
        sprintf(message, "FPS: %llu MEM: %luMB/%luMB",
            delta,
            (ULONG)(memStatus.dwAvailPhys / (1024 * 1024)),
            (ULONG)(memStatus.dwTotalPhys / (1024 * 1024)));
    }

    int x = 0;
    for (int i = 0; i < (int)strlen(message); i++)
    {
        int gap = WIDTH - x;
        if (gap < 8) 
        {
            break;
        }

        int symbol = message[i] - ' ';
        if (symbol >= 96) 
        {
            symbol = 0;
        }

        int symbol_col = (symbol & 0xf);
        int symbol_row = (symbol >> 4) & 0xf;
        int symbol_offset = symbol_col + ((symbol_row << 4) * 13);

        for (int y = 0; y < 13; y++)
        {
            unsigned char* fb_row = &framebuffer[(y * PITCH) + (x << 1)];
            const unsigned char* font_row = &FontImageData[(y << 4) + symbol_offset];
            for (int dx = 0; dx < 8; dx++)
            {
                fb_row[dx << 1] = ((font_row[0] >> (7 - dx)) & 0x1) * 0xFF;
            }
        }

        x += 8;
    }

    int width = x - 1;
    int height = HEIGHT;

    // Enable PVIDEO
    {
        unsigned long base = 0xFD000000 + NV_PVIDEO;

        *(volatile unsigned long*)(base + NV_PVIDEO_STOP) = 0; // Stop PVIDEO
        *(volatile unsigned long*)(base + NV_PVIDEO_INTR_EN) = 0; // Disable interrupts
        *(volatile unsigned long*)(base + NV_PVIDEO_INTR) = *(volatile unsigned long*)(base + NV_PVIDEO_INTR); // Clear interrupts

        *(volatile unsigned long*)(base + NV_PVIDEO_LUMINANCE) = (0x0 << 16) | 0x1000;
        *(volatile unsigned long*)(base + NV_PVIDEO_CHROMINANCE) = (0x0 << 16) | 0x1000;

        *(volatile unsigned long*)(base + NV_PVIDEO_BASE) = 0x00000000;
        *(volatile unsigned long*)(base + NV_PVIDEO_LIMIT) = 0xFFFFFFFF;
        *(volatile unsigned long*)(base + NV_PVIDEO_OFFSET) = (unsigned long)MmGetPhysicalAddress(framebuffer);

        *(volatile unsigned long*)(base + NV_PVIDEO_POINT_IN) = (0 << 16) | 0;
        *(volatile unsigned long*)(base + NV_PVIDEO_SIZE_IN) = (HEIGHT << 16) | width;

        *(volatile unsigned long*)(base + NV_PVIDEO_POINT_OUT) = (OUT_Y << 16) | OUT_X;
        *(volatile unsigned long*)(base + NV_PVIDEO_SIZE_OUT) = (height << 16) | width;

        *(volatile unsigned long*)(base + NV_PVIDEO_DS_DX) = (width << 20) / width;
        *(volatile unsigned long*)(base + NV_PVIDEO_DT_DY) = (height << 20) / height;

        *(volatile unsigned long*)(base + NV_PVIDEO_FORMAT) = NV_PVIDEO_FORMAT_MATRIX | (NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8 << 16) | PITCH;

        *(volatile unsigned long*)(base + NV_PVIDEO_BUFFER) = NV_PVIDEO_BUFFER_0_USE;
    }

    // Flush all caches
    __asm wbinvd;
}