#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "BufferStructs.h"
#include "PathHelpers.h"
#include "DX12Helper.h"


// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;
using namespace std;

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// Direct3D itself, and our window, are not ready at this point!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,			// The application's handle
		L"DirectX Game",	// Text for the window's title bar (as a wide-character string)
		1280,				// Width of the window's client area
		720,				// Height of the window's client area
		false,				// Sync the framerate to the monitor refresh? (lock framerate)
		true)				// Show extra stats (fps) in title bar?
{
#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Delete all objects manually created within this class
//  - Release() all Direct3D objects created within this class
// --------------------------------------------------------
Game::~Game()
{
	// Call delete or delete[] on any objects or arrays you've
	// created using new or new[] within this class
	// - Note: this is unnecessary if using smart pointers

	// Call Release() on any Direct3D objects made within this class
	// - Note: this is unnecessary for D3D objects stored in ComPtrs

	// Cannot delete until the GPU is done with its work
	DX12Helper::GetInstance().WaitForGPU();

}

// --------------------------------------------------------
// Called once per program, after Direct3D and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	CreateRootSigAndPipelineState();
	CreateBasicGeometry();
}

// --------------------------------------------------------
// Loads the two basic shaders, then creates the root signature 
// and pipeline state object for our very basic demo.
// --------------------------------------------------------
void Game::CreateRootSigAndPipelineState()
{
	// Blobs to hold raw shader byte code used in several steps below
	Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderByteCode;

	// Load shaders
	{
		// Read our compiled vertex shader code into a blob
		// - Essentially just "open the file and plop its contents here"
		D3DReadFileToBlob(FixPath(L"VertexShader.cso").c_str(), vertexShaderByteCode.GetAddressOf());
		D3DReadFileToBlob(FixPath(L"PixelShader.cso").c_str(), pixelShaderByteCode.GetAddressOf());
	}

	// Input layout
	const unsigned int inputElementCount = 4;
	D3D12_INPUT_ELEMENT_DESC inputElements[inputElementCount] = {};
	{
		inputElements[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT; // R32 G32 B32 = float3
		inputElements[0].SemanticName = "POSITION"; // Name must match semantic in shader
		inputElements[0].SemanticIndex = 0; // This is the first POSITION semantic (why is it first, what comes after?)

		inputElements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[1].Format = DXGI_FORMAT_R32G32_FLOAT; // R32 G32 = float2
		inputElements[1].SemanticName = "TEXCOORD";
		inputElements[1].SemanticIndex = 0; // This is the first TEXCOORD semantic

		inputElements[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[2].Format = DXGI_FORMAT_R32G32B32_FLOAT; // R32 G32 B32 = float3
		inputElements[2].SemanticName = "NORMAL";
		inputElements[2].SemanticIndex = 0; // This is the first NORMAL semantic

		inputElements[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[3].Format = DXGI_FORMAT_R32G32B32_FLOAT; // R32 G32 B32 = float3
		inputElements[3].SemanticName = "TANGENT";
		inputElements[3].SemanticIndex = 0; // This is the first TANGENT semantic
	}

	// Root Signature
	{
		// Describe the range of CBVs needed for the vertex shader
		D3D12_DESCRIPTOR_RANGE cbvRangeVS = {};
		cbvRangeVS.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangeVS.NumDescriptors = 1;
		cbvRangeVS.BaseShaderRegister = 0;
		cbvRangeVS.RegisterSpace = 0;
		cbvRangeVS.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Describe the range of CBVs needed for the pixel shader
		D3D12_DESCRIPTOR_RANGE cbvRangePS = {};
		cbvRangePS.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangePS.NumDescriptors = 1;
		cbvRangePS.BaseShaderRegister = 0;
		cbvRangePS.RegisterSpace = 0;
		cbvRangePS.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Create a range of SRV's for textures
		D3D12_DESCRIPTOR_RANGE srvRange = {};
		srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange.NumDescriptors = 4;	// Set to max number of textures at once (match pixel shader!)
		srvRange.BaseShaderRegister = 0;	// Starts at s0 (match pixel shader!)
		srvRange.RegisterSpace = 0;
		srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Create the root parameters
		D3D12_ROOT_PARAMETER rootParams[3] = {}; // 3 root params for this root sig, which is shared by both PS and VS

		// CBV table param for vertex shader
		rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // It's a desc. table
		rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // Visible to just VS
		rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[0].DescriptorTable.pDescriptorRanges = &cbvRangeVS;

		// CBV table param for pixel shader
		rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // It's a desc. table
		rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // Visible to just PS
		rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[1].DescriptorTable.pDescriptorRanges = &cbvRangePS;

		// SRV table param
		rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[2].DescriptorTable.pDescriptorRanges = &srvRange;

		// Create a single static sampler (available to all pixel shaders at the same slot)
		D3D12_STATIC_SAMPLER_DESC anisoWrap = {};
		anisoWrap.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // Texture wraps when coordinate is outside 0-1 range
		anisoWrap.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.Filter = D3D12_FILTER_ANISOTROPIC;
		anisoWrap.MaxAnisotropy = 16; // High level of max anistropy
		anisoWrap.MaxLOD = D3D12_FLOAT32_MAX;
		anisoWrap.ShaderRegister = 0;  // register(s0)
		anisoWrap.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // Sampler is visible to just PS
		D3D12_STATIC_SAMPLER_DESC samplers[] = { anisoWrap }; // All samplers in root sig

		// Describe and serialize the root signature
		D3D12_ROOT_SIGNATURE_DESC rootSig = {};
		rootSig.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSig.NumParameters = ARRAYSIZE(rootParams);
		rootSig.pParameters = rootParams;
		rootSig.NumStaticSamplers = ARRAYSIZE(samplers);
		rootSig.pStaticSamplers = samplers;

		ID3DBlob* serializedRootSig = 0;
		ID3DBlob* errors = 0;

		D3D12SerializeRootSignature(
			&rootSig,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&serializedRootSig,
			&errors);

		// Check for errors during serialization
		if (errors != 0)
		{
			OutputDebugString((wchar_t*)errors->GetBufferPointer());
		}

		// Actually create the root sig
		device->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(rootSignature.GetAddressOf()));
	}

	// Pipeline state
	{
		// Describe the pipeline state
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

		// -- Input assembler related ---
		psoDesc.InputLayout.NumElements = inputElementCount;
		psoDesc.InputLayout.pInputElementDescs = inputElements;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		// Root sig
		psoDesc.pRootSignature = rootSignature.Get();

		// -- Shaders (VS/PS) --- 
		psoDesc.VS.pShaderBytecode = vertexShaderByteCode->GetBufferPointer();
		psoDesc.VS.BytecodeLength = vertexShaderByteCode->GetBufferSize();
		psoDesc.PS.pShaderBytecode = pixelShaderByteCode->GetBufferPointer();
		psoDesc.PS.BytecodeLength = pixelShaderByteCode->GetBufferSize();

		// -- Render targets ---
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;

		// -- States ---
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.RasterizerState.DepthClipEnable = true;

		psoDesc.DepthStencilState.DepthEnable = true;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

		psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		// -- Misc ---
		psoDesc.SampleMask = 0xffffffff;

		// Create the pipe state object
		device->CreateGraphicsPipelineState(&psoDesc,
			IID_PPV_ARGS(pipelineState.GetAddressOf()));
	}
}

// ----------------------------------------
// Creates the geometry we're going to draw
// ----------------------------------------
void Game::CreateBasicGeometry()
{
	// Camera creation
	camera = std::make_shared<Camera>(Camera((float)this->windowWidth / this->windowHeight, XMFLOAT3(0, 0, -10)));

	// Texture Loading
	D3D12_CPU_DESCRIPTOR_HANDLE bronzeAlbedoHandle = DX12Helper::GetInstance().LoadTexture(FixPath(L"..\\..\\Assets\\Textures\\bronze_albedo.png").c_str());
	D3D12_CPU_DESCRIPTOR_HANDLE bronzeNormalHandle = DX12Helper::GetInstance().LoadTexture(FixPath(L"..\\..\\Assets\\Textures\\bronze_normals.png").c_str());
	D3D12_CPU_DESCRIPTOR_HANDLE bronzeRoughHandle = DX12Helper::GetInstance().LoadTexture(FixPath(L"..\\..\\Assets\\Textures\\bronze_roughness.png").c_str());
	D3D12_CPU_DESCRIPTOR_HANDLE bronzeMetalHandle = DX12Helper::GetInstance().LoadTexture(FixPath(L"..\\..\\Assets\\Textures\\bronze_metal.png").c_str());

	D3D12_CPU_DESCRIPTOR_HANDLE scratchedAlbedoHandle = DX12Helper::GetInstance().LoadTexture(FixPath(L"..\\..\\Assets\\Textures\\scratched_albedo.png").c_str());
	D3D12_CPU_DESCRIPTOR_HANDLE scratchedNormalHandle = DX12Helper::GetInstance().LoadTexture(FixPath(L"..\\..\\Assets\\Textures\\scratched_normals.png").c_str());
	D3D12_CPU_DESCRIPTOR_HANDLE scratchedRoughHandle = DX12Helper::GetInstance().LoadTexture(FixPath(L"..\\..\\Assets\\Textures\\scratched_roughness.png").c_str());
	D3D12_CPU_DESCRIPTOR_HANDLE scratchedMetalHandle = DX12Helper::GetInstance().LoadTexture(FixPath(L"..\\..\\Assets\\Textures\\scratched_metal.png").c_str());

	// Material Creation
	shared_ptr<Material> bronze = make_shared<Material>(pipelineState, DirectX::XMFLOAT3(1, 1, 1), DirectX::XMFLOAT2(1, 1), DirectX::XMFLOAT2(0, 0));
	bronze->AddTexture(bronzeAlbedoHandle, 0);
	bronze->AddTexture(bronzeNormalHandle, 1);
	bronze->AddTexture(bronzeRoughHandle, 2);
	bronze->AddTexture(bronzeMetalHandle, 3);
	bronze->FinalizeMaterial();

	shared_ptr<Material> scratched = make_shared<Material>(pipelineState, DirectX::XMFLOAT3(1, 1, 1), DirectX::XMFLOAT2(1, 1), DirectX::XMFLOAT2(0, 0));
	scratched->AddTexture(scratchedAlbedoHandle, 0);
	scratched->AddTexture(scratchedNormalHandle, 1);
	scratched->AddTexture(scratchedRoughHandle, 2);
	scratched->AddTexture(scratchedMetalHandle, 3);
	scratched->FinalizeMaterial();

	// Mesh Creation
	shared_ptr<Mesh> cubeMesh = make_shared<Mesh>(FixPath(L"..\\..\\Assets\\Meshes\\cube.obj").c_str());
	shared_ptr<Mesh> sphereMesh = make_shared<Mesh>(FixPath(L"..\\..\\Assets\\Meshes\\sphere.obj").c_str());
	shared_ptr<Mesh> torusMesh = make_shared<Mesh>(FixPath(L"..\\..\\Assets\\Meshes\\torus.obj").c_str());
	
	entities = std::vector<std::shared_ptr<Entity>>();
	entities.push_back(make_shared<Entity>(cubeMesh, bronze));
	entities.push_back(make_shared<Entity>(sphereMesh, scratched));
	entities.push_back(make_shared<Entity>(torusMesh, bronze));

	// Arranging entities regularly
	{
		int numCols = 5;
		float colSpacing = 3.5f;
		float rowSpacing = 3.2f;
		for (unsigned int i = 0; i < entities.size(); i++) {
			std::shared_ptr<Entity> entity = entities[i];
			float colIndex = (i % numCols) - numCols / 2.0f;
			float rowIndex = i / numCols - (entities.size() / numCols) / 2.0f; // Offsets to center arrangement on approximately 0, 0
			entity->GetTransform()->MoveBy(colIndex * colSpacing, -rowIndex * rowSpacing, 0);
		}
	}
}


// --------------------------------------------------------
// Handle resizing to match the new window size.
//  - DXCore needs to resize the back buffer
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();
	camera->UpdateProjectionMatrix((float)this->windowWidth / this->windowHeight);
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	for (unsigned int i = 0; i < entities.size(); i++) {
		std::shared_ptr<Entity> entity = entities[i];
		//entity->GetTransform()->RotateBy(0.0f, deltaTime/4, 0.0f);
		entity->GetTransform()->SetPosition(entity->GetTransform()->GetPosition()->x, sin(totalTime), entity->GetTransform()->GetPosition()->z);
	}

	camera->Update(deltaTime);

	// Example input checking: Quit if the escape key is pressed
	if (Input::GetInstance().KeyDown(VK_ESCAPE))
		Quit();
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// ============ CLEAR RENDER TARGET ============
	// Grab the current back buffer for this frame
	Microsoft::WRL::ComPtr<ID3D12Resource> currentBackBuffer = backBuffers[currentSwapBuffer];

	// Clearing the render target
	{
		// Transition the back buffer from present to render target
		D3D12_RESOURCE_BARRIER rb = {};
		rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		rb.Transition.pResource = currentBackBuffer.Get();
		rb.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		rb.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &rb);

		// Background color (Cornflower Blue in this case) for clearing
		float color[] = { 0.4f, 0.6f, 0.75f, 1.0f };

		// Clear the RTV
		commandList->ClearRenderTargetView(
			rtvHandles[currentSwapBuffer],
			color,
			0, 0); // No scissor rectangles

		// Clear the depth buffer, too
		commandList->ClearDepthStencilView(
			dsvHandle,
			D3D12_CLEAR_FLAG_DEPTH,
			1.0f,	// Max depth = 1.0f
			0,		// Not clearing stencil, but need a value
			0, 0);	// No scissor rects
	}

	// ============ RENDERING ============
	
	// Setting the pipeline's CBV descriptor heap
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap =
		DX12Helper::GetInstance().GetCBVSRVDescriptorHeap();

	commandList->SetDescriptorHeaps(1, descriptorHeap.GetAddressOf());

	// Acrually rendering
	{
		// Set overall pipeline state
		commandList->SetPipelineState(pipelineState.Get());

		// Root sig (must happen before root descriptor table)
		commandList->SetGraphicsRootSignature(rootSignature.Get());

		// Set up other commands for rendering
		commandList->OMSetRenderTargets(1, &rtvHandles[currentSwapBuffer], true, &dsvHandle);
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);
		//commandList->IASetVertexBuffers(0, 1, &vbView);
		//commandList->IASetIndexBuffer(&ibView);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Draw
		//commandList->DrawIndexedInstanced(3, 1, 0, 0, 0);
		for (unsigned int i = 0; i < entities.size(); i++) {
			std::shared_ptr<Mesh> mesh = entities[i]->GetMesh();
			std::shared_ptr<Material> mat = entities[i]->GetMaterial();

			VertexShaderExternalData vsed = {};
			vsed.world = entities[i]->GetTransform()->GetWorldMatrix();
			vsed.worldInvTranspose = entities[i]->GetTransform()->GetWorldInverseTransposeMatrix();
			vsed.view = camera->GetViewMatrix();
			vsed.projection = camera->GetProjectionMatrix();

			D3D12_GPU_DESCRIPTOR_HANDLE vsedHandle = DX12Helper::GetInstance().FillNextConstantBufferAndGetGPUDescriptorHandle(&vsed, 3 * sizeof(DirectX::XMFLOAT4X4));
			commandList->SetGraphicsRootDescriptorTable(0, vsedHandle);

			D3D12_VERTEX_BUFFER_VIEW vbView = mesh->GetVertexBufferView();
			D3D12_INDEX_BUFFER_VIEW ibView = mesh->GetIndexBufferView();
			commandList->IASetVertexBuffers(0, 1, &vbView);
			commandList->IASetIndexBuffer(&ibView);

			// Set the SRV descriptor handle for this material's textures
			// Note: This assumes that descriptor table 2 is for textures (as per our root sig)
			//commandList->SetPipelineState(mat->GetPipelineState().Get()); // Why do we have to do this again???
			commandList->SetGraphicsRootDescriptorTable(2, mat->GetGPUHandleForFirstSRV());


			commandList->DrawIndexedInstanced(mesh->GetIndexCount(), 1, 0, 0, 0);
		}
	}

	// ============ PRESENTING ============
	// Present
	{
		// Transition back to present
		D3D12_RESOURCE_BARRIER rb = {};
		rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		rb.Transition.pResource = currentBackBuffer.Get();
		rb.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		rb.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &rb);

		// Must occur BEFORE present
		DX12Helper::GetInstance().CloseExecuteAndResetCommandList();

		// Present the current back buffer
		bool vsyncNecessary = vsync || !deviceSupportsTearing || isFullscreen;
		swapChain->Present(
			vsyncNecessary ? 1 : 0,
			vsyncNecessary ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Figure out which buffer is next
		currentSwapBuffer++;
		if (currentSwapBuffer >= numBackBuffers)
			currentSwapBuffer = 0;
	}

}