#include "RenderDocManager.hpp"
#include "Console.hpp"
#include <iostream>
#include <sstream>

#ifdef WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace Utilities 
{
    RenderDocManager::RenderDocManager() : renderDocAPI_(nullptr)
    {
    }

    bool RenderDocManager::Initialize()
    {
        Console::Write(Severity::Info, []() {
            std::cout << "[RenderDoc] Initializing integration..." << std::endl;
        });
        
        // Try to load RenderDoc module
        void* module = LoadRenderDocModule();
        if (!module)
        {
            Console::Write(Severity::Warning, []() {
                std::cout << "[RenderDoc] Module not found - RenderDoc integration disabled" << std::endl;
                std::cout << "[RenderDoc] To use: Launch your app through RenderDoc or inject the DLL" << std::endl;
            });
            return false;
        }

        // Get the API function
        pRENDERDOC_GetAPI RENDERDOC_GetAPI = nullptr;
        
#ifdef WIN32
        RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress((HMODULE)module, "RENDERDOC_GetAPI");
#else
        RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(module, "RENDERDOC_GetAPI");
#endif

        if (!RENDERDOC_GetAPI)
        {
            Console::Write(Severity::Error, []() {
                std::cout << "[RenderDoc] Failed to get API function" << std::endl;
            });
            return false;
        }

        // Initialize the API
        int result = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&renderDocAPI_);
        if (result != 1 || !renderDocAPI_)
        {
            Console::Write(Severity::Error, []() {
                std::cout << "[RenderDoc] Failed to initialize API" << std::endl;
            });
            return false;
        }

        // Get version info
        int major, minor, patch;
        GetVersion(major, minor, patch);
        
        Console::Write(Severity::Info, [major, minor, patch]() {
            std::cout << "[RenderDoc] Successfully initialized!" << std::endl;
            std::cout << "[RenderDoc] Version: " << major << "." << minor << "." << patch << std::endl;
            std::cout << "[RenderDoc] Press F12 to capture current frame" << std::endl;
            std::cout << "[RenderDoc] Press Ctrl+F12 to start/stop manual capture" << std::endl;
        });
        
        return true;
    }

    void RenderDocManager::StartCapture()
    {
        if (!renderDocAPI_) return;
        
        if (IsCapturing())
        {
            Console::Write(Severity::Warning, []() {
                std::cout << "[RenderDoc] Capture already in progress" << std::endl;
            });
            return;
        }
        
        Console::Write(Severity::Info, []() {
            std::cout << "[RenderDoc] Starting frame capture..." << std::endl;
        });
        renderDocAPI_->StartFrameCapture(nullptr, nullptr);
        
        lastCaptureInfo_ = "Manual capture started";
    }

    void RenderDocManager::EndCapture()
    {
        if (!renderDocAPI_) return;
        
        if (!IsCapturing())
        {
            Console::Write(Severity::Warning, []() {
                std::cout << "[RenderDoc] No active capture to end" << std::endl;
            });
            return;
        }
        
        Console::Write(Severity::Info, []() {
            std::cout << "[RenderDoc] Ending frame capture" << std::endl;
        });
        uint32_t result = renderDocAPI_->EndFrameCapture(nullptr, nullptr);
        
        if (result == 1)
        {
            Console::Write(Severity::Info, []() {
                std::cout << "[RenderDoc] Frame captured successfully!" << std::endl;
            });
            lastCaptureInfo_ = "Manual capture completed successfully";
        }
        else
        {
            Console::Write(Severity::Error, []() {
                std::cout << "[RenderDoc] Frame capture failed" << std::endl;
            });
            lastCaptureInfo_ = "Manual capture failed";
        }
    }

    bool RenderDocManager::IsCapturing() const
    {
        if (!renderDocAPI_) return false;
        return renderDocAPI_->IsFrameCapturing() == 1;
    }

    void RenderDocManager::TriggerCapture()
    {
        if (!renderDocAPI_) return;
        
        Console::Write(Severity::Info, []() {
            std::cout << "[RenderDoc] Triggering single frame capture..." << std::endl;
        });
        renderDocAPI_->TriggerCapture();
        
        lastCaptureInfo_ = "Single frame capture triggered";
        Console::Write(Severity::Info, []() {
            std::cout << "[RenderDoc] Single frame captured!" << std::endl;
        });
    }

    void RenderDocManager::GetVersion(int& major, int& minor, int& patch) const
    {
        if (renderDocAPI_)
        {
            renderDocAPI_->GetAPIVersion(&major, &minor, &patch);
        }
        else
        {
            major = minor = patch = 0;
        }
    }

    void* RenderDocManager::LoadRenderDocModule()
    {
#ifdef WIN32
        // On Windows, RenderDoc injects itself as renderdoc.dll
        HMODULE module = GetModuleHandleA("renderdoc.dll");
        if (!module)
        {
            // Try alternative names
            module = GetModuleHandleA("renderdoc_app.dll");
        }
        return module;
#else
        // On Linux, try common RenderDoc library names
        void* module = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD);
        if (!module)
        {
            module = dlopen("librenderdoc.so.1", RTLD_NOW | RTLD_NOLOAD);
        }
        return module;
#endif
    }
}
