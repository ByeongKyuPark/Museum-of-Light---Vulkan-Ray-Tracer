#pragma once

#include "RenderDocAPI.hpp"
#include <memory>
#include <string>

namespace Utilities 
{
    /**
     * @brief RenderDoc integration for graphics debugging and profiling
     * 
     * This class provides an easy interface to RenderDoc's programmatic API
     * for capturing frames, which is essential for graphics programming in
     * professional game development and graphics software companies.
     * 
     * Usage:
     * - Initialize once at application startup
     * - Call StartCapture() before rendering interesting frames
     * - Call EndCapture() to save the frame data
     * - Use TriggerCapture() for single-frame captures
     */
    class RenderDocManager
    {
    public:
        RenderDocManager();
        ~RenderDocManager() = default;

        // Initialize RenderDoc API connection
        bool Initialize();
        
        // Check if RenderDoc is available and connected
        bool IsAvailable() const { return renderDocAPI_ != nullptr; }
        
        // Manual frame capture control
        void StartCapture();
        void EndCapture();
        bool IsCapturing() const;
        
        // Single frame capture (easier to use)
        void TriggerCapture();
        
        // Get RenderDoc version info
        void GetVersion(int& major, int& minor, int& patch) const;
        
        // Get capture statistics
        const std::string& GetLastCaptureInfo() const { return lastCaptureInfo_; }

    private:
        RENDERDOC_API_1_1_2* renderDocAPI_;
        std::string lastCaptureInfo_;
        
        // Platform-specific module loading
        void* LoadRenderDocModule();
    };
}
