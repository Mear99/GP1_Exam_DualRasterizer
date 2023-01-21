float4x4 gWorldViewProjMat : WorldViewProjectionMatrix;
float4x4 gWorldMat : WORLD;
float4x4 gViewInvMat : VIEWINVERSE;

SamplerState gSamPoint : SamplerPoint;

Texture2D gDiffuseMap : DiffuseMap;
Texture2D gNormalMap : NormalMap;
Texture2D gSpecularMap : SpecularMap;
Texture2D gGlossyMap : GlossyMap;

BlendState gBlendState {
	BlendEnable[0] = false;
};

DepthStencilState gDepthStencilState {
	DepthEnable = true;
	DepthWriteMask = all;
	DepthFunc = less;
	StencilEnable = false;
};

struct VS_INPUT {
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
};

struct VS_OUTPUT {
	float4 Position : SV_POSITION;
	float4 WorldPosition : POSITION;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float2 TexCoord : TEXCOORD;
};

// Vertex Shader
VS_OUTPUT VS(VS_INPUT input) {
	VS_OUTPUT output = (VS_OUTPUT)0;
	output.Position = mul(float4(input.Position, 1.0f), gWorldViewProjMat);
	output.WorldPosition = mul(float4(input.Position, 1.0f), gWorldMat);
	output.Normal = mul(normalize(input.Normal), (float3x3)gWorldMat);
	output.Tangent = mul(normalize(input.Tangent), (float3x3)gWorldMat);
	output.TexCoord = input.TexCoord;
	return output;
}

// Pixel Shader
float4 PS(VS_OUTPUT input) : SV_TARGET{

	float3 viewDirection = normalize(input.WorldPosition.xyz - gViewInvMat[3].xyz);
	float3 lightDirection = float3(0.577f, -0.577f, 0.577f);
	float PI = 3.1415926535f;
	float lightIntensity = 7.0f;
	float shininess = 25.0f;

	float3 finalColor = float3(0,0,0);
	float3 ambientColor = float3(0.025f, 0.025f, 0.025f);

	float3 binormal = cross(input.Normal, input.Tangent);
	float4x4 tangentSpaceAxis = float4x4(float4(input.Tangent,0), float4(binormal,0), float4(input.Normal,0), float4(0,0,0,1));
	float3 sampledNormal = gNormalMap.Sample(gSamPoint, input.TexCoord).rgb;
	sampledNormal = (2.0f * sampledNormal) - float3(1,1,1);
	sampledNormal = mul(normalize(sampledNormal), (float3x3)tangentSpaceAxis);

	// Cosine law
	float observedArea = dot(sampledNormal, -lightDirection);

	if (observedArea > 0) {

		// Diffuse lambert color
		float3 diffuseColor = lightIntensity * (gDiffuseMap.Sample(gSamPoint, input.TexCoord).rgb) / PI;

		// Specular Color
		float3 ks = gSpecularMap.Sample(gSamPoint, input.TexCoord).rgb;
		float exp = gGlossyMap.Sample(gSamPoint, input.TexCoord).r * shininess;

		float dotproduct =  max(dot(sampledNormal,-lightDirection),0);
		float3 r = (-lightDirection) - 2 * dotproduct * sampledNormal;
		float cosine = max(dot(r, viewDirection),0);
		float3 specularPhong =  ks * pow(cosine,exp);

		finalColor = (diffuseColor + specularPhong) * observedArea + ambientColor;
	}

	return float4(finalColor, 0.0f);
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