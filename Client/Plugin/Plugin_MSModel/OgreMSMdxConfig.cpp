
#include "OgreMSMdxConfig.h"

namespace Ogre
{
	//---------------------------------
	//-----------CMSMdxConfigPointer-----
	//---------------------------------
	CMSMdxConfigPointer::CMSMdxConfigPointer(void* in) 
	{
		p = in;
	}

	//---------------------------------
	//-----------CMSMdxTreeNode----------
	//---------------------------------
	CMSMdxTreeNode::CMSMdxTreeNode()
	{
		m_hItem = NULL;
		m_pParent = NULL;
	}
	
	CMSMdxTreeNode::~CMSMdxTreeNode()
	{

	}

	BOOL CMSMdxTreeNode::CanModify()
	{ 
		return FALSE;
	}

	BOOL CMSMdxTreeNode::SetValue( const char* pszValue )
	{ 
		return FALSE; 
	}

	void CMSMdxTreeNode::SetItem( HTREEITEM hItem )
	{ 
		m_hItem = hItem; 
	}

	HTREEITEM CMSMdxTreeNode::GetItem()
	{
		return m_hItem; 
	}

	void CMSMdxTreeNode::SetParent( CMSMdxTreeNode* pParent )
	{
		m_pParent = pParent; 
	}

	CMSMdxTreeNode* CMSMdxTreeNode::GetParent()
	{ 
		return m_pParent; 
	}

	//---------------------------------
	//-----------CMSNumberParam----------
	//---------------------------------
	CMSNumberParam::CMSNumberParam()
		:CMSMdxTreeNode()
	{
		m_nNumber = 0;
		m_szTitle[0] = 0;
	}

	CMSNumberParam::~CMSNumberParam()
	{

	}

	DWORD CMSNumberParam::GetType()
	{ 
		return CMSMdxTreeNode::eNumberParam; 
	}

	char* CMSNumberParam::GetText()
	{ 
		static char s[CMSMdxTreeNode::eMaxString];
		memset( s, 0x00, sizeof(s) );
		sprintf( s, "%s:%ld", m_szTitle, m_nNumber );
		return s;
	}

	BOOL CMSNumberParam::CanModify()
	{ 
		return TRUE;
	}

	BOOL CMSNumberParam::SetValue( const char* pszValue )
	{ 
		m_nNumber = atoi( pszValue ); 
		return TRUE; 
	}

	void CMSNumberParam::SetNumber( int nNumber )
	{ 
		m_nNumber = nNumber; 
	}

	int	CMSNumberParam::GetNumber()
	{ 
		return m_nNumber;
	}

	void CMSNumberParam::SetTitle( const char* pszTitle )
	{ 
		strcpy( m_szTitle, pszTitle );
	}

	//---------------------------------
	//-----------CMSStringParam----------
	//---------------------------------
	CMSStringParam::CMSStringParam()
		:CMSMdxTreeNode()
	{
		memset( m_szTitle, 0x00, sizeof(m_szTitle) );
		memset( m_szString, 0x00, sizeof(m_szString) );
	}

	CMSStringParam::~CMSStringParam()
	{

	}

	DWORD CMSStringParam::GetType()
	{ 
		return CMSMdxTreeNode::eNumberParam; 
	}

	char* CMSStringParam::GetText()
	{ 
		static char s[CMSMdxTreeNode::eMaxString];
		memset( s, 0x00, sizeof(s) );
		sprintf( s, "%s:%s", m_szTitle, m_szString );
		return s;
	}

	BOOL CMSStringParam::CanModify()
	{ 
		return TRUE;
	}

	BOOL CMSStringParam::SetValue( const char* pszValue )
	{ 
		SetString( pszValue ); 
		return TRUE; 
	}

	void CMSStringParam::SetString( const char* pszString )
	{ 
		strcpy( m_szString, pszString );
	}

	char* CMSStringParam::GetString()
	{ 
		return m_szString; 
	}

	void CMSStringParam::SetTitle( const char* pszTitle )
	{ 
		strcpy( m_szTitle, pszTitle ); 
	}

	//---------------------------------
	//-----------CMSMdxCfgSequence-------
	//---------------------------------
	CMSMdxCfgSequence::CMSMdxCfgSequence()
		:CMSMdxTreeNode()
	{
		m_spAnimName.SetTitle( "Name" );
		m_npStartFrameId.SetTitle( "StartFrameId" );
		m_npEndFrameId.SetTitle( "EndFrameId" );
		m_npRealHitPoint.SetTitle( "Real Hit Point" );
		m_npHasLightTrack.SetTitle( "Has Light Track" );
	}

	CMSMdxCfgSequence::~CMSMdxCfgSequence()
	{

	}

	DWORD CMSMdxCfgSequence::GetType()
	{ 
		return CMSMdxTreeNode::eSequence;
	}

