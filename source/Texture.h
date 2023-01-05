#pragma once
#include <SDL_surface.h>
#include <string>
#include "ColorRGB.h"

using namespace dae;

class Texture
{
public:
	Texture() {};
	~Texture();

	void LoadFromFile(ID3D11Device* pDevice, const std::string& path);
	ID3D11ShaderResourceView* GetSRV();
	ColorRGB Sample(const Vector2& uv) const;

private:
	Texture(ID3D11ShaderResourceView* pSRV);

	ID3D11ShaderResourceView* m_pSRV{ nullptr };
	ID3D11Texture2D* m_pResource{ nullptr };

	SDL_Surface* m_pSurface{ nullptr };
	uint32_t* m_pSurfacePixels{ nullptr };
};



