#pragma once

#include "volk.h"



enum { FRAMES_IN_FLIGHT = 2 };

typedef struct graphics_t
{
	VkDevice device;
	VkQueue graphics_queue;
	VkSwapchainKHR swapchain;
	VkExtent2D swapchain_extent;
	uint32_t swapchain_image_count;
	VkRenderPass render_pass;
	VkFramebuffer* framebuffers;
	VkSemaphore acquire_image_semaphores[FRAMES_IN_FLIGHT];
	VkSemaphore* present_semaphores;
	VkFence in_flight_fences[FRAMES_IN_FLIGHT];
	VkPipeline pipeline;
	VkCommandBuffer command_buffers[FRAMES_IN_FLIGHT];
	int32_t current_frame;
} graphics_t;

void graphics_init(HINSTANCE instance_handle, HWND window_handle, graphics_t* graphics);
void graphics_deinit(graphics_t* graphics);
void graphics_render(graphics_t* graphics);