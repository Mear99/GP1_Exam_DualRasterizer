#pragma once
#include "Math.h"
#include "vector"

namespace dae 
{

	struct Vertex_Out
	{
		Vector4 position{};
		Vector2 uv{};
		Vector3 normal{};
		Vector3 tangent{};
		Vector3 viewDirection{};
	};

	struct VertexColor {
		Vector3 position;
		ColorRGB Color;
	};

	struct VertexUV {
		Vector3 position;
		Vector2 uv;
		Vector3 normal;
		Vector3 tangent;
	};

	enum class PrimitiveTopology { TriangleList, TriangleStrip };
}