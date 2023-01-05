#pragma once
#include "DataTypes.h"
#include "Camera.h"
#include "Effect.h"
#include "Texture.h"
#include "ColorRGB.h"

using namespace dae;

template<typename Vertex>
class Mesh
{
	public:
		Mesh(ID3D11Device* pDevice, std::vector<Vertex> vertices, std::vector<uint32_t> indices);
		~Mesh();

		void Update(float deltaTime);
		void RenderHardware(ID3D11DeviceContext* pDeviceContext, Camera camera);

		void TransformVerts(const Camera& camera);

		void SetMaps(Texture* diffuseMap, Texture* normalMap = nullptr, Texture* specularMap = nullptr, Texture* glossyMap = nullptr);
		void SetEffect(Effect* effect);
		ColorRGB PixelShading(const Vertex_Out& v);

		std::vector<Vertex> m_Vertices{};
		std::vector<uint32_t> m_Indices{};
		PrimitiveTopology primitiveTopology{ PrimitiveTopology::TriangleStrip };

		std::vector<Vertex_Out> vertices_out{};

	private:

		Effect* m_pEffect;
		ID3D11Buffer* m_pVertexBuffer;
		ID3D11Buffer* m_pIndexBuffer;
		uint32_t m_NumIndices;
		Matrix m_WorldMatrix;

		Texture* m_pDiffuseMap;
		Texture* m_pNormalMap;
		Texture* m_pSpecularMap;
		Texture* m_pGlossyMap;

		float m_CurrentAngle{ 0.0f };
		float m_RotationSpeedRad{ PI_DIV_2 };
};






template<typename Vertex>
Mesh<Vertex>::Mesh(ID3D11Device* pDevice, std::vector<Vertex> vertices, std::vector<uint32_t> indices) {
	
	// Software variables
	m_Vertices = vertices;
	m_Indices = indices;

	// Vertex buffer
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.ByteWidth = sizeof(Vertex) * static_cast<uint32_t>(vertices.size());
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = vertices.data();

	HRESULT result = pDevice->CreateBuffer(&bd, &initData, &m_pVertexBuffer);
	if (FAILED(result)) {
		return;
	}

	// Index buffer
	m_NumIndices = static_cast<uint32_t>(indices.size());
	bd.ByteWidth = sizeof(uint32_t) * m_NumIndices;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

	initData.pSysMem = indices.data();

	result = pDevice->CreateBuffer(&bd, &initData, &m_pIndexBuffer);
	if (FAILED(result)) {
		return;
	}

	m_WorldMatrix = Matrix::CreateTranslation(0, 0, 0);
}

template<typename Vertex>
void Mesh<Vertex>::RenderHardware(ID3D11DeviceContext* pDeviceContext, Camera camera) {

	// Set World View Projection Matrix
	Matrix WVPMatrix{ m_WorldMatrix * camera.viewMatrix * camera.projectionMatrix };
	m_pEffect->SetWVPMatrix(WVPMatrix);
	m_pEffect->SetSampleMethod();

	// Set other matrices
	Effect_Vertex* pEffectVertex = dynamic_cast<Effect_Vertex*>(m_pEffect);
	if (pEffectVertex) {
		pEffectVertex->SetWorldMatrix(m_WorldMatrix);
		pEffectVertex->SetViewInvMatrix(camera.invViewMatrix);
	}

	// Set Primitive Technology
	pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set Input Layout
	pDeviceContext->IASetInputLayout(m_pEffect->GetInputLayout());

	// Set Vertex Buffer
	constexpr UINT stride = sizeof(Vertex);
	constexpr UINT offset = 0;
	pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

	// Set Index Buffer
	pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Draw
	D3DX11_TECHNIQUE_DESC techDesc{};
	m_pEffect->GetTechnique()->GetDesc(&techDesc);

	for (UINT p{ 0 }; p < techDesc.Passes; ++p) {
		m_pEffect->GetTechnique()->GetPassByIndex(p)->Apply(0, pDeviceContext);
		pDeviceContext->DrawIndexed(m_NumIndices, 0, 0);
	}
}

template<typename Vertex>
Mesh<Vertex>::~Mesh() {
	m_pIndexBuffer->Release();
	m_pVertexBuffer->Release();
	delete m_pEffect;
	delete m_pDiffuseMap;
	delete m_pNormalMap;
	delete m_pSpecularMap;
	delete m_pGlossyMap;
}

template<typename Vertex>
void Mesh<Vertex>::Update(float deltaTime) {
	m_CurrentAngle += m_RotationSpeedRad * deltaTime;
	if (m_CurrentAngle < 0) {
		m_CurrentAngle += PI_2;
	}
	if (m_CurrentAngle >= PI_2) {
		m_CurrentAngle -= PI_2;
	}

	m_WorldMatrix = Matrix::CreateRotationY(m_CurrentAngle);
}

