// Ben Coukos-Wiley
// 2/25/2024
// A bundle of textures

#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <DirectXMath.h>

class Material
{
public:
	Material(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState, DirectX::XMFLOAT3 colorTint, DirectX::XMFLOAT2 uvScale, DirectX::XMFLOAT2 uvOffset);
	~Material();

	// TODO: Add necessary getters/setters
	DirectX::XMFLOAT3 GetColorTint();
	DirectX::XMFLOAT2 GetUVScale();
	DirectX::XMFLOAT2 GetUVOffset();
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandleForFirstSRV();
	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPipelineState();

	/// <summary>
	/// Adds the given texture SRV to this material in the specified slot (1 = Albedo, 2 = Roughness, 3 = Normal, 4 = Metalness)
	/// </summary>
	/// <param name="srv">A texture SRV</param>
	/// <param name="slot">The slot to store the SRV in</param>
	void AddTexture(D3D12_CPU_DESCRIPTOR_HANDLE srv, int slot);
	/// <summary>
	/// Uploads all stored descriptors to the GPU, making this material ready to use.
	/// </summary>
	void FinalizeMaterial();
private:
	DirectX::XMFLOAT3 colorTint;
	DirectX::XMFLOAT2 uvScale;
	DirectX::XMFLOAT2 uvOffset;

	bool finalized;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
	D3D12_CPU_DESCRIPTOR_HANDLE textureSRVsBySlot [4];
	D3D12_GPU_DESCRIPTOR_HANDLE finalGPUHandleForFirstSRV;
};

