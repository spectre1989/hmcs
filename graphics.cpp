#include "graphics.h"

#include <vulkan/vulkan.h>

#include "types.h"


void graphics_init(HINSTANCE app_instance, HWND window, uint32 window_width, uint32 window_height)
{ 
	VkInstance instance = nullptr;
	{
		VkApplicationInfo application_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
		application_info.apiVersion = VK_API_VERSION_1_1;

		const char* extensions[] = {
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME
		};
		constexpr uint32 extension_count = sizeof(extensions) / sizeof(extensions[0]);

		VkInstanceCreateInfo create_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		create_info.pApplicationInfo = &application_info;
		create_info.enabledExtensionCount = extension_count;
		create_info.ppEnabledExtensionNames = extensions;

		vkCreateInstance(&create_info, 0, &instance);
		// todo: error checking
	}

	VkSurfaceKHR surface;
	{
		VkWin32SurfaceCreateInfoKHR create_info = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
		create_info.hinstance = app_instance;
		create_info.hwnd = window;

		vkCreateWin32SurfaceKHR(instance, &create_info, nullptr, &surface);
	}
	
	uint32 physical_device_count;
	vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);

	VkPhysicalDevice* physical_devices = new VkPhysicalDevice[physical_device_count];
	vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices);

	VkPhysicalDevice physical_device = nullptr;
	for (uint32 i = 0; i < physical_device_count; ++i)
	{
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(physical_devices[i], &props);

		// TODO actually choose best gpu by some algo
		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			physical_device = physical_devices[i];
			break;
		}
	}

	VkDevice device;
	uint32 queue_family_index;
	{
		uint32 queue_family_property_count;
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_property_count, nullptr);
		
		VkQueueFamilyProperties* queue_family_properties = new VkQueueFamilyProperties[queue_family_property_count];
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_property_count, queue_family_properties);

		for (uint32 i = 0; i < queue_family_property_count; ++i)
		{
			if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				queue_family_index = i;
				break;
			}
		}

		const float queue_priority = 1.0f;
		VkDeviceQueueCreateInfo queue_create_info = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		queue_create_info.pQueuePriorities = &queue_priority;
		queue_create_info.queueFamilyIndex = queue_family_index;
		queue_create_info.queueCount = 1;

		const char* extensions[] = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		constexpr uint32 extension_count = sizeof(extensions) / sizeof(extensions[0]);

		VkDeviceCreateInfo create_info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		create_info.queueCreateInfoCount = 1;
		create_info.pQueueCreateInfos = &queue_create_info;
		create_info.enabledExtensionCount = extension_count;
		create_info.ppEnabledExtensionNames = extensions;

		vkCreateDevice(physical_device, &create_info, nullptr, &device);
	}

	VkSwapchainKHR swapchain;
	{
		VkSwapchainCreateInfoKHR create_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		create_info.surface = surface;
		create_info.minImageCount = 2;
		create_info.imageFormat = VK_FORMAT_R8G8B8A8_UNORM; // TODO
		create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		create_info.imageExtent.width = window_width;
		create_info.imageExtent.height = window_height;
		create_info.imageArrayLayers = 1;
		create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		create_info.queueFamilyIndexCount = 1;
		create_info.pQueueFamilyIndices = &queue_family_index;
		create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	
		vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain);
	}
}