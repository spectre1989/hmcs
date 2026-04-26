#pragma once

#include "types.h"



#define PI_F32 3.1415926535897932384626433832f
#define DEG_TO_RAD (PI_F32 / 180.0f)

typedef struct mat4_t
{
	// m11 m12 m13 m14
	// m21 m22 m23 m24
	// m31 m32 m33 m34
	// m41 m42 m43 m44

	// column major
	float32_t m11, m21, m31, m41,
		m12, m22, m32, m42,
		m13, m23, m33, m43,
		m14, m24, m34, m44;
} mat4_t;


void mat4_identity(mat4_t* matrix);
void mat4_projection(float32_t fov_y, 
	float32_t aspect_ratio, 
	float32_t near_plane, 
	float32_t far_plane, 
	mat4_t* matrix);
void mat4_mul(mat4_t* lhs, mat4_t* rhs, mat4_t* matrix);