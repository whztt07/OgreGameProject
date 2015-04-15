

#ifndef __OgreResource_h__
#define __OgreResource_h__

#include "OgrePrerequisites.h"
#include "OgreString.h"
#include "OgreArchive.h"

class CResourceManager;

namespace Ogre
{
	class OgreExport CResource
	{
	public:
		CResource(CString szGroup, CString szName);
		virtual ~CResource();

		bool Load();
		void UnLoad();

		CString& GetName();
		CString& GetGroupName();
		int GetSize();
		void AddRefer();
		void DelRefer();
	protected:
		virtual bool LoadImpl();
		virtual void PreLoadImpl();
		virtual void PostLoadImpl();
		virtual int CalcSize();

		CString m_szName;	// Unique name of the resource
		CString m_szGroup;	// The name of the resource group
		int m_nSize;	// The size of the resource in bytes
		int m_nRefer;
		CDataStream *m_pDataStream;
	};

	class OgreExport CResourceManager
	{
	public:
		CResourceManager();
		virtual ~CResourceManager();

		virtual CResource* Load(CString szGroup, CString szName) = 0;
	protected:
		virtual CResource* CreateImpl(CString szGroup, CString szName) = 0;

		CResource* Create(CString szName, CString szGroup);
		CResource* GetResource(CString szName);

		typedef std::map<CString, CResource*> ResourceMap;
		ResourceMap m_mapResource;
	};

	class OgreExport CResourceGroupManager
	{
	public:
		CResourceGroupManager();
		~CResourceGroupManager();

		static CResourceGroupManager& Instance();

		typedef std::map<CString, CArchive*> ResourceLocationIndex;
		class CResourceGroup
		{
		public:
			CResourceGroup();
			~CResourceGroup();

			CString m_szName;
			ResourceLocationIndex m_mapResIndexCaseSensitive;
			ResourceLocationIndex m_mapResIndexCaseInsensitive;
		};

		typedef std::map<CString, CResourceGroup*> ResourceGroupMap;
		typedef std::vector<CArchive*> ArchiveList;

		CResourceGroup* GetResourceGroup(CString szGroupName);
		CResourceGroup* CreateResourceGroup(CString szGroupName);
		CDataStream* OpenResource(CString szGroupName, CString szResName);
		StringVector& ListResourceNames(CString ext);
		void AddResourceLocation(CString szArcType, CString szArcName, CString szGroupName);
	private:
		static CResourceGroupManager *s_Instance;
		ResourceGroupMap m_mapResourceGroup;
		ArchiveList m_vecArchive;
		StringVector m_vecFindFile;
	};
}

#endif