#include "pch.h"
#include "Renderer.h"
#include "Utils.h"
#include "Texture.h"
#include "Effect.h"

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	// Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	// Init software pipeline
	InitSoftware(pWindow);

	// Initialize DirectX pipeline
	const HRESULT result = InitializeDirectX();
	if (result == S_OK)
	{
		m_IsInitialized = true;
		std::cout << "DirectX is initialized and ready!\n";
	}
	else
	{
		std::cout << "DirectX initialization failed!\n";
	}

	// Init Camera
	m_Camera.Initialize(45.f, { 0.0f,0.0f,0.0f }, float(m_Width) / m_Height);

	// Init meshes
	InitMeshes();
}

Renderer::~Renderer()
{
	// Delete meshes
	delete m_pVehicleMesh;
	delete m_pFireMesh;

	// Delete software buffer
	delete[] m_pDepthBufferPixels;

	// Release DirectX pipeline
	m_pRenderTargetView->Release();
	m_pRenderTargetBuffer->Release();
	m_pDepthStencilView->Release();
	m_pDepthStencilBuffer->Release();
	m_pSwapChain->Release();

	m_pRasterState_BackCulling->Release();
	m_pRasterState_FrontCulling->Release();
	m_pRasterState_NoCulling->Release();

	m_pPointSamplerState->Release();
	m_pLinearSamplerState->Release();
	m_pAnisotropicSamplerState->Release();

	if (m_pDeviceContext) {
		m_pDeviceContext->ClearState();
		m_pDeviceContext->Flush();
		m_pDeviceContext->Release();
	}

	m_pDevice->Release();
}

void Renderer::Update(const Timer* pTimer)
{
	m_Camera.Update(pTimer);

	if (m_ShouldRotate) {
		m_pVehicleMesh->Update(pTimer->GetElapsed());
		m_pFireMesh->Update(pTimer->GetElapsed());
	}
}


void Renderer::Render()
{
	switch (m_RenderMode) {
		case RenderMode::software:
			RenderSoftware();
			break;
		case RenderMode::hardware:
			RenderHardware();
			break;
	}
}

