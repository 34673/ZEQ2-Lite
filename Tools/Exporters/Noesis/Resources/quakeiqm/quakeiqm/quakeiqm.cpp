//General requirements and/or suggestions for Noesis plugins:
//
// -Use 1-byte struct alignment for all shared structures. (plugin MSVC project file defaults everything to 1-byte-aligned)
// -Always clean up after yourself. Your plugin stays in memory the entire time Noesis is loaded, so you don't want to crap up the process heaps.
// -Try to use reliable type-checking, to ensure your plugin doesn't conflict with other file types and create false-positive situations.
// -Really try not to write crash-prone logic in your data check function! This could lead to Noesis crashing from trivial things like the user browsing files.
// -When using the rpg begin/end interface, always make your Vertex call last, as it's the function which triggers a push of the vertex with its current attributes.
// -!!!! Check the web site and documentation for updated suggestions/info! !!!!

#include "stdafx.h"
#include "quakeiqm.h"

extern bool Model_IQM_Write(noesisModel_t *mdl, RichBitStream *outStream, noeRAPI_t *rapi);
extern void Model_IQM_WriteAnim(noesisAnim_t *anim, noeRAPI_t *rapi);

const char *g_pPluginName = "quakeiqm";
const char *g_pPluginDesc = "IQM format handler, by Dick.";

int g_fmtHandle = -1;

//map iqm standard types to rpgeo types
rpgeoDataType_e g_iqmToRPG[9] =
{
	RPGEODATA_BYTE,
	RPGEODATA_UBYTE,
	RPGEODATA_SHORT,
	RPGEODATA_USHORT,
	RPGEODATA_INT,
	RPGEODATA_UINT,
	RPGEODATA_HALFFLOAT,
	RPGEODATA_FLOAT,
	RPGEODATA_DOUBLE
};
//iqm standard type sizes
int g_iqmTypeSize[9] =
{
	1,
	1,
	2,
	2,
	4,
	4,
	2,
	4,
	8
};

static int g_iqmElemDefaultCounts[IQM_NUM_VTYPES] =
{
	3, //IQMTYPE_POSITION
	2, //IQMTYPE_TEXCOORD
	3, //IQMTYPE_NORMAL
	4, //IQMTYPE_TANGENT
	4, //IQMTYPE_BLENDINDICES
	4, //IQMTYPE_BLENDWEIGHTS
	4, //IQMTYPE_COLOR
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1
};

//see if something is valid iqm data
bool Model_IQM_Check(BYTE *fileBuffer, int bufferLen, noeRAPI_t *rapi)
{
	if (bufferLen < sizeof(iqmHdr_t))
	{
		return false;
	}
	iqmHdr_t *hdr = (iqmHdr_t *)fileBuffer;
	if (memcmp(hdr->id, "INTERQUAKEMODEL", 15))
	{
		return false;
	}
	if (hdr->ver != 1 && hdr->ver != 2)
	{
		return false;
	}
	if (hdr->fileSize <= 0 || hdr->fileSize > bufferLen)
	{
		return false;
	}
	return true;
}

//convert translation+rotation+scale to a matrix
static void Model_IQM_JointDataToMat(int ver, RichVec3 &trans, float *rot, RichVec3 &scale, RichMat43 &mat, bool dq)
{
	mat = RichQuat(rot, (ver <= 1)).ToMat43();
	if (ver > 1)
	{ //retarded
		mat.Transpose();
	}
	mat[0] *= scale[0];
	mat[1] *= scale[1];
	mat[2] *= scale[2];
	mat[3] = trans;
}

