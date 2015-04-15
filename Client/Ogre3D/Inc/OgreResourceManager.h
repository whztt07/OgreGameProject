

#ifndef __OgreResourceManager_h__
#define __OgreResourceManager_h__

#include "OgrePrerequisites.h"
#include "OgreTexture.h"
#include "OgreArchive.h"

namespace Ogre
{
	class CResourceGroupManager;
	class CHighLevelGpuProgram;

	class OgreExport CTextureManager : public CResourceManager
	{
	public:
		CTextureManager();
		virtual ~CTextureManager();

		static CTextureManager& Instance();
		virtual CResource* Load(CString szGroup, CString szName){return NULL;}
		virtual void OnLostDevice(){}
		virtual void OnResetDevice(){}
	private:
		virtual CResource* CreateImpl(CString szGroup, CString szName){return NULL;}
		static CTextureManager *s_Instance;
	};

	class OgreExport CMdxMeshManager : public CResourceManager
	{
	public:
		CMdxMeshManager();
		~CMdxMeshManager();

		static CMdxMeshManager& Instance();

		virtual CResource* Load(CString szGroup, CString szName);
	private:
		virtual CResource* CreateImpl(CString szGroup, CString szName);

		static CMdxMeshManager *s_Instance;
	};

	class OgreExport CGpuProgramManager : public CResourceManager
	{
	public:
		CGpuProgramManager();
		virtual ~CGpuProgramManager();

		static CGpuProgramManager& Instance();
		virtual CResource* Load(CString szGroup, CString szName){return NULL;}
		virtual CHighLevelGpuProgram* GetHLSLProgram(std::string szName){return NULL;}
		virtual void OnLostDevice() = 0;
		virtual void OnResetDevice() = 0;
	protected:
		virtual CResource* CreateImpl(CString szGroup, CString szName){return NULL;}
		static CGpuProgramManager *s_Instance;
	};
}

#endif