#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "editor_app.h"
#include "editor_icon.h"
#include "editor_settings.h"

namespace {
	VkAllocationCallbacks* g_allocator = nullptr;
	VkInstance g_instance = VK_NULL_HANDLE;
	VkPhysicalDevice g_physical_device = VK_NULL_HANDLE;
	VkDevice g_device = VK_NULL_HANDLE;
	uint32_t g_queue_family = static_cast<uint32_t>(-1);
	VkQueue g_queue = VK_NULL_HANDLE;
	VkPipelineCache g_pipeline_cache = VK_NULL_HANDLE;
	VkDescriptorPool g_descriptor_pool = VK_NULL_HANDLE;
	ImGui_ImplVulkanH_Window g_main_window_data;
	int g_min_image_count = 2;
	bool g_swap_chain_rebuild = false;

	/**
	 * Handles Vulkan errors consistently and aborts on fatal failures.
	 * @param err Vulkan result code to check.
	 */
	void check_vk_result(const VkResult err) {
		if (err == VK_SUCCESS) {
			return;
		}
		std::fprintf(stderr, "Vulkan error: VkResult = %d\n", err);
		if (err < 0) {
			std::abort();
		}
	}

	/**
	 * Loads the editor UI font and merges FontAwesome glyphs when available.
	 */
	void load_fonts() {
		ImGuiIO& io = ImGui::GetIO();
		ImFontConfig base_config;
		base_config.SizePixels = 16.0f;
		io.Fonts->AddFontDefaultVector(&base_config);
#ifdef G13_FA_SOLID_FONT_PATH
		if (std::filesystem::exists(G13_FA_SOLID_FONT_PATH)) {
			ImFontConfig config;
			config.MergeMode = true;
			config.GlyphMinAdvanceX = 13.0f;
			io.Fonts->AddFontFromFileTTF(G13_FA_SOLID_FONT_PATH, 13.0f, &config);
		}
#endif
	}

	/**
	 * Finds a connected monitor by the display name stored in settings.
	 * @param name monitor name to find.
	 * @return matching monitor, or nullptr when unavailable.
	 */
	GLFWmonitor* find_monitor_by_name(const std::string& name) {
		if (name.empty()) {
			return nullptr;
		}

		int count = 0;
		GLFWmonitor** monitors = glfwGetMonitors(&count);
		for (int i = 0; i < count; ++i) {
			const char* monitor_name = glfwGetMonitorName(monitors[i]);
			if (monitor_name && name == monitor_name) {
				return monitors[i];
			}
		}
		return nullptr;
	}

	/**
	 * Finds the monitor with the largest work-area overlap for a window rectangle.
	 * @param x window x coordinate.
	 * @param y window y coordinate.
	 * @param width window width in pixels.
	 * @param height window height in pixels.
	 * @return monitor with the largest overlap, or nullptr when no monitor overlaps.
	 */
	GLFWmonitor* find_monitor_for_rect(const int x, const int y, const int width, const int height) {
		int count = 0;
		GLFWmonitor** monitors = glfwGetMonitors(&count);
		GLFWmonitor* best_monitor = nullptr;
		int best_area = 0;

		for (int i = 0; i < count; ++i) {
			int work_x = 0;
			int work_y = 0;
			int work_width = 0;
			int work_height = 0;
			glfwGetMonitorWorkarea(monitors[i], &work_x, &work_y, &work_width, &work_height);
			const int overlap_x = std::max(0, std::min(x + width, work_x + work_width) - std::max(x, work_x));
			const int overlap_y = std::max(0, std::min(y + height, work_y + work_height) - std::max(y, work_y));
			const int overlap_area = overlap_x * overlap_y;
			if (overlap_area > best_area) {
				best_area = overlap_area;
				best_monitor = monitors[i];
			}
		}
		return best_monitor;
	}

	/**
	 * Clamps restored window placement to a currently visible monitor work area.
	 * @param settings settings whose window placement fields are clamped in place.
	 */
	void clamp_window_placement(G13::Editor::EditorSettings& settings) {
		if (!settings.has_window_placement) {
			return;
		}

		GLFWmonitor* monitor = find_monitor_by_name(settings.monitor_name);
		if (!monitor) {
			monitor = find_monitor_for_rect(settings.window_x, settings.window_y,
					settings.window_width, settings.window_height);
		}
		if (!monitor) {
			monitor = glfwGetPrimaryMonitor();
		}
		if (!monitor) {
			return;
		}

		int work_x = 0;
		int work_y = 0;
		int work_width = 0;
		int work_height = 0;
		glfwGetMonitorWorkarea(monitor, &work_x, &work_y, &work_width, &work_height);
		if (work_width <= 0 || work_height <= 0) {
			return;
		}

		settings.window_width = std::min(std::max(settings.window_width, 800), work_width);
		settings.window_height = std::min(std::max(settings.window_height, 600), work_height);
		settings.window_x = std::clamp(settings.window_x, work_x, work_x + work_width - settings.window_width);
		settings.window_y = std::clamp(settings.window_y, work_y, work_y + work_height - settings.window_height);
	}

