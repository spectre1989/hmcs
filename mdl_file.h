#pragma once

#include "types.h"


struct MDL_Header
{
	int32	id;
	int32	version;

	char	name[64];
	int32	length;

	Vec_3f	eyeposition;	// ideal eye position
	Vec_3f	min;			// ideal movement hull size
	Vec_3f	max;

	Vec_3f	bbmin;			// clipping bounding box
	Vec_3f	bbmax;

	int32	flags;

	int32	numbones;			// bones
	int32	boneindex;

	int32	numbonecontrollers;		// bone controllers
	int32	bonecontrollerindex;

	int32	numhitboxes;			// complex bounding boxes
	int32	hitboxindex;

	int32	numseq;				// animation sequences
	int32	seqindex;

	int32	numseqgroups;		// demand loaded sequences
	int32	seqgroupindex;

	int32	numtextures;		// raw textures
	int32	textureindex;
	int32	texturedataindex;

	int32	numskinref;			// replaceable textures
	int32	numskinfamilies;
	int32	skinindex;

	int32	numbodyparts;
	int32	bodypartindex;

	int32	numattachments;		// queryable attachable points
	int32	attachmentindex;

	int32	soundtable;
	int32	soundindex;
	int32	soundgroups;
	int32	soundgroupindex;

	int32	numtransitions;		// animation node to animation node transition graph
	int32	transitionindex;
};