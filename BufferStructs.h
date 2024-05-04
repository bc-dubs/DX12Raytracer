#pragma once

#include <DirectXMath.h>
#include "Lights.h"

struct VertexShaderExternalData {
	DirectX::XMFLOAT4X4 world;
	DirectX::XMFLOAT4X4 worldInvTranspose;
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 projection;
};

struct PixelShaderExternalData {
	DirectX::XMFLOAT2 uvScale;
	DirectX::XMFLOAT2 uvOffset;
	DirectX::XMFLOAT3 cameraPosition;
	int lightCount;
	Light lights[5];
};

// Overall scene data for raytracing
struct RaytracingSceneData
{
	DirectX::XMFLOAT4X4 inverseViewProjection;
	DirectX::XMFLOAT3 cameraPosition;
};

struct RaytracingMaterialData
{
	DirectX::XMFLOAT4 color;
	int albedoIndex;
	int roughnessIndex;
	int normalsIndex;
	int metalIndex;
};

// Ensure this matches Raytracing shader define!
#define MAX_INSTANCES_PER_BLAS 100
struct RaytracingEntityData
{
	DirectX::XMFLOAT4X4 worldInvTranspose[MAX_INSTANCES_PER_BLAS];
	RaytracingMaterialData materialData[MAX_INSTANCES_PER_BLAS];
};
