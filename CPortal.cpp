#include "CPortal.h"



CPortal::CPortal(Mesh* mesh, const CVector3 & startingPos, const CVector3 &startingRotation)
{
	mpBody = new Model(mesh, startingPos, startingRotation);
	mCamera = new Camera(startingPos + (-5 * Normalise(mpBody->WorldMatrix().GetZAxis())),
						 startingRotation + CVector3{ ToRadians(20.0f), ToRadians(345.0f), 0 });
}


CPortal::~CPortal()
{
}

ID3D11Texture2D** CPortal::GetPortalTexture()
{
	return &mTexture;
}

ID3D11ShaderResourceView** CPortal::GetPortalTextureSRV()
{
	return &mTextureSRV;
}

ID3D11RenderTargetView** CPortal::GetPortalRenderTarget()
{
	return &mRenderTarget;
}

Camera* CPortal::GetCamera()
{
	return mCamera;
}

void CPortal::SetPosition(const CVector3& pos)
{
	mpBody->SetPosition(pos);
}

void CPortal::SetRotation(const CVector3& rotation)
{
	mpBody->SetRotation(rotation);
}

void CPortal::SetCamPosition(const CVector3& pos)
{
	mCamera->SetPosition(pos);
}

void CPortal::SetCamRotation(const CVector3& rotation)
{
	mCamera->SetRotation(rotation);
}


void CPortal::Release()
{
	if (mTexture) mTexture->Release();
	if (mTextureSRV) mTextureSRV->Release();
	if (mRenderTarget) mRenderTarget->Release();
	delete mCamera;
	mCamera = nullptr;
	delete mpBody;
	mpBody = nullptr;

}

void CPortal::Render()
{
	mpBody->Render();
}

void CPortal::Control(float frameTime, KeyCode turnUp, KeyCode turnDown, KeyCode turnLeft, KeyCode turnRight,
					  KeyCode turnCW, KeyCode turnCCW, KeyCode moveForward, KeyCode moveBackward)
{
	mpBody->Control(frameTime, turnUp, turnDown, turnLeft, turnRight, turnCW, turnCCW, moveForward, moveBackward);
}

bool CPortal::CreateTexture(const D3D11_TEXTURE2D_DESC &portalDesc, const D3D11_SHADER_RESOURCE_VIEW_DESC &srDesc)
{
	if (FAILED(gD3DDevice->CreateTexture2D(&portalDesc, NULL, &mTexture)))
	{
		gLastError = "Error creating portal texture";
		return false;
	}

	// We created the portal texture above, now we get a "view" of it as a render target, i.e. get a special pointer to the texture that
	// we use when rendering to it (see RenderScene function below)
	if (FAILED(gD3DDevice->CreateRenderTargetView(mTexture, NULL, &mRenderTarget)))
	{
		gLastError = "Error creating portal render target view";
		return false;
	}

	if (FAILED(gD3DDevice->CreateShaderResourceView(mTexture, &srDesc, &mTextureSRV)))
	{
		gLastError = "Error creating portal shader resource view";
		return false;
	}

	return true;
}