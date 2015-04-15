

#include "OgreDataChunkWrite.h"

namespace Ogre
{
	//----------------------------------------
	//------------CDataChunkWrite-------------
	//----------------------------------------
	CDataChunkWrite::CDataChunkWrite(DWORD dwBufferSize, BOOL bSkip0SizeChunk)
	{
		m_dwBufferSize = dwBufferSize;
		m_pbyBuffer = new BYTE[dwBufferSize];
		m_dwOffset = 0;
		m_bExternalMemory = FALSE;
		m_bSkip0SizeChunk = bSkip0SizeChunk;
	}
	
	CDataChunkWrite::~CDataChunkWrite()
	{

	}

	DWORD CDataChunkWrite::Write( void* pBuffer, DWORD dwBlockSize, DWORD dwBlockCount )
	{
		if( m_bExternalMemory )
		{
			if( m_dwOffset + dwBlockSize * dwBlockCount >  m_dwBufferSize ) 
			{
				assert( false );
				return 0;
			}
		}
		else
		{
			while( m_dwOffset + dwBlockSize * dwBlockCount > m_dwBufferSize )
			{
				DWORD dwNewBufferSize = m_dwBufferSize * 2;
				BYTE* pbyNewBuffer = new BYTE[dwNewBufferSize];
				memcpy(pbyNewBuffer, m_pbyBuffer, min(m_dwBufferSize, dwNewBufferSize));
				SAFE_DELETE_ARRAY(m_pbyBuffer);
				m_pbyBuffer = pbyNewBuffer;
				m_dwBufferSize = dwNewBufferSize;
			}
		}

		BYTE* pSrc = (BYTE*)pBuffer;
		for( DWORD i = 0; i < dwBlockCount; i++ )
		{
			memcpy( &m_pbyBuffer[m_dwOffset], pSrc, dwBlockSize );
			pSrc += dwBlockSize;
			m_dwOffset += dwBlockSize;
		}

		return m_dwOffset;
	}

	DWORD CDataChunkWrite::StartChunk( DWORD dwName )
	{
		ChunkDesc desc;
		desc.header.dwName = dwName;
		desc.header.dwSize = 0;
		desc.dwOffset = m_dwOffset;
		m_stackChunkDesc.push( desc );

		Write( &desc.header, sizeof( ChunkHdr ), 1 );
		return 0;
	}

	DWORD CDataChunkWrite::EndChunk( DWORD dwName )
	{
		assert( !m_stackChunkDesc.empty() );
		ChunkDesc desc = m_stackChunkDesc.top();
		m_stackChunkDesc.pop();

		assert( desc.header.dwName == dwName );
		if (desc.dwOffset > m_dwBufferSize)
		{
			assert(false);
			return 0;
		}
		
		ChunkHdr* pHdr = (ChunkHdr*)&m_pbyBuffer[desc.dwOffset];
		pHdr->dwSize = m_dwOffset - ( desc.dwOffset + sizeof( ChunkHdr ) );
		if( m_bSkip0SizeChunk && pHdr->dwSize == 0 )
			m_dwOffset = desc.dwOffset;

		return 0;
	}

	DWORD CDataChunkWrite::WriteInt( int i )
	{ 
		return Write( &i, sizeof( int ), 1 ); 
	}

	DWORD CDataChunkWrite::WriteString( char* str )
	{
		DWORD dwOffset = WriteInt( strlen( str ) );
		if( strlen( str ) > 0 )
			return Write( str, strlen( str ), 1 );
		return dwOffset;
	}

	BOOL CDataChunkWrite::SaveToFile( const char* pszFilename )
	{
		assert( m_stackChunkDesc.empty() );
		FILE* fp = fopen( pszFilename, "wb" );
		if( !fp )
			return FALSE;
		if( m_dwOffset == 0 )
			return FALSE;
		
		fwrite( m_pbyBuffer, m_dwOffset, 1, fp );
		fclose( fp );
		return TRUE;
	}

	void CDataChunkWrite::Destroy()
	{
		if( m_bExternalMemory )
		{
			m_pbyBuffer = NULL;
		}
		else
		{
			SAFE_DELETE_ARRAY(m_pbyBuffer);
		}
		
		m_dwBufferSize = 0;
		m_dwOffset = 0;
	}
}