HRESULT Renderer::InitializeDirectX()
{
	// Create Device Context
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
	uint32_t createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags, &featureLevel, 1, D3D11_SDK_VERSION, &m_pDevice, nullptr, &m_pDeviceContext);
	if (FAILED(result)) {
		return result;
	}

	// Create DGXIFactory
	IDXGIFactory1* pDXGIFactory{};
	result = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pDXGIFactory));
	if (FAILED(result)) {
		return result;
	}

	// Get Window Handle from SDL
	SDL_SysWMinfo sysInfo{};
	SDL_VERSION(&sysInfo.version);
	SDL_GetWindowWMInfo(m_pWindow, &sysInfo);

	// Create Swapchain
	DXGI_SWAP_CHAIN_DESC swapChainDesc{};
	swapChainDesc.BufferDesc.Width = m_Width;
	swapChainDesc.BufferDesc.Height = m_Height;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 1;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 60;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = 0;
	swapChainDesc.OutputWindow = sysInfo.info.win.window;

	result = pDXGIFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
	if (FAILED(result)) {
		return result;
	}

	// Release the factory
	pDXGIFactory->Release();

	// Create DepthStencil and DepthStencilView
	D3D11_TEXTURE2D_DESC depthStencilDesc{};
	depthStencilDesc.Width = m_Width;
	depthStencilDesc.Height = m_Height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
	depthStencilViewDesc.Format = depthStencilDesc.Format;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	result = m_pDevice->CreateTexture2D(&depthStencilDesc, nullptr, &m_pDepthStencilBuffer);
	if (FAILED(result)) {
		return result;
	}

	result = m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &depthStencilViewDesc, &m_pDepthStencilView);
	if (FAILED(result)) {
		return result;
	}

	// Create Render Target and Render Target View
	result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pRenderTargetBuffer));
	if (FAILED(result)) {
		return result;
	}

	result = m_pDevice->CreateRenderTargetView(m_pRenderTargetBuffer, nullptr, &m_pRenderTargetView);
	if (FAILED(result)) {
		return result;
	}

	// Bind both views to Output Merger
	m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);

	// Set up viewport
	D3D11_VIEWPORT viewport{};
	viewport.Width = static_cast<float>(m_Width);
	viewport.Height = static_cast<float>(m_Height);
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	m_pDeviceContext->RSSetViewports(1, &viewport);

	// Culling raster states
	D3D11_RASTERIZER_DESC rasterStateDesc{};
	rasterStateDesc.FillMode = D3D11_FILL_SOLID;
	rasterStateDesc.FrontCounterClockwise = true;
	rasterStateDesc.DepthBias = 0;
	rasterStateDesc.SlopeScaledDepthBias = 0.0f;
	rasterStateDesc.DepthBiasClamp = 0.0f;
	rasterStateDesc.DepthClipEnable = true;
	rasterStateDesc.ScissorEnable = false;
	rasterStateDesc.MultisampleEnable = false;
	rasterStateDesc.AntialiasedLineEnable = false;

	rasterStateDesc.CullMode = D3D11_CULL_BACK;
	m_pDevice->CreateRasterizerState(&rasterStateDesc, &m_pRasterState_BackCulling);

	rasterStateDesc.CullMode = D3D11_CULL_NONE;
	m_pDevice->CreateRasterizerState(&rasterStateDesc, &m_pRasterState_NoCulling);

	rasterStateDesc.CullMode = D3D11_CULL_FRONT;
	m_pDevice->CreateRasterizerState(&rasterStateDesc, &m_pRasterState_FrontCulling);

	D3D11_SAMPLER_DESC samplerStateDesc{};
	samplerStateDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerStateDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerStateDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerStateDesc.MinLOD = -FLT_MAX;
	samplerStateDesc.MaxLOD = FLT_MAX;
	samplerStateDesc.MipLODBias = 0.0f;
	samplerStateDesc.MaxAnisotropy = 1;
	samplerStateDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

	samplerStateDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	m_pDevice->CreateSamplerState(&samplerStateDesc, &m_pPointSamplerState);

	samplerStateDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	m_pDevice->CreateSamplerState(&samplerStateDesc, &m_pLinearSamplerState);

	samplerStateDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	m_pDevice->CreateSamplerState(&samplerStateDesc, &m_pAnisotropicSamplerState);

	return result;
}

void Renderer::RenderHardware() const{
	if (!m_IsInitialized)
		return;

	// Clear buffers
	ColorRGB clearColor{ (m_UseUniformBackground) ? colors::Uniform :colors::Hardware };
	m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, &clearColor.r);
	m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);

	// Set right culling option
	switch (m_CullMode)
	{
		case CullMode::back:
			m_pDeviceContext->RSSetState(m_pRasterState_BackCulling);
			break;
		case CullMode::front:
			m_pDeviceContext->RSSetState(m_pRasterState_FrontCulling);
			break;
		case CullMode::none:
			m_pDeviceContext->RSSetState(m_pRasterState_NoCulling);
			break;
	}

	// Set right culling option
	ID3D11SamplerState* samplerState{};
	switch (m_Filtering)
	{
		case Filtering::point:
			samplerState = m_pPointSamplerState;
			break;
		case Filtering::linear:
			samplerState = m_pLinearSamplerState;
			break;
		case Filtering::anisotropic:
			samplerState = m_pAnisotropicSamplerState;
			break;
	}

	// Drawing
	m_pVehicleMesh->RenderHardware(m_pDeviceContext, m_Camera, samplerState);
	if (m_DrawFireMesh) {
		m_pDeviceContext->RSSetState(m_pRasterState_NoCulling);
		m_pFireMesh->RenderHardware(m_pDeviceContext, m_Camera, samplerState);
	}

	// Swap buffers
	m_pSwapChain->Present(0, 0);
}

	
void Renderer::RenderSoftware(){

	SDL_LockSurface(m_pBackBuffer);

	// Clear buffers
	ColorRGB clearColor{ (m_UseUniformBackground) ? colors::Uniform : colors::Software };
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, Uint8(255 * clearColor.r), Uint8(255 * clearColor.g), Uint8(255 * clearColor.b)));
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

	// Add objects to the render vector
	std::vector<Mesh*> m_Meshes;
	m_Meshes.push_back(m_pVehicleMesh);

	// Render each mesh
	for (auto& mesh : m_Meshes) {
		std::vector<Vertex_Out> verts;

		verts = VertexShader(*mesh);
		verts = InterPolateAttributes(*mesh, verts);
		PixelShader(*mesh, verts);
	}

	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::InitSoftware(SDL_Window* pWindow) {
	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height];
}

