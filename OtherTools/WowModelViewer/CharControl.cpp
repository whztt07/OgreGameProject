

#include "CharControl.h"
#include "ModelCanvas.h"
extern CMpqTextureManager g_TextureManager;

void CCharDetails::Reset()
{
	skinColor = 0;
	faceType = 0;
	hairColor = 0;
	hairStyle = 0;
	facialHair = 0;

	showUnderwear = true;
	showHair = true;
	showFacialHair = true;
	showEars = true;
	showFeet = false;

	for (int i=0; i<NUM_CHAR_SLOTS; i++) 
	{
		equipment[i] = 0;
	}
}

void CCharTexture::AddLayer(std::string fn, int region, int layer)
{
	if (fn.length() == 0)
	{
		return;
	}

	CharTextureComponent ct;
	ct.name = fn;
	ct.region = region;
	ct.layer = layer;
	components.push_back(ct);
}

void CCharTexture::Compose(int& texID)
{
	// if we only have one texture then don't bother with compositing
	if (components.size() == 1)
	{
		texID = g_TextureManager.Add(components[0].name);
		return;
	}

	std::sort(components.begin(), components.end());
	unsigned char destbuf[256*256*4], tempbuf[256*256*4];
	memset(destbuf, 0, 256*256*4);

	int nCount = 0;
	for (std::vector<CharTextureComponent>::iterator it = components.begin(); 
		it != components.end(); ++it) 
	{
		//nCount++;
		//if (nCount == 7)
		//{
		//	int i = 0;
		//}

		CharTextureComponent &comp = *it;
		const CharRegionCoords &coords = regions[comp.region];
		int temptex = g_TextureManager.Add( comp.name );
		if (temptex == -1)
		{
			continue;
		}

		g_TextureManager.GetPixel(tempbuf, temptex);

		CMpqTexture *pTexture = g_TextureManager.FindTexture(temptex);
		if ( pTexture->m_nWidth == coords.xsize &&
			 pTexture->m_nHeight == coords.ysize) 
		{
			for (int y = 0, dy = coords.ypos; y < pTexture->m_nHeight; y++, dy++) 
			{
				for (int x = 0, dx = coords.xpos; x < pTexture->m_nWidth; x++,dx++)
				{
					unsigned char *src = tempbuf + y * pTexture->m_nWidth * 4 + x * 4;
					unsigned char *dest = destbuf + dy * 256 * 4 + dx * 4;

					// this is slow and ugly but I don't care
					float r = src[3] / 255.0f;
					float ir = 1.0f - r;
					// zomg RGBA?
					dest[0] = (unsigned char)(dest[0] * ir + src[0] * r);
					dest[1] = (unsigned char)(dest[1] * ir + src[1] * r);
					dest[2] = (unsigned char)(dest[2] * ir + src[2] * r);
					//dest[2] = (unsigned char)(dest[0] * ir + src[0] * r);
					//dest[1] = (unsigned char)(dest[1] * ir + src[1] * r);
					//dest[0] = (unsigned char)(dest[2] * ir + src[2] * r);
					dest[3] = 255;
				}
			}
		}

		g_TextureManager.Del(temptex);
	}

	// 这里的RGBA需要反一下显示
	for (int y = 0; y < 256; y++)
	{
		for (int x = 0; x < 256; x++)
		{
			unsigned char *dest = destbuf + y * 256 * 4 + x * 4;
			unsigned char temp = dest[0];
			dest[0] = dest[2];
			dest[2] = temp;
		}
	}

	g_TextureManager.ComposePixel(destbuf, 256, 256, texID);
}

std::string TabardDetails::GetBackgroundTex(int slot)
{
	std::ostringstream tmpStream;

	std::string tmpU = "textures\\GuildEmblems\\Background_";
	if (Background < 10)
		tmpU += "0";
	tmpStream << Background;
	tmpU += tmpStream.str();
	tmpU += "_TU_U.blp";

	std::string tmpL = tmpU;
	tmpL[37] = 'L';

	if (slot == CR_TORSO_UPPER)
		return tmpU;
	else
		return tmpL;
}

std::string TabardDetails::GetBorderTex(int slot)
{
	std::ostringstream tmpStream;

	std::string tmpU = "textures\\GuildEmblems\\Border_";

	if (Border < 10)
		tmpU += "0";
	tmpStream << Border << "_";
	tmpU += tmpStream.str();
	tmpStream.flush();
	tmpStream.str("");

	if (BorderColor < 10)
		tmpU += "0";
	tmpStream << BorderColor;
	tmpU += tmpStream.str();

	tmpU += "_TU_U.blp";

	std::string tmpL = tmpU;
	tmpL[36] = 'L';

	if (slot == CR_TORSO_UPPER)
		return tmpU;
	else
		return tmpL;
}

std::string TabardDetails::GetIconTex(int slot)
{
	std::ostringstream tmpStream;

	std::string tmpU = "textures\\GuildEmblems\\Emblem_";

	if(Icon < 10)
		tmpU += "0";
	tmpStream << Icon << "_";
	tmpU += tmpStream.str();
	tmpStream.flush();
	tmpStream.str("");

	if(IconColor < 10)
		tmpU += "0";
	tmpStream << IconColor;
	tmpU += tmpStream.str();

	tmpU += "_TU_U.blp";

	std::string tmpL = tmpU;
	tmpL[tmpU.length() - 7] = 'L';

	if(slot == CR_TORSO_UPPER)
		return tmpU;
	else
		return tmpL;
}
