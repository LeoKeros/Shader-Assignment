#pragma once
#include "Common.h"
#include "GraphicsHelpers.h"
#include <string>

class CTexture
{
private:
	ID3D11Resource*          mSpecularMap = nullptr; // This object represents the memory used by the texture on the GPU
	ID3D11ShaderResourceView* mSpecularMapSRV = nullptr; // This object is used to give shaders access to the texture above (SRV = shader resource view)

public:
	CTexture();
	~CTexture();

	ID3D11Resource** GetSpecularMap();
	ID3D11ShaderResourceView** GetSpecularMapSRV();
	void Release();
};