	char* CMSMdxCfgSequence::GetText()
	{ 
		static char s[CMSMdxTreeNode::eMaxString];
		memset( s, 0x00, sizeof(s) );
		sprintf( s, "%s", m_spAnimName.GetString() );
		return s;
	}

	int	CMSMdxCfgSequence::BlendFrame( float t )
	{
		if( t < 0 )
			t = 0;

		if( t > 1 )
			t = 1;
		
		int nFrame = m_npStartFrameId.GetNumber() + t * 
			(m_npEndFrameId.GetNumber() - m_npStartFrameId.GetNumber());
		
		return nFrame;
	}

	float CMSMdxCfgSequence::GetBlendFrame( float t )
	{
		if( t < 0 )
			t = 0;
		
		if( t > 1 )
			t = 1;

		float fFrame = (float)m_npStartFrameId.GetNumber() * ONEFRAMEINTERVAL +
			t * (float)(m_npEndFrameId.GetNumber() - m_npStartFrameId.GetNumber()) * ONEFRAMEINTERVAL;
		
		return fFrame;
	}

	BOOL CMSMdxCfgSequence::CreateTree(	CMSMdxTreeNode* pParent, HWND hTree, HTREEITEM hRoot )
	{
		TVINSERTSTRUCT tvInsert;
		ZeroMemory(&tvInsert,sizeof(TVINSERTSTRUCT));

		tvInsert.hInsertAfter = NULL;
		tvInsert.item.mask = TVIF_TEXT|TVIF_PARAM;

		tvInsert.hParent = hRoot;
		tvInsert.item.pszText = LPWSTR( GetText() );
		tvInsert.item.lParam = (LPARAM)this;
		HTREEITEM hSequence = TreeView_InsertItem(hTree,&tvInsert);
		SetItem( hSequence );
		SetParent( pParent );


		tvInsert.hParent = hSequence;
		tvInsert.item.pszText = LPWSTR( m_spAnimName.GetText() );
		tvInsert.item.lParam = (LPARAM)&m_spAnimName;
		HTREEITEM hAnimName = TreeView_InsertItem(hTree,&tvInsert);
		m_spAnimName.SetParent( this );
		m_spAnimName.SetItem( hAnimName );


		tvInsert.hParent = hSequence;
		tvInsert.item.pszText = LPWSTR( m_npStartFrameId.GetText() );
		tvInsert.item.lParam = (LPARAM)&m_npStartFrameId;
		HTREEITEM hStartFrameIdId = TreeView_InsertItem(hTree,&tvInsert);
		m_npStartFrameId.SetParent( this );
		m_npStartFrameId.SetItem( hStartFrameIdId );

		tvInsert.hParent = hSequence;
		tvInsert.item.pszText = LPWSTR( m_npEndFrameId.GetText() );
		tvInsert.item.lParam = (LPARAM)&m_npEndFrameId;
		HTREEITEM hEndFrameIdId = TreeView_InsertItem(hTree,&tvInsert);
		m_npEndFrameId.SetParent( this );
		m_npEndFrameId.SetItem( hEndFrameIdId );

		//m_hitpoints.CreateTree( this, hTree, hSequence );

		tvInsert.hParent = hSequence;
		tvInsert.item.pszText = LPWSTR( m_npRealHitPoint.GetText() );
		tvInsert.item.lParam = (LPARAM)&m_npRealHitPoint;
		HTREEITEM hAttack = TreeView_InsertItem(hTree,&tvInsert);
		m_npRealHitPoint.SetParent( this );
		m_npRealHitPoint.SetItem( hAttack );

		tvInsert.hParent = hSequence;
		tvInsert.item.pszText = LPWSTR( m_npHasLightTrack.GetText() );
		tvInsert.item.lParam = (LPARAM)&m_npHasLightTrack;
		HTREEITEM hLightTrack = TreeView_InsertItem(hTree,&tvInsert);
		m_npHasLightTrack.SetParent( this );
		m_npHasLightTrack.SetItem( hLightTrack );

		return TRUE;
	}

	BOOL CMSMdxCfgSequence::Read( CMSMdxConfigPointer inP, int nSize )
	{
		CMSMdxConfigPointer p(inP.p);

		char* pName = p.c;
		m_spAnimName.SetString( pName );
		p.c += CMSMdxTreeNode::eMaxString;
		m_npStartFrameId.SetNumber( *p.dw++ );
		m_npEndFrameId.SetNumber( *p.dw++ );

		assert( m_npEndFrameId.GetNumber() >= m_npStartFrameId.GetNumber() );

		while( p.c < inP.c+nSize )
		{
			switch( MDX_TAG( *p.dw ) )
			{
			case 'hpts':
				{
					p.dw++;
					int size = *p.i++;
					//m_hitpoints.Read( p, size );
					p.c += size; 
				}
				break;
			case 'rhpt':
				{
					p.dw++;
					int size = *p.i++;
					m_npRealHitPoint.SetNumber( *p.i );
					p.c += size; 
				}
				break;
			case 'ltrk':
				{
					p.dw++;
					int size = *p.i++;
					m_npHasLightTrack.SetNumber( *p.i );
					p.c += size;
				}
				break;
			default:
				assert( false );
			}
		}
		
		return TRUE;
	}

