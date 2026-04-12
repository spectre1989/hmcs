#include <Windows.h>
#include "volk.h"


/*
typedef struct
{
	0 int					id;
	4 int					version;

	8 char				name[64];
	72 int					length;

	76 vec3_t				eyeposition;	// ideal eye position
	88 vec3_t				min;			// ideal movement hull size
	100 vec3_t				max;

	112 vec3_t				bbmin;			// clipping bounding box
	124 vec3_t				bbmax;

	136 int					flags;

	140 int					numbones;			// bones
	144 int					boneindex;

	148 int					numbonecontrollers;		// bone controllers
	152 int					bonecontrollerindex;

	156 int					numhitboxes;			// complex bounding boxes
	160 int					hitboxindex;

	164 int					numseq;				// animation sequences
	168 int					seqindex;

	172 int					numseqgroups;		// demand loaded sequences
	176 int					seqgroupindex;

	180 int					numtextures;		// raw textures
	184 int					textureindex;
	188 int					texturedataindex;

	192 int					numskinref;			// replaceable textures
	196 int					numskinfamilies;
	200 int					skinindex;

	204 int					numbodyparts;
	208 int					bodypartindex;

	int					numattachments;		// queryable attachable points
	int					attachmentindex;

	int					soundtable;
	int					soundindex;
	int					soundgroups;
	int					soundgroupindex;

	int					numtransitions;		// animation node to animation node transition graph
	int					transitionindex;
} studiohdr_t;

76 typedef struct
{
	0 char				name[64];
	64 int					nummodels;
	68 int					base;
	72 int					modelindex; // index into models array
} mstudiobodyparts_t;

// studio models
112 typedef struct
{
	0 char				name[64];

	64 int					type;

	68 float				boundingradius;

	72 int					nummesh;
	76 int					meshindex;

	80 int					numverts;		// number of unique vertices
	84 int					vertinfoindex;	// vertex bone info
	88 int					vertindex;		// vertex vec3_t
	92 int					numnorms;		// number of unique surface normals
	96 int					norminfoindex;	// normal bone info
	100 int					normindex;		// normal vec3_t

	104 int					numgroups;		// deformation groups
	108 int					groupindex;
} mstudiomodel_t;


// vec3_t	boundingbox[model][bone][2];	// complex intersection info


// meshes
typedef struct
{
	int					numtris;
	int					triindex;
	int					skinref;
	int					numnorms;		// per mesh normals
	int					normindex;		// normal vec3_t
} mstudiomesh_t;

The actual triangle data at triindex is a tristrip/trifan stream — it's a sequence of mstudiotrivert_t entries 
(vertex index, normal index, texture s/t coords), terminated by a 0 short. A positive count starts a fan, negative count starts a strip.
Read a short → that's your count header
If 0, you're done
If positive, read that many mstudiotrivert_ts and interpret as a fan
If negative, read abs(count) mstudiotrivert_ts and interpret as a strip
Go back to 1

typedef struct
{
	short				vertindex;		// index into vertex array
	short				normindex;		// index into normal array
	short				s,t;			// s,t position on skin
} mstudiotrivert_t;
*/

#include "types.h"
#include "assert.h"
#include <stdio.h>



static char* g_vk_debug_buffer = NULL;
enum {VK_DEBUG_BUFFER_SIZE = 1024};

