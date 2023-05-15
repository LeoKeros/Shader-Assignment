#include "CTexture.h"



CTexture::CTexture()
{
}


CTexture::~CTexture()
{
}


ID3D11Resource** CTexture::GetSpecularMap()
{
	return &mSpecularMap;
}

ID3D11ShaderResourceView** CTexture::GetSpecularMapSRV()
{
	return &mSpecularMapSRV;
}

void CTexture::Release()
{
	if (mSpecularMapSRV)    mSpecularMapSRV->Release();
	if (mSpecularMap)       mSpecularMap->Release();
}