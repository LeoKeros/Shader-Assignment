//--------------------------------------------------------------------------------------
// Loading GPU shaders
// Creation of constant buffers to help send C++ values to shaders each frame
//--------------------------------------------------------------------------------------

#include "Scene.h"
#include <fstream>
#include <vector>
#include <d3dcompiler.h>

//--------------------------------------------------------------------------------------
// Shader creation / destruction
//--------------------------------------------------------------------------------------

// Load shaders required for this app, returns true on success
bool CSceneManager::LoadShaders()
{
    // Shaders must be added to the Visual Studio project to be compiled, they use the extension ".hlsl".
    // To load them for use, include them here without the extension. Use the correct function for each.
    // Ensure you release the shaders in the ShutdownDirect3D function below
	mVertexShaders[vs_PixelLighting]	= LoadVertexShader("ShadowMapping_vs");
	mPixelShaders[ps_PixelLighting]		= LoadPixelShader("ShadowMapping_ps");
	mVertexShaders[vs_BasicTransform]	= LoadVertexShader("BasicTransform_vs");
	mPixelShaders[ps_LightModel]		= LoadPixelShader("LightModel_ps");
    mPixelShaders[ps_DepthOnly]			= LoadPixelShader ("DepthOnly_ps");
	mPixelShaders[ps_Portal]			= LoadPixelShader("PortalShader_ps");
	mVertexShaders[vs_Wiggle]			= LoadVertexShader("Wiggle_vs");
	mPixelShaders[ps_Wiggle]			= LoadPixelShader("Wiggle_ps");
	mVertexShaders[vs_NormalMap]		= LoadVertexShader("NormalMapping_vs");
	mVertexShaders[vs_WiggleTangent]	= LoadVertexShader("WiggleParallax_vs");
	mPixelShaders[ps_NormalMap]			= LoadPixelShader("NormalMapping_ps");
	mPixelShaders[ps_ParallaxMap]		= LoadPixelShader("ParallaxMapping_ps");
	mPixelShaders[ps_Fade]				= LoadPixelShader("Fade_ps");
	mPixelShaders[ps_Transparent]		= LoadPixelShader("TextureAlpha_ps");

	for (auto &shader : mVertexShaders)
	{
		if (shader == nullptr)
		{
			gLastError = "Error loading vertex shaders";
			return false;
		}
	}
	for (auto &shader : mPixelShaders)
	{
		if (shader == nullptr)
		{
			gLastError = "Error loading pixel shaders";
			return false;
		}
	}

    return true;
}


void CSceneManager::ReleaseShaders()
{
	for (auto &shader : mVertexShaders)
	{
		if (shader) shader->Release();
	}
	for (auto &shader : mPixelShaders)
	{
		if (shader) shader->Release();
	}
}



// Load a vertex shader, include the file in the project and pass the name (without the .hlsl extension)
// to this function. The returned pointer needs to be released before quitting. Returns nullptr on failure. 
ID3D11VertexShader* LoadVertexShader(std::string shaderName)
{
    // Open compiled shader object file
    std::ifstream shaderFile(shaderName + ".cso", std::ios::in | std::ios::binary | std::ios::ate);
    if (!shaderFile.is_open())
    {
        return nullptr;
    }

    // Read file into vector of chars
    std::streamoff fileSize = shaderFile.tellg();
    shaderFile.seekg(0, std::ios::beg);
    std::vector<char> byteCode(fileSize);
    shaderFile.read(&byteCode[0], fileSize);
    if (shaderFile.fail())
    {
        return nullptr;
    }

    // Create shader object from loaded file (we will use the object later when rendering)
    ID3D11VertexShader* shader;
    HRESULT hr = gD3DDevice->CreateVertexShader(byteCode.data(), byteCode.size(), nullptr, &shader);
    if (FAILED(hr))
    {
        return nullptr;
    }

    return shader;
}


