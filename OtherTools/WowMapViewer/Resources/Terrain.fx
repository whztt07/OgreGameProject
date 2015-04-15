
texture Base;
sampler sBase = sampler_state
{
	Texture	= <Base>;
};

texture Layer1;
sampler sLayer1 = sampler_state
{
	Texture	= <Layer1>;
};

texture Layer2;
sampler sLayer2 = sampler_state
{
	Texture	= <Layer2>;
};

texture Layer3;
sampler sLayer3 = sampler_state
{
	Texture	= <Layer3>;
};

texture Blend;
sampler sBlend = sampler_state
{
	Texture	= <Blend>;
	AddressU = Clamp;
	AddressV = Clamp;
};

float4 Terrain_PS(	float2 iTex : TEXCOORD0,
					float2 iAlphaTex : TEXCOORD1 ) : COLOR0
{
	float4 baseColor = tex2D(sBase, iTex);
	float4 layer1Color = tex2D(sLayer1, iTex);
	float4 layer2Color = tex2D(sLayer2, iTex);
	float4 layer3Color = tex2D(sLayer3, iTex);
	float4 blendColor = tex2D(sBlend, iAlphaTex);
	
	float4 outColor = baseColor;
	outColor = layer1Color * blendColor.r + outColor * (1 - blendColor.r);
	outColor = layer2Color * blendColor.g + outColor * (1 - blendColor.g);
	outColor = layer3Color * blendColor.b + outColor * (1 - blendColor.b);
	float shadowColor = blendColor.a * 0.3 + 0.7;
	outColor.rgb *= shadowColor;

	return outColor;
}

technique RenderTerrain
{
	pass P0
	{
		vertexshader = NULL;
		pixelshader = compile ps_2_0 Terrain_PS();
	}
}