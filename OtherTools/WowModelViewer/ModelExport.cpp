

#include "ModelExport.h"
#include "OgreFile.h"

extern CMpqTextureManager g_TextureManager;

CModelExport::CModelExport(CModel *pModel)
{
	m_pModel = pModel;
}

CModelExport::~CModelExport()
{

}

void CModelExport::Export(char *filedir)
{
	ExportModelFile(filedir);
	ExportSkeletonFile(filedir);
}

void CModelExport::ExportSkeletonFile(char *filedir)
{
	if (!m_pModel->m_bAnimBones)
	{
		return;
	}

	CFilePath path;
	path.Split((char*)m_pModel->m_strName.c_str());
	char szFile[MAX_PATH];
	sprintf(szFile, "%s%s%s%s%s", filedir, "\\", path.GetDirectory(), path.GetFileName(), ".skeleton");

	CDataChunkWrite w(1024 * 1024 * 100);
	w.StartChunk(DC_TAG(CMdxMesh::SKELETON));
	WriteSkeleton(w);
	w.EndChunk(DC_TAG(CMdxMesh::SKELETON));

	w.StartChunk(DC_TAG(CMdxMesh::ANIMSET));
	WriteAnimSet(w);
	w.EndChunk(DC_TAG(CMdxMesh::ANIMSET));

	w.SaveToFile(szFile);
}

void CModelExport::WriteSkeleton(CDataChunkWrite &w)
{
	w.WriteInt(CMdxSkeleton::SKELETONTYPE_WOW);

	int nBoneNum = m_pModel->m_ModelHeader.nBones;
	for (int i = 0; i < nBoneNum; i++)
	{
		w.StartChunk(DC_TAG(CMdxSkeleton::SKELETON_BONE_WOW));
		WriteBone(w, i);
		w.EndChunk(DC_TAG(CMdxSkeleton::SKELETON_BONE_WOW));
	}
}

void CModelExport::WriteAnimSet(CDataChunkWrite &w)
{
	w.WriteInt(CMdxAnimSet::ANIMSET_TYPE_WOW);

	int nAnimNum = m_pModel->m_ModelHeader.nAnimations;
	for (int i = 0; i < nAnimNum; i++)
	{
		w.StartChunk(DC_TAG(CMdxAnimSet::ANIMSET_ANIM));
		w.Write((char*)m_pModel->m_vecAnimName[i].c_str(), 1, CBaseAnim::ANIM_NAME_SIZE);
		CModelAnimation &anim = m_pModel->m_pAnim[i];
		w.WriteDWORD(anim.timeStart);
		w.WriteDWORD(anim.timeEnd);
		w.EndChunk(DC_TAG(CMdxAnimSet::ANIMSET_ANIM));
	}
}

void CModelExport::WriteBone(CDataChunkWrite &w, int nIdx)
{
	w.StartChunk(DC_TAG(CWOWBone::WOW_BONE_BASE_INFO));
	CBone &bone = m_pModel->m_pBone[nIdx];
	int nTransTimeNum = bone.m_vecTransTime.size();	
	int nTimeRangeNum = bone.m_vecTransRange.size();
	int nTransNum = bone.m_vecTrans.size();
	int nScaleTimeNum = bone.m_vecScaleTime.size();
	int nScaleRangeNum = bone.m_vecScaleRange.size();
	int nScaleNum = bone.m_vecScale.size();
	int nQuatTimeNum = bone.m_vecQuatTime.size();
	int nQuatRangeNum = bone.m_vecQuatRange.size();
	int nQuatNum = bone.m_vecQuat.size();
	w.WriteInt(bone.m_nParent);
	w.WriteInt(bone.m_bUsedTrans);
	w.WriteInt(bone.m_bUsedScale);
	w.WriteInt(bone.m_bUsedQuat);
	w.WriteInt(nTransTimeNum);
	w.WriteInt(nScaleTimeNum);
	w.WriteInt(nQuatTimeNum);
	w.WriteInt(nTimeRangeNum);
	w.WriteInt(nScaleRangeNum);
	w.WriteInt(nQuatRangeNum);
	w.WriteInt(nTransNum);
	w.WriteInt(nScaleNum);
	w.WriteInt(nQuatNum);
	
	w.Write(&bone.m_vPivot, sizeof(CVector), 1);
	if (nTransTimeNum > 0)
	{
		w.Write(&bone.m_vecTransTime[0], sizeof(uint32), nTransTimeNum);
	}
	if (nScaleTimeNum > 0)
	{
		w.Write(&bone.m_vecScaleTime[0], sizeof(uint32), nScaleTimeNum);
	}
	if (nQuatTimeNum > 0)
	{
		w.Write(&bone.m_vecQuatTime[0], sizeof(uint32), nQuatTimeNum);
	}
	if (nTimeRangeNum > 0)
	{
		w.Write(&bone.m_vecTransRange[0], sizeof(Ogre::MdxAnimRange), nTimeRangeNum);
	}
	if (nScaleRangeNum > 0)
	{
		w.Write(&bone.m_vecScaleRange[0], sizeof(Ogre::MdxAnimRange), nScaleRangeNum);
	}
	if (nQuatRangeNum > 0)
	{
		w.Write(&bone.m_vecQuatRange[0], sizeof(Ogre::MdxAnimRange), nQuatRangeNum);
	}
	if (nTransNum > 0)
	{
		w.Write(&bone.m_vecTrans[0], sizeof(Ogre::CVector), nTransNum);
	}
	if (nScaleNum > 0)
	{
		w.Write(&bone.m_vecScale[0], sizeof(Ogre::CVector), nScaleNum);
	}
	if (nQuatNum > 0)
	{
		w.Write(&bone.m_vecQuat[0], sizeof(Ogre::CQuaternion), nQuatNum);
	}
	w.EndChunk(DC_TAG(CWOWBone::WOW_BONE_BASE_INFO));
}

