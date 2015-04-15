

#ifndef __OgreDataChunkWrite_h__
#define __OgreDataChunkWrite_h__

#include "OgrePrerequisites.h"
#include "OgreUseful.h"

namespace Ogre
{
	class OgreExport CDataChunkWrite
	{
	public:
		CDataChunkWrite(DWORD dwBufferSize, BOOL bSkip0SizeChunk = TRUE);
		~CDataChunkWrite();
	public:
		struct ChunkHdr
		{
			DWORD dwName;
			DWORD dwSize;
		};
		struct ChunkDesc
		{
			ChunkHdr header;
			DWORD dwOffset;
		};

		DWORD StartChunk( DWORD dwName );
		DWORD EndChunk( DWORD dwName );
		DWORD Write( void* pBuffer, DWORD dwBlockSize, DWORD dwBlockCount );
		DWORD WriteChar( char s ) { return Write( &s, sizeof( char ), 1 ); }
		DWORD WriteDWORD( DWORD dw ) { return Write( &dw, sizeof( DWORD ), 1 ); }
		DWORD WriteInt( int i );
		DWORD WriteFloat( float f ){ return Write( &f, sizeof( float ), 1 ); }
		DWORD WriteShort( short s ){ return Write( &s, sizeof( short ), 1 ); }
		DWORD WriteByte( BYTE b ){ return Write( &b, sizeof( BYTE ), 1 ); }
		DWORD WriteString( char* str );
		BOOL SaveToFile( const char* pszFilename );
		void Destroy();
	protected:
		std::stack<ChunkDesc> m_stackChunkDesc;
		DWORD	m_dwBufferSize;
		BYTE*	m_pbyBuffer;
		DWORD	m_dwOffset;
		BOOL	m_bSkip0SizeChunk;		// 是否skip没有内容的文件头
		BOOL	m_bExternalMemory;		// 是否自建内存
	};
}

#endif