	/**
	 * Returns the best matching monitor name for the current window rectangle.
	 * @param window GLFW window to inspect.
	 * @return monitor name, or an empty string when no monitor is found.
	 */
	std::string monitor_name_for_window(GLFWwindow* window) {
		int window_x = 0;
		int window_y = 0;
		int window_width = 0;
		int window_height = 0;
		glfwGetWindowPos(window, &window_x, &window_y);
		glfwGetWindowSize(window, &window_width, &window_height);

		if (GLFWmonitor* monitor = find_monitor_for_rect(window_x, window_y, window_width, window_height)) {
			if (const char* name = glfwGetMonitorName(monitor)) {
				return name;
			}
		}
		return {};
	}

	/**
	 * Captures window position, size, and monitor name into settings.
	 * @param window GLFW window to inspect.
	 * @param settings settings object updated with the captured placement.
	 */
	void capture_window_placement(GLFWwindow* window, G13::Editor::EditorSettings& settings) {
		glfwGetWindowPos(window, &settings.window_x, &settings.window_y);
		glfwGetWindowSize(window, &settings.window_width, &settings.window_height);
		settings.monitor_name = monitor_name_for_window(window);
		settings.has_window_placement = true;
	}

	/**
	 * Applies the embedded editor icon images to the GLFW window.
	 * @param window GLFW window that receives the icons.
	 */
	void set_window_icon(GLFWwindow* window) {
		std::vector<GLFWimage> images;
		images.reserve(G13::Editor::EDITOR_ICONS.size());
		for (const auto& icon : G13::Editor::EDITOR_ICONS) {
			images.push_back(GLFWimage{icon.width, icon.height, const_cast<unsigned char*>(icon.pixels)});
		}
		glfwSetWindowIcon(window, static_cast<int>(images.size()), images.data());
	}

	/**
	 * Creates the Vulkan instance, device, queue, and descriptor pool.
	 * @param extensions Vulkan instance extensions required by GLFW.
	 * @param extension_count number of entries in extensions.
	 */
	void setup_vulkan(const char** extensions, uint32_t extension_count) {
		VkApplicationInfo app_info{};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = "g13_profile_editor";
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName = "No Engine";
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &app_info;
		create_info.enabledExtensionCount = extension_count;
		create_info.ppEnabledExtensionNames = extensions;
		check_vk_result(vkCreateInstance(&create_info, g_allocator, &g_instance));

		uint32_t gpu_count = 0;
		check_vk_result(vkEnumeratePhysicalDevices(g_instance, &gpu_count, nullptr));
		if (gpu_count == 0) {
			throw std::runtime_error("No Vulkan physical device found.");
		}
		std::vector<VkPhysicalDevice> gpus(gpu_count);
		check_vk_result(vkEnumeratePhysicalDevices(g_instance, &gpu_count, gpus.data()));
		g_physical_device = gpus[0];

		uint32_t queue_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(g_physical_device, &queue_count, nullptr);
		std::vector<VkQueueFamilyProperties> queues(queue_count);
		vkGetPhysicalDeviceQueueFamilyProperties(g_physical_device, &queue_count, queues.data());
		for (uint32_t i = 0; i < queue_count; ++i) {
			if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				g_queue_family = i;
				break;
			}
		}
		if (g_queue_family == static_cast<uint32_t>(-1)) {
			throw std::runtime_error("No Vulkan graphics queue family found.");
		}

		const char* device_extensions[] = {"VK_KHR_swapchain"};
		constexpr float queue_priority = 1.0f;
		VkDeviceQueueCreateInfo queue_info{};
		queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_info.queueFamilyIndex = g_queue_family;
		queue_info.queueCount = 1;
		queue_info.pQueuePriorities = &queue_priority;

