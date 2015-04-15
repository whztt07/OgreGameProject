

#ifndef __OgreMdxConfig_h__
#define __OgreMdxConfig_h__

#include "OgrePrerequisites.h"
#include "OgreFile.h"
#include "OgreString.h"

namespace Ogre
{
	union OgreExport CMSMdxConfigPointer
	{
		CMSMdxConfigPointer(void* in);

		int*			i;
		DWORD*			dw;
		char*			c;
		void*			p;
		float*			f;
	};

	class CMSMdxTreeNode
	{
	public:
		CMSMdxTreeNode();
		~CMSMdxTreeNode();

		enum
		{
			eMaxString = 256
		};

		enum
		{
			eMdx,
			eSequences,
			eSequence,
			eHitPoint,
			eHitPoints,
			eNumberParam,
			eStringParam,
			eModelScale,
			eModelBoundingObject,
			eSoundEffect,
			eAvatarSetting,
			eAvatarSettings,
			eBodyRadius,
			eIsComponent,
		};

		virtual DWORD GetType() = 0;
		virtual char* GetText() = 0;
		virtual BOOL CanModify();
		virtual BOOL SetValue( const char* pszValue );

		void SetItem( HTREEITEM hItem );
		void SetParent( CMSMdxTreeNode* pParent );
		HTREEITEM GetItem();
		CMSMdxTreeNode* GetParent();
	protected:
		HTREEITEM m_hItem;
		CMSMdxTreeNode* m_pParent;
	};

	class CMSNumberParam : public CMSMdxTreeNode
	{
	public:
		CMSNumberParam();
		~CMSNumberParam();
		
		virtual DWORD GetType();
		virtual char* GetText();
		virtual BOOL CanModify();
		virtual BOOL SetValue( const char* pszValue );

		void	SetNumber( int nNumber );
		int		GetNumber();
		void	SetTitle( const char* pszTitle );
	protected:
		int		m_nNumber;
		char	m_szTitle[CMSMdxTreeNode::eMaxString];
	};

	class CMSStringParam : public CMSMdxTreeNode
	{
	public:
		CMSStringParam();
		~CMSStringParam();

		virtual DWORD GetType();
		virtual char* GetText();
		virtual BOOL CanModify();
		virtual BOOL SetValue( const char* pszValue );

		void	SetString( const char* pszString );
		char* GetString();
		void	SetTitle( const char* pszTitle );
	protected:
		char	m_szTitle[CMSMdxTreeNode::eMaxString];
		char	m_szString[CMSMdxTreeNode::eMaxString];
	};

	class CMSMdxCfgSequence : public CMSMdxTreeNode
	{
	public:
		CMSMdxCfgSequence();
		~CMSMdxCfgSequence();

		virtual DWORD GetType();
		virtual char*  GetText();

		BOOL	Read( CMSMdxConfigPointer inP, int nSize );
		BOOL	CreateTree( CMSMdxTreeNode* pParent, HWND hTree, HTREEITEM hRoot );	
		int		BlendFrame( float t );
		float	GetBlendFrame( float t );
	public:
		CMSStringParam m_spAnimName;
		CMSNumberParam m_npStartFrameId;
		CMSNumberParam m_npEndFrameId;
		CMSNumberParam m_npRealHitPoint;
		CMSNumberParam m_npHasLightTrack;
	};

	class CMSMdxCfgSequences : public CMSMdxTreeNode
	{
	public:
		CMSMdxCfgSequences();
		~CMSMdxCfgSequences();

		virtual DWORD GetType();
		virtual char* GetText();

		CMSMdxCfgSequence* GetSequence(char *name);
		BOOL Read( CMSMdxConfigPointer inP, int nSize );
		BOOL CreateTree( CMSMdxTreeNode* pParent, HWND hTree, HTREEITEM hRoot );
	protected:
		std::vector<CMSMdxCfgSequence*> m_vectorSequence;
		typedef std::map<CString, CMSMdxCfgSequence*> SequenceMap;
		SequenceMap m_mapSequence;
		HTREEITEM m_hRootItem;
	};

	class CMSMdxCfg : public CMSMdxTreeNode
	{
	public:
		CMSMdxCfg();
		~CMSMdxCfg();

		virtual DWORD GetType();
		virtual char* GetText();

		CMSMdxCfgSequence* GetSequence( char* pszActionName );
		BOOL	LoadFromFile( char* pszFilename );
		BOOL	SaveToFile( char* pszFilename );
	public:
		CMSMdxCfgSequences m_sequences;
		HTREEITEM		m_hRootItem;
	};
}

#endif