void CModelExport::ExportModelFile(char *filedir)
{
	ProcessRenderPass();
	ExportTexutre(filedir);

	CDataChunkWrite w(1024 * 1024 * 100);
	w.StartChunk(DC_TAG(CMdxMesh::GEOMETRY));
	WriteGeometry(w);
	w.EndChunk(DC_TAG(CMdxMesh::GEOMETRY));
	
	w.StartChunk(DC_TAG(CMdxMesh::MATERIALS));
	WriteMaterials(w);
	w.EndChunk(DC_TAG(CMdxMesh::MATERIALS));

	w.StartChunk(DC_TAG(CMdxMesh::WOWCOLOR));
	WriteColors(w);
	w.EndChunk(DC_TAG(CMdxMesh::WOWCOLOR));
	
	std::string name = m_pModel->m_strName;
	Ogre::CFilePath path;
	path.Split((char*)name.c_str());
	Ogre::CFilePath::MakeDirectory((char*)name.c_str());

	char szFile[MAX_PATH];
	sprintf(szFile, "%s%s%s", m_szFileDir, path.GetFileName(), ".mdx");
	w.SaveToFile(szFile);

	w.Destroy();
}

void CModelExport::WriteGeometry(CDataChunkWrite &w)
{
	int nCount = 0;

	for (int i = 0; i < m_pModel->m_ModelHeader.nVertices; i++)
	{
		m_pModel->m_pVertices[i] = m_pModel->m_pOrigVertices[i].pos;
		m_pModel->m_pNormals[i] = m_pModel->m_pOrigVertices[i].normal;
	}

	int nPass = m_pModel->m_vecOutputPass.size();
	for (int i = 0; i < nPass; i++)
	{
		CModelRenderPass *pPass = &m_pModel->m_vecOutputPass[i];
		if (!pPass->m_bRender)
		{
			continue;
		}

		w.StartChunk(DC_TAG(CMdxGeometry::GEOMETRY_CHUNK));
		WriteChunk(w, nCount);
		w.EndChunk(DC_TAG(CMdxGeometry::GEOMETRY_CHUNK));

		nCount++;
	}
}

void CModelExport::WriteMaterials(CDataChunkWrite &w)
{
	int nCount = 0;

	int nPass = m_pModel->m_vecOutputPass.size();
	for (int i = 0; i < nPass; i++)
	{
		CModelRenderPass *pPass = &m_pModel->m_vecOutputPass[i];
		if (!pPass->m_bRender)
		{
			continue;
		}

		w.StartChunk(DC_TAG(CMdxMaterials::MATERIALS_INFO));
		WriteMaterial(w, nCount);
		w.EndChunk(DC_TAG(CMdxMaterials::MATERIALS_INFO));

		nCount++;
	}
}