		VkDeviceCreateInfo device_info{};
		device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_info.queueCreateInfoCount = 1;
		device_info.pQueueCreateInfos = &queue_info;
		device_info.enabledExtensionCount = 1;
		device_info.ppEnabledExtensionNames = device_extensions;
		check_vk_result(vkCreateDevice(g_physical_device, &device_info, g_allocator, &g_device));
		vkGetDeviceQueue(g_device, g_queue_family, 0, &g_queue);

		const VkDescriptorPoolSize pool_sizes[] = {
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
		};
		VkDescriptorPoolCreateInfo pool_info{};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000;
		pool_info.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
		pool_info.pPoolSizes = pool_sizes;
		check_vk_result(vkCreateDescriptorPool(g_device, &pool_info, g_allocator, &g_descriptor_pool));
	}

	/**
	 * Creates or resizes the Vulkan swapchain resources for the main window.
	 * @param wd ImGui Vulkan window data to initialize or resize.
	 * @param surface Vulkan surface backing the window.
	 * @param width framebuffer width in pixels.
	 * @param height framebuffer height in pixels.
	 */
	void setup_vulkan_window(ImGui_ImplVulkanH_Window* wd, const VkSurfaceKHR surface, int width, int height) {
		wd->Surface = surface;
		const VkFormat request_formats[] = {
				VK_FORMAT_B8G8R8A8_UNORM,
				VK_FORMAT_R8G8B8A8_UNORM,
				VK_FORMAT_B8G8R8_UNORM,
				VK_FORMAT_R8G8B8_UNORM,
		};
		wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(g_physical_device, wd->Surface,
																  request_formats, std::size(request_formats),
																  VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
		const VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_FIFO_KHR};
		wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(g_physical_device, wd->Surface,
															  present_modes, std::size(present_modes));
		ImGui_ImplVulkanH_CreateOrResizeWindow(g_instance, g_physical_device, g_device, wd, g_queue_family,
											   g_allocator, width, height, g_min_image_count,
											   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	}

	/**
	 * Releases Vulkan resources tied to the main window swapchain.
	 */
	void cleanup_vulkan_window() {
		ImGui_ImplVulkanH_DestroyWindow(g_instance, g_device, &g_main_window_data, g_allocator);
	}

	/**
	 * Releases process-wide Vulkan objects owned by the editor.
	 */
	void cleanup_vulkan() {
		vkDestroyDescriptorPool(g_device, g_descriptor_pool, g_allocator);
		vkDestroyDevice(g_device, g_allocator);
		vkDestroyInstance(g_instance, g_allocator);
	}

	/**
	 * Records and submits one ImGui frame to the Vulkan command queue.
	 * @param wd ImGui Vulkan window data used for rendering.
	 * @param draw_data ImGui draw data for the current frame.
	 */
	void frame_render(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data) {
		VkResult err;
		VkSemaphore image_acquired_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
		VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
		err = vkAcquireNextImageKHR(g_device, wd->Swapchain, UINT64_MAX, image_acquired_semaphore,
									VK_NULL_HANDLE, &wd->FrameIndex);
		if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
			g_swap_chain_rebuild = true;
			return;
		}
		check_vk_result(err);

		ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
		check_vk_result(vkWaitForFences(g_device, 1, &fd->Fence, VK_TRUE, UINT64_MAX));
		check_vk_result(vkResetFences(g_device, 1, &fd->Fence));
		check_vk_result(vkResetCommandPool(g_device, fd->CommandPool, 0));

		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		check_vk_result(vkBeginCommandBuffer(fd->CommandBuffer, &begin_info));

		VkRenderPassBeginInfo render_pass_info{};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = wd->RenderPass;
		render_pass_info.framebuffer = fd->Framebuffer;
		render_pass_info.renderArea.extent.width = wd->Width;
		render_pass_info.renderArea.extent.height = wd->Height;
		VkClearValue clear_value{};
		clear_value.color.float32[0] = 0.10f;
		clear_value.color.float32[1] = 0.11f;
		clear_value.color.float32[2] = 0.12f;
		clear_value.color.float32[3] = 1.0f;
		render_pass_info.clearValueCount = 1;
		render_pass_info.pClearValues = &clear_value;
		vkCmdBeginRenderPass(fd->CommandBuffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
		ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);
		vkCmdEndRenderPass(fd->CommandBuffer);
		check_vk_result(vkEndCommandBuffer(fd->CommandBuffer));

		VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = &image_acquired_semaphore;
		submit_info.pWaitDstStageMask = &wait_stage;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &fd->CommandBuffer;
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = &render_complete_semaphore;
		check_vk_result(vkQueueSubmit(g_queue, 1, &submit_info, fd->Fence));
	}

	/**
	 * Presents the rendered swapchain image and marks rebuilds when needed.
	 * @param wd ImGui Vulkan window data whose swapchain should be presented.
	 */
	void frame_present(ImGui_ImplVulkanH_Window* wd) {
		if (g_swap_chain_rebuild) {
			return;
		}
		VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
		VkPresentInfoKHR info{};
		info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		info.waitSemaphoreCount = 1;
		info.pWaitSemaphores = &render_complete_semaphore;
		info.swapchainCount = 1;
		info.pSwapchains = &wd->Swapchain;
		info.pImageIndices = &wd->FrameIndex;
		const VkResult err = vkQueuePresentKHR(g_queue, &info);
		if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
			g_swap_chain_rebuild = true;
			return;
		}
		check_vk_result(err);
		wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount;
	}
}

