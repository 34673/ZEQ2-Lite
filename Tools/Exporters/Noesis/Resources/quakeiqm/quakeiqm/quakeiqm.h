#include <math.h>

#define NUM_IQM_ANIM_CHANNELS		10 //was 9 in v1

typedef struct iqmHdr_s
{
	BYTE			id[16];
	int				ver;
	int				fileSize;
	DWORD			flags;
	DWORD			numText;
	DWORD			ofsText;
	DWORD			numMeshes;
	DWORD			ofsMeshes;
	DWORD			numVertAr;
	DWORD			numVerts;
	DWORD			ofsVertAr;
	DWORD			numTri;
	DWORD			ofsTri;
	DWORD			ofsAdj;
	DWORD			numJoints;
	DWORD			ofsJoints;
	DWORD			numPoses;
	DWORD			ofsPoses;
	DWORD			numAnims;
	DWORD			ofsAnims;
	DWORD			numFrames;
	DWORD			numFrameChannels;
	DWORD			ofsFrames;
	DWORD			ofsBounds;
	DWORD			numComment;
	DWORD			ofsComment;
	DWORD			numExt;
	DWORD			ofsExt;
} iqmHdr_t;
typedef struct iqmMesh_s
{
	DWORD			name;
	DWORD			material;
	DWORD			firstVert;
	DWORD			numVerts;
	DWORD			firstTri;
	DWORD			numTris;
} iqmMesh_t;
typedef struct iqmVArray_s
{
	DWORD			type;
	DWORD			flags;
	DWORD			format;
	DWORD			size;
	DWORD			offset;
} iqmVArray_t;
typedef struct iqmTri_s
{
	DWORD			idx[3];
} iqmTri_t;
typedef struct iqmAdj_s
{
	DWORD			tri[3];
} iqmAdj_t;
typedef struct iqmJoint_s
{
	DWORD			name;
	int				parent;
	RichVec3		trans;
	float			rot[3];
	RichVec3		scale;
} iqmJoint_t;
typedef struct iqmJointv2_s
{
	DWORD			name;
	int				parent;
	RichVec3		trans;
	float			rot[4];
	RichVec3		scale;
} iqmJointv2_t;
typedef struct iqmPose_s
{
	int				parent;
	DWORD			channelMask;
	float			chanOfs[9];
	float			chanScale[9];
} iqmPose_t;
typedef struct iqmPosev2_s
{
	int				parent;
	DWORD			channelMask;
	float			chanOfs[10];
	float			chanScale[10];
} iqmPosev2_t;
typedef struct iqmAnim_s
{
	DWORD			name;
	DWORD			firstFrame;
	DWORD			numFrames;
	float			framerate;
	DWORD			flags;
} iqmAnim_t;
typedef struct iqmBounds_s
{
	float			mins[3];
	float			maxs[3];
	float			xyRad;
	float			rad;
} iqmBounds_t;
#define MAX_IQM_DEFAULT_ELEM_COUNT	4
typedef enum
{
	IQMTYPE_POSITION = 0,
	IQMTYPE_TEXCOORD,
	IQMTYPE_NORMAL,
	IQMTYPE_TANGENT,
	IQMTYPE_BLENDINDICES,
	IQMTYPE_BLENDWEIGHTS,
	IQMTYPE_COLOR,
	IQMTYPE_CUSTOM = 16,
	IQM_NUM_VTYPES
} iqmVType_e;

typedef enum
{
	IQMDATA_BYTE = 0,
	IQMDATA_UBYTE,
	IQMDATA_SHORT,
	IQMDATA_USHORT,
	IQMDATA_INT,
	IQMDATA_UINT,
	IQMDATA_HALF,
	IQMDATA_FLOAT,
	IQMDATA_DOUBLE
} iqmDType;

extern rpgeoDataType_e g_iqmToRPG[9];
extern int g_iqmTypeSize[9];
extern mathImpFn_t *g_mfn;
extern noePluginFn_t *g_nfn;
extern int g_fmtHandle;
