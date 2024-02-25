#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>

class DX12Helper
{
#pragma region Singleton
public:
	// Gets the one and only instance of this class
	static DX12Helper& GetInstance()
	{
		if (!instance)
		{
			instance = new DX12Helper();
		}

		return *instance;
	}

	// Remove these functions (C++ 11 version)
	DX12Helper(DX12Helper const&) = delete;
	void operator=(DX12Helper const&) = delete;
	/// <summary>
	/// Loads a texture from the file reference, copies that texture to the GPU, creates an SRV for that texture, and stores said SRV in a CPU-side descriptor heap. Returns handle for the CPU descriptor heap.
	/// </summary>
	/// <param name="file">Texture file path</param>
	/// <param name="generateMips">Whether to generate mipmaps for the texture</param>
	/// <returns>The CPU handle for the descriptor heap that holds the SRV describing the texture we created</returns>
	D3D12_CPU_DESCRIPTOR_HANDLE LoadTexture(const wchar_t* file, bool generateMips = true);
	/// <summary>
	/// Uploads a table of SRVs and cppies them to the CBV/SRV decriptor heap on the GPU. Returns a GPU handle so we can find the table of SRVs again.
	/// </summary>
	/// <param name="firstDescriptorToCopy">The CPU address of the first SRV to copy over</param>
	/// <param name="numDescriptorsToCopy">The number of SRVs to copy over</param>
	/// <returns>A GPU handle that points to the first SRV uploaded</returns>
	D3D12_GPU_DESCRIPTOR_HANDLE CopySRVsToDescriptorHeapAndGetGPUDescriptorHandle(
		D3D12_CPU_DESCRIPTOR_HANDLE firstDescriptorToCopy,
		unsigned int numDescriptorsToCopy);
private:
	static DX12Helper* instance;
	DX12Helper() {};
#pragma endregion

public:
	~DX12Helper();

	// Initialization for singleton
	void Initialize(
		Microsoft::WRL::ComPtr<ID3D12Device> device,
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator);

	// Resource creation
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateStaticBuffer(
		unsigned int dataStride,
		unsigned int dataCount,
		void* data);

	// Command list & synchronization
	void CloseExecuteAndResetCommandList();
	void WaitForGPU();
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetCBVSRVDescriptorHeap();
	/// <summary>
	/// Copies the given data into the next constant buffer in the upload heap, creates a CBV that points to it, and returns the GPU handle of that CBV
	/// </summary>
	/// <param name="data">The data to place into the next constant buffer</param>
	/// <param name="dataSizeInBytes"></param>
	/// <returns></returns>
	D3D12_GPU_DESCRIPTOR_HANDLE FillNextConstantBufferAndGetGPUDescriptorHandle(void* data, unsigned int dataSizeInBytes);
private:
	// Overall device
	Microsoft::WRL::ComPtr<ID3D12Device> device;

	// Command list related
	// Note: We're assuming a single command list for the entire
	// engine at this point.  That's not always true for more
	// complex engines but should be fine for now
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	commandList;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>		commandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator>	commandAllocator;

	// Basic CPU/GPU synchronization
	Microsoft::WRL::ComPtr<ID3D12Fence> waitFence;
	HANDLE                              waitFenceEvent;
	unsigned long                       waitFenceCounter;

	// Maximum number of constant buffers, assuming each buffer
	// is 256 bytes or less. Larger buffers are fine, but will
	// result in fewer buffers in use at any time
	const unsigned int maxConstantBuffers = 1000;

	// GPU-side constant buffer upload heap
	Microsoft::WRL::ComPtr<ID3D12Resource> cbUploadHeap;
	UINT64 cbUploadHeapSizeInBytes; // Total size of the entire CB upload heap
	UINT64 cbUploadHeapOffsetInBytes; // How many CBs have been filled so far (loops around)
	void* cbUploadHeapStartAddress; // The address to the upload heap that is stored when it is mapped

	// GPU-side CBV/SRV descriptor heap
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cbvSrvDescriptorHeap;
	SIZE_T cbvSrvDescriptorHeapIncrementSize; // How big the increments for each descriptor are (inherent GPU variable)
	unsigned int cbvDescriptorOffset; // Count of how many descriptors have been made (with looping)

	// Maximum number of texture descriptors (SRVs) we can have.
	// Each material will have a chunk of this,
	// Note: If we delayed the creation of this heap until 
	//       after all textures and materials were created,
	//       we could come up with an exact amount.  The following
	//       constant ensures we (hopefully) never run out of room.
	const unsigned int maxTextureDescriptors = 1000;
	unsigned int srvDescriptorOffset; // Where the SRV portion of the CBV/SRV buffer begins

	// Texture resources we need to keep alive
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> textures; // All textures we've created, stored as C++ objects (isn't this slow??)
	std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> cpuSideTextureDescriptorHeaps; // Holds the "baby heaps" on the CPU as they're created
	
	/// <summary>
	/// Creates the program's CB upload heap during initialization
	/// </summary>
	void CreateConstantBufferUploadHeap();
	/// <summary>
	/// Creates the program's CBV/SRV descriptor heap during initialization
	/// </summary>
	void CreateCBVSRVDescriptorHeap();
};