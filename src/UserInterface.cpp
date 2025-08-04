#include "UserInterface.hpp"
#include "SceneList.hpp"
#include "UserSettings.hpp"
#include "Utilities/Exception.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/FrameBuffer.hpp"
#include "Vulkan/Instance.hpp"
#include "Vulkan/RenderPass.hpp"
#include "Vulkan/SingleTimeCommands.hpp"
#include "Vulkan/Surface.hpp"
#include "Vulkan/SwapChain.hpp"
#include "Vulkan/Window.hpp"

#include <imgui.h>
#include <imgui_freetype.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <array>

namespace
{
	void CheckVulkanResultCallback(const VkResult err)
	{
		if (err != VK_SUCCESS)
		{
			Throw(std::runtime_error(std::string("ImGui Vulkan error (") + Vulkan::ToString(err) + ")"));
		}
	}
}

UserInterface::UserInterface(
	Vulkan::CommandPool& commandPool, 
	const Vulkan::SwapChain& swapChain, 
	const Vulkan::DepthBuffer& depthBuffer,
	UserSettings& userSettings) :
	userSettings_(userSettings)
{
	const auto& device = swapChain.Device();
	const auto& window = device.Surface().Instance().Window();

	// Initialise descriptor pool and render pass for ImGui.
	const std::vector<Vulkan::DescriptorBinding> descriptorBindings =
	{
		{0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0},
	};
	descriptorPool_.reset(new Vulkan::DescriptorPool(device, descriptorBindings, 1));
	renderPass_.reset(new Vulkan::RenderPass(swapChain, depthBuffer, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_LOAD));

	// Initialise ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Initialise ImGui GLFW adapter
	if (!ImGui_ImplGlfw_InitForVulkan(window.Handle(), true))
	{
		Throw(std::runtime_error("failed to initialise ImGui GLFW adapter"));
	}

	// Initialise ImGui Vulkan adapter
	ImGui_ImplVulkan_InitInfo vulkanInit = {};
	vulkanInit.Instance = device.Surface().Instance().Handle();
	vulkanInit.PhysicalDevice = device.PhysicalDevice();
	vulkanInit.Device = device.Handle();
	vulkanInit.QueueFamily = device.GraphicsFamilyIndex();
	vulkanInit.Queue = device.GraphicsQueue();
	vulkanInit.PipelineCache = nullptr;
	vulkanInit.DescriptorPool = descriptorPool_->Handle();
	vulkanInit.RenderPass = renderPass_->Handle();
	vulkanInit.MinImageCount = swapChain.MinImageCount();
	vulkanInit.ImageCount = static_cast<uint32_t>(swapChain.Images().size());
	vulkanInit.Allocator = nullptr;
	vulkanInit.CheckVkResultFn = CheckVulkanResultCallback;

	if (!ImGui_ImplVulkan_Init(&vulkanInit))
	{
		Throw(std::runtime_error("failed to initialise ImGui vulkan adapter"));
	}

	auto& io = ImGui::GetIO();

	// No ini file.
	io.IniFilename = nullptr;

	// Window scaling and style.
	const auto scaleFactor = window.ContentScale();

	ImGui::StyleColorsDark();
	ImGui::GetStyle().ScaleAllSizes(scaleFactor);

	// Upload ImGui fonts (use ImGuiFreeType for better font rendering, see https://github.com/ocornut/imgui/tree/master/misc/freetype).
	io.Fonts->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType();
	if (!io.Fonts->AddFontFromFileTTF("../assets/fonts/Cousine-Regular.ttf", 13 * scaleFactor))
	{
		Throw(std::runtime_error("failed to load ImGui font"));
	}

	Vulkan::SingleTimeCommands::Submit(commandPool, [] (VkCommandBuffer commandBuffer)
	{
		if (!ImGui_ImplVulkan_CreateFontsTexture())
		{
			Throw(std::runtime_error("failed to create ImGui font textures"));
		}
	});
}

