

#include "OgreResource.h"
//#include "OgreResourceManager.h"
#include "OgreUseful.h"

namespace Ogre
{
	// -------------------------
	// GlobalParam
	// -------------------------
	CResourceGroupManager* CResourceGroupManager::s_Instance = NULL;

	// ------------------------------
	// CResource
	// ------------------------------
	CResource::CResource(CString szGroup, CString szName)
	{
		m_szGroup = szGroup;
		m_szName = szName;
		m_nSize = 0;
		m_nRefer = 0;
		m_pDataStream = NULL;
	}

	CResource::~CResource()
	{

	}

	CString& CResource::GetName()
	{
		return m_szName;
	}

	CString& CResource::GetGroupName()
	{
		return m_szGroup;
	}

	int CResource::GetSize()
	{
		return m_nSize;
	}

	bool CResource::Load()
	{
		PreLoadImpl();
		LoadImpl();
		PostLoadImpl();
		
		m_nSize = CalcSize();
		return true;
	}

	void CResource::UnLoad()
	{

	}

	bool CResource::LoadImpl()
	{
		return true;
	}

	void CResource::PreLoadImpl()
	{
		m_pDataStream = CResourceGroupManager::Instance().OpenResource(m_szGroup, m_szName);
	}

	void CResource::PostLoadImpl()
	{
		SAFE_DELETE(m_pDataStream);
	}

	int CResource::CalcSize()
	{
		return 0;
	}

	void CResource::AddRefer()
	{
		m_nRefer++;
	}

	void CResource::DelRefer()
	{
		m_nRefer--;
	}

	// -------------------------
	// CResourceManager
	// -------------------------
	CResourceManager::CResourceManager()
	{
		m_mapResource.clear();
	}

	CResourceManager::~CResourceManager()
	{

	}

	CResource* CResourceManager::GetResource(CString szName)
	{
		ResourceMap::iterator iter = m_mapResource.find(szName);
		if (iter != m_mapResource.end())
		{
			return iter->second;
		}

		return NULL;
	}

	CResource* CResourceManager::Create(CString szName, CString szGroup)
	{
		CResource *pResource = GetResource(szName);
		if (pResource != NULL)
		{
			pResource->AddRefer();
			return pResource;
		}

		pResource = CreateImpl(szGroup, szName);
		pResource->AddRefer();
		m_mapResource[szName] = pResource;
		return pResource;
	}

	// -------------------------
	// CResourceGroupManager
	// -------------------------
	CResourceGroupManager::CResourceGroup::CResourceGroup()
	{

	}

	CResourceGroupManager::CResourceGroup::~CResourceGroup()
	{

	}

	CResourceGroupManager::CResourceGroupManager()
	{
		s_Instance = this;
		m_mapResourceGroup.clear();
		m_vecArchive.clear();
	}

	CResourceGroupManager::~CResourceGroupManager()
	{

	}

	CResourceGroupManager& CResourceGroupManager::Instance()
	{
		return *s_Instance;
	}

	CResourceGroupManager::CResourceGroup* CResourceGroupManager::GetResourceGroup(CString szGroupName)
	{
		ResourceGroupMap::iterator iter = m_mapResourceGroup.find(szGroupName);
		if (iter != m_mapResourceGroup.end())
		{
			return iter->second;
		}

		return NULL;
	}

	CResourceGroupManager::CResourceGroup* CResourceGroupManager::CreateResourceGroup(CString szGroupName)
	{
		CResourceGroup *pGroup = new CResourceGroup;
		if (pGroup != NULL)
		{
			pGroup->m_szName = szGroupName;
			m_mapResourceGroup[szGroupName] = pGroup;
		}

		return pGroup;
	}

	CDataStream* CResourceGroupManager::OpenResource(CString szGroupName, CString szResName)
	{
		CResourceGroup *pGroup = GetResourceGroup(szGroupName);
		if (pGroup == NULL)
		{
			return NULL;
		}

		ResourceLocationIndex::iterator iter = pGroup->m_mapResIndexCaseSensitive.find(szResName);
		if (iter == pGroup->m_mapResIndexCaseSensitive.end())
		{
			return NULL;
		}

		CArchive *pArchive = iter->second;
		if (pArchive == NULL)
		{
			return NULL;
		}

		return pArchive->Open((char*)szResName.c_str());
	}

	void CResourceGroupManager::AddResourceLocation(CString szArcType, CString szArcName, CString szGroupName)
	{
		CResourceGroup *pGroup = GetResourceGroup(szGroupName);
		if (pGroup == NULL)
		{
			pGroup = CreateResourceGroup(szGroupName);
		}

		if (pGroup == NULL)
		{
			return;
		}

		CArchive *pArchive = CArchiveManager::Instance().Load(szArcName, szArcType);
		if (pArchive == NULL)
		{
			return;
		}

		m_vecArchive.push_back(pArchive);

		int nNum = pArchive->GetFileNum();
		for (int i = 0; i < nNum; i++)
		{
			CString szName = pArchive->GetFileName(i);
			pGroup->m_mapResIndexCaseSensitive[szName] = pArchive;
		}
	}

	StringVector& CResourceGroupManager::ListResourceNames(CString ext)
	{
		m_vecFindFile.clear();

		ResourceGroupMap::iterator iter = m_mapResourceGroup.begin();
		for (; iter != m_mapResourceGroup.end(); iter++)
		{
			CResourceGroup *pGroup = iter->second;
			if (pGroup == NULL)
			{
				continue;
			}

			ResourceLocationIndex::iterator iterIndex = pGroup->m_mapResIndexCaseSensitive.begin();
			for (; iterIndex != pGroup->m_mapResIndexCaseSensitive.end(); iterIndex++)
			{
				CString szName = iterIndex->first;
				bool bFind = CStringUtil::Find(szName, ext);
				if (bFind)
				{
					m_vecFindFile.push_back(szName);
				}	
			}
		}

		return m_vecFindFile;
	}
}