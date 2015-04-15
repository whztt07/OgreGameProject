
texture tBase;
sampler sBase = sampler_state
{
	Texture	= <tBase>;
};

texture tNormal;
sampler sNormal = sampler_state
{
	Texture = <tNormal>;
};

texture tSpecular;
sampler sSpecular = sampler_state
{
	Texture = <tSpecular>;
};

float4x4 mWorld;
float4x4 mView;
float4x4 mViewProj;
float bHasSpecular;
float bAlphaTest;

void Mesh_VS(	float3 iPos : POSITION0,
				float3 iNormal : NORMAL0,
				float2 iTex : TEXCOORD0,
				out float4 oPos : POSITION0,
				out float2 oTex : TEXCOORD0,
				out float2 oSpecTex : TEXCOORD1 )
{
	float3 vWorldPos = mul(float4(iPos, 1.0), mWorld);
	oPos = mul(float4(vWorldPos, 1.0), mViewProj);
	oTex = iTex;

	if(bHasSpecular)
	{
		float3 viewPos = mul(float4(iPos, 1.0), mView);
		float3 viewNormal = mul(float4(iNormal, 0.0), mView);
		viewPos = normalize(viewPos);
		viewNormal = normalize(viewNormal);
		float fSpec = dot(viewPos, viewNormal) * 2;
		float3 vSpec = fSpec * (-viewNormal) + viewPos;
		float2 texSpec = normalize(vSpec);
		oSpecTex = texSpec * 0.5 + 0.5;
	}
}

float4 Mesh_PS( float2 iTex : TEXCOORD0,
			    float2 iSpecTex : TEXCOORD1 ) : COLOR0
{
	float4 baseColor = tex2D(sBase, iTex);
	float4 outColor = baseColor;
	
	if(bAlphaTest)
	{
		clip(baseColor.a - 0.7);
	}
	
	if(bHasSpecular)
	{
		float4 specColor = tex2D(sSpecular, iSpecTex);
		outColor = baseColor * specColor * 2;
	}
	
	return outColor;
}

technique CommonTech
{
	pass P0
	{
		vertexshader = compile vs_2_0 Mesh_VS();
		pixelshader = compile ps_2_0 Mesh_PS();
	}
}