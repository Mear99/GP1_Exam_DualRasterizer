#pragma once
#include "Camera.h"
#include "Mesh.h"
#include "DataTypes.h"
using namespace dae;

struct SDL_Window;
struct SDL_Surface;

class Renderer final
{
public:
	Renderer(SDL_Window* pWindow);
	~Renderer();

	Renderer(const Renderer&) = delete;
	Renderer(Renderer&&) noexcept = delete;
	Renderer& operator=(const Renderer&) = delete;
	Renderer& operator=(Renderer&&) noexcept = delete;

	void Update(const Timer* pTimer);
	void Render();

	// Controlling functions
	void SwitchRenderMode();
	void SwitchFilteringMethod();
	void ToggleRotation();
	void ToggleUniformBackground();
	void ToggleCullMode();
	void ToggleFireMesh();
	void ToggleShadingMode();
	void ToggleBoundingBoxes();
	void ToggleDepthBuffer();
	void ToggleNormalMap();

private:
	SDL_Window* m_pWindow{};

	int m_Width{};
	int m_Height{};

	bool m_IsInitialized{ false };

	Camera m_Camera;

	// Controling variables
	RenderMode m_RenderMode{ RenderMode::software };
	bool m_ShouldRotate{ true };
	CullMode m_CullMode{ CullMode::back };
	bool m_UseUniformBackground{ false };
	bool m_DrawFireMesh{ false };
	ShadingMode m_ShadingMode{ ShadingMode::combined };
	bool m_VisualizeBoundingBoxes{ false };
	bool m_VisualizeDepthBuffer{ false };
	bool m_UseNormalMap{ true };
	Filtering m_Filtering{ Filtering::point };

	// Hardware
	HRESULT InitializeDirectX();
	void RenderHardware() const;

	// Software
	void RenderSoftware();
	void InitSoftware(SDL_Window* pWindow);
	std::vector<Vertex_Out> VertexShader(const Mesh& mesh);
	std::vector<Vertex_Out> InterPolateAttributes(const Mesh& mesh, const std::vector<Vertex_Out>& verts);
	void PixelShader(const Mesh& mesh, const std::vector<Vertex_Out>& verts);
	float Remap(float value, float min, float max);

	// Shared
	void InitMeshes();

	// Pipeline variables
	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pDeviceContext;
	IDXGISwapChain* m_pSwapChain;

	ID3D11Texture2D* m_pDepthStencilBuffer;
	ID3D11DepthStencilView* m_pDepthStencilView;

	ID3D11Texture2D* m_pRenderTargetBuffer;
	ID3D11RenderTargetView* m_pRenderTargetView;

	ID3D11RasterizerState* m_pRasterState_FrontCulling;
	ID3D11RasterizerState* m_pRasterState_BackCulling;
	ID3D11RasterizerState* m_pRasterState_NoCulling;

	ID3D11SamplerState* m_pPointSamplerState;
	ID3D11SamplerState* m_pLinearSamplerState;
	ID3D11SamplerState* m_pAnisotropicSamplerState;

	// Meshes
	Mesh* m_pVehicleMesh;
	Mesh* m_pFireMesh;

	// Software variables
	SDL_Surface* m_pFrontBuffer{ nullptr };
	SDL_Surface* m_pBackBuffer{ nullptr };
	uint32_t* m_pBackBufferPixels{};
	float* m_pDepthBufferPixels{};
};