void CModelExport::WriteColors(CDataChunkWrite &w)
{
	int nNum = m_pModel->m_ModelHeader.nColors;
	for (int i = 0; i < nNum; i++)
	{
		CModelColor * pColor = &m_pModel->m_pColors[i];
		if (pColor)
		{
			w.StartChunk(DC_TAG(CWOWColorGroup::WOWCOLOR_COLOR));

			int nNumTime = pColor->opacity.times.size();
			int nNumRange = pColor->opacity.ranges.size();
			int nNumColor = pColor->opacity.data.size();

			w.WriteInt(nNumTime);
			w.WriteInt(nNumRange);
			w.WriteInt(nNumColor);

			if (nNumTime)
			{
				w.Write(&pColor->opacity.times[0], sizeof(DWORD), nNumTime);
			}

			if (nNumRange)
			{
				w.Write(&pColor->opacity.ranges[0], sizeof(Ogre::MdxAnimRange), nNumRange);
			}

			if (nNumColor)
			{
				w.Write(&pColor->opacity.data[0], sizeof(float), nNumColor);
			}

			w.EndChunk(DC_TAG(CWOWColorGroup::WOWCOLOR_COLOR));
		}
	}
}

void CModelExport::WriteChunk(CDataChunkWrite &w, int nPass)
{
	w.StartChunk(DC_TAG(CMdxGeoChunk::CHUNK_BASE_INFO));
	WrieteChunkBaseInfo(w, nPass);
	w.EndChunk(DC_TAG(CMdxGeoChunk::CHUNK_BASE_INFO));

	w.StartChunk(DC_TAG(CMdxGeoChunk::CHUNK_BONE_INFO));
	WriteChunkBoneInfo(w, nPass);
	w.EndChunk(DC_TAG(CMdxGeoChunk::CHUNK_BONE_INFO));
}

void CModelExport::WriteMaterial(CDataChunkWrite &w, int nPass)
{
	w.StartChunk(DC_TAG(CMdxMaterial::MATERIAL_BASE_INFO));
	WrieteMaterialBaseInfo(w, nPass);
	w.EndChunk(DC_TAG(CMdxMaterial::MATERIAL_BASE_INFO));
}

void CModelExport::WrieteMaterialBaseInfo(CDataChunkWrite &w, int nPass)
{
	if (m_pModel == NULL)
	{
		return;
	}

	CModelRenderPass *pPass = &m_pModel->m_vecOutputPass[nPass];
	if (pPass)
	{
		int nTexID = m_pModel->GetOutputTexID(nPass);
		CMpqTexture *pTex = g_TextureManager.FindTexture(nTexID);

		Ogre::CFilePath path;
		path.Split((char*)pTex->m_strName.c_str());

		char szFile[MAX_PATH];
		sprintf(szFile, "%s%s", path.GetFileName(), ".dds");

		w.Write(szFile, 1, CMdxMaterial::MATERIAL_NAME_SIZE);
		
		bool bSpecular = pPass->m_bHasSpecular;
		w.WriteInt(bSpecular);
		if (bSpecular)
		{
			nTexID = m_pModel->GetOutputTexID(pPass->m_nSpecularPassID);
			pTex = g_TextureManager.FindTexture(nTexID);
			path.Split((char*)pTex->m_strName.c_str());
			sprintf(szFile, "%s%s", path.GetFileName(), ".dds");
			w.Write(szFile, 1, CMdxMaterial::MATERIAL_NAME_SIZE);
		}
		
		w.WriteInt(false);
		
		DWORD dwFlag = 0;
		bool bAlphaTest = (pPass->blendmode == BM_TRANSPARENT);
		if (bAlphaTest)
		{
			dwFlag |= CMdxMaterial::MATERIAL_ALPHA_TEST;
		}
		w.WriteDWORD(dwFlag);

		int nColorId = pPass->color;
		w.WriteInt(nColorId);
	}
}

void CModelExport::WrieteChunkBaseInfo(CDataChunkWrite &w, int nPass)
{
	if (m_pModel == NULL)
	{
		return;
	}

	static WORD sIndex[10000];
	CModelRenderPass *pPass = &m_pModel->m_vecOutputPass[nPass];
	if (pPass)
	{
		char szName[MAX_PATH];
		sprintf(szName, "%s%d\n", "Chunk", nPass);
		w.Write((void*)szName, 1, CMdxGeoChunk::CHUNK_NAME_SIZE);

		Ogre::CVector *pVertex = m_pModel->m_pVertices;
		Ogre::CVector *pNormal = m_pModel->m_pNormals;
		Ogre::CVector2D *pUV = m_pModel->m_pTexcoord;
		WORD *pIndex = m_pModel->m_pIndices;
		int nStartPos = pPass->vertexStart;
		int nEndPos = pPass->vertexEnd;
		int nStartIndex = pPass->indexStart;
		int nIndexNum = pPass->indexCount;
		int nPosNum = nEndPos - nStartPos;

		for (int i = 0; i < nIndexNum; i++)
		{
			sIndex[i] = pIndex[nStartIndex + i] - nStartPos;
		}

		w.WriteInt(nPosNum);
		w.WriteInt(nIndexNum);
		w.WriteInt(nPass);
		w.WriteInt(false);
		w.Write(&pVertex[nStartPos], sizeof(Ogre::CVector), nPosNum);
		w.Write(&pNormal[nStartPos], sizeof(Ogre::CVector), nPosNum);
		w.Write(&pUV[nStartPos], sizeof(Ogre::CVector2D), nPosNum);
		w.Write(&sIndex[0], sizeof(WORD), nIndexNum);
	}
}

