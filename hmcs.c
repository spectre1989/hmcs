#include <Windows.h>
#include "types.h"
#include "assert.h"
#include "graphics.h"
#include <stdio.h>


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



enum {WINDOW_WIDTH = 1280, WINDOW_HEIGHT = 800};

LRESULT CALLBACK window_proc(HWND window_handle, UINT msg, WPARAM w_param, LPARAM l_param)
{
	return DefWindowProc(window_handle, msg, w_param, l_param);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance, PWSTR cmd_line, int cmd_show)
{
	WNDCLASS window_class = {
		.style = CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = window_proc,
		.hInstance = instance,
		.hCursor = LoadCursor(NULL, IDC_ARROW),
		.lpszClassName = L"hmcs"
	};
	RegisterClass(&window_class);

	HWND window_handle = CreateWindow(L"hmcs", L"hmcs", WS_OVERLAPPEDWINDOW, 200, 100, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, instance, NULL);
	assert(window_handle);
	ShowWindow(window_handle, SW_SHOW);

	graphics_t* graphics = malloc(sizeof(graphics_t));
	graphics_init(instance, window_handle, graphics);

	/*uint16_t file_path[MAX_PATH];
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
			/*memcpy(name, models, 64);

			int32_t mesh_count;
			int32_t vert_count;

			memcpy(&mesh_count, models + 72, 4);
			memcpy(&vert_count, models + 80, 4);

			models += 112;
		}

		body_parts += 76;
	}*/

	while (1)
	{
		graphics_render(graphics);
	}

	return 0;
}