UserInterface::~UserInterface()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void UserInterface::Render(VkCommandBuffer commandBuffer, const Vulkan::FrameBuffer& frameBuffer, const Statistics& statistics)
{
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplVulkan_NewFrame();
	ImGui::NewFrame();

	DrawSettings();
	DrawOverlay(statistics);
	DrawRenderDocDebugger(statistics);
	//ImGui::ShowStyleEditor();
	ImGui::Render();

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass_->Handle();
	renderPassInfo.framebuffer = frameBuffer.Handle();
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = renderPass_->SwapChain().Extent();
	renderPassInfo.clearValueCount = 0;
	renderPassInfo.pClearValues = nullptr;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	vkCmdEndRenderPass(commandBuffer);
}

bool UserInterface::WantsToCaptureKeyboard() const
{
	return ImGui::GetIO().WantCaptureKeyboard;
}

bool UserInterface::WantsToCaptureMouse() const
{
	return ImGui::GetIO().WantCaptureMouse;
}

void UserInterface::DrawSettings()
{
	if (!Settings().ShowSettings)
	{
		return;
	}

	// Modern UI style with larger main panel
	const float distance = 20.0f;
	const ImVec2 pos = ImVec2(distance, distance);
	const ImVec2 posPivot = ImVec2(0.0f, 0.0f);
	ImGui::SetNextWindowPos(pos, ImGuiCond_Always, posPivot);
	ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_Always);

	const auto flags =
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings;

	// Custom styling for modern look
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 8.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 12));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.12f, 0.95f));

	if (ImGui::Begin("🎮 Vulkan Gallery Renderer - Control Panel", &Settings().ShowSettings, flags))
	{
		// Header with project branding
		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Assuming first font is larger
		ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Graphics Debugging Studio");
		ImGui::PopFont();
		ImGui::Separator();

		std::vector<const char*> scenes;
		for (const auto& scene : SceneList::AllScenes)
		{
			scenes.push_back(scene.first.c_str());
		}

		const auto& window = descriptorPool_->Device().Surface().Instance().Window();

		// Scene Selection with modern style
		ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.4f, 1.0f), "🎬 Scene Selection");
		ImGui::Separator();
		ImGui::PushItemWidth(-1);
		if (ImGui::Combo("##SceneList", &Settings().SceneIndex, scenes.data(), static_cast<int>(scenes.size())))
		{
			// Scene changed
		}
		ImGui::PopItemWidth();
		ImGui::Spacing();

		// Ray Tracing Controls
		ImGui::TextColored(ImVec4(0.9f, 0.4f, 1.0f, 1.0f), "⚡ Ray Tracing Engine");
		ImGui::Separator();
		ImGui::Checkbox("🔥 Enable Real-time Ray Tracing", &Settings().IsRayTraced);
		ImGui::Checkbox("📈 Accumulate Samples", &Settings().AccumulateRays);
		
		uint32_t min = 1, max = 128;
		ImGui::Text("Samples per Pixel:");
		ImGui::SliderScalar("##Samples", ImGuiDataType_U32, &Settings().NumberOfSamples, &min, &max, "%d spp");
		
		min = 1, max = 32;
		ImGui::Text("Light Bounces:");
		ImGui::SliderScalar("##Bounces", ImGuiDataType_U32, &Settings().NumberOfBounces, &min, &max, "%d bounces");
		ImGui::Spacing();

		// Camera Controls
		ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "📷 Camera Settings");
		ImGui::Separator();
		ImGui::SliderFloat("Field of View", &Settings().FieldOfView, UserSettings::FieldOfViewMinValue, UserSettings::FieldOfViewMaxValue, "%.0f°");
		ImGui::SliderFloat("Aperture (DoF)", &Settings().Aperture, 0.0f, 1.0f, "f/%.2f");
		ImGui::SliderFloat("Focus Distance", &Settings().FocusDistance, 0.1f, 20.0f, "%.1f m");
		ImGui::Spacing();

		// Performance Profiler
		ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.4f, 1.0f), "🔥 Performance Profiler");
		ImGui::Separator();
		ImGui::Checkbox("🌡️  Show GPU Heatmap", &Settings().ShowHeatmap);
		ImGui::SliderFloat("Heatmap Scale", &Settings().HeatmapScale, 0.10f, 10.0f, "%.2fx", ImGuiSliderFlags_Logarithmic);
		ImGui::Spacing();

		// Controls Help
		ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "⌨️  Controls");
		ImGui::Separator();
		ImGui::BulletText("F1: Toggle this panel");
		ImGui::BulletText("F2: Toggle statistics");
		ImGui::BulletText("%c%c%c%c + SHIFT/CTRL: Camera movement", 
			std::toupper(window.GetKeyName(GLFW_KEY_W, 0)[0]),
			std::toupper(window.GetKeyName(GLFW_KEY_A, 0)[0]),
			std::toupper(window.GetKeyName(GLFW_KEY_S, 0)[0]),
			std::toupper(window.GetKeyName(GLFW_KEY_D, 0)[0]));
		ImGui::BulletText("Mouse: Camera rotation");
	}
	
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(4);
	ImGui::End();
}

