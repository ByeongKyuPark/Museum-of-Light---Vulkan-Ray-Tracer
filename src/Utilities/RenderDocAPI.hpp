#pragma once

#include <stdint.h>

#ifdef WIN32
#define RENDERDOC_CC __cdecl
#else
#define RENDERDOC_CC
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum RENDERDOC_Version {
    eRENDERDOC_API_Version_1_1_2 = 10102,
} RENDERDOC_Version;

// Function pointer types
typedef int (RENDERDOC_CC *pRENDERDOC_GetAPIVersion)(int* major, int* minor, int* patch);
typedef void (RENDERDOC_CC *pRENDERDOC_StartFrameCapture)(void* device, void* wndHandle);
typedef uint32_t (RENDERDOC_CC *pRENDERDOC_EndFrameCapture)(void* device, void* wndHandle);
typedef uint32_t (RENDERDOC_CC *pRENDERDOC_IsFrameCapturing)();
typedef void (RENDERDOC_CC *pRENDERDOC_TriggerCapture)();

typedef struct RENDERDOC_API_1_1_2
{
    pRENDERDOC_GetAPIVersion GetAPIVersion;
    
    // We'll use these for manual frame capture
    pRENDERDOC_StartFrameCapture StartFrameCapture;
    pRENDERDOC_EndFrameCapture EndFrameCapture;
    pRENDERDOC_IsFrameCapturing IsFrameCapturing;
    pRENDERDOC_TriggerCapture TriggerCapture;
    
    // Padding for other functions we don't need
    void* padding[20];
} RENDERDOC_API_1_1_2;

typedef int (RENDERDOC_CC *pRENDERDOC_GetAPI)(RENDERDOC_Version version, void **outAPIPointers);

#ifdef __cplusplus
}
#endif
