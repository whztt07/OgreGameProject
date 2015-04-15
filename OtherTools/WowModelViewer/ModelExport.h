
#ifndef __ModelExport_h__
#define __ModelExport_h__

#include "modelcanvas.h"
#include "Ogre3d.h"
#include "OgreDataChunkWrite.h"
using namespace Ogre;

class CModelExport
{
public:
	CModelExport(CModel *pModel);
	~CModelExport();

	void Export(char *filedir);
private:
	void ExportModelFile(char *filedir);
	void ExportSkeletonFile(char *filedir);
	void ExportTexutre(char *filedir);
	void ProcessRenderPass();
	void WriteSkeleton(CDataChunkWrite &w);
	void WriteAnimSet(CDataChunkWrite &w);
	void WriteBone(CDataChunkWrite &w, int nIdx);
	void WriteGeometry(CDataChunkWrite &w);
	void WriteMaterials(CDataChunkWrite &w);
	void WriteColors(CDataChunkWrite &w);
	void WriteChunk(CDataChunkWrite &w, int nPass);
	void WriteMaterial(CDataChunkWrite &w, int nPass);
	void WrieteChunkBaseInfo(CDataChunkWrite &w, int nPass);
	void WriteChunkBoneInfo(CDataChunkWrite &w, int nPass);
	void WrieteMaterialBaseInfo(CDataChunkWrite &w, int nPass);

	char m_szFileDir[MAX_PATH];
	CModel *m_pModel;
};

#endif