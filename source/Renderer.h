#pragma once
#include "Camera.h"
#include "Mesh.h"

struct SDL_Window;
struct SDL_Surface;

enum class RenderMode { software, hardware };

namespace dae
{
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

	private:
		SDL_Window* m_pWindow{};

		int m_Width{};
		int m_Height{};

		bool m_IsInitialized{ false };

		Camera m_Camera;

		// Controling variables
		RenderMode m_RenderMode{ RenderMode::software };

		// Hardware
		HRESULT InitializeDirectX();
		void RenderHardware() const;

		// Software
		void RenderSoftware();

		// Pipeline variables
		ID3D11Device* m_pDevice;
		ID3D11DeviceContext* m_pDeviceContext;
		IDXGISwapChain* m_pSwapChain;

		ID3D11Texture2D* m_pDepthStencilBuffer;
		ID3D11DepthStencilView* m_pDepthStencilView;

		ID3D11Texture2D* m_pRenderTargetBuffer;
		ID3D11RenderTargetView* m_pRenderTargetView;

		// Meshes
		Mesh<VertexUV>* m_pVehicleMesh;
		Mesh<VertexUV>* m_pFireMesh;

		// Software variables
		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};
	};
}
