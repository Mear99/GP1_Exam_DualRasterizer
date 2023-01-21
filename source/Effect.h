#pragma once
#include "DataTypes.h"
using namespace dae;
class Texture;

class Effect
{
	public:
		Effect(ID3D11Device* pDevice, const std::wstring& path);
		virtual ~Effect();

		void Initialize();

		static ID3DX11Effect* LoadEffect(ID3D11Device* pDevice, const std::wstring& path);
		ID3DX11Effect* GetEffect();
		ID3DX11EffectTechnique* GetTechnique();
		ID3D11InputLayout* GetInputLayout();

		void SetWVPMatrix(const Matrix& WVPMatrix);
		void SetSampleMethod(Filtering filter);

	protected:

		virtual void BuildInputLayout() = 0;
		virtual void BuildVariables() = 0;

		ID3DX11Effect* m_pEffect;
		ID3DX11EffectTechnique* m_pTechnique;
		ID3D11InputLayout* m_pInputLayout;
		ID3D11Device* m_pDevice;

		ID3DX11EffectMatrixVariable* m_pMatWorldViewProjVariable;

		ID3DX11EffectSamplerVariable* m_pSamplerPointVariable;
		ID3D11SamplerState* m_pSamplerState;
};

class Effect_Vertex : public Effect
{
	public:
		Effect_Vertex(ID3D11Device* pDevice);

		void SetMaps(Texture* pDiffuseMap, Texture* pNormalMap, Texture* pSpecularMap, Texture* pGlossyMap);


		void SetWorldMatrix(const Matrix& WorldMatrix);
		void SetViewInvMatrix(const Matrix& ViewInvMatrix);

	private:
		virtual void BuildInputLayout() override;
		virtual void BuildVariables() override;

		ID3DX11EffectShaderResourceVariable* m_pDiffuseMapVariable;
		ID3DX11EffectShaderResourceVariable* m_pNormalMapVariable;
		ID3DX11EffectShaderResourceVariable* m_pSpecularMapVariable;
		ID3DX11EffectShaderResourceVariable* m_pGlossyMapVariable;


		ID3DX11EffectMatrixVariable* m_pMatWorldVariable;
		ID3DX11EffectMatrixVariable* m_pMatViewInvVariable;
};

class Effect_DiffuseAlpha : public Effect
{
	public:
		Effect_DiffuseAlpha(ID3D11Device* pDevice);

		void SetDiffuseMap(Texture* pDiffuseMap);

	private:
		virtual void BuildInputLayout() override;
		virtual void BuildVariables() override;

		ID3DX11EffectShaderResourceVariable* m_pDiffuseMapVariable;
};