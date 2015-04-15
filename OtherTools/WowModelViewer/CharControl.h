

#ifndef __CharControl_h__
#define __CharControl_h__
#include "enums.h"
#include <string>
#include <vector>
#include <sstream>
#include <iostream> 

struct CharRegionCoords
{
	int xpos, ypos, xsize, ysize;
};

const CharRegionCoords regions[NUM_REGIONS] =
{
	{0, 0, 256, 256},	// base
	{0, 0, 128, 64},	// arm upper
	{0, 64, 128, 64},	// arm lower
	{0, 128, 128, 32},	// hand
	{0, 160, 128, 32},	// face upper
	{0, 192, 128, 64},	// face lower
	{128, 0, 128, 64},	// torso upper
	{128, 64, 128, 32},	// torso lower
	{128, 96, 128, 64}, // pelvis upper
	{128, 160, 128, 64},// pelvis lower
	{128, 224, 128, 32}	// foot
};

struct CCharDetails
{
	void Reset();

	unsigned int skinColor;
	unsigned int faceType;
	unsigned int hairColor;
	unsigned int hairStyle;
	unsigned int facialHair;

	unsigned int facialColor;
	unsigned int maxFacialColor;

	unsigned int maxHairStyle, maxHairColor, maxSkinColor, maxFaceType, maxFacialHair;

	unsigned int race, gender;

	unsigned int useNPC;

	bool showUnderwear, showEars, showHair, showFacialHair, showFeet;

	int equipment[NUM_CHAR_SLOTS];
	int geosets[16];
};

struct TabardDetails
{
	int Icon;
	int IconColor;
	int Border;
	int BorderColor;
	int Background;

	int maxIcon;
	int maxIconColor;
	int maxBorder;
	int maxBorderColor;
	int maxBackground;

	bool showCustom;

	std::string GetIconTex(int slot);
	std::string GetBorderTex(int slot);
	std::string GetBackgroundTex(int slot);
};

struct CharTextureComponent
{
	std::string name;
	int region;
	int layer;

	const bool operator<(const CharTextureComponent& c) const
	{
		return layer < c.layer;
	}
};

struct CCharTexture
{
	void AddLayer(std::string fn, int region, int layer);
	void Compose(int& texID);
	
	std::vector<CharTextureComponent> components;
};

#endif