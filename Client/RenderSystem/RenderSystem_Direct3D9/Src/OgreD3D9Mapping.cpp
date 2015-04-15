

#include "OgreD3D9Mapping.h"

namespace Ogre
{
	DWORD D3D9Mappings::Get(CHardwareBuffer::Usage usage)
	{
		DWORD ret = 0;
		if (usage & CHardwareBuffer::HBU_DYNAMIC)
		{
			ret |= D3DUSAGE_DYNAMIC;
		}

		if (usage & CHardwareBuffer::HBU_WRITE_ONLY)
		{
			ret |= D3DUSAGE_WRITEONLY;
		}

		return ret;
	}

	DWORD D3D9Mappings::Get(CHardwareBuffer::LockOptions options, CHardwareBuffer::Usage usage)
	{
		DWORD ret = 0;
		if (options == CHardwareBuffer::HBL_DISCARD)
		{
			if (usage & CHardwareBuffer::HBU_DYNAMIC)
				ret |= D3DLOCK_DISCARD;
		}

		if (options == CHardwareBuffer::HBL_READ_ONLY)
		{
			if (!(usage & CHardwareBuffer::HBU_WRITE_ONLY))
				ret |= D3DLOCK_READONLY;
		}

		if (options == CHardwareBuffer::HBL_NO_OVERWRITE)
		{
			if (usage & CHardwareBuffer::HBU_DYNAMIC)
				ret |= D3DLOCK_NOOVERWRITE;
		}

		return ret;
	}

	D3DFORMAT D3D9Mappings::Get(CHardwareIndexBuffer::IndexType itype)
	{
		if (itype == CHardwareIndexBuffer::IT_32BIT)
		{
			return D3DFMT_INDEX32;
		}
		else
		{
			return D3DFMT_INDEX16;
		}
	}

	D3DDECLTYPE D3D9Mappings::Get(VertexElementType vType)
	{
		switch (vType)
		{
		case VET_COLOUR:
		case VET_COLOUR_ABGR:
		case VET_COLOUR_ARGB:
			return D3DDECLTYPE_D3DCOLOR;
			break;
		case VET_FLOAT1:
			return D3DDECLTYPE_FLOAT1;
			break;
		case VET_FLOAT2:
			return D3DDECLTYPE_FLOAT2;
			break;
		case VET_FLOAT3:
			return D3DDECLTYPE_FLOAT3;
			break;
		case VET_FLOAT4:
			return D3DDECLTYPE_FLOAT4;
			break;
		case VET_SHORT2:
			return D3DDECLTYPE_SHORT2;
			break;
		case VET_SHORT4:
			return D3DDECLTYPE_SHORT4;
			break;
		case VET_UBYTE4:
			return D3DDECLTYPE_UBYTE4;
			break;
		}

		// to keep compiler happy
		return D3DDECLTYPE_FLOAT3;
	}

	D3DDECLUSAGE D3D9Mappings::Get(VertexElementSemantic sem)
	{
		switch (sem)
		{
		case VES_BLEND_INDICES:
			return D3DDECLUSAGE_BLENDINDICES;
			break;
		case VES_BLEND_WEIGHTS:
			return D3DDECLUSAGE_BLENDWEIGHT;
			break;
		case VES_DIFFUSE:
			return D3DDECLUSAGE_COLOR; // NB index will differentiate
			break;
		case VES_SPECULAR:
			return D3DDECLUSAGE_COLOR; // NB index will differentiate
			break;
		case VES_NORMAL:
			return D3DDECLUSAGE_NORMAL;
			break;
		case VES_POSITION:
			return D3DDECLUSAGE_POSITION;
			break;
		case VES_TEXTURE_COORDINATES:
			return D3DDECLUSAGE_TEXCOORD;
			break;
		case VES_BINORMAL:
			return D3DDECLUSAGE_BINORMAL;
			break;
		case VES_TANGENT:
			return D3DDECLUSAGE_TANGENT;
			break;
		}

		// to keep compiler happy
		return D3DDECLUSAGE_POSITION;
	}
}