void Renderer::InitMeshes() {
	std::vector<VertexUV> vertices;
	std::vector<uint32_t> indices;

	//  --Vehicle mesh--
	//
	Utils::ParseOBJ("Resources/vehicle.obj", vertices, indices);
	m_pVehicleMesh = new Mesh(m_pDevice, vertices, indices);

	// Load Textures
	Texture* diffuseMap = new Texture();
	diffuseMap->LoadFromFile(m_pDevice, "Resources/vehicle_diffuse.png");

	Texture* normalMap = new Texture();
	normalMap->LoadFromFile(m_pDevice, "Resources/vehicle_normal.png");

	Texture* specularMap = new Texture();
	specularMap->LoadFromFile(m_pDevice, "Resources/vehicle_specular.png");

	Texture* glossyMap = new Texture();
	glossyMap->LoadFromFile(m_pDevice, "Resources/vehicle_gloss.png");

	m_pVehicleMesh->SetMaps(diffuseMap, normalMap, specularMap, glossyMap);
	m_pVehicleMesh->SetPosition({ 0.0f, 0.0f, 50.0f });

	// Load effect
	Effect* effect = new Effect_Vertex(m_pDevice);
	effect->Initialize();
	m_pVehicleMesh->SetEffect(effect);

	//  --Fire FX mesh--
	//
	Utils::ParseOBJ("Resources/fireFX.obj", vertices, indices);
	m_pFireMesh = new Mesh(m_pDevice, vertices, indices);

	// Load Textures
	diffuseMap = new Texture();
	diffuseMap->LoadFromFile(m_pDevice, "Resources/fireFX_diffuse.png");
	m_pFireMesh->SetMaps(diffuseMap);
	m_pFireMesh->SetPosition({ 0.0f, 0.0f, 50.0f });

	// Load effect
	effect = new Effect_DiffuseAlpha(m_pDevice);
	effect->Initialize();
	m_pFireMesh->SetEffect(effect);
}

std::vector<Vertex_Out> Renderer::VertexShader(const Mesh& mesh) {

	std::vector<VertexUV> vertices_in{mesh.GetVertices()};
	std::vector<Vertex_Out> vertices_out{};
	vertices_out.reserve(vertices_in.size());
	Matrix worldMatrix{ mesh.GetWorldMatrix() };
	Matrix WVPMatrix{ worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix };

	for (const VertexUV& vertex : vertices_in) {

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
		vertexOut.normal = worldMatrix.TransformVector(vertexOut.normal);
		vertexOut.tangent = worldMatrix.TransformVector(vertexOut.tangent);

		// Calculate viewDirection
		vertexOut.viewDirection = worldMatrix.TransformPoint(vertex.position) - m_Camera.origin;
		vertexOut.viewDirection.Normalize();

		vertices_out.emplace_back(vertexOut);
	}

	return vertices_out;
}

