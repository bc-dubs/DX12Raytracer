#pragma once

// Ben Coukos-Wiley
// 1/24/2023
// A set of vertices and indices that defines an object

#include <d3d12.h>
#include <wrl/client.h>
#include "Vertex.h"

struct MeshRaytracingData
{
	D3D12_GPU_DESCRIPTOR_HANDLE IndexbufferSRV{ };
	D3D12_GPU_DESCRIPTOR_HANDLE VertexBufferSRV{ };
	Microsoft::WRL::ComPtr<ID3D12Resource> BLAS;
	unsigned int HitGroupIndex = 0;
};

class Mesh
{
public:
	/// <summary>
	/// Constructor that takes the raw vertex/index data
	/// </summary>
	Mesh(Vertex* vertices, int vertexCount, unsigned int* indices, int indexCount);
	/// <summary>
	/// Constructor that takes the name of an OBJ file to load from
	/// </summary>
	Mesh(const wchar_t* filename);
	~Mesh();

	/// <summary>
	/// Returns this mesh's vertex buffer
	/// </summary>
	/// <returns>This mesh's vertex buffer</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> GetVertexBuffer();
	/// <summary>
	/// Returns this mesh's index buffer
	/// </summary>
	/// <returns>This mesh's index buffer</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> GetIndexBuffer();
	/// <summary>
	/// Returns  this mesh's vertex buffer view
	/// </summary>
	/// <returns>This mesh's vertex buffer view</returns>
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView();
	/// <summary>
	/// Returns this mesh's index buffer view
	/// </summary>
	/// <returns>This mesh's index buffer view</returns>
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView();
	/// <summary>
	/// Returns the number of vertices in this mesh
	/// </summary>
	/// <returns>The number of vertices in this mesh</returns>
	unsigned int GetVertexCount();
	/// <summary>
	/// Returns the number of indices in this mesh
	/// </summary>
	/// <returns>The number of indices in this mesh</returns>
	unsigned int GetIndexCount();
	/// <summary>
	/// Returns this mesh's raytracing data
	/// </summary>
	/// <returns>This mesh's raytracing data</returns>
	MeshRaytracingData GetRaytracingData();
	/// <summary>
	/// Draws this mesh
	/// </summary>
	//void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;

	D3D12_VERTEX_BUFFER_VIEW vbView;
	D3D12_INDEX_BUFFER_VIEW ibView;

	unsigned int vertexCount;
	unsigned int indexCount;

	MeshRaytracingData raytraceData;

	void Init(Vertex* vertices, int vertexCount, unsigned int* indices, int indexCount);
	void CalculateTangents(Vertex* verts, int numVerts, unsigned int* indices, int numIndices);
};

