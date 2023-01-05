#include "pch.h"
#include "Effect.h"
#include <assert.h>
#include "Texture.h"

Filtering Effect::m_Filtering{ Filtering::point };

Effect::Effect(ID3D11Device* pDevice, const std::wstring& path)
	: m_pDevice{ pDevice }
{
	// Load Effect
	m_pEffect = LoadEffect(pDevice, path);
	m_pTechnique = m_pEffect->GetTechniqueByName("DefaultTechnique");
	if (!m_pTechnique->IsValid()) {
		std::wcout << L"Technique not valid!\n";
	}

	// World View Projection Matrix
	m_pMatWorldViewProjVariable = m_pEffect->GetVariableByName("gWorldViewProjMat")->AsMatrix();
	if (!m_pMatWorldViewProjVariable->IsValid()) {
		std::wcout << L"m_pWorldViewProjVariable not valid!\n";
	}
}

Effect::~Effect() {
	m_pInputLayout->Release();
	m_pTechnique->Release();
	m_pEffect->Release();
}

void Effect::Initialize() {

	BuildVariables();
	BuildInputLayout();
}

ID3DX11Effect* Effect::LoadEffect(ID3D11Device* pDevice, const std::wstring& path) {
	HRESULT result;
	ID3D10Blob* pErrorBlob{ nullptr };
	ID3DX11Effect* pEffect;

	DWORD shaderFlags{ 0 };
#if defined(DEBUG) || defined(_DEBUG)
	shaderFlags |= D3DCOMPILE_DEBUG;
	shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	result = D3DX11CompileEffectFromFile(path.c_str(), nullptr, nullptr, shaderFlags, 0, pDevice, &pEffect, &pErrorBlob);

	if (FAILED(result)) {
		if (pErrorBlob) {

			const char* pErrors{ static_cast<char*>(pErrorBlob->GetBufferPointer()) };
			std::wstringstream stream;

			for (unsigned int i{ 0 }; i < pErrorBlob->GetBufferSize(); ++i) {
				stream << pErrors[i];
			}

			OutputDebugStringW(stream.str().c_str());
			pErrorBlob->Release();
			pErrorBlob = nullptr;

			std::wcout << stream.str() << "n";
		}
		else {
			std::wstringstream stream;
			stream << "Failed to create Effect: " << path;
			std::wcout << stream.str() << "n";
			return nullptr;
		}
	}

	return pEffect;
}

ID3DX11Effect* Effect::GetEffect() {
	return m_pEffect;
}

ID3DX11EffectTechnique* Effect::GetTechnique() {
	return m_pTechnique;
}

ID3D11InputLayout* Effect::GetInputLayout() {
	return m_pInputLayout;
}

void Effect::SetWVPMatrix(const Matrix& WVPMatrix) {

	float matrix[16]{};
	WVPMatrix.ConvertToFloatArray(matrix);

	m_pMatWorldViewProjVariable->SetMatrix(matrix);
}


void Effect::SetSampleMethod() {
	if (m_pSamplerPointVariable) {

		D3D11_SAMPLER_DESC desc{};
		desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.MinLOD = -FLT_MAX;
		desc.MaxLOD = FLT_MAX;
		desc.MipLODBias = 0.0f;
		desc.MaxAnisotropy = 1;
		desc.ComparisonFunc = D3D11_COMPARISON_NEVER;

		switch (m_Filtering) {
		case Filtering::point:
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			break;
		case Filtering::linear:
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			break;
		case Filtering::anisotropic:
			desc.Filter = D3D11_FILTER_ANISOTROPIC;
			break;
		}

		// Release previous sampler state
		if (m_pSamplerState) {
			m_pSamplerState->Release();
		}

		m_pDevice->CreateSamplerState(&desc, &m_pSamplerState);

		m_pSamplerPointVariable->SetSampler(0, m_pSamplerState);
	}
}

void Effect::SwitchFilteringMethod() {
	m_Filtering = Filtering((int(m_Filtering) + 1) % 3);
}

// Vertex Effect

Effect_Vertex::Effect_Vertex(ID3D11Device* pDevice)
	: Effect(pDevice, L"Resources/Vertex3D.fx")
{}

Effect_Vertex::~Effect_Vertex() {
	if (m_pSamplerState) {
		m_pSamplerState->Release();
	}
}