void Renderer::PixelShader(const Mesh& mesh, const std::vector<Vertex_Out>& verts) {
	
	for (const auto& vertex : verts) {
		ColorRGB finalColor{ mesh.PixelShading(vertex, m_ShadingMode, m_UseNormalMap) };

		//Update Color in Buffer
		finalColor.MaxToOne();

		m_pBackBufferPixels[int(vertex.position.x) + (int(vertex.position.y) * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
			static_cast<uint8_t>(finalColor.r * 255),
			static_cast<uint8_t>(finalColor.g * 255),
			static_cast<uint8_t>(finalColor.b * 255));
	}
}

std::vector<Vertex_Out> Renderer::InterPolateAttributes(const Mesh& mesh, const std::vector<Vertex_Out>& verts) {
	
	std::vector<uint32_t> indices{ mesh.GetIndices() };
	std::vector<Vertex_Out> vertices_out{};
	
	for (int triangleIndex{ 0 }; triangleIndex + 2 < indices.size(); ++triangleIndex) {

		// Get correct indexes based on the mesh's topology
		int index0{}, index1{}, index2{};
		if (mesh.primitiveTopology == PrimitiveTopology::TriangleList) {

			index0 = indices[triangleIndex + 0];
			index1 = indices[triangleIndex + 1];
			index2 = indices[triangleIndex + 2];
			triangleIndex += 2;
		}

		if (mesh.primitiveTopology == PrimitiveTopology::TriangleStrip) {

			index0 = indices[triangleIndex + 0];
			if (triangleIndex % 2 == 0) {
				index1 = indices[triangleIndex + 1];
				index2 = indices[triangleIndex + 2];
			}
			else {
				index1 = indices[triangleIndex + 2];
				index2 = indices[triangleIndex + 1];
			}

			if (index0 == index1 || index1 == index2 || index2 == index0) {
				continue;
			}
		}

		// Render triangle
		Vertex_Out v0 = verts[index0];
		Vertex_Out v1 = verts[index1];
		Vertex_Out v2 = verts[index2];

		// Culling triangles out of view
		if (abs(v0.position.x) > 1 || abs(v0.position.y) > 1 || v0.position.z < 0 || v0.position.z > 1) {
			continue;
		}
		if (abs(v1.position.x) > 1 || abs(v1.position.y) > 1 || v1.position.z < 0 || v1.position.z > 1) {
			continue;
		}
		if (abs(v2.position.x) > 1 || abs(v2.position.y) > 1 || v2.position.z < 0 || v2.position.z > 1) {
			continue;
		}

		// Cull base on cull mode
		Vector3 TriangleNormal{ (v0.normal + v1.normal + v2.normal)/3 };
		if ( m_CullMode == CullMode::back && Vector3::Dot(TriangleNormal, m_Camera.forward) > 0) {
			continue;
		}

		if (m_CullMode == CullMode::front && Vector3::Dot(TriangleNormal, m_Camera.forward) < 0) {
			continue;
		}

		// To Screen Space
		v0.position.x = (v0.position.x + 1) * m_Width / 2;
		v0.position.y = (-v0.position.y + 1) * m_Height / 2;
		v1.position.x = (v1.position.x + 1) * m_Width / 2;
		v1.position.y = (-v1.position.y + 1) * m_Height / 2;
		v2.position.x = (v2.position.x + 1) * m_Width / 2;
		v2.position.y = (-v2.position.y + 1) * m_Height / 2;

		// Find bounding box
		Int2 pMin{}, pMax{};
		pMin.x = Clamp(int(std::min(v2.position.x, std::min(v0.position.x, v1.position.x))), 0, m_Width);
		pMin.y = Clamp(int(std::min(v2.position.y, std::min(v0.position.y, v1.position.y))), 0, m_Height);
		pMax.x = Clamp(int(std::max(v2.position.x, std::max(v0.position.x, v1.position.x))), 0, m_Width);
		pMax.y = Clamp(int(std::max(v2.position.y, std::max(v0.position.y, v1.position.y))), 0, m_Height);

		// Loop over pixels
		for (int px{ pMin.x }; px <= pMax.x; ++px) {
			for (int py{ pMin.y }; py <= pMax.y; ++py) {

				Vector2 pixel{ float(px),float(py) };

				// Visualize the bouding boxes
				if (m_VisualizeBoundingBoxes) {
					m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(255),
						static_cast<uint8_t>(255),
						static_cast<uint8_t>(255));
					continue;
				}

				//Side A
				Vector2 side{ v1.position.GetXY() - v0.position.GetXY() };
				Vector2 pointToSide{ pixel - v0.position.GetXY() };
				float w2{ Vector2::Cross(side,pointToSide) };

				//Side B
				side = { v2.position.GetXY() - v1.position.GetXY() };
				pointToSide = { pixel - v1.position.GetXY() };
				float w0{ Vector2::Cross(side,pointToSide) };

				//Side C
				side = { v0.position.GetXY() - v2.position.GetXY() };
				pointToSide = { pixel - v2.position.GetXY() };
				float w1{ Vector2::Cross(side,pointToSide) };

				if (w0 >= 0 && w1 >= 0 && w2 >= 0) {

					// Calculate Barycentric weights
					const float totalArea = w0 + w1 + w2;
					w0 /= totalArea;
					w1 /= totalArea;
					w2 /= totalArea;

					float interpolatedDepth{ 1.0f / (w0 * (1 / v0.position.z) + w1 * (1 / v1.position.z) + w2 * (1 / v2.position.z)) };
					bool depthTestPassed{ interpolatedDepth < m_pDepthBufferPixels[px + (py * m_Width)] };

					if (depthTestPassed) {

						// Update Depth Buffer
						m_pDepthBufferPixels[px + (py * m_Width)] = interpolatedDepth;

						// Visualize the depth buffer
						if (m_VisualizeDepthBuffer) {

							float depthColor{ Remap(interpolatedDepth, 0.997f, 1.0f) };

							m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
								static_cast<uint8_t>(depthColor * 255),
								static_cast<uint8_t>(depthColor * 255),
								static_cast<uint8_t>(depthColor * 255));
							continue;
						}

						// InterpolatedW
						float interpolatedW{ 1.0f / (w0 * (1 / v0.position.w) + w1 * (1 / v1.position.w) + w2 * (1 / v2.position.w)) };

						// Interpolated UV
						Vector2 interpolatedUV{ w0 * (v0.uv / v0.position.w) + w1 * (v1.uv / v1.position.w) + w2 * (v2.uv / v2.position.w) };
						interpolatedUV *= interpolatedW;

						// Interpolated Normal
						Vector3 InterpolatedNormal{ w0 * (v0.normal / v0.position.w) + w1 * (v1.normal / v1.position.w) + w2 * (v2.normal / v2.position.w) };
						InterpolatedNormal *= interpolatedW;

						// Interpolated Tangent
						Vector3 InterpolatedTangent{ w0 * (v0.tangent / v0.position.w) + w1 * (v1.tangent / v1.position.w) + w2 * (v2.tangent / v2.position.w) };
						InterpolatedTangent *= interpolatedW;

						// Interpolated view direction
						Vector3 InterpolatedViewDirection{ w0 * (v0.viewDirection / v0.position.w) + w1 * (v1.viewDirection / v1.position.w) + w2 * (v2.viewDirection / v2.position.w) };
						InterpolatedViewDirection *= interpolatedW;

						Vertex_Out pixelVertex{};
						pixelVertex.position = { pixel.x, pixel.y, interpolatedDepth, interpolatedW };
						pixelVertex.uv = interpolatedUV;
						pixelVertex.normal = InterpolatedNormal.Normalized();
						pixelVertex.tangent = InterpolatedTangent.Normalized();
						pixelVertex.viewDirection = InterpolatedViewDirection.Normalized();

						vertices_out.push_back(pixelVertex);
					}
				}
			}
		}
	}

	return vertices_out;
}

