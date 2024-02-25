#include "Material.h"
#include "DX12Helper.h"

Material::Material(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState, DirectX::XMFLOAT3 colorTint, DirectX::XMFLOAT2 uvScale, DirectX::XMFLOAT2 uvOffset) :
	pipelineState(pipelineState),
	colorTint(colorTint),
	uvScale(uvScale),
	uvOffset(uvOffset),
	finalized(false)
{
	finalGPUHandleForFirstSRV = {};
}

Material::~Material()
{
}

D3D12_GPU_DESCRIPTOR_HANDLE Material::GetGPUHandleForFirstSRV()
{
	if(finalized)
		return finalGPUHandleForFirstSRV;
	return D3D12_GPU_DESCRIPTOR_HANDLE();
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> Material::GetPipelineState()
{
	return pipelineState;
}

void Material::AddTexture(D3D12_CPU_DESCRIPTOR_HANDLE srv, int slot)
{
	textureSRVsBySlot[slot] = srv;
}

void Material::FinalizeMaterial()
{
	if (finalized) // Don't finalize twice
		return;

	// Store GPU reference to first SRV copied over
	finalGPUHandleForFirstSRV = DX12Helper::GetInstance().CopySRVsToDescriptorHeapAndGetGPUDescriptorHandle(textureSRVsBySlot[0], 1);
	for (unsigned int i = 1; i < 4; i++) {
		// Can't copy all 4 at once because they are in different heaps
		DX12Helper::GetInstance().CopySRVsToDescriptorHeapAndGetGPUDescriptorHandle(textureSRVsBySlot[i], 1);
	}

	finalized = true;
}
