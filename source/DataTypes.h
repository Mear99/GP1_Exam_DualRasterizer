#pragma once
#include "Math.h"
#include "vector"

using namespace dae;

struct Vertex_Out
{
	Vector4 position{};
	Vector2 uv{};
	Vector3 normal{};
	Vector3 tangent{};
	Vector3 viewDirection{};
};

struct VertexUV {
	Vector3 position;
	Vector2 uv;
	Vector3 normal;
	Vector3 tangent;
};

enum class PrimitiveTopology { TriangleList, TriangleStrip };
enum class Filtering { point, linear, anisotropic };
enum class RenderMode { software, hardware };
enum class CullMode{ back, front, none};
enum class ShadingMode { observerdArea, diffuse, specular, combined };