void Effect_Vertex::BuildInputLayout() {

	// Vertex layout
	static constexpr uint32_t numElements{ 4 };
	D3D11_INPUT_ELEMENT_DESC vertexDesc[numElements]{};

	vertexDesc[0].SemanticName = "POSITION";
	vertexDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[0].AlignedByteOffset = 0;
	vertexDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[1].SemanticName = "TEXCOORD";
	vertexDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	vertexDesc[1].AlignedByteOffset = 12;
	vertexDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[2].SemanticName = "NORMAL";
	vertexDesc[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[2].AlignedByteOffset = 20;
	vertexDesc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[3].SemanticName = "TANGENT";
	vertexDesc[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[3].AlignedByteOffset = 32;
	vertexDesc[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	// Input layout
	D3DX11_PASS_DESC passDesc{};
	m_pTechnique->GetPassByIndex(0)->GetDesc(&passDesc);

	const HRESULT result{ m_pDevice->CreateInputLayout(vertexDesc, numElements,passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &m_pInputLayout) };
	if (FAILED(result)) {
		assert(false);
	}
}

void Effect_Vertex::BuildVariables() {

	// Diffuse texture map
	m_pDiffuseMapVariable = m_pEffect->GetVariableByName("gDiffuseMap")->AsShaderResource();
	if (!m_pDiffuseMapVariable->IsValid()) {
		std::wcout << L"m_pDiffuseMapVariable not valid!\n";
	}

	// Normal texture map
	m_pNormalMapVariable = m_pEffect->GetVariableByName("gNormalMap")->AsShaderResource();
	if (!m_pNormalMapVariable->IsValid()) {
		std::wcout << L"m_pNormalMapVariable not valid!\n";
	}

	// Specular texture map
	m_pSpecularMapVariable = m_pEffect->GetVariableByName("gSpecularMap")->AsShaderResource();
	if (!m_pSpecularMapVariable->IsValid()) {
		std::wcout << L"m_pSpecularMapVariable not valid!\n";
	}

	// Glossy texture map
	m_pGlossyMapVariable = m_pEffect->GetVariableByName("gGlossyMap")->AsShaderResource();
	if (!m_pGlossyMapVariable->IsValid()) {
		std::wcout << L"m_pGlossyMapVariable not valid!\n";
	}

	// SamplerPoint
	m_pSamplerPointVariable = m_pEffect->GetVariableByName("gSamPoint")->AsSampler();
	if (!m_pSamplerPointVariable->IsValid()) {
		std::wcout << L"m_pDiffuseMapVariable not valid!\n";
	}

	// World Matrix
	m_pMatWorldVariable = m_pEffect->GetVariableByName("gWorldMat")->AsMatrix();
	if (!m_pMatWorldVariable->IsValid()) {
		std::wcout << L"m_pMatWorldVariable not valid!\n";
	}

	// World View Projection Matrix
	m_pMatViewInvVariable = m_pEffect->GetVariableByName("gViewInvMat")->AsMatrix();
	if (!m_pMatViewInvVariable->IsValid()) {
		std::wcout << L"m_pMatViewInvVariable not valid!\n";
	}
}

void Effect_Vertex::SetMaps(Texture* pDiffuseMap, Texture* pNormalMap, Texture* pSpecularMap, Texture* pGlossyMap) {
	if (m_pDiffuseMapVariable) {
		m_pDiffuseMapVariable->SetResource(pDiffuseMap->GetSRV());
	}

	if (m_pNormalMapVariable) {
		m_pNormalMapVariable->SetResource(pNormalMap->GetSRV());
	}

	if (m_pSpecularMapVariable) {
		m_pSpecularMapVariable->SetResource(pSpecularMap->GetSRV());
	}

	if (m_pGlossyMapVariable) {
		m_pGlossyMapVariable->SetResource(pGlossyMap->GetSRV());
	}
}

void Effect_Vertex::SetWorldMatrix(const Matrix& worldMatrix) {

	float matrix[16]{};
	worldMatrix.ConvertToFloatArray(matrix);

	m_pMatWorldVariable->SetMatrix(matrix);
}

void Effect_Vertex::SetViewInvMatrix(const Matrix& viewInvMatrix) {

	float matrix[16]{};
	viewInvMatrix.ConvertToFloatArray(matrix);

	m_pMatViewInvVariable->SetMatrix(matrix);
}

// Diffuse Alpha Effect

Effect_DiffuseAlpha::Effect_DiffuseAlpha(ID3D11Device* pDevice)
	: Effect(pDevice, L"Resources/Alpha3D.fx")
{}

Effect_DiffuseAlpha::~Effect_DiffuseAlpha() {
	if (m_pSamplerState) {
		m_pSamplerState->Release();
	}
}

void Effect_DiffuseAlpha::BuildInputLayout() {

	// Vertex layout
	static constexpr uint32_t numElements{ 2 };
	D3D11_INPUT_ELEMENT_DESC vertexDesc[numElements]{};

	vertexDesc[0].SemanticName = "POSITION";
	vertexDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[0].AlignedByteOffset = 0;
	vertexDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[1].SemanticName = "TEXCOORD";
	vertexDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	vertexDesc[1].AlignedByteOffset = 12;
	vertexDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	// Input layout
	D3DX11_PASS_DESC passDesc{};
	m_pTechnique->GetPassByIndex(0)->GetDesc(&passDesc);

	const HRESULT result{ m_pDevice->CreateInputLayout(vertexDesc, numElements,passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &m_pInputLayout) };
	if (FAILED(result)) {
		assert(false);
	}
}

void Effect_DiffuseAlpha::BuildVariables() {

	// Diffuse texture map
	m_pDiffuseMapVariable = m_pEffect->GetVariableByName("gDiffuseMap")->AsShaderResource();
	if (!m_pDiffuseMapVariable->IsValid()) {
		std::wcout << L"m_pDiffuseMapVariable not valid!\n";
	}
	// SamplerPoint
	m_pSamplerPointVariable = m_pEffect->GetVariableByName("gSamPoint")->AsSampler();
	if (!m_pSamplerPointVariable->IsValid()) {
		std::wcout << L"m_pDiffuseMapVariable not valid!\n";
	}
}

void Effect_DiffuseAlpha::SetDiffuseMap(Texture* pDiffuseMap) {
	if (m_pDiffuseMapVariable) {
		m_pDiffuseMapVariable->SetResource(pDiffuseMap->GetSRV());
	}
}
