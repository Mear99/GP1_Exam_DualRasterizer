#pragma once
#include "DataTypes.h"
#include "Camera.h"
#include "ColorRGB.h"

class Texture;
class Effect;

using namespace dae;

class Mesh
{
	public:
		Mesh(ID3D11Device* pDevice, std::vector<VertexUV> vertices, std::vector<uint32_t> indices);
		~Mesh();

		void Update(float deltaTime);
		void RenderHardware(ID3D11DeviceContext* pDeviceContext, Camera camera);

		void SetMaps(Texture* diffuseMap, Texture* normalMap = nullptr, Texture* specularMap = nullptr, Texture* glossyMap = nullptr);
		void SetEffect(Effect* effect);
		void SetPosition(const Vector3& pos) { m_Position = pos; }
		ColorRGB PixelShading(const Vertex_Out& v) const;

		std::vector<VertexUV> GetVertices() const { return m_Vertices; }
		std::vector<uint32_t> GetIndices() const { return m_Indices; }
		Matrix GetWorldMatrix() const { return m_WorldMatrix; }
		
		PrimitiveTopology primitiveTopology{ PrimitiveTopology::TriangleList };

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

		std::vector<VertexUV> m_Vertices{};
		std::vector<uint32_t> m_Indices{};

		Vector3 m_Position{ 0.0f,0.0f,0.0f };
};