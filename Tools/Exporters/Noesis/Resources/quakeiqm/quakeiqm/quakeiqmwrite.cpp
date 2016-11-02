#include "stdafx.h"
#include "quakeiqm.h"

typedef struct iqmAnimHold_s
{
	RichMat43			*mats;
	int					numFrames;
	float				frameRate;
	int					numBones;
} iqmAnimHold_t;

typedef struct iqmPoseData_s
{
	float				d[10];
} iqmPoseData_t;
typedef struct iqmPoseEncData_s
{
	float				minVals[10];
	float				maxVals[10];
} iqmPoseEncData_t;

//retrives animation data
static iqmAnimHold_t *Model_IQM_GetAnimData(noeRAPI_t *rapi)
{
	int animDataSize;
	BYTE *animData = rapi->Noesis_GetExtraAnimData(animDataSize);
	if (!animData)
	{
		return NULL;
	}

	noesisAnim_t *anim = rapi->Noesis_AnimAlloc("animout", animData, animDataSize); //animation containers are pool-allocated, so don't worry about freeing them
	//copy off the raw matrices for the animation frames
	iqmAnimHold_t *iqma = (iqmAnimHold_t *)rapi->Noesis_PooledAlloc(sizeof(iqmAnimHold_t));
	memset(iqma, 0, sizeof(iqmAnimHold_t));
	iqma->mats = (RichMat43 *)rapi->rpgMatsFromAnim(anim, iqma->numFrames, iqma->frameRate, &iqma->numBones, true);

	return iqma;
}

//convert a matrix to translation+rotation+scale
static void Model_IQM_MatToJointData(RichVec3 &trans, float *rot, RichVec3 &scale, RichMat43 &mat)
{
	scale[0] = mat[0].Length();
	scale[1] = mat[1].Length();
	scale[2] = mat[2].Length();
	//eliminate custom scale if it's not significant
	for (int i = 0; i < 3; i++)
	{
		if (fabsf(1.0f-scale[i]) < 0.01f)
		{
			scale[i] = 1.0f;
		}
	}
//	RichQuat q = mat.ToQuat();
//	q.ToQuat3(rot);
	//v2 change
	RichQuat *q = (RichQuat *)rot;
	*q = mat.GetTranspose().ToQuat();
	trans = mat[3];
}

//macro to copy per-mesh elements into a flat array
#define COPY_FLAT_MESH_ARRAY(dstMem, cmpnName, cmpnSize) \
for (int nv = 0; nv < pmdl->numAbsVerts; nv++) \
{ \
	BYTE *dst = dstMem + nv*cmpnSize; \
	sharedVMap_t *svm = pmdl->absVertMap+nv; \
	assert(svm->meshIdx >= 0 && svm->meshIdx < pmdl->numMeshes); \
	sharedMesh_t *mesh = pmdl->meshes+svm->meshIdx; \
	assert(svm->vertIdx >= 0 && svm->vertIdx < mesh->numVerts); \
	BYTE *src = ((BYTE *)mesh->cmpnName) + cmpnSize*svm->vertIdx; \
	memcpy(dst, src, cmpnSize); \
}

//export to iqm
//this just shoves everything out in a fairly fixed format. if people actually have need of it, i'll add more options.
bool Model_IQM_Write(noesisModel_t *mdl, RichBitStream *outStream, noeRAPI_t *rapi)
{
	//MessageBox(g_nfn->NPAPI_GetMainWnd(), L"You could spawn an export options dialog resource here if you wanted.", L"Potential Prompt", MB_OK);

	iqmAnimHold_t *iqma = Model_IQM_GetAnimData(rapi);
	sharedModel_t *pmdl = rapi->rpgGetSharedModel(mdl,
													NMSHAREDFL_WANTNEIGHBORS | //calculate triangle neighbors (can be timely on complex models, but this format wants them)
													NMSHAREDFL_WANTGLOBALARRAY | //calculate giant flat vertex/triangle arrays
													NMSHAREDFL_WANTTANGENTS4 | //make sure tangents are available
													NMSHAREDFL_FLATWEIGHTS | //create flat vertex weight arrays
													NMSHAREDFL_FLATWEIGHTS_FORCE4 | //force 4 weights per vert for the flat weight array data
													NMSHAREDFL_REVERSEWINDING //reverse the face winding (as per Quake) - most formats will not want you to do this!
													);

	noeStringPool_t *strPool = rapi->Noesis_CreateStrPool();

	iqmHdr_t hdr;
	memset(&hdr, 0, sizeof(hdr));
	memcpy(hdr.id, "INTERQUAKEMODEL\0", 16);
	hdr.ver = 2;//1;

	bool exportVertexWeights = true;

	//create iqm mesh list
	iqmMesh_t *meshes = (iqmMesh_t *)rapi->Noesis_PooledAlloc(sizeof(iqmMesh_t)*pmdl->numMeshes);
	memset(meshes, 0, sizeof(iqmMesh_t)*pmdl->numMeshes);
	for (int i = 0; i < pmdl->numMeshes; i++)
	{
		iqmMesh_t *mesh = meshes+i;
		sharedMesh_t *meshSrc = pmdl->meshes+i;
		if (!meshSrc->verts || !meshSrc->tris || !meshSrc->triNeighbors)
		{
			rapi->Noesis_DestroyStrPool(strPool);
			rapi->LogOutput("ERROR: Encountered a mesh with no geometry in IQM export!\n");
			return false;
		}
		if (!meshSrc->flatBoneIdx || !meshSrc->flatBoneWgt)
		{
			exportVertexWeights = false;
		}
		mesh->name = rapi->Noesis_StrPoolGetOfs(strPool, meshSrc->name);
		mesh->material = rapi->Noesis_StrPoolGetOfs(strPool, meshSrc->skinName);
		mesh->firstVert = meshSrc->firstVert;
		mesh->numVerts = meshSrc->numVerts;
		mesh->firstTri = meshSrc->firstTri;
		mesh->numTris = meshSrc->numTris;
	}

	if (!exportVertexWeights)
	{
		rapi->LogOutput("WARNING: One or more meshes were found without vertex weight data. Vertex weighting will not be exported.\n");
	}

	hdr.ofsText = sizeof(iqmHdr_t);
	//allocate the rest of the necessary strings now
	int animName = rapi->Noesis_StrPoolGetOfs(strPool, "animframes");
	int *boneNames = NULL;
	if (pmdl->numBones > 0)
	{
		boneNames = (int *)_alloca(sizeof(int)*pmdl->numBones);
		for (int i = 0; i < pmdl->numBones; i++)
		{
			boneNames[i] = rapi->Noesis_StrPoolGetOfs(strPool, pmdl->bones[i].name);
		}
	}
	hdr.numText = rapi->Noesis_StrPoolSize(strPool);

	//set mesh info
	hdr.ofsMeshes = hdr.ofsText + hdr.numText;
	hdr.numMeshes = pmdl->numMeshes;

	//create vertex arrays
	hdr.ofsVertAr = hdr.ofsMeshes + sizeof(iqmMesh_t)*hdr.numMeshes;
	hdr.numVertAr = (exportVertexWeights) ? 5 : 3;
	int *vsizes = (int *)_alloca(sizeof(int)*hdr.numVertAr);
	iqmVArray_t *varrays = (iqmVArray_t *)rapi->Noesis_PooledAlloc(sizeof(iqmVArray_t)*hdr.numVertAr);
	int varofs = hdr.ofsVertAr + sizeof(iqmVArray_t)*hdr.numVertAr;
	memset(varrays, 0, sizeof(iqmVArray_t)*hdr.numVertAr);
	BYTE **varMem = (BYTE **)_alloca(sizeof(BYTE *)*hdr.numVertAr);
	//set up position array
	varrays[0].type = IQMTYPE_POSITION;
	varrays[0].format = IQMDATA_FLOAT;
	varrays[0].size = 3;//sizeof(modelVert_t)*pmdl->numAbsVerts;
	vsizes[0] = varrays[0].size*sizeof(float)*pmdl->numAbsVerts;
	varrays[0].offset = varofs;
	varMem[0] = (BYTE *)rapi->Noesis_PooledAlloc(vsizes[0]);
	COPY_FLAT_MESH_ARRAY(varMem[0], verts, sizeof(modelVert_t));
	varofs += vsizes[0];
	//set up texcoord array
	varrays[1].type = IQMTYPE_TEXCOORD;
	varrays[1].format = IQMDATA_FLOAT;
	varrays[1].size = 2;//sizeof(modelTexCoord_t)*pmdl->numAbsVerts;
	vsizes[1] = varrays[1].size*sizeof(float)*pmdl->numAbsVerts;
	varrays[1].offset = varofs;
	varMem[1] = (BYTE *)rapi->Noesis_PooledAlloc(vsizes[1]);
	COPY_FLAT_MESH_ARRAY(varMem[1], uvs, sizeof(modelTexCoord_t));
	varofs += vsizes[1];
	//set up normal array
	varrays[2].type = IQMTYPE_NORMAL;
	varrays[2].format = IQMDATA_FLOAT;
	varrays[2].size = 3;//sizeof(modelVert_t)*pmdl->numAbsVerts;
	vsizes[2] = varrays[2].size*sizeof(float)*pmdl->numAbsVerts;
	varrays[2].offset = varofs;
	varMem[2] = (BYTE *)rapi->Noesis_PooledAlloc(vsizes[2]);
	COPY_FLAT_MESH_ARRAY(varMem[2], normals, sizeof(modelVert_t));
	varofs += vsizes[2];
	if (exportVertexWeights)
	{
		//set up blend index array
		varrays[3].type = IQMTYPE_BLENDINDICES;
		varrays[3].format = IQMDATA_INT;
		varrays[3].size = 4;
		vsizes[3] = varrays[3].size*sizeof(int)*pmdl->numAbsVerts;
		varrays[3].offset = varofs;
		varMem[3] = (BYTE *)rapi->Noesis_PooledAlloc(vsizes[3]);
		COPY_FLAT_MESH_ARRAY(varMem[3], flatBoneIdx, (sizeof(int)*4));
		varofs += vsizes[3];
		//set up blend weight array
		varrays[4].type = IQMTYPE_BLENDWEIGHTS;
		varrays[4].format = IQMDATA_FLOAT;
		varrays[4].size = 4;//sizeof(float)*4*pmdl->numAbsVerts;
		vsizes[4] = varrays[4].size*sizeof(float)*pmdl->numAbsVerts;
		varrays[4].offset = varofs;
		varMem[4] = (BYTE *)rapi->Noesis_PooledAlloc(vsizes[4]);
		COPY_FLAT_MESH_ARRAY(varMem[4], flatBoneWgt, (sizeof(float)*4));
		varofs += vsizes[4];
	}
	hdr.numVerts = pmdl->numAbsVerts;

	//fill in triangles
	hdr.ofsTri = varofs;
	hdr.numTri = pmdl->numAbsTris;
	hdr.ofsAdj = hdr.ofsTri + hdr.numTri*sizeof(iqmTri_t);

	iqmJointv2_t *joints = NULL;
	iqmPosev2_t *poses = NULL;
	iqmAnim_t *anims = NULL;
	WORD *frameData = NULL;
	int frameDataSize = 0;
	iqmBounds_t *bounds = NULL;

	const DWORD postGeoOffset = hdr.ofsAdj + hdr.numTri*sizeof(iqmAdj_t);
	if (pmdl->numBones > 0)
	{
		//create bones
		hdr.ofsJoints = postGeoOffset;
		hdr.numJoints = pmdl->numBones;
		joints = (iqmJointv2_t *)rapi->Noesis_PooledAlloc(sizeof(iqmJointv2_t)*hdr.numJoints);
		for (int i = 0; i < (int)hdr.numJoints; i++)
		{
			iqmJointv2_t *joint = joints+i;
			modelBone_t *bone = pmdl->bones+i;
			joint->name = boneNames[i];
			joint->parent = (bone->eData.parent) ? bone->eData.parent->index : -1;
			if (bone->eData.parent)
			{ //iqm stores base pose matrices as parent-relative, even though it should probably just store the inverse
				RichMat43 pinv(bone->eData.parent->mat);
				pinv.Inverse();
				RichMat43 &bmat = *(RichMat43 *)&bone->mat;

				RichMat43 prel = bmat*pinv;
				Model_IQM_MatToJointData(joint->trans, joint->rot, joint->scale, prel);
			}
			else
			{
				Model_IQM_MatToJointData(joint->trans, joint->rot, joint->scale, *(RichMat43 *)&bone->mat);
			}
		}

		if (iqma && iqma->mats && iqma->numBones == hdr.numJoints && iqma->numFrames > 0)
		{ //add in the animation data
			//create poses and anim data
			hdr.numPoses = hdr.numJoints;
			hdr.numAnims = 1;
			hdr.numFrames = iqma->numFrames;
			hdr.ofsPoses = hdr.ofsJoints + hdr.numJoints*sizeof(iqmJointv2_t);
			hdr.ofsAnims = hdr.ofsPoses + hdr.numPoses*sizeof(iqmPosev2_t);

			anims = (iqmAnim_t *)rapi->Noesis_PooledAlloc(sizeof(iqmAnim_t)*hdr.numAnims);
			memset(anims, 0, sizeof(iqmAnim_t)*hdr.numAnims);
			anims->name = animName;
			anims->firstFrame = 0;
			anims->numFrames = hdr.numFrames;
			anims->framerate = iqma->frameRate;
			hdr.ofsFrames = hdr.ofsAnims + sizeof(iqmAnim_t)*hdr.numAnims;

			iqmPoseData_t *pds = (iqmPoseData_t *)rapi->Noesis_PooledAlloc(sizeof(iqmPoseData_t)*iqma->numBones*iqma->numFrames);
			iqmPoseEncData_t *peds = (iqmPoseEncData_t *)rapi->Noesis_PooledAlloc(sizeof(iqmPoseEncData_t)*hdr.numJoints);
			//convert all of the anim matrices to iqm's t+r+s, figure out pose constraints
			for (int i = 0; i < (int)hdr.numFrames; i++)
			{
				for (int j = 0; j < (int)hdr.numPoses; j++)
				{
					RichMat43 *animMat = iqma->mats + i*hdr.numPoses + j;
					iqmPoseData_t *animPD = pds + i*hdr.numPoses + j;
					Model_IQM_MatToJointData(*(RichVec3 *)&animPD->d[0], &animPD->d[3], *(RichVec3 *)&animPD->d[/*6*/7], *animMat);
					iqmPoseEncData_t *ped = peds+j;
					for (int k = 0; k < NUM_IQM_ANIM_CHANNELS; k++)
					{
						if (i == 0)
						{
							ped->minVals[k] = animPD->d[k];
							ped->maxVals[k] = animPD->d[k];
						}
						else
						{
							ped->minVals[k] = g_mfn->Math_Min2(ped->minVals[k], animPD->d[k]);
							ped->maxVals[k] = g_mfn->Math_Max2(ped->maxVals[k], animPD->d[k]);
						}
					}
				}
			}
			//create the pose data
			poses = (iqmPosev2_t *)rapi->Noesis_PooledAlloc(sizeof(iqmPosev2_t)*hdr.numPoses);
			memset(poses, 0, sizeof(iqmPosev2_t)*hdr.numPoses);
			hdr.numFrameChannels = 0;
			float invSRange = 1.0f/65535.0f;
			for (int i = 0; i < (int)hdr.numPoses; i++)
			{
				iqmPosev2_t *pose = poses+i;
				iqmJointv2_t *joint = joints+i;
				pose->parent = joint->parent;
				iqmPoseEncData_t *ped = peds+i;
				for (int j = 0; j < NUM_IQM_ANIM_CHANNELS; j++)
				{
					if (ped->minVals[j] != ped->maxVals[j])
					{ //it moved
						pose->channelMask |= (1<<j);
						pose->chanOfs[j] = ped->minVals[j];
						pose->chanScale[j] = (ped->maxVals[j]-ped->minVals[j])*invSRange;
						hdr.numFrameChannels++;
					}
					else
					{
						pose->chanOfs[j] = ped->minVals[j];
						pose->chanScale[j] = 0.0f;
					}
				}
			}

			//encode the frame data
			frameData = (WORD *)rapi->Noesis_PooledAlloc(sizeof(WORD)*hdr.numPoses*hdr.numFrames*NUM_IQM_ANIM_CHANNELS);
			for (int i = 0; i < (int)hdr.numFrames; i++)
			{
				for (int j = 0; j < (int)hdr.numPoses; j++)
				{
					iqmPoseEncData_t *ped = peds+j;
					iqmPosev2_t *pose = poses+j;
					iqmPoseData_t *animPD = pds + i*hdr.numPoses + j;
					for (int k = 0; k < NUM_IQM_ANIM_CHANNELS; k++)
					{
						if (pose->channelMask & (1<<k))
						{
							float f = ((animPD->d[k]-ped->minVals[k])/(ped->maxVals[k]-ped->minVals[k])) * 65535.0f;
							WORD *fd = frameData+frameDataSize/2;
							frameDataSize += 2;
							*fd = (WORD)g_mfn->Math_Min2(f, 65535.0f);
						}
					}
				}
			}

			//create per-frame bounds
			hdr.ofsBounds = hdr.ofsFrames + frameDataSize;
			bounds = (iqmBounds_t *)rapi->Noesis_PooledAlloc(hdr.numFrames*sizeof(iqmBounds_t));
			for (int i = 0; i < (int)hdr.numFrames; i++)
			{
				iqmBounds_t *bb = bounds+i;
				rapi->rpgTransformModel(pmdl, (modelMatrix_t *)iqma->mats, i);
				rapi->rpgGetModelExtents(pmdl, bb->mins, bb->maxs, &bb->rad, &bb->xyRad, false);
			}

			hdr.fileSize = hdr.ofsBounds + hdr.numFrames*sizeof(iqmBounds_t);
		}
		else
		{
			hdr.fileSize = hdr.ofsJoints + hdr.numJoints*sizeof(iqmJointv2_t);
		}
	}
	else
	{
		rapi->LogOutput("WARNING: Non-skeletal IQM's may be unsupported and/or less than optimal in any given implementation.\n");
		if (iqma)
		{
			rapi->LogOutput("WARNING: Skeletal animation is being dropped on IQM export because model has no skeleton.\n");
		}
		hdr.fileSize = postGeoOffset;
	}

	//now write it all out
	outStream->WriteBytes(&hdr, sizeof(hdr));
	if (hdr.numText > 0)
	{
		outStream->WriteBytes(rapi->Noesis_StrPoolMem(strPool), rapi->Noesis_StrPoolSize(strPool));
	}
	if (hdr.numMeshes > 0)
	{
		outStream->WriteBytes(meshes, sizeof(iqmMesh_t)*hdr.numMeshes);
	}
	if (hdr.numVertAr > 0)
	{
		outStream->WriteBytes(varrays, sizeof(iqmVArray_t)*hdr.numVertAr);
		for (int i = 0; i < (int)hdr.numVertAr; i++)
		{
			outStream->WriteBytes(varMem[i], vsizes[i]);
		}
	}
	if (hdr.numTri > 0)
	{
		outStream->WriteBytes(pmdl->absTris, hdr.numTri*sizeof(iqmTri_t));
		outStream->WriteBytes(pmdl->absTriNeighbors, hdr.numTri*sizeof(iqmAdj_t));
	}
	if (hdr.numJoints > 0 && joints)
	{
		outStream->WriteBytes(joints, hdr.numJoints*sizeof(iqmJointv2_t));
	}
	if (hdr.numPoses > 0 && poses)
	{
		outStream->WriteBytes(poses, hdr.numPoses*sizeof(iqmPosev2_t));
	}
	if (hdr.numAnims > 0 && anims)
	{
		outStream->WriteBytes(anims, hdr.numAnims*sizeof(iqmAnim_t));
	}
	if (hdr.numFrames > 0 && frameData && frameDataSize > 0)
	{
		outStream->WriteBytes(frameData, frameDataSize);
	}
	if (hdr.numFrames > 0 && bounds)
	{
		outStream->WriteBytes(bounds, hdr.numFrames*sizeof(iqmBounds_t));
	}

	rapi->Noesis_DestroyStrPool(strPool);

	return true;
}

//catch anim writes
//(note that this function would normally write converted data to a file at anim->filename, but for this format it instead saves the data to combine with the model output)
void Model_IQM_WriteAnim(noesisAnim_t *anim, noeRAPI_t *rapi)
{
	if (!rapi->Noesis_HasActiveGeometry() || rapi->Noesis_GetActiveType() != g_fmtHandle)
	{ //could instead export an anim-only iqm in this condition, but i suspect the functionality wouldn't be used enough to even be worth writing
		rapi->LogOutput("WARNING: Stand-alone animations cannot be converted to IQM.\nNothing will be written.\n");
		return;
	}

	rapi->Noesis_SetExtraAnimData(anim->data, anim->dataLen);
}
