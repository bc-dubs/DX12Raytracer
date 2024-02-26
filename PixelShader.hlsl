#include "ShaderIncludes.hlsli"

#define MAX_LIGHTS 5

Texture2D AlbedoTexture		: register(t0);	// "t" registers for textures
Texture2D RoughnessMap		: register(t1);
Texture2D NormalMap			: register(t2);
Texture2D MetalnessMap		: register(t3);
SamplerState BasicSampler	: register(s0);	// "s" registers for samplers

cbuffer ExternalData : register(b0)
{
	float2 uvScale;
	float2 uvOffset;
	float3 cameraPosition;
	int lightCount;
	Light lights[MAX_LIGHTS];
}

// Struct representing the data we expect to receive from earlier pipeline stages
// - Should match the output of our corresponding vertex shader
// - The name of the struct itself is unimportant
// - The variable names don't have to match other shaders (just the semantics)
// - Each variable must have a semantic, which defines its usage
struct VertexToPixel
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
	float4 screenPosition	: SV_POSITION;	// XYZW position (System Value Position)
	float3 normal			: NORMAL;		// Surface normal
	float3 tangent			: TANGENT;		// Surface tangent
	float3 worldPosition	: POSITION;		// Position in world space
	float2 uv				: TEXCOORD;     // UV position
};


// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
	// Normal map code
	float3 textureNormal = NormalMap.Sample(BasicSampler, input.uv).rgb * 2 - 1;
	textureNormal = normalize(textureNormal);

	float3x3 TBN = CalculateTBN(normalize(input.normal), normalize(input.tangent));
	input.normal = mul(textureNormal, TBN);

	// Texture code
	float3 albedo = pow(AlbedoTexture.Sample(BasicSampler, input.uv).rgb, 2.2f); // un-gamma correcting
	float roughness = RoughnessMap.Sample(BasicSampler, input.uv).r;
	float metalness = MetalnessMap.Sample(BasicSampler, input.uv).r;

	float3 fresnelAt0 = lerp(F0_NON_METAL, albedo.rgb, metalness); // Metals' f0 value is stored in albedo textures, nonmetals' is a constant

	// Vector code
	float3 viewVector = normalize(cameraPosition - input.worldPosition);

	float3 totalColor;

	// Using BRDFs
	for (int i = 0; i < lightCount; i++) {
		float3 directionToLight = -lights[i].direction;
		float intensity = lights[i].intensity;
		if (lights[i].type == 1) { // Values for point lights
			directionToLight = lights[i].position - input.worldPosition;
			intensity *= Attenuate(lights[i], input.worldPosition);
		}
		directionToLight = normalize(directionToLight);

		float3 diffuse = CalculateDiffuse(input.normal, directionToLight); // diffuse component for this pixel
		float3 fresnel;
		float3 specular = MicrofacetBRDF(input.normal, directionToLight, viewVector, roughness, fresnelAt0, fresnel); // specular component for this pixel
		diffuse = DiffuseEnergyConserve(diffuse, fresnel, metalness);

		float3 lightColor = (diffuse * albedo + specular) * lights[i].intensity * lights[i].color;

		totalColor += lightColor;
	}

	// Gamma correcting and returning
	return float4(pow(totalColor, 1 / 2.2f), 1);

	//float3 iNorm = (input.normal + float3(1, 1, 1)) / 2;
	//iNorm = normalize(iNorm);

	//return float4(iNorm.x, iNorm.y, iNorm.z, 1);
}