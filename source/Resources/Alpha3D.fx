float4x4 gWorldViewProjMat : WorldViewProjectionMatrix;
Texture2D gDiffuseMap : DiffuseMap;
SamplerState gSamPoint : SamplerPoint;

BlendState gBlendState {
	BlendEnable[0] = true;
	SrcBlend = src_alpha;
	DestBlend = inv_src_alpha;
	BlendOp = add;
	SrcBlendAlpha = zero;
	DestBlendAlpha = zero;
	BlendOpAlpha = add;
	RenderTargetWritemask[0] = 0x0F;
};

DepthStencilState gDepthStencilState {
	DepthEnable = true;
	DepthWriteMask = zero;
	DepthFunc = less;
	StencilEnable = false;
};

struct VS_INPUT {
	float3 Position : POSITION;
	float2 UV : TEXCOORD;
};

struct VS_OUTPUT {
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD;
};

// Vertex Shader
VS_OUTPUT VS(VS_INPUT input) {
	VS_OUTPUT output = (VS_OUTPUT)0;
	output.Position = mul(float4(input.Position, 1.0f), gWorldViewProjMat);
	output.UV = input.UV;
	return output;
}

// Pixel Shader
float4 PS(VS_OUTPUT input) : SV_TARGET{
	return gDiffuseMap.Sample(gSamPoint,input.UV);
}

// Technique
technique11 DefaultTechnique {
	pass P0 {
		SetDepthStencilState(gDepthStencilState, 0);
		SetBlendState(gBlendState, float4(0, 0, 0, 0), 0xFFFFFFFF);
		SetVertexShader(CompileShader(vs_5_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, PS()));
	}
};