#include "graphics.h"

#include <stdio.h>
#include <Windows.h>

#include "assert.h"
#include "file.h"
#include "types.h"



typedef struct vertex_t
{
	float32_t position[3];
	float32_t colour[3];
} vertex_t;

vertex_t vertices[3] = {
	{.position = {0.0f, -0.5f, 0.0f}, .colour = {1.0f, 0.0f, 0.0f}},
	{.position = {-0.5f, 0.5f, 0.0f}, .colour = {0.0f, 1.0f, 0.0f}},
	{.position = {0.0f, 0.5f, 0.0f}, .colour = {0.0f, 0.0f, 1.0f}}
};
uint16_t indices[3] = { 0,1,2 };

#define DEBUG_BUFFER_SIZE 1024

static char* g_debug_buffer = NULL;

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	snprintf(g_debug_buffer, DEBUG_BUFFER_SIZE, "[Vulkan] %s\n", pCallbackData->pMessage);
	OutputDebugStringA(g_debug_buffer);
	return VK_FALSE; // don't abort the call
}

static uint32_t find_memory_type_index(VkPhysicalDeviceMemoryProperties* device_memory_properties, VkMemoryPropertyFlags desired_memory_properties, uint32_t memory_type_bits)
{
	for (uint32_t i = 0; i < device_memory_properties->memoryTypeCount; ++i)
	{
		if ((device_memory_properties->memoryTypes[i].propertyFlags & desired_memory_properties) == desired_memory_properties &&
			memory_type_bits & (1 << i))
		{
			return i;
		}
	}

	return -1;
}

static VkDeviceMemory alloc_device_memory(graphics_t* graphics, VkDeviceSize size, VkMemoryPropertyFlags desired_memory_properties, uint32_t memory_type_bits)
{
	// todo proper allocator
	VkMemoryAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = size,
		.memoryTypeIndex = find_memory_type_index(&graphics->device_memory_properties, desired_memory_properties, memory_type_bits)
	};

	VkDeviceMemory memory;
	VkResult result = vkAllocateMemory(graphics->device, &alloc_info, NULL, &memory);
	assert(result == VK_SUCCESS);

	return memory;
}

static vk_buffer_t create_buffer(graphics_t* graphics, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags desired_memory_properties)
{
	VkBufferCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	VkBuffer buffer;
	VkResult result = vkCreateBuffer(graphics->device, &create_info, NULL, &buffer);
	assert(result == VK_SUCCESS);

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(graphics->device, buffer, &memory_requirements);

	VkDeviceMemory memory = alloc_device_memory(graphics, memory_requirements.size, desired_memory_properties, memory_requirements.memoryTypeBits);

	result = vkBindBufferMemory(graphics->device, buffer, memory, 0);
	assert(result == VK_SUCCESS);

	return (vk_buffer_t) {
		.buffer = buffer,
		.memory = memory
	};
}

static void copy_buffer(graphics_t* graphics, VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = graphics->transfer_command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};
	VkCommandBuffer cmd;
	VkResult result = vkAllocateCommandBuffers(graphics->device, &alloc_info, &cmd);
	assert(result == VK_SUCCESS);

	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	result = vkBeginCommandBuffer(cmd, &begin_info);
	assert(result == VK_SUCCESS);

	VkBufferCopy copy = { .size = size };
	vkCmdCopyBuffer(cmd, src, dst, 1, &copy);

	result = vkEndCommandBuffer(cmd);
	assert(result == VK_SUCCESS);

	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmd
	};
	result = vkQueueSubmit(graphics->transfer_queue, 1, &submit_info, VK_NULL_HANDLE);
	assert(result == VK_SUCCESS);
	result = vkQueueWaitIdle(graphics->transfer_queue);
	assert(result == VK_SUCCESS);

	vkFreeCommandBuffers(graphics->device, graphics->transfer_command_pool, 1, &cmd);
}