void Renderer::SwitchRenderMode() {
	m_RenderMode = (m_RenderMode == RenderMode::software) ? m_RenderMode = RenderMode::hardware : RenderMode::software;

	std::cout << "Rasterizer mode = " << ((m_RenderMode == RenderMode::software) ? "SOFTWARE" : "HARDWARE") << "\n";
}

void Renderer::ToggleCullMode() { 
	m_CullMode = CullMode((int(m_CullMode) + 1) % 3); 
	
	switch (m_CullMode)
	{
		std::cout << "Cull Mode = ";
		case CullMode::back:
			std::cout << "BACK-FACE CULLING\n";
			break;
		case CullMode::front:
			std::cout << "FRONT-FACE CULLING\n";
			break;
		case CullMode::none:
			std::cout << "NO CULLING\n";
			break;
	}
}

void Renderer::ToggleShadingMode() {
	if (m_RenderMode == RenderMode::software) {
		m_ShadingMode = ShadingMode((int(m_ShadingMode) + 1) % 4);

		std::cout << "Shading mode = ";
		switch (m_ShadingMode)
		{
			case ShadingMode::observerdArea:
				std::cout << "OBSERVED AREA\n";
				break;
			case ShadingMode::diffuse:
				std::cout << "DIFFUSE\n";
				break;
			case ShadingMode::specular:
				std::cout << "SPECULAR\n";
				break;
			case ShadingMode::combined:
				std::cout << "COMBINED\n";
				break;
		}
	}
}

