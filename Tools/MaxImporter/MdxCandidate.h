

#ifndef __MdxCandidate_h__
#define __MdxCandidate_h__

#include "OgreMdx.h"
#include "MaxInterface.h"
using namespace Ogre;

class CMdxCandidate
{
public:
	CMdxCandidate();
	~CMdxCandidate();

	void ImportMdx(char *filename);
private:
	void OnImportVertex(CMdxGeoChunk *pChunk, Mesh &mesh);
	void OnImportFace(CMdxGeoChunk *pChunk, Mesh &mesh);
	void OnImportTVert(CMdxGeoChunk *pChunk, Mesh &mesh);
	void OnImportTexture(char *filename, CMdx *pMdx, CMdxGeoChunk *pChunk, Mesh &mesh, INode *pNode);

	CMdx *m_pMdx;
};

#endif