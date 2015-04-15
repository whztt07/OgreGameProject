

#include "OgreResourceManager.h"
#include "OgreMdxMesh.h"

namespace Ogre
{
	// -------------------------
	// GlobalParam
	// -------------------------
	CMdxMeshManager* CMdxMeshManager::s_Instance = NULL;
	CTextureManager* CTextureManager::s_Instance = NULL;
	CGpuProgramManager* CGpuProgramManager::s_Instance = NULL;

	// -------------------------
	// CTextureManager
	// -------------------------
	CTextureManager::CTextureManager()
	{
		s_Instance = this;
	}

	CTextureManager::~CTextureManager()
	{

	}

	CTextureManager& CTextureManager::Instance()
	{
		return *s_Instance;
	}

	// -------------------------
	// CMdxMeshManager
	// -------------------------
	CMdxMeshManager::CMdxMeshManager()
	{
		s_Instance = this;
	}

	CMdxMeshManager::~CMdxMeshManager()
	{

	}

	CMdxMeshManager& CMdxMeshManager::Instance()
	{
		return *s_Instance;
	}

	CResource* CMdxMeshManager::Load(CString szGroup, CString szName)
	{
		CResource *pResource = Create(szGroup, szName);
		pResource->Load();
		return pResource;
	}

	CResource* CMdxMeshManager::CreateImpl(CString szGroup, CString szName)
	{
		CResource *pResource = new CMdxMesh(szGroup, szName);

		return pResource;
	}

	// -------------------------
	// CGpuProgramManager
	// -------------------------
	CGpuProgramManager::CGpuProgramManager()
		:CResourceManager()
	{
		s_Instance = this;
	}

	CGpuProgramManager::~CGpuProgramManager()
	{

	}

	CGpuProgramManager& CGpuProgramManager::Instance()
	{
		return *s_Instance;
	}

}