void UserInterface::DrawOverlay(const Statistics& statistics)
{
	if (!Settings().ShowOverlay)
	{
		return;
	}

	const auto& io = ImGui::GetIO();
	const float distance = 20.0f;
	const ImVec2 pos = ImVec2(io.DisplaySize.x - distance, distance);
	const ImVec2 posPivot = ImVec2(1.0f, 0.0f);
	ImGui::SetNextWindowPos(pos, ImGuiCond_Always, posPivot);
	ImGui::SetNextWindowSize(ImVec2(380, 0), ImGuiCond_Always);

	const auto flags =
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings;

	// Modern styling for debugging panel
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 10));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.08f, 0.92f));

	if (ImGui::Begin("🔬 Graphics Debug Console", &Settings().ShowOverlay, flags))
	{
		// Header with professional branding
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "Professional Graphics Debugging");
		ImGui::Separator();

		// RenderDoc Integration - Most prominent section
		ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "🎯 RenderDoc Integration");
		ImGui::Separator();
		
		if (statistics.RenderDocAvailable)
		{
			// Status indicator with icon
			ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "✅ RenderDoc API: Connected");
			
			// Capture status with animated indicator
			if (statistics.RenderDocCapturing)
			{
				float time = (float)ImGui::GetTime();
				float alpha = 0.5f + 0.5f * sin(time * 6.0f); // Pulsing effect
				ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, alpha), "🔴 RECORDING FRAME");
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.8f), "(Ctrl+F12 to stop)");
			}
			else
			{
				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), "⚪ Ready for Capture");
			}
			
			// Professional control buttons
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.5f, 1.0f), "Capture Controls:");
			ImGui::BulletText("F12: Single Frame Capture");
			ImGui::BulletText("Ctrl+F12: Manual Recording Mode");
			ImGui::BulletText("Analysis: Open .rdc in RenderDoc");
			
			// Additional debugging info
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "💡 Debug Workflow:");
			ImGui::Text("  • Press F12 during interesting frames");
			ImGui::Text("  • Analyze GPU workload in RenderDoc");
			ImGui::Text("  • Profile draw calls & shaders");
			ImGui::Text("  • Optimize performance bottlenecks");
		}
		else
		{
			ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.4f, 1.0f), "❌ RenderDoc: Not Detected");
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Launch app through RenderDoc for debugging");
			ImGui::Spacing();
			ImGui::Text("To enable debugging:");
			ImGui::BulletText("Install RenderDoc from GitHub");
			ImGui::BulletText("Launch this app through RenderDoc");
			ImGui::BulletText("Or inject renderdoc.dll");
		}

		ImGui::Spacing();
		ImGui::Separator();

		// Performance Statistics
		ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.8f, 1.0f), "📊 Real-time Performance");
		ImGui::Separator();
		
		// Frame metrics with visual indicators
		ImGui::Text("Resolution: %dx%d", statistics.FramebufferSize.width, statistics.FramebufferSize.height);
		
		// FPS with color coding
		float fps = statistics.FrameRate;
		ImVec4 fpsColor = fps > 60 ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : 
		                  fps > 30 ? ImVec4(1.0f, 1.0f, 0.2f, 1.0f) : 
		                            ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
		ImGui::TextColored(fpsColor, "Frame Rate: %.1f fps", fps);
		
		// Ray tracing performance
		ImGui::Text("Ray Throughput: %.2f Gr/s", statistics.RayRate);
		ImGui::Text("Accumulated Samples: %u", statistics.TotalSamples);
		
		// Performance gauge visual
		ImGui::Spacing();
		float performance = fps / 60.0f; // Normalize to 60fps
		performance = performance > 1.0f ? 1.0f : performance;
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "GPU Performance:");
		ImGui::ProgressBar(performance, ImVec2(-1, 0), "");

		ImGui::Spacing();
		ImGui::Separator();
		
		// Professional footer
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.7f, 1.0f), "Graphics Programming Portfolio Project");
		ImGui::Text("Real-time Ray Tracing • Vulkan API • RenderDoc Integration");
	}
	
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(3);
	ImGui::End();
}