	//---------------------------------
	//-----------CMSMdxCfgSequences------
	//---------------------------------
	CMSMdxCfgSequences::CMSMdxCfgSequences()
		:CMSMdxTreeNode()
	{
		m_hRootItem = NULL;	
	}

	CMSMdxCfgSequences::~CMSMdxCfgSequences()
	{
		for( int i = 0; i < (int)m_vectorSequence.size(); i++ )
		{
			if( m_vectorSequence[i] )
				delete m_vectorSequence[i];
		}
		m_vectorSequence.clear();
	}

	DWORD CMSMdxCfgSequences::GetType()
	{
		return CMSMdxTreeNode::eSequences; 
	}

	char* CMSMdxCfgSequences::GetText()
	{ 
		return "Sequences";
	}

	BOOL CMSMdxCfgSequences::CreateTree(	CMSMdxTreeNode* pParent, 
		HWND hTree, HTREEITEM hRoot )
	{
		TVINSERTSTRUCT tvInsert;
		ZeroMemory(&tvInsert,sizeof(TVINSERTSTRUCT));

		tvInsert.hParent = hRoot;
		tvInsert.hInsertAfter = NULL;
		tvInsert.item.mask = TVIF_TEXT|TVIF_PARAM;
		tvInsert.item.pszText = LPWSTR(GetText());
		tvInsert.item.lParam = (LPARAM)this;
		HTREEITEM hChild = TreeView_InsertItem(hTree,&tvInsert);
		SetItem( hChild );
		SetParent( pParent );

		for( int i = 0; i < m_vectorSequence.size(); i++ )
			m_vectorSequence[i]->CreateTree( this, hTree, hChild );
		m_hRootItem = hChild;
		return TRUE;
	}


	CMSMdxCfgSequence* CMSMdxCfgSequences::GetSequence(char *name)
	{
		SequenceMap::iterator iter = m_mapSequence.find(name);
		if (iter != m_mapSequence.end())
		{
			return iter->second;
		}

		return NULL;
	}
	
	BOOL CMSMdxCfgSequences::Read( CMSMdxConfigPointer inP, int nSize )
	{
		CMSMdxConfigPointer p(inP.p);
		while( p.c < inP.c+nSize )
		{
			switch( MDX_TAG( *p.dw ) )
			{
			case 'sequ':
				{
					p.dw++;
					int size = *p.i++;
					CMSMdxCfgSequence* sequence = new CMSMdxCfgSequence;
					sequence->Read( p, size );
					m_vectorSequence.push_back( sequence );
					m_mapSequence[sequence->GetText()] = sequence;
					p.c += size; 
				}
				break;
			default:
				assert( false );
			}
		}

		return TRUE;
	}

	//---------------------------------
	//-----------CMSMdxCfg-----------
	//---------------------------------
	CMSMdxCfg::CMSMdxCfg()
	{
	}

	CMSMdxCfg::~CMSMdxCfg()
	{

	}

	DWORD CMSMdxCfg::GetType()
	{
		return CMSMdxTreeNode::eMdx; 
	}

	char* CMSMdxCfg::GetText()
	{
		return "Mdx"; 
	}

	BOOL CMSMdxCfg::SaveToFile( char* pszFilename )
	{
		return TRUE;
	}

	BOOL CMSMdxCfg::LoadFromFile( char* pszFilename )
	{
		CFile fp;
		if(fp.fopen(pszFilename, "rb"))
		{
			int nFileSize = fp.GetBufferSize();
			BYTE* pBuffer = fp.GetBuffer();

			CMSMdxConfigPointer p(pBuffer);
			while (p.c < (char*)pBuffer + nFileSize)
			{
				switch(MDX_TAG(*p.dw))
				{
				case 'seqs':
					{
						p.dw++;
						int size = *p.i++;
						m_sequences.Read( p, size );
						p.c += size; 
					}
					break;
				default:
					{
						p.dw++;
						int size = *p.i++;
						p.c += size;
					}
					break;
				}
			}

			fp.fclose();
			return TRUE;
		}

		return FALSE;
	}

	CMSMdxCfgSequence* CMSMdxCfg::GetSequence( char* pszActionName )
	{
		if( pszActionName )
		{
			return m_sequences.GetSequence(pszActionName);
		}

		return NULL;
	}
}