void Renderer::ToggleBoundingBoxes() {
	if (m_RenderMode == RenderMode::software) {
		m_VisualizeBoundingBoxes = !m_VisualizeBoundingBoxes;
		std::cout << "Visualize Bounding Boxes " << ((m_VisualizeBoundingBoxes) ? "ON" : "OFF") << "\n";
	}
}

void Renderer::ToggleDepthBuffer() {
	if (m_RenderMode == RenderMode::software) {
		m_VisualizeDepthBuffer = !m_VisualizeDepthBuffer;
		std::cout << "Show Depth Buffer " << ((m_VisualizeDepthBuffer) ? "ON" : "OFF") << "\n";
	}
}

void Renderer::ToggleNormalMap() {
	if (m_RenderMode == RenderMode::software) {
		m_UseNormalMap = !m_UseNormalMap;
		std::cout << "Normal Map " << ((m_UseNormalMap) ? "ON" : "OFF") << "\n";
	}
}

void Renderer::ToggleRotation() {
	m_ShouldRotate = !m_ShouldRotate;
	std::cout << "Vehicle Rotation " << ((m_ShouldRotate) ? "ON" : "OFF") << "\n";
}

void Renderer::ToggleUniformBackground() { 
	m_UseUniformBackground = !m_UseUniformBackground; 
	std::cout << "Uniform Background " << ((m_UseUniformBackground) ? "ON" : "OFF") << "\n";
}

void Renderer::ToggleFireMesh() { 
	if (m_RenderMode == RenderMode::hardware) {
		m_DrawFireMesh = !m_DrawFireMesh;
		std::cout << "Fire Effect " << ((m_DrawFireMesh) ? "ON" : "OFF") << "\n";
	}
}

float Renderer::Remap(float value, float min, float max) {
	return (value - min) / (max - min);
}

void Renderer::SwitchFilteringMethod() {
	if (m_RenderMode == RenderMode::hardware) {
		m_Filtering = Filtering((int(m_Filtering) + 1) % 3);

		std::cout << "Sample Filter = ";
		switch (m_Filtering)
		{
		case Filtering::point:
			std::cout << "POINT\n";
			break;
		case Filtering::linear:
			std::cout << "LINEAR\n";
			break;
		case Filtering::anisotropic:
			std::cout << "ANISOTROPIC\n";
			break;
		}
	}
}