void UserInterface::DrawRenderDocDebugger(const Statistics& statistics)
{
	// Only show if RenderDoc is available and we want detailed debugging
	if (!statistics.RenderDocAvailable)
		return;

	const auto& io = ImGui::GetIO();
	const float distance = 20.0f;
	const ImVec2 pos = ImVec2(distance, io.DisplaySize.y - distance);
	const ImVec2 posPivot = ImVec2(0.0f, 1.0f);
	ImGui::SetNextWindowPos(pos, ImGuiCond_Always, posPivot);
	ImGui::SetNextWindowSize(ImVec2(500, 200), ImGuiCond_Always);

	const auto flags =
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings;

	// Professional debugging panel styling
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.02f, 0.02f, 0.95f));
	ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.8f, 0.1f, 0.1f, 0.8f));
	ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(1.0f, 0.2f, 0.2f, 0.9f));

	bool renderDocOpen = true;
	if (ImGui::Begin("🎯 RenderDoc Professional Debugger", &renderDocOpen, flags))
	{
		// Professional header
		ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "🔬 GPU Frame Analysis & Debugging Tools");
		ImGui::Separator();

		// Two column layout for professional appearance
		ImGui::Columns(2, "RenderDocColumns", true);
		
		// Left column - Capture controls
		ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "📸 Frame Capture");
		ImGui::Separator();
		
		if (statistics.RenderDocCapturing)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "🔴 RECORDING");
			ImGui::SameLine();
			float time = (float)ImGui::GetTime();
			int dots = ((int)(time * 2)) % 4;
			std::string recording = "ACTIVE";
			for (int i = 0; i < dots; ++i) recording += ".";
			ImGui::Text("%s", recording.c_str());
		}
		else
		{
			ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "✅ Ready for Capture");
		}

		ImGui::Spacing();
		ImGui::Text("Quick Actions:");
		ImGui::BulletText("F12: Instant frame grab");
		ImGui::BulletText("Ctrl+F12: Recording mode");
		ImGui::BulletText("ESC: Cancel recording");

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "💡 Pro Tips:");
		ImGui::Text("• Capture during complex scenes");
		ImGui::Text("• Focus on shader-heavy frames");
		ImGui::Text("• Monitor GPU memory usage");

		ImGui::NextColumn();

		// Right column - Analysis info
		ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "📊 Analysis Workflow");
		ImGui::Separator();
		
		ImGui::Text("After capturing:");
		ImGui::BulletText("Open .rdc file in RenderDoc");
		ImGui::BulletText("Analyze draw call hierarchy");
		ImGui::BulletText("Inspect shader performance");
		ImGui::BulletText("Check GPU resource usage");
		ImGui::BulletText("Profile render targets");

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 1.0f, 1.0f), "🎯 Debug Focus Areas:");
		ImGui::Text("• Ray tracing performance");
		ImGui::Text("• Acceleration structure builds");
		ImGui::Text("• Shader execution time");
		ImGui::Text("• Memory bandwidth usage");
		ImGui::Text("• Pipeline state validation");

		ImGui::Columns(1);
		ImGui::Separator();
		
		// Footer with professional branding
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.8f, 1.0f), 
			"Industry-standard graphics debugging • Professional development workflow");
	}
	
	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar(2);
	ImGui::End();
}
