#include "pch.h"
#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>

Texture::~Texture() {

	if (m_pSurface)
	{
		SDL_FreeSurface(m_pSurface);
		m_pSurface = nullptr;
	}

	m_pResource->Release();
	m_pSRV->Release();
}

 void Texture::LoadFromFile(ID3D11Device* pDevice, const std::string& path) {

	m_pSurface = IMG_Load(path.c_str());
	m_pSurfacePixels = (uint32_t*)m_pSurface->pixels;

	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = m_pSurface->w;
	desc.Height = m_pSurface->h;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = m_pSurface->pixels;
	initData.SysMemPitch = static_cast<UINT>(m_pSurface->pitch);
	initData.SysMemSlicePitch = static_cast<UINT>(m_pSurface->h * m_pSurface->pitch);

	HRESULT hr = pDevice->CreateTexture2D(&desc, &initData, &m_pResource);

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
	SRVDesc.Format = format;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = 1;

	hr = pDevice->CreateShaderResourceView(m_pResource, &SRVDesc, &m_pSRV);
}

 ID3D11ShaderResourceView* Texture::GetSRV() {
	 return m_pSRV;
 }

 ColorRGB Texture::Sample(const Vector2& uv) const
 {
	 //TODO
	 //Sample the correct texel for the given uv
	 int width = m_pSurface->w;
	 int height = m_pSurface->h;

	 int px{ int(uv.x * (width - 1)) % width };
	 int py{ int(uv.y * (height - 1)) % height };

	 if (px < 0) {
		 px += width;
	 }

	 if (py < 0) {
		 py += height;
	 }

	 Uint8 r{}, g{}, b{};
	 SDL_GetRGB(m_pSurfacePixels[px + py * width], m_pSurface->format, &r, &g, &b);
	 ColorRGB sampledColor{ float(r), float(g), float(b) };

	 return (sampledColor / 255.0f);
 }