template<typename Vertex>
void Mesh<Vertex>::SetMaps(Texture* diffuseMap, Texture* normalMap, Texture* specularMap, Texture* glossyMap) {
	m_pDiffuseMap = diffuseMap;
	m_pNormalMap = normalMap;
	m_pSpecularMap = specularMap;
	m_pGlossyMap = glossyMap;
}

template<typename Vertex>
void Mesh<Vertex>::SetEffect(Effect* effect) {
	m_pEffect = effect;

	// Link texture to effect
	Effect_Vertex* pEffectVertex = dynamic_cast<Effect_Vertex*>(m_pEffect);
	if (pEffectVertex) {
		pEffectVertex->SetMaps(m_pDiffuseMap, m_pNormalMap, m_pSpecularMap, m_pGlossyMap);
	}

	// Link texture to effect
	Effect_DiffuseAlpha* pEffectAlpha = dynamic_cast<Effect_DiffuseAlpha*>(m_pEffect);
	if (pEffectAlpha) {
		pEffectAlpha->SetDiffuseMap(m_pDiffuseMap);
	}
}
template<typename Vertex>
void Mesh<Vertex>::TransformVerts(const Camera& camera)
{
	vertices_out.clear();
	vertices_out.reserve(m_Vertices.size());

	Matrix WVPMatrix{ m_WorldMatrix * camera.viewMatrix * camera.projectionMatrix };

	for (const Vertex& vertex : m_Vertices) {

		// Init out vertex
		Vertex_Out vertexOut{};
		vertexOut.position = { vertex.position.x, vertex.position.y, vertex.position.z, 1 };
		vertexOut.uv = vertex.uv;
		vertexOut.normal = vertex.normal;
		vertexOut.tangent = vertex.tangent;

		// World to NDC space
		vertexOut.position = WVPMatrix.TransformPoint(vertexOut.position);

		// Perspective divide
		vertexOut.position.x /= vertexOut.position.w;
		vertexOut.position.y /= vertexOut.position.w;
		vertexOut.position.z /= vertexOut.position.w;

		// Transform normal and tangent To World space
		vertexOut.normal = m_WorldMatrix.TransformVector(vertexOut.normal);
		vertexOut.tangent = m_WorldMatrix.TransformVector(vertexOut.tangent);

		// Calculate viewDirection
		vertexOut.viewDirection = m_WorldMatrix.TransformPoint(vertex.position) - camera.origin;
		vertexOut.viewDirection.Normalize();

		vertices_out.emplace_back(vertexOut);
	}
}

template<typename Vertex>
ColorRGB Mesh<Vertex>::PixelShading(const Vertex_Out& v) {

	const Vector3 lightDirection{ 0.577f, -0.577f, 0.577f };
	const float lightIntensity{ 7.0f };
	const float shininess{ 25.0f };
	ColorRGB finalColor{ 0,0,0 };
	ColorRGB ambientColor{ 0.025f, 0.025f, 0.025f };

	Vector3 sampledNomal{ v.normal };

	// Normal map calculations
	Vector3 binormal{ Vector3::Cross(v.normal, v.tangent) };
	Matrix tangentSpaceAxis{ Matrix{v.tangent, binormal, v.normal, Vector3::Zero} };

	ColorRGB normalColor{ m_pNormalMap->Sample(v.uv) };
	sampledNomal = { normalColor.r, normalColor.g, normalColor.b };
	sampledNomal = (2.0f * sampledNomal) - Vector3{ 1.0f, 1.0f, 1.0f };
	sampledNomal = tangentSpaceAxis.TransformVector(sampledNomal);


	// Cosine law
	float observedArea{ Vector3::Dot(sampledNomal, -lightDirection) };

	if (observedArea > 0) {

		// Diffuse lambert color
		ColorRGB diffuseColor = lightIntensity * m_pDiffuseMap->Sample(v.uv) / PI;

		// Specular Color
		const ColorRGB ks{ m_pSpecularMap->Sample(v.uv) };
		const float exp{ m_pGlossyMap->Sample(v.uv).r * shininess };

		float dotproduct{ std::max(Vector3::Dot(sampledNomal,-lightDirection),0.0f) };
		Vector3 r{ (-lightDirection) - 2 * dotproduct * sampledNomal };
		float cosine{ std::max(Vector3::Dot(r,v.viewDirection),0.0f) };
		ColorRGB specularPhong{ ks * powf(cosine,exp) };

		finalColor = (diffuseColor + specularPhong) * observedArea + ambientColor;
	}

	return finalColor;
}