#pragma once

#include "volk.h"
#include "mathutil.h"



// todo should move graphics_t to graphics.c
enum { FRAMES_IN_FLIGHT = 2 };

typedef struct vk_buffer_t
{
	VkBuffer buffer;
	VkDeviceMemory memory;
} vk_buffer_t;

typedef struct graphics_t
{
	VkDevice device;
	VkQueue graphics_queue;
	VkQueue transfer_queue;
	VkSwapchainKHR swapchain;
	VkExtent2D swapchain_extent;
	uint32_t swapchain_image_count;
	VkRenderPass render_pass;
	VkFramebuffer* framebuffers;
	VkSemaphore acquire_image_semaphores[FRAMES_IN_FLIGHT];
	VkSemaphore* present_semaphores;
	VkFence in_flight_fences[FRAMES_IN_FLIGHT];
	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;
	VkCommandBuffer command_buffers[FRAMES_IN_FLIGHT];
	int32_t current_frame;
	VkPhysicalDeviceMemoryProperties device_memory_properties;
	VkCommandPool graphics_command_pool;
	VkCommandPool transfer_command_pool;
	vk_buffer_t vertex_buffer;
	vk_buffer_t index_buffer;
	mat4_t projection_matrix;
} graphics_t;

void graphics_init(HINSTANCE instance_handle, HWND window_handle, graphics_t* graphics);
void graphics_deinit(graphics_t* graphics);
void graphics_render(graphics_t* graphics);