// Load a pixel shader, include the file in the project and pass the name (without the .hlsl extension)
// to this function. The returned pointer needs to be released before quitting. Returns nullptr on failure. 
// Basically the same code as above but for pixel shaders
ID3D11PixelShader* LoadPixelShader(std::string shaderName)
{
    // Open compiled shader object file
    std::ifstream shaderFile(shaderName + ".cso", std::ios::in | std::ios::binary | std::ios::ate);
    if (!shaderFile.is_open())
    {
        return nullptr;
    }

    // Read file into vector of chars
    std::streamoff fileSize = shaderFile.tellg();
    shaderFile.seekg(0, std::ios::beg);
    std::vector<char>byteCode(fileSize);
    shaderFile.read(&byteCode[0], fileSize);
    if (shaderFile.fail())
    {
        return nullptr;
    }

    // Create shader object from loaded file (we will use the object later when rendering)
    ID3D11PixelShader* shader;
    HRESULT hr = gD3DDevice->CreatePixelShader(byteCode.data(), byteCode.size(), nullptr, &shader);
    if (FAILED(hr))
    {
        return nullptr;
    }

    return shader;
}

// Very advanced topic: When creating a vertex layout for geometry (see Scene.cpp), you need the signature
// (bytecode) of a shader that uses that vertex layout. This is an annoying requirement and tends to create
// unnecessary coupling between shaders and vertex buffers.
// This is a trick to simplify things - pass a vertex layout to this function and it will write and compile
// a temporary shader to match. You don't need to know about the actual shaders in use in the app.
// Release the signature (called a ID3DBlob!) after use. Returns nullptr on failure.
ID3DBlob* CreateSignatureForVertexLayout(const D3D11_INPUT_ELEMENT_DESC vertexLayout[], int numElements)
{
    std::string shaderSource = "float4 main(";
    for (int elt = 0; elt < numElements; ++elt)
    {
        auto& format = vertexLayout[elt].Format;
        // This list should be more complete for production use
        if      (format == DXGI_FORMAT_R32G32B32A32_FLOAT) shaderSource += "float4";
        else if (format == DXGI_FORMAT_R32G32B32_FLOAT)    shaderSource += "float3";
        else if (format == DXGI_FORMAT_R32G32_FLOAT)       shaderSource += "float2";
        else if (format == DXGI_FORMAT_R32_FLOAT)          shaderSource += "float";
        else return nullptr; // Unsupported type in layout

        uint8_t index = static_cast<uint8_t>(vertexLayout[elt].SemanticIndex);
        std::string semanticName = vertexLayout[elt].SemanticName;
        semanticName += ('0' + index);

        shaderSource += " ";
        shaderSource += semanticName;
        shaderSource += " : ";
        shaderSource += semanticName;
        if (elt != numElements - 1)  shaderSource += " , ";
    }
    shaderSource += ") : SV_Position {return 0;}";

    ID3DBlob* compiledShader;
    HRESULT hr = D3DCompile(shaderSource.c_str(), shaderSource.length(), NULL, NULL, NULL, "main",
        "vs_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL0, 0, &compiledShader, NULL);
    if (FAILED(hr))
    {
        return nullptr;
    }

    return compiledShader;
}


//--------------------------------------------------------------------------------------
// Constant buffer creation / destruction
//--------------------------------------------------------------------------------------

// Constant Buffers are a way of passing data from C++ to the GPU. They are called constants but that only means
// they are constant for the duration of a single GPU draw call. The "constants" correspond to variables in C++
// that we will change per-model, or per-frame etc.
//
// We typically set up a C++ structure to exactly match the values we need in a shader and then create a constant
// buffer the same size as the structure. That makes updating values from C++ to shader easy - see the main code.

// Create and return a constant buffer of the given size
// The returned pointer needs to be released before quitting. Returns nullptr on failure. 
ID3D11Buffer* CreateConstantBuffer(int size)
{
    D3D11_BUFFER_DESC cbDesc;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.ByteWidth = 16 * ((size + 15) / 16);     // Constant buffer size must be a multiple of 16 - this maths rounds up to the nearest multiple
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;             // Indicates that the buffer is frequently updated
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // CPU is only going to write to the constants (not read them)
    cbDesc.MiscFlags = 0;
    ID3D11Buffer* constantBuffer;
    HRESULT hr = gD3DDevice->CreateBuffer(&cbDesc, nullptr, &constantBuffer);
    if (FAILED(hr))
    {
        return nullptr;
    }

    return constantBuffer;
}