void CModelExport::WriteChunkBoneInfo(CDataChunkWrite &w, int nPass)
{
	CModelVertex *pVertex = m_pModel->m_pOrigVertices;
	CModelRenderPass *pPass = &m_pModel->m_vecOutputPass[nPass];
	if (pPass)
	{
		int nStartPos = pPass->vertexStart;
		int nEndPos = pPass->vertexEnd;
		int nPosNum = nEndPos - nStartPos;
		for (int i = 0; i < nPosNum; i++)
		{
			w.Write(m_pModel->m_pOrigVertices[nStartPos + i].weights, sizeof(byte), 4);
		}

		for (int i = 0; i < nPosNum; i++)
		{
			w.Write(m_pModel->m_pOrigVertices[nStartPos + i].bones, sizeof(byte), 4);
		}
	}
}

void CModelExport::ExportTexutre(char *filedir)
{
	// ---------
	std::string name = m_pModel->m_strName;
	Ogre::CFilePath path;
	path.Split((char*)name.c_str());
	Ogre::CFilePath::SetCurDirectory(filedir);
	Ogre::CFilePath::MakeDirectory((char*)name.c_str());
	char szFile[MAX_PATH];
	sprintf(m_szFileDir, "%s%s%s", filedir, "\\", path.GetDirectory());

	// ---------
	int nSize = m_pModel->m_vecOutputPass.size();
	for (int nPass = 0; nPass < nSize; nPass++)
	{
		int nTexID = m_pModel->GetOutputTexID(nPass);
		LPDIRECT3DTEXTURE9 pD3D9Tex = g_TextureManager.FindD3D9Texture(nTexID);
		CMpqTexture *pTex = g_TextureManager.FindTexture(nTexID);

		Ogre::CFilePath path;
		path.Split((char*)pTex->m_strName.c_str());

		char szFile[MAX_PATH];
		sprintf(szFile, "%s%s%s", m_szFileDir, path.GetFileName(), ".dds");

		if (!Ogre::CFilePath::IsFileExist(szFile))
		{
			D3DXSaveTextureToFileA(szFile, D3DXIFF_DDS, pD3D9Tex, NULL);
		}
	}
}

void CModelExport::ProcessRenderPass()
{
	// ------
	m_pModel->m_vecOutputPass.clear();
	int nSize = m_pModel->m_vecPass.size();
	for (int i = 0; i < nSize; i++)
	{
		CModelRenderPass *pPass = &m_pModel->m_vecPass[i];
		bool bRender = pPass->IsRender(m_pModel);
		if (bRender)
		{
			m_pModel->m_vecOutputPass.push_back(*pPass);
		}
	}

	// ------
	nSize = m_pModel->m_vecOutputPass.size();
	for (int i = 0; i < nSize - 1; i++)
	{
		CModelRenderPass *pPass = &m_pModel->m_vecOutputPass[i];
		for (int j = i + 1; j < nSize; j++)
		{
			CModelRenderPass *pComPass = &m_pModel->m_vecOutputPass[j];
			if (pComPass->indexCount == pPass->indexCount &&
				pComPass->vertexStart == pPass->vertexStart &&
				pComPass->vertexEnd == pPass->vertexEnd &&
				pComPass->indexStart == pPass->indexStart)
			{
				pComPass->m_bRender = false;
			}
		}
	}

	nSize = m_pModel->m_vecOutputPass.size();
	for (int i = 0; i < nSize; i++)
	{
		CModelRenderPass *pPass = &m_pModel->m_vecOutputPass[i];
		if (pPass->useEnvMap)
		{
			pPass->m_bRender = false;
			for (int j = 0; j < nSize; j++)
			{
				CModelRenderPass *pComPass = &m_pModel->m_vecOutputPass[j];
				if (i == j)
				{
					continue;
				}

				if (pComPass->m_bRender)
				{
					if (pComPass->indexCount == pPass->indexCount &&
						pComPass->vertexStart == pPass->vertexStart &&
						pComPass->vertexEnd == pPass->vertexEnd &&
						pComPass->indexStart == pPass->indexStart)
					{
						pComPass->m_bHasSpecular = true;
						pComPass->m_nSpecularPassID = i;
					}
				}
			}
		}
	}
}