void graphics_init(HINSTANCE instance_handle, HWND window_handle, graphics_t* graphics)
{
	g_debug_buffer = malloc(DEBUG_BUFFER_SIZE);

	VkResult result = volkInitialize();
	assert(result == VK_SUCCESS);

	memset(graphics, 0, sizeof(graphics_t));

	VkInstance instance;
	{
		VkApplicationInfo application_info = {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = NULL,
			.pApplicationName = "hmcs",
			.applicationVersion = 0,
			.pEngineName = "hmcs",
			.engineVersion = 0,
			.apiVersion = VK_API_VERSION_1_1
		};

		const char* layers[] = {
			"VK_LAYER_KHRONOS_validation"
		};

		const char* extensions[] = {
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME    // needed for validation output
		};

		VkInstanceCreateInfo create_info = {
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.pApplicationInfo = &application_info,
			.enabledLayerCount = sizeof(layers) / sizeof(layers[0]),
			.ppEnabledLayerNames = layers,
			.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]),
			.ppEnabledExtensionNames = extensions
		};
		result = vkCreateInstance(&create_info, NULL, &instance);
		assert(result == VK_SUCCESS);
		volkLoadInstance(instance);
	}

	VkDebugUtilsMessengerEXT debug_messenger;
	{
		VkDebugUtilsMessengerCreateInfoEXT create_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
								VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
							VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
							VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = debug_callback
		};

		result = vkCreateDebugUtilsMessengerEXT(instance, &create_info, NULL, &debug_messenger);
		assert(result == VK_SUCCESS);
	}

	VkPhysicalDevice physical_device = NULL;
	{
		uint32_t count;
		result = vkEnumeratePhysicalDevices(instance, &count, NULL);
		assert(result == VK_SUCCESS);

		VkPhysicalDevice* physical_devices = malloc(sizeof(VkPhysicalDevice) * count);
		result = vkEnumeratePhysicalDevices(instance, &count, physical_devices);
		assert(result == VK_SUCCESS);

		uint32_t chosen_physical_device = -1;
		for (uint32_t i = 0; i < count; ++i)
		{
			VkPhysicalDeviceProperties2 properties = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
			};
			vkGetPhysicalDeviceProperties2(physical_devices[i], &properties);

			if (properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				chosen_physical_device = i;
				break;
			}
			else if (properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
			{
				chosen_physical_device = i;
			}
		}

		assert(chosen_physical_device != -1);
		physical_device = physical_devices[chosen_physical_device];

		vkGetPhysicalDeviceMemoryProperties(physical_device, &graphics->device_memory_properties);
	}

	uint32_t graphics_queue_family_index = -1;
	uint32_t transfer_queue_family_index = -1;
	{
		uint32_t count;
		vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &count, NULL);

		VkQueueFamilyProperties2* queue_family_properties = malloc(sizeof(VkQueueFamilyProperties2) * count);
		for (uint32_t i = 0; i < count; ++i)
		{
			queue_family_properties[i] = (VkQueueFamilyProperties2){
				.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2
			};
		}
		vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &count, queue_family_properties);

		for (uint32_t i = 0; i < count; ++i)
		{
			if (queue_family_properties[i].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				graphics_queue_family_index = i;
				break;
			}
		}
		for (uint32_t i = 0; i < count; ++i)
		{
			if (queue_family_properties[i].queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				if (i != graphics_queue_family_index)
				{
					transfer_queue_family_index = i;

					if (!(queue_family_properties[i].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT))
					{
						// this is the best case, a queue which supports transfer and NOT graphics
						break;
					}
				}
			}
		}
		if (transfer_queue_family_index == -1)
		{
			// fallback to using the graphics queue for transfer
			transfer_queue_family_index = graphics_queue_family_index;
		}

		assert(graphics_queue_family_index != -1 && transfer_queue_family_index != -1);
	}

	uint32_t queue_family_index_count;
	uint32_t queue_family_indices[2] = { graphics_queue_family_index, transfer_queue_family_index };
	{
		float32_t priority = 1.0f;
		VkDeviceQueueCreateInfo* queue_create_info;
		if (graphics_queue_family_index == transfer_queue_family_index)
		{
			queue_family_index_count = 1;
			queue_create_info = malloc(sizeof(VkDeviceQueueCreateInfo));
			queue_create_info[0] = (VkDeviceQueueCreateInfo){
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.pNext = NULL,
				.queueFamilyIndex = graphics_queue_family_index,
				.queueCount = 1,
				.pQueuePriorities = &priority
			};
		}
		else
		{
			queue_family_index_count = 2;
			queue_create_info = malloc(sizeof(VkDeviceQueueCreateInfo) * queue_family_index_count);
			queue_create_info[0] = (VkDeviceQueueCreateInfo){
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.pNext = NULL,
				.queueFamilyIndex = graphics_queue_family_index,
				.queueCount = 1,
				.pQueuePriorities = &priority
			};
			queue_create_info[1] = (VkDeviceQueueCreateInfo){
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.pNext = NULL,
				.queueFamilyIndex = transfer_queue_family_index,
				.queueCount = 1,
				.pQueuePriorities = &priority
			};
		}

		const char* extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		VkDeviceCreateInfo create_info = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.queueCreateInfoCount = queue_family_index_count,
			.pQueueCreateInfos = queue_create_info,
			.enabledLayerCount = 0,
			.ppEnabledLayerNames = NULL,
			.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]),
			.ppEnabledExtensionNames = extensions,
			.pEnabledFeatures = NULL
		};
		result = vkCreateDevice(physical_device, &create_info, NULL, &graphics->device);
		assert(result == VK_SUCCESS);

		vkGetDeviceQueue(graphics->device, graphics_queue_family_index, 0, &graphics->graphics_queue);
		vkGetDeviceQueue(graphics->device, transfer_queue_family_index, 0, &graphics->transfer_queue);
	}
	volkLoadDevice(graphics->device);

	VkSurfaceKHR surface;
	{
		VkWin32SurfaceCreateInfoKHR create_info = {
			.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.flags = 0,
			.hinstance = instance_handle,
			.hwnd = window_handle
		};
		result = vkCreateWin32SurfaceKHR(instance, &create_info, NULL, &surface);
		assert(result == VK_SUCCESS);

		VkBool32 supported;
		result = vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, graphics_queue_family_index, surface, &supported);
		assert(result == VK_SUCCESS);
		assert(supported);
	}

	VkSurfaceCapabilitiesKHR surface_capabilities;
	VkSurfaceFormatKHR surface_format = { 0 };
	{
		result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);
		assert(result == VK_SUCCESS);

		graphics->swapchain_extent = surface_capabilities.currentExtent;

		uint32_t count;
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &count, NULL);
		assert(result == VK_SUCCESS);

		VkSurfaceFormatKHR* surface_formats = malloc(sizeof(VkSurfaceFormatKHR) * count);
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &count, surface_formats);
		assert(result == VK_SUCCESS);

		for (uint32_t i = 0; i < count; ++i)
		{
			if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
			{
				// this is the ideal
				surface_format = surface_formats[i];
				break;
			}
			else if (surface_formats[i].format == VK_FORMAT_R8G8B8A8_SRGB)
			{
				// this is the backup
				surface_format = surface_formats[i];
			}
			// todo: do we need to support unorm formats if neither of these are supported?
		}
		assert(surface_format.format);

		VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
		{
			result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &count, NULL);
			assert(result == VK_SUCCESS);

			VkPresentModeKHR* present_modes = malloc(sizeof(VkPresentModeKHR) * count);
			result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &count, present_modes);
			assert(result == VK_SUCCESS);

			for (uint32_t i = 0; i < count; ++i)
			{
				if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
					break;
				}
			}
			// todo should be driven by settings, vsync on = FIFO, vsync off = Mailbox if available otherwise immediate
		}

		VkSwapchainCreateInfoKHR create_info = {
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = surface,
			.minImageCount = surface_capabilities.minImageCount + 1,
			.imageFormat = surface_format.format,
			.imageColorSpace = surface_format.colorSpace,
			.imageExtent = surface_capabilities.currentExtent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE, // we're assuming that our graphics queue will be able to present
			.queueFamilyIndexCount = queue_family_index_count,
			.pQueueFamilyIndices = queue_family_indices,
			.preTransform = surface_capabilities.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = present_mode,
			.clipped = VK_TRUE,
			.oldSwapchain = NULL // this should be non-null if resizing the swapchain
		};
		result = vkCreateSwapchainKHR(graphics->device, &create_info, NULL, &graphics->swapchain);
		assert(result == VK_SUCCESS);
	}

	VkImageView* swapchain_image_views;
	{
		result = vkGetSwapchainImagesKHR(graphics->device, graphics->swapchain, &graphics->swapchain_image_count, NULL);
		assert(result == VK_SUCCESS);

		VkImage* images = malloc(sizeof(VkImage) * graphics->swapchain_image_count);
		result = vkGetSwapchainImagesKHR(graphics->device, graphics->swapchain, &graphics->swapchain_image_count, images);
		assert(result == VK_SUCCESS);

		swapchain_image_views = malloc(sizeof(VkImageView) * graphics->swapchain_image_count);
		for (uint32_t i = 0; i < graphics->swapchain_image_count; ++i)
		{
			VkImageViewCreateInfo create_info = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = images[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = surface_format.format,
				.components = {
					.r = VK_COMPONENT_SWIZZLE_IDENTITY,
					.g = VK_COMPONENT_SWIZZLE_IDENTITY,
					.b = VK_COMPONENT_SWIZZLE_IDENTITY,
					.a = VK_COMPONENT_SWIZZLE_IDENTITY
				},
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseArrayLayer = 0,
					.layerCount = 1,
					.baseMipLevel = 0,
					.levelCount = 1
				}
			};
			result = vkCreateImageView(graphics->device, &create_info, NULL, &swapchain_image_views[i]);
			assert(result == VK_SUCCESS);
		}
	}

	VkImage depth_buffer;
	VkFormat depth_buffer_format = 0;
	{
		VkFormat depth_formats[] = {
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM
		};
		for (int32_t i = 0; i < 3; ++i)
		{
			VkFormatProperties properties;
			vkGetPhysicalDeviceFormatProperties(physical_device, depth_formats[i], &properties);
			if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				depth_buffer_format = depth_formats[i];
				break;
			}
		}
		assert(depth_buffer_format);

		VkImageCreateInfo create_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = depth_buffer_format,
			.extent = {
				.width = surface_capabilities.currentExtent.width,
				.height = surface_capabilities.currentExtent.height,
				.depth = 1,
			},
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			// note - no queue family indices needed with an exclusive image
		};
		result = vkCreateImage(graphics->device, &create_info, NULL, &depth_buffer);
		assert(result == VK_SUCCESS);

		VkMemoryRequirements memory_requirements = { 0 };
		vkGetImageMemoryRequirements(graphics->device, depth_buffer, &memory_requirements);

		VkDeviceMemory memory = alloc_device_memory(graphics, memory_requirements.size, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memory_requirements.memoryTypeBits);

		result = vkBindImageMemory(graphics->device, depth_buffer, memory, 0);
		assert(result == VK_SUCCESS);
	}

	VkImageView depth_buffer_image_view;
	{
		VkImageViewCreateInfo create_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = depth_buffer,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = depth_buffer_format,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
				.baseArrayLayer = 0,
				.layerCount = 1,
				.baseMipLevel = 0,
				.levelCount = 1
			}
		};
		result = vkCreateImageView(graphics->device, &create_info, NULL, &depth_buffer_image_view);
		assert(result == VK_SUCCESS);
	}

	{
		VkAttachmentDescription attachments[2] = {
			// colour
			[0] = {
				.format = surface_format.format,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			},
			// depth
			[1] = {
				.format = depth_buffer_format,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			},
		};

		VkAttachmentReference colour_ref = {
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		VkAttachmentReference depth_ref = {
			.attachment = 1,
			.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		VkSubpassDescription subpass = {
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colour_ref,
			.pDepthStencilAttachment = &depth_ref,
		};

		VkSubpassDependency dependency = {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
						   | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
						   | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
						   | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		};

		VkRenderPassCreateInfo create_info = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = 2,
			.pAttachments = attachments,
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 1,
			.pDependencies = &dependency,
		};
		result = vkCreateRenderPass(graphics->device, &create_info, NULL, &graphics->render_pass);
		assert(result == VK_SUCCESS);
	}

	{
		graphics->framebuffers = malloc(sizeof(VkFramebuffer) * graphics->swapchain_image_count);

		for (uint32_t i = 0; i < graphics->swapchain_image_count; ++i)
		{
			VkImageView attachments[2] = { swapchain_image_views[i], depth_buffer_image_view };

			VkFramebufferCreateInfo create_info = {
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass = graphics->render_pass,
				.attachmentCount = 2,
				.pAttachments = attachments,
				.width = surface_capabilities.currentExtent.width,
				.height = surface_capabilities.currentExtent.height,
				.layers = 1
			};

			result = vkCreateFramebuffer(graphics->device, &create_info, NULL, &graphics->framebuffers[i]);
			assert(result == VK_SUCCESS);
		}
	}

	{
		const char* paths[2] = {
			"shaders/shader.vert.spv",
			"shaders/shader.frag.spv"
		};
		VkShaderModule shader_modules[2];

		for (int32_t i = 0; i < 2; ++i)
		{
			buffer_t file = file_read(paths[i]);

			VkShaderModuleCreateInfo create_info = {
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.pCode = file.data,
				.codeSize = file.size
			};
			result = vkCreateShaderModule(graphics->device, &create_info, NULL, &shader_modules[i]);
			assert(result == VK_SUCCESS);
		}

		VkPipelineShaderStageCreateInfo shader_stages[2] = {
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_VERTEX_BIT,
				.module = shader_modules[0],
				.pName = "main"
			},
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
				.module = shader_modules[1],
				.pName = "main"
			}
		};

		VkPipelineLayoutCreateInfo layout_info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 0,
			.pSetLayouts = NULL,
			.pushConstantRangeCount = 0,
			.pPushConstantRanges = NULL,
		};
		VkPipelineLayout pipeline_layout;
		result = vkCreatePipelineLayout(graphics->device, &layout_info, NULL, &pipeline_layout);
		assert(result == VK_SUCCESS);

		VkVertexInputBindingDescription binding_desc = {
			.binding = 0,
			.stride = sizeof(vertex_t),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		};
		
		VkVertexInputAttributeDescription attribute_descs[2] = {
			{
				.binding = 0,
				.location = 0,
				.format = VK_FORMAT_R32G32B32A32_SFLOAT,
				.offset = offsetof(vertex_t, position)
			},
			{
				.binding = 0,
				.location = 1,
				.format = VK_FORMAT_R32G32B32A32_SFLOAT,
				.offset = offsetof(vertex_t, colour)
			}
		};

		VkPipelineVertexInputStateCreateInfo vertex_input = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &binding_desc,
			.vertexAttributeDescriptionCount = 2,
			.pVertexAttributeDescriptions = attribute_descs
		};

		VkPipelineInputAssemblyStateCreateInfo input_assembly = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE,
		};

		// Viewport and scissor as dynamic state so we don't need to
		// recreate the pipeline on window resize
		VkPipelineViewportStateCreateInfo viewport_state = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.scissorCount = 1,
		};

		VkPipelineRasterizationStateCreateInfo rasterizer = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_BACK_BIT,
			.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.depthBiasEnable = VK_FALSE,
			.lineWidth = 1.0f,
		};

		VkPipelineMultisampleStateCreateInfo multisampling = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
		};

		VkPipelineDepthStencilStateCreateInfo depth_stencil = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = VK_TRUE,
			.depthWriteEnable = VK_TRUE,
			.depthCompareOp = VK_COMPARE_OP_LESS,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
		};

		VkPipelineColorBlendAttachmentState blend_attachment = {
			.blendEnable = VK_FALSE,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
							  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		};

		VkPipelineColorBlendStateCreateInfo color_blending = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.logicOpEnable = VK_FALSE,
			.attachmentCount = 1,
			.pAttachments = &blend_attachment,
		};

		VkDynamicState dynamic_states[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
		};

		VkPipelineDynamicStateCreateInfo dynamic_state = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = 2,
			.pDynamicStates = dynamic_states,
		};

		VkGraphicsPipelineCreateInfo pipeline_info = {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = 2,
			.pStages = shader_stages,
			.pVertexInputState = &vertex_input,
			.pInputAssemblyState = &input_assembly,
			.pViewportState = &viewport_state,
			.pRasterizationState = &rasterizer,
			.pMultisampleState = &multisampling,
			.pDepthStencilState = &depth_stencil,
			.pColorBlendState = &color_blending,
			.pDynamicState = &dynamic_state,
			.layout = pipeline_layout,
			.renderPass = graphics->render_pass,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE
		};

		result = vkCreateGraphicsPipelines(graphics->device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &graphics->pipeline);
		assert(result == VK_SUCCESS);

		vkDestroyShaderModule(graphics->device, shader_modules[0], NULL);
		vkDestroyShaderModule(graphics->device, shader_modules[1], NULL);
	}

	{
		VkCommandPoolCreateInfo pool_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = graphics_queue_family_index,
		};
		result = vkCreateCommandPool(graphics->device, &pool_info, NULL, &graphics->graphics_command_pool);
		assert(result == VK_SUCCESS);

		if (graphics_queue_family_index == transfer_queue_family_index)
		{
			graphics->transfer_command_pool = graphics->graphics_command_pool;
		}
		else
		{
			pool_info.queueFamilyIndex = transfer_queue_family_index;
			result = vkCreateCommandPool(graphics->device, &pool_info, NULL, &graphics->transfer_command_pool);
			assert(result == VK_SUCCESS);
		}

		VkCommandBufferAllocateInfo alloc_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = graphics->graphics_command_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = FRAMES_IN_FLIGHT,
		};
		result = vkAllocateCommandBuffers(graphics->device, &alloc_info, graphics->command_buffers);
		assert(result == VK_SUCCESS);

		VkSemaphoreCreateInfo semaphore_info = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};

		VkFenceCreateInfo fence_info = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		};

		graphics->present_semaphores = malloc(sizeof(VkSemaphore) * graphics->swapchain_image_count);
		for (uint32_t i = 0; i < graphics->swapchain_image_count; ++i)
		{
			result = vkCreateSemaphore(graphics->device, &semaphore_info, NULL, &graphics->present_semaphores[i]);
			assert(result == VK_SUCCESS);
		}

		for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) 
		{
			result = vkCreateSemaphore(graphics->device, &semaphore_info, NULL, &graphics->acquire_image_semaphores[i]);
			assert(result == VK_SUCCESS);
			result = vkCreateFence(graphics->device, &fence_info, NULL, &graphics->in_flight_fences[i]);
			assert(result == VK_SUCCESS);
		}
	}

	{
		VkDeviceSize vertices_size = sizeof(vertices);
		vk_buffer_t staging_buffer = create_buffer(graphics, vertices_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		graphics->vertex_buffer = create_buffer(graphics, vertices_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		void* data;
		result = vkMapMemory(graphics->device, staging_buffer.memory, 0, vertices_size, 0, &data);
		assert(result == VK_SUCCESS);
		memcpy(data, vertices, vertices_size);
		vkUnmapMemory(graphics->device, staging_buffer.memory);

		copy_buffer(graphics, staging_buffer.buffer, graphics->vertex_buffer.buffer, vertices_size);

		vkDestroyBuffer(graphics->device, staging_buffer.buffer, NULL);
		vkFreeMemory(graphics->device, staging_buffer.memory, NULL);
	}

	{
		VkDeviceSize indices_size = sizeof(indices);
		vk_buffer_t staging_buffer = create_buffer(graphics, indices_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		graphics->index_buffer = create_buffer(graphics, indices_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		void* data;
		result = vkMapMemory(graphics->device, staging_buffer.memory, 0, indices_size, 0, &data);
		assert(result == VK_SUCCESS);
		memcpy(data, indices, indices_size);
		vkUnmapMemory(graphics->device, staging_buffer.memory);

		copy_buffer(graphics, staging_buffer.buffer, graphics->index_buffer.buffer, indices_size);

		vkDestroyBuffer(graphics->device, staging_buffer.buffer, NULL);
		vkFreeMemory(graphics->device, staging_buffer.memory, NULL);
	}
}

void graphics_deinit(graphics_t* graphics)
{
	vkDeviceWaitIdle(graphics->device);

	// todo
}

void graphics_render(graphics_t* graphics)
{
	// Wait for the previous frame using this slot to finish
	vkWaitForFences(graphics->device, 1, &graphics->in_flight_fences[graphics->current_frame], VK_TRUE, UINT64_MAX);
	vkResetFences(graphics->device, 1, &graphics->in_flight_fences[graphics->current_frame]);

	// Acquire the next swapchain image
	uint32_t swapchain_image_index;
	vkAcquireNextImageKHR(
		graphics->device, 
		graphics->swapchain, 
		UINT64_MAX,
		graphics->acquire_image_semaphores[graphics->current_frame], 
		VK_NULL_HANDLE, 
		&swapchain_image_index);

	// Record command buffer
	vkResetCommandBuffer(graphics->command_buffers[graphics->current_frame], 0);

	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	};
	vkBeginCommandBuffer(graphics->command_buffers[graphics->current_frame], &begin_info);

	VkClearValue clear_values[2] = {
		[0] = {.color = {{ 0.0f, 0.0f, 0.0f, 1.0f }} },
		[1] = {.depthStencil = { 1.0f, 0 } },
	};

	VkRenderPassBeginInfo render_pass_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = graphics->render_pass,
		.framebuffer = graphics->framebuffers[swapchain_image_index],
		.renderArea = {.offset = { 0, 0 }, .extent = graphics->swapchain_extent },
		.clearValueCount = 2,
		.pClearValues = clear_values,
	};
	vkCmdBeginRenderPass(graphics->command_buffers[graphics->current_frame],
		&render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(graphics->command_buffers[graphics->current_frame],
		VK_PIPELINE_BIND_POINT_GRAPHICS, graphics->pipeline);

	VkViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = (float)graphics->swapchain_extent.width,
		.height = (float)graphics->swapchain_extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	vkCmdSetViewport(graphics->command_buffers[graphics->current_frame], 0, 1, &viewport);

	VkRect2D scissor = {
		.offset = { 0, 0 },
		.extent = graphics->swapchain_extent,
	};
	vkCmdSetScissor(graphics->command_buffers[graphics->current_frame], 0, 1, &scissor);

	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(graphics->command_buffers[graphics->current_frame], 0, 1, &graphics->vertex_buffer.buffer, &offset);
	vkCmdBindIndexBuffer(graphics->command_buffers[graphics->current_frame], graphics->index_buffer.buffer, 0, VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(graphics->command_buffers[graphics->current_frame], 3, 1, 0, 0, 0);

	vkCmdEndRenderPass(graphics->command_buffers[graphics->current_frame]);
	vkEndCommandBuffer(graphics->command_buffers[graphics->current_frame]);

	// Submit
	VkPipelineStageFlags wait_stage =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &graphics->acquire_image_semaphores[graphics->current_frame], // submit (render) will wait until the swapchain image is acquired
		.pWaitDstStageMask = &wait_stage,
		.commandBufferCount = 1,
		.pCommandBuffers = &graphics->command_buffers[graphics->current_frame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &graphics->present_semaphores[swapchain_image_index], // when finished it signals the present semaphore
	};
	vkQueueSubmit(graphics->graphics_queue, 1, &submit_info, graphics->in_flight_fences[graphics->current_frame]);

	// Present
	VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &graphics->present_semaphores[swapchain_image_index],
		.swapchainCount = 1,
		.pSwapchains = &graphics->swapchain,
		.pImageIndices = &swapchain_image_index,
	};
	vkQueuePresentKHR(graphics->graphics_queue, &present_info);

	graphics->current_frame = (graphics->current_frame + 1) % FRAMES_IN_FLIGHT;
}