

#ifndef __D3D9MAPPINGS_H__
#define __D3D9MAPPINGS_H__

#include "OgreD3D9Prerequisites.h"
#include "OgreHardwareBuffer.h"
#include "OgreRenderPass.h"

namespace Ogre 
{
	class D3D9Mappings
	{
	public:
		 static DWORD Get(CHardwareBuffer::Usage usage);
		 static DWORD Get(CHardwareBuffer::LockOptions options, CHardwareBuffer::Usage usage);
		 static D3DFORMAT Get(CHardwareIndexBuffer::IndexType itype);
		 // Get vertex data type
		 static D3DDECLTYPE Get(VertexElementType vType);
		/// Get vertex semantic
		 static D3DDECLUSAGE Get(VertexElementSemantic sem);
	};
}

#endif