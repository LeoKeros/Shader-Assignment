#pragma once
#include "Model.h"
#include "CVector3.h"
#include "Camera.h"

class CPortal
{
private:
	Model* mpBody;
	Camera* mCamera;

	ID3D11Texture2D*          mTexture = nullptr; // This object represents the memory used by the texture on the GPU
	ID3D11ShaderResourceView* mTextureSRV = nullptr; // This object is used to give shaders access to the texture above (SRV = shader resource view)
	ID3D11RenderTargetView* mRenderTarget = nullptr; // This object is used when we want to render to the texture above

public:
	CPortal(Mesh* mesh, const CVector3 &startingPos = { 0,0,0 }, const CVector3 & startingRotation = { 0,0,0 });
	~CPortal();
	
	ID3D11Texture2D** GetPortalTexture();
	ID3D11ShaderResourceView** GetPortalTextureSRV();
	ID3D11RenderTargetView** GetPortalRenderTarget();
	
	Camera* GetCamera();
	void SetPosition(const CVector3& pos);
	void SetRotation(const CVector3& rotation);
	void SetCamPosition(const CVector3& pos);
	void SetCamRotation(const CVector3& rotation);
	bool CreateTexture(const D3D11_TEXTURE2D_DESC &portalDesc, const D3D11_SHADER_RESOURCE_VIEW_DESC &srDesc);
	void Release();
	void Render();
	void Control(float frameTime, KeyCode turnUp, KeyCode turnDown, KeyCode turnLeft, KeyCode turnRight,
				 KeyCode turnCW, KeyCode turnCCW, KeyCode moveForward, KeyCode moveBackward);


};