/**
 * Launches the Dear ImGui/Vulkan profile editor.
 * @return process exit status.
 */
int main() {
	glfwSetErrorCallback([](int error, const char* description) {
		std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
	});

	if (!glfwInit()) {
		return 1;
	}
	if (!glfwVulkanSupported()) {
		std::fprintf(stderr, "GLFW reports Vulkan is not supported.\n");
		return 1;
	}

	uint32_t extension_count = 0;
	const char** extensions = glfwGetRequiredInstanceExtensions(&extension_count);
	setup_vulkan(extensions, extension_count);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	auto startup_settings = G13::Editor::load_editor_settings();
	clamp_window_placement(startup_settings);
	GLFWwindow* window = glfwCreateWindow(startup_settings.window_width, startup_settings.window_height,
			"G13 Profile Editor", nullptr, nullptr);
	set_window_icon(window);
	if (startup_settings.has_window_placement) {
		glfwSetWindowPos(window, startup_settings.window_x, startup_settings.window_y);
	}

	VkSurfaceKHR surface = VK_NULL_HANDLE;
	check_vk_result(glfwCreateWindowSurface(g_instance, window, g_allocator, &surface));
	int width = 0;
	int height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	setup_vulkan_window(&g_main_window_data, surface, width, height);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	std::filesystem::create_directories(G13::Editor::editor_config_dir());
	static const std::string imgui_ini_path = G13::Editor::editor_imgui_ini_path().string();
	io.IniFilename = imgui_ini_path.c_str();
	load_fonts();

	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForVulkan(window, true);
	ImGui_ImplVulkan_InitInfo init_info{};
	init_info.Instance = g_instance;
	init_info.PhysicalDevice = g_physical_device;
	init_info.Device = g_device;
	init_info.QueueFamily = g_queue_family;
	init_info.Queue = g_queue;
	init_info.PipelineCache = g_pipeline_cache;
	init_info.DescriptorPool = g_descriptor_pool;
	init_info.PipelineInfoMain.RenderPass = g_main_window_data.RenderPass;
	init_info.PipelineInfoMain.Subpass = 0;
	init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.MinImageCount = g_min_image_count;
	init_info.ImageCount = g_main_window_data.ImageCount;
	init_info.Allocator = g_allocator;
	init_info.CheckVkResultFn = check_vk_result;
	ImGui_ImplVulkan_Init(&init_info);

	G13::Editor::EditorApp app;
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		int framebuffer_width = 0;
		int framebuffer_height = 0;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		if (framebuffer_width > 0 && framebuffer_height > 0 &&
			(g_swap_chain_rebuild || g_main_window_data.Width != framebuffer_width ||
			 g_main_window_data.Height != framebuffer_height)) {
			ImGui_ImplVulkan_SetMinImageCount(g_min_image_count);
			ImGui_ImplVulkanH_CreateOrResizeWindow(g_instance, g_physical_device, g_device,
												   &g_main_window_data, g_queue_family, g_allocator,
												   framebuffer_width, framebuffer_height, g_min_image_count,
												   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
			g_main_window_data.FrameIndex = 0;
			g_swap_chain_rebuild = false;
		}

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		app.draw();

		ImGui::Render();
		frame_render(&g_main_window_data, ImGui::GetDrawData());
		frame_present(&g_main_window_data);
	}

	auto shutdown_settings = app.settings();
	capture_window_placement(window, shutdown_settings);
	G13::Editor::save_editor_settings(shutdown_settings);

	check_vk_result(vkDeviceWaitIdle(g_device));
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	cleanup_vulkan_window();
	cleanup_vulkan();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