static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	snprintf(g_vk_debug_buffer, VK_DEBUG_BUFFER_SIZE, "[Vulkan] %s\n", pCallbackData->pMessage);
	OutputDebugStringA(g_vk_debug_buffer);
	return VK_FALSE; // don't abort the call
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance, PWSTR cmd_line, int cmd_show)
{
	g_vk_debug_buffer = malloc(VK_DEBUG_BUFFER_SIZE);

	VkResult result = volkInitialize();
	assert(result == VK_SUCCESS);

	VkInstance vk_instance;
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
		result = vkCreateInstance(&create_info, NULL, &vk_instance);
		assert(result == VK_SUCCESS);
		volkLoadInstance(vk_instance);
	}
	
	VkDebugUtilsMessengerEXT vk_debug_messenger;
	{
		VkDebugUtilsMessengerCreateInfoEXT create_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
								VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
							VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
							VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = vk_debug_callback
		};

		result = vkCreateDebugUtilsMessengerEXT(vk_instance, &create_info, NULL, &vk_debug_messenger);
		assert(result == VK_SUCCESS);
	}

	VkPhysicalDevice vk_physical_device = NULL;
	{
		uint32_t count;
		result = vkEnumeratePhysicalDevices(vk_instance, &count, NULL);
		assert(result == VK_SUCCESS);

		VkPhysicalDevice* physical_devices = malloc(sizeof(VkPhysicalDevice) * count);
		result = vkEnumeratePhysicalDevices(vk_instance, &count, physical_devices);
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
		vk_physical_device = physical_devices[chosen_physical_device];
	}

	uint32_t graphics_queue_index = -1;
	uint32_t transfer_queue_index = -1;
	{
		uint32_t count;
		vkGetPhysicalDeviceQueueFamilyProperties2(vk_physical_device, &count, NULL);

		VkQueueFamilyProperties2* queue_family_properties = malloc(sizeof(VkQueueFamilyProperties2) * count);
		for (uint32_t i = 0; i < count; ++i)
		{
			queue_family_properties[i] = (VkQueueFamilyProperties2){
				.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2
			};
		}
		vkGetPhysicalDeviceQueueFamilyProperties2(vk_physical_device, &count, queue_family_properties);

		for (uint32_t i = 0; i < count; ++i)
		{
			if (queue_family_properties[i].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				graphics_queue_index = i;
				break;
			}
		}
		for (uint32_t i = 0; i < count; ++i)
		{
			if (queue_family_properties[i].queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				if (i != graphics_queue_index)
				{
					transfer_queue_index = i;

					if (!(queue_family_properties[i].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT))
					{
						// this is the best case, a queue which supports transfer and NOT graphics
						break;
					}
				}
			}
		}
		if (transfer_queue_index == -1)
		{
			// fallback to using the graphics queue for transfer
			transfer_queue_index = graphics_queue_index;
		}

		assert(graphics_queue_index != -1 && transfer_queue_index != -1);
	}

	VkDevice vk_device;
	{
		uint32_t queue_create_info_count;
		float32_t priority = 1.0f;
		VkDeviceQueueCreateInfo* queue_create_info;
		if (graphics_queue_index == transfer_queue_index)
		{
			queue_create_info_count = 1;
			queue_create_info = malloc(sizeof(VkDeviceQueueCreateInfo));
			queue_create_info[0] = (VkDeviceQueueCreateInfo){
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.pNext = NULL,
				.queueFamilyIndex = graphics_queue_index,
				.queueCount = 1,
				.pQueuePriorities = &priority
			};
		}
		else
		{
			queue_create_info_count = 2;
			queue_create_info = malloc(sizeof(VkDeviceQueueCreateInfo) * queue_create_info_count);
			queue_create_info[0] = (VkDeviceQueueCreateInfo){
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.pNext = NULL,
				.queueFamilyIndex = graphics_queue_index,
				.queueCount = 1,
				.pQueuePriorities = &priority
			};
			queue_create_info[1] = (VkDeviceQueueCreateInfo){
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.pNext = NULL,
				.queueFamilyIndex = transfer_queue_index,
				.queueCount = 1,
				.pQueuePriorities = &priority
			};
		}

		VkDeviceCreateInfo create_info = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.queueCreateInfoCount = queue_create_info_count,
			.pQueueCreateInfos = queue_create_info,
			.enabledLayerCount = 0,
			.ppEnabledLayerNames = NULL,
			.enabledExtensionCount = 0,
			.ppEnabledExtensionNames = NULL,
			.pEnabledFeatures = NULL
		};
		result = vkCreateDevice(vk_physical_device, &create_info, NULL, &vk_device);
		assert(result == VK_SUCCESS);
	}
	volkLoadDevice(vk_device);

	uint16_t file_path[MAX_PATH];
	int32_t cmd_line_len = lstrlenW(cmd_line);
	lstrcpynW(file_path, cmd_line, MAX_PATH);
	lstrcpynW(file_path + cmd_line_len, L"\\cstrike_hd\\models\\player\\sas\\sas.mdl", MAX_PATH - cmd_line_len);

	HANDLE file = CreateFileW(file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	DWORD file_size = GetFileSize(file, NULL);
	uint8_t* file_data = malloc(file_size);
	ReadFile(file, file_data, file_size, NULL, NULL);

	CloseHandle(file);

	uint8_t name[64];
	int32_t body_part_count;
	int32_t body_parts_offset;

	memcpy(name, file_data + 8, 64);
	memcpy(&body_part_count, file_data + 204, 4);
	memcpy(&body_parts_offset, file_data + 208, 4);

	uint8_t* body_parts = file_data + body_parts_offset;
	for (int32_t i = 0; i < body_part_count; ++i)
	{
		memcpy(name, body_parts, 64);
		int32_t model_count;
		int32_t models_offset;
		memcpy(&model_count, body_parts + 64, 4);
		memcpy(&models_offset, body_parts + 72, 4);

		uint8_t* models = file_data + models_offset;
		for (int32_t model_i = 0; model_i < model_count; ++model_i)
		{
			/*
			112 typedef struct
{
	0 char				name[64];

	64 int					type;

	68 float				boundingradius;

	72 int					nummesh;
	76 int					meshindex;

	80 int					numverts;		// number of unique vertices
	84 int					vertinfoindex;	// vertex bone info
	88 int					vertindex;		// vertex vec3_t
	92 int					numnorms;		// number of unique surface normals
	96 int					norminfoindex;	// normal bone info
	100 int					normindex;		// normal vec3_t

	104 int					numgroups;		// deformation groups
	108 int					groupindex;
} mstudiomodel_t;
			*/
			memcpy(name, models, 64);

			int32_t mesh_count;
			int32_t vert_count;

			memcpy(&mesh_count, models + 72, 4);
			memcpy(&vert_count, models + 80, 4);

			models += 112;
		}

		body_parts += 76;
	}

	return 0;
}