#pragma once

#include "Entity.h"
#include "Camera.h"
#include "Lights.h"

#include <memory>
#include <vector>
#include "DXCore.h"
#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <unordered_map>

class Game 
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);

private:

	// Initialization helper methods - feel free to customize, combine, remove, etc.
	void CreateRootSigAndPipelineState();
	void CreateBasicGeometry();

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	// NEW DX12 items
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;

	// Game variables
	std::vector<std::shared_ptr<Entity>> entities;
	std::shared_ptr<Camera> camera;

	// Lighting variables
	std::unordered_map<int, std::shared_ptr<Light>> activeLights;
	std::vector<Light> lightsToRender;
	std::vector<std::shared_ptr<Light>> allLights;
};

