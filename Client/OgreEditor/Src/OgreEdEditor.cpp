

#include "OgreEdEditor.h"
#include "OgreEdCamera.h"
#include "OgreRoot.h"
#include "OgreRenderSystem.h"

namespace OgreEd
{
	CEditor::CEditor()
	{
		m_pRoot = new Ogre::CRoot;
		m_pCamera = new CEdCamera(m_pRoot->GetCamera());
	}

	CEditor::~CEditor()
	{

	}

	void CEditor::Create(HWND hWnd)
	{
		RECT rc;
		::GetClientRect(hWnd, &rc);
		int nWidth = rc.right - rc.left;
		int nHeight = rc.bottom - rc.top;

		SetupResources();

		m_pRoot->Initialize(hWnd, nWidth, nHeight);

		Ogre::CGpuProgramManager::Instance().Load("", "./Resources/StaticMesh.fx");
	}

	void CEditor::SetupResources()
	{
		return;
		Ogre::CConfigFile file;
		file.Load("resource.cfg");

		Ogre::CConfigFile::SettingSectionMap &mapSection = file.GetMap();
		Ogre::CConfigFile::SettingSectionMap::iterator iter = mapSection.begin();
		for (; iter != mapSection.end(); iter++)
		{
			Ogre::CConfigFile::SettingSection *pSection = iter->second;
			Ogre::CString group = iter->first;
			if (pSection != NULL)
			{
				Ogre::CConfigFile::SettingSection::iterator iterSection = pSection->begin();
				for (; iterSection != pSection->end(); iterSection++)
				{
					Ogre::CString type = iterSection->first;
					Ogre::CString name = iterSection->second;
					Ogre::CResourceGroupManager::Instance().AddResourceLocation(type, name, group);
				}
			}
		}
	}

	void CEditor::RenderEditor()
	{
		Ogre::CRenderSystem *system = Ogre::CRoot::Instance().GetActiveRenderer();
		if (system != NULL)
		{
			system->BeginFrame();
			Render();
			system->EndFrame();
		}
	}

	void CEditor::ResetEditor(int width, int height)
	{
		Ogre::CRenderSystem *system = Ogre::CRoot::Instance().GetActiveRenderer();
		{
			system->SetViewport(0, 0, width, height);
		}
		m_pRoot->Reset(width, height);
	}
}