//load it (note that you don't need to worry about validation here, if it was done in the Check function)
noesisModel_t *Model_IQM_Load(BYTE *fileBuffer, int bufferLen, int &numMdl, noeRAPI_t *rapi)
{
	iqmHdr_t *hdr = (iqmHdr_t *)fileBuffer;
	if (hdr->numMeshes == 0 || hdr->numTri == 0 || hdr->numVertAr == 0 || hdr->numVerts == 0)
	{ //only support mesh-inclusive files
		return NULL;
	}
	int numUsedChannels = (hdr->ver <= 1) ? 9 : 10;
	void *pgctx = rapi->rpgCreateContext();

	iqmTri_t *tris = (iqmTri_t *)(fileBuffer+hdr->ofsTri);
	iqmMesh_t *meshes = (iqmMesh_t *)(fileBuffer+hdr->ofsMeshes);

	//bind all of the arrays
	iqmVArray_t *varrays = (iqmVArray_t *)(fileBuffer+hdr->ofsVertAr);
	for (int i = 0; i < (int)hdr->numVertAr; i++)
	{
		iqmVArray_t *varray = varrays+i;
		int elemCount = varray->size;
		if (elemCount > MAX_IQM_DEFAULT_ELEM_COUNT)
		{ //fix to handle old iqm output which was writing buffer size to the size field
			elemCount = g_iqmElemDefaultCounts[varray->type];
		}
		switch (varray->type)
		{
		case IQMTYPE_POSITION:
			rapi->rpgBindPositionBuffer(fileBuffer+varray->offset, g_iqmToRPG[varray->format], g_iqmTypeSize[varray->format]*elemCount);
			break;
		case IQMTYPE_TEXCOORD:
			rapi->rpgBindUV1Buffer(fileBuffer+varray->offset, g_iqmToRPG[varray->format], g_iqmTypeSize[varray->format]*elemCount);
			break;
		case IQMTYPE_NORMAL:
			rapi->rpgBindNormalBuffer(fileBuffer+varray->offset, g_iqmToRPG[varray->format], g_iqmTypeSize[varray->format]*elemCount);
			break;
		case IQMTYPE_COLOR:
			rapi->rpgBindColorBuffer(fileBuffer+varray->offset, g_iqmToRPG[varray->format], g_iqmTypeSize[varray->format]*elemCount, elemCount);
			break;
		case IQMTYPE_BLENDINDICES:
			rapi->rpgBindBoneIndexBuffer(fileBuffer+varray->offset, g_iqmToRPG[varray->format], g_iqmTypeSize[varray->format]*elemCount, elemCount);
			break;
		case IQMTYPE_BLENDWEIGHTS:
			rapi->rpgBindBoneWeightBuffer(fileBuffer+varray->offset, g_iqmToRPG[varray->format], g_iqmTypeSize[varray->format]*elemCount, elemCount);
			break;
		default:
			break;
		}
	}
	
	//run through the meshes and draw into the rpgeo
	for (int i = 0; i < (int)hdr->numMeshes; i++)
	{
		iqmMesh_t *mesh = meshes+i;
		iqmTri_t *meshTris = tris+mesh->firstTri;
		char *meshName = (mesh->name) ? (char *)(fileBuffer+hdr->ofsText+mesh->name) : NULL;
		char *matName = (mesh->material) ? (char *)(fileBuffer+hdr->ofsText+mesh->material) : NULL;

		rapi->rpgSetName(meshName);
		rapi->rpgSetMaterial(matName);
		rapi->rpgCommitTriangles(meshTris, RPGEODATA_UINT, mesh->numTris*3, RPGEO_TRIANGLE, false);
	}

	int jointSize = (hdr->ver <= 1) ? sizeof(iqmJoint_t) : sizeof(iqmJointv2_t);
	int poseSize = (hdr->ver <= 1) ? sizeof(iqmPose_t) : sizeof(iqmPosev2_t);

	float framerate = 20.0f;
	if (hdr->numJoints > 0)
	{ //convert joints
		modelBone_t *bones = rapi->Noesis_AllocBones(hdr->numJoints);
		for (int i = 0; i < (int)hdr->numJoints; i++)
		{
			iqmJoint_t *joint = (iqmJoint_t *)(fileBuffer+hdr->ofsJoints + i*jointSize);
			modelBone_t *bone = bones+i;
			if (joint->name)
			{
				strncpy_s(bone->name, MAX_BONE_NAME_LEN-1, (char *)(fileBuffer+hdr->ofsText+joint->name), MAX_BONE_NAME_LEN-2);
			}
			else
			{
				sprintf_s(bone->name, MAX_BONE_NAME_LEN-1, "bone%i", i);
			}
			bone->eData.parent = (joint->parent >= 0) ? bones+joint->parent : NULL;
			if (hdr->ver > 1)
			{
				iqmJointv2_t *joint2 = (iqmJointv2_t *)joint;
				Model_IQM_JointDataToMat(hdr->ver, joint2->trans, joint2->rot, joint2->scale, *(RichMat43 *)&bone->mat, true);
			}
			else
			{
				Model_IQM_JointDataToMat(hdr->ver, joint->trans, joint->rot, joint->scale, *(RichMat43 *)&bone->mat, true);
			}
		}
		rapi->rpgMultiplyBones(bones, hdr->numJoints);
		rapi->rpgSetExData_Bones(bones, hdr->numJoints);

		if (hdr->numAnims > 0 && hdr->numFrames > 0 && hdr->numPoses == hdr->numJoints)
		{ //load anims
			iqmAnim_t *anims = (iqmAnim_t *)(fileBuffer+hdr->ofsAnims);
			modelMatrix_t *animMats = (modelMatrix_t *)rapi->Noesis_UnpooledAlloc(sizeof(modelMatrix_t)*hdr->numJoints*hdr->numFrames);
			WORD *frameData = (WORD *)(fileBuffer+hdr->ofsFrames);
			for (int i = 0; i < (int)hdr->numFrames; i++)
			{
				for (int j = 0; j < (int)hdr->numPoses; j++)
				{
					float transRotScale[10];
					if (hdr->ver == 1)
					{
						iqmPose_t *pose = (iqmPose_t *)(fileBuffer+hdr->ofsPoses + j*poseSize);
						for (int k = 0; k < numUsedChannels; k++)
						{
							transRotScale[k] = pose->chanOfs[k];
							if (pose->channelMask & (1<<k))
							{
								WORD d = *frameData++;
								transRotScale[k] += (float)d * pose->chanScale[k];
							}
						}
						modelMatrix_t *animMat = animMats + i*hdr->numJoints + j;
						Model_IQM_JointDataToMat(hdr->ver, *(RichVec3 *)&transRotScale[0], &transRotScale[3], *(RichVec3 *)&transRotScale[6], *(RichMat43 *)animMat, true);
					}
					else
					{
						iqmPosev2_t *pose = (iqmPosev2_t *)(fileBuffer+hdr->ofsPoses + j*poseSize);
						for (int k = 0; k < numUsedChannels; k++)
						{
							transRotScale[k] = pose->chanOfs[k];
							if (pose->channelMask & (1<<k))
							{
								WORD d = *frameData++;
								transRotScale[k] += (float)d * pose->chanScale[k];
							}
						}
						modelMatrix_t *animMat = animMats + i*hdr->numJoints + j;
						Model_IQM_JointDataToMat(hdr->ver, *(RichVec3 *)&transRotScale[0], &transRotScale[3], *(RichVec3 *)&transRotScale[7], *(RichMat43 *)animMat, true);
					}
				}
			}

			framerate = anims->framerate; //take the framerate from the first sequence - maybe bother to implement this per-sequence some day
			noesisAnim_t *anim = rapi->rpgAnimFromBonesAndMatsFinish(bones, hdr->numJoints, animMats, hdr->numFrames, anims->framerate);
			rapi->rpgSetExData_Anims(anim);
			rapi->Noesis_UnpooledFree(animMats);
		}
	}

	rapi->rpgSetTriWinding(true);
	rapi->rpgOptimize();
	noesisModel_t *mdl = rapi->rpgConstructModel();
	if (mdl)
	{
		numMdl = 1; //it's important to set this on success! you can set it to > 1 if you have more than 1 contiguous model in memory
		rapi->SetPreviewAnimSpeed(framerate);
		//this'll rotate the model (only in preview mode) into quake-friendly coordinates
		static float mdlAngOfs[3] = {0.0f, 180.0f, 0.0f};
		rapi->SetPreviewAngOfs(mdlAngOfs);
	}

	rapi->rpgDestroyContext(pgctx);
	return mdl;
}

//called by Noesis to init the plugin
bool NPAPI_InitLocal(void)
{
	g_fmtHandle = g_nfn->NPAPI_Register("Inter-Quake Model", ".iqm");
	if (g_fmtHandle < 0)
	{
		return false;
	}

	//set the data handlers for this format
	g_nfn->NPAPI_SetTypeHandler_TypeCheck(g_fmtHandle, Model_IQM_Check);
	g_nfn->NPAPI_SetTypeHandler_LoadModel(g_fmtHandle, Model_IQM_Load);
	g_nfn->NPAPI_SetTypeHandler_WriteModel(g_fmtHandle, Model_IQM_Write);
	g_nfn->NPAPI_SetTypeHandler_WriteAnim(g_fmtHandle, Model_IQM_WriteAnim);

	return true;
}

//called by Noesis before the plugin is freed
void NPAPI_ShutdownLocal(void)
{
	//nothing to do in this plugin
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
    return TRUE;
}
