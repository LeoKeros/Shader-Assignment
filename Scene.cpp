//--------------------------------------------------------------------------------------
// Scene geometry and layout preparation
// Scene rendering & update
//--------------------------------------------------------------------------------------

#include "Scene.h"


// Constants controlling speed of movement/rotation (measured in units per second because we're using frame time)
const float ROTATION_SPEED = 2.0f;  // 2 radians per second for rotation
const float MOVEMENT_SPEED = 50.0f; // 50 units per second for movement (what a unit of length is depends on 3D model - i.e. an artist decision usually)

//--------------------------------------------------------------------------------------
//**** Shadow Texture  ****//
//--------------------------------------------------------------------------------------
// This texture will have the scene from the point of view of the light renderered on it. This texture is then used for shadow mapping

// The shadow texture - effectively a depth buffer of the scene **from the light's point of view**
//                      Each frame it is rendered to, then the texture is used to help the per-pixel lighting shader identify pixels in shadow
//*********************//



//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
// Variables sent over to the GPU each frame
// The structures are now in Common.h
// IMPORTANT: Any new data you add in C++ code (CPU-side) is not automatically available to the GPU
//            Anything the shaders need (per-frame or per-model) needs to be sent via a constant buffer

PerFrameConstants gPerFrameConstants;      // The constants that need to be sent to the GPU each frame (see common.h for structure)
ID3D11Buffer*     gPerFrameConstantBuffer; // The GPU buffer that will recieve the constants above

PerModelConstants gPerModelConstants;      // As above, but constant that change per-model (e.g. world matrix)
ID3D11Buffer*     gPerModelConstantBuffer; // --"--

//--------------------------------------------------------------------------------------
// Initialise scene geometry, constant buffers and states
//--------------------------------------------------------------------------------------

// Prepare the geometry required for the scene
// Returns true on success
bool CSceneManager::InitGeometry()
{
    // Load mesh geometry data, just like TL-Engine this doesn't create anything in the scene. Create a Model for that.
    // IMPORTANT NOTE: Will only keep the first object from the mesh - multipart objects will have parts missing - see later lab for more robust loader
    try 
    {
		mMeshArray[Mesh_Teapot]	= new Mesh("Teapot.x");
        mMeshArray[Mesh_Troll]	= new Mesh("Troll.x");
		mMeshArray[Mesh_Crate]	= new Mesh("CargoContainer.x");
		mMeshArray[Mesh_Ground]	= new Mesh("Hills.x");
		mMeshArray[Mesh_Light]	= new Mesh("Light.x");
		mMeshArray[Mesh_Portal]	= new Mesh("Portal.x");
		mMeshArray[Mesh_Sphere]	= new Mesh("Sphere.x");
		mMeshArray[Mesh_SphereTangent] = new Mesh("Sphere.x", true);
		mMeshArray[Mesh_Cube]	= new Mesh("Cube.x");
		mMeshArray[Mesh_CubeTangent] = new Mesh("Cube.x", true);
		mMeshArray[Mesh_Decal]	= new Mesh("Decal.x");
    }
    catch (std::runtime_error e)  // Constructors cannot return error messages so use exceptions to catch mesh errors (fairly standard approach this)
    {
        gLastError = e.what(); // This picks up the error message put in the exception (see Mesh.cpp)
        return false;
    }


    // Load the shaders required for the geometry we will use (see Shader.cpp / .h)
    if (!LoadShaders())
    {
        gLastError = "Error loading shaders";
        return false;
    }


    // Create GPU-side constant buffers to receive the gPerFrameConstants and gPerModelConstants structures above
    // These allow us to pass data from CPU to shaders such as lighting information or matrices
    // See the comments above where these variable are declared and also the UpdateScene function
    gPerFrameConstantBuffer = CreateConstantBuffer(sizeof(gPerFrameConstants));
    gPerModelConstantBuffer = CreateConstantBuffer(sizeof(gPerModelConstants));
    if (gPerFrameConstantBuffer == nullptr || gPerModelConstantBuffer == nullptr)
    {
        gLastError = "Error creating constant buffers";
        return false;
    }


    //// Load / prepare textures on the GPU ////

    // Load textures and create DirectX objects for them
    // The LoadTexture function requires you to pass a ID3D11Resource* (e.g. &gCubeDiffuseMap), which manages the GPU memory for the
    // texture and also a ID3D11ShaderResourceView* (e.g. &gCubeDiffuseMapSRV), which allows us to use the texture in shaders
    // The function will fill in these pointers with usable data. The variables used here are globals found near the top of the file.
    if (!LoadTexture("TrollDiffuseSpecular.dds",	mTextures[TrollTexture].GetSpecularMap(),	mTextures[TrollTexture].GetSpecularMapSRV())	||
		!LoadTexture("StoneDiffuseSpecular.dds",	mTextures[StoneTexture].GetSpecularMap(),	mTextures[StoneTexture].GetSpecularMapSRV())	||
		!LoadTexture("brick1.jpg",					mTextures[BrickTexture].GetSpecularMap(),	mTextures[BrickTexture].GetSpecularMapSRV())	||
		!LoadTexture("Moogle.png",					mTextures[MoogleTexture].GetSpecularMap(),	mTextures[MoogleTexture].GetSpecularMapSRV())	||
        !LoadTexture("CargoA.dds",					mTextures[CargoTexture].GetSpecularMap(),	mTextures[CargoTexture].GetSpecularMapSRV())	||
		!LoadTexture("WoodDiffuseSpecular.dds",		mTextures[WoodTexture].GetSpecularMap(),	mTextures[WoodTexture].GetSpecularMapSRV())		||
		!LoadTexture("WoodNormal.dds",				mTextures[WoodNormal].GetSpecularMap(),		mTextures[WoodNormal].GetSpecularMapSRV())		||
		!LoadTexture("GrassDiffuseSpecular.dds",	mTextures[GrassTexture].GetSpecularMap(),	mTextures[GrassTexture].GetSpecularMapSRV())	||
        !LoadTexture("MetalDiffuseSpecular.dds",	mTextures[MetalTexture].GetSpecularMap(),	mTextures[MetalTexture].GetSpecularMapSRV())	||
		!LoadTexture("MetalNormal.dds",				mTextures[MetalNormal].GetSpecularMap(),	mTextures[MetalNormal].GetSpecularMapSRV())		||
		!LoadTexture("PatternDiffuseSpecular.dds",	mTextures[PatternTexture].GetSpecularMap(),	mTextures[PatternTexture].GetSpecularMapSRV())	||
		!LoadTexture("PatternNormalHeight.dds",		mTextures[PatternNormalH].GetSpecularMap(),	mTextures[PatternNormalH].GetSpecularMapSRV())	||
		!LoadTexture("BrainDiffuseSpecular.dds",	mTextures[BrainTexture].GetSpecularMap(),	mTextures[BrainTexture].GetSpecularMapSRV())	||
		!LoadTexture("BrainNormalHeight.dds",		mTextures[BrainNormalH].GetSpecularMap(),	mTextures[BrainNormalH].GetSpecularMapSRV())	||
		!LoadTexture("CobbleDiffuseSpecular.dds",	mTextures[CobbleTexture].GetSpecularMap(),	mTextures[CobbleTexture].GetSpecularMapSRV())	||
		!LoadTexture("CobbleNormalHeight.dds",		mTextures[CobbleNormalH].GetSpecularMap(),	mTextures[CobbleNormalH].GetSpecularMapSRV())	||
		!LoadTexture("TechDiffuseSpecular.dds",		mTextures[TechTexture].GetSpecularMap(),	mTextures[TechTexture].GetSpecularMapSRV())		||
		!LoadTexture("TechNormalHeight.dds",		mTextures[TechNormalH].GetSpecularMap(),	mTextures[TechNormalH].GetSpecularMapSRV())		||
		!LoadTexture("WallDiffuseSpecular.dds",		mTextures[WallTexture].GetSpecularMap(),	mTextures[WallTexture].GetSpecularMapSRV())		||
		!LoadTexture("WallNormalHeight.dds",		mTextures[WallNormalH].GetSpecularMap(),	mTextures[WallNormalH].GetSpecularMapSRV())		||
		!LoadTexture("tv.dds",						mTextures[TVTexture].GetSpecularMap(),		mTextures[TVTexture].GetSpecularMapSRV())		||
		!LoadTexture("Flare.jpg",					mTextures[FlareTexture].GetSpecularMap(),	mTextures[FlareTexture].GetSpecularMapSRV())	||
		!LoadTexture("glass.jpg",					mTextures[GlassTexture].GetSpecularMap(),	mTextures[GlassTexture].GetSpecularMapSRV()))
    {
        gLastError = "Error loading textures";
        return false;
    }



	//**** Create Shadow Map texture ****//

	// We also need a depth buffer to go with our portal
	mShadowMapTextureDesc.Width  = mShadowMapSize; // Size of the shadow map determines quality / resolution of shadows
	mShadowMapTextureDesc.Height = mShadowMapSize;
	mShadowMapTextureDesc.MipLevels = 1; // 1 level, means just the main texture, no additional mip-maps. Usually don't use mip-maps when rendering to textures (or we would have to render every level)
	mShadowMapTextureDesc.ArraySize = 1;
	mShadowMapTextureDesc.Format = DXGI_FORMAT_R32_TYPELESS; // The shadow map contains a single 32-bit value [tech gotcha: have to say typeless because depth buffer and shaders see things slightly differently]
	mShadowMapTextureDesc.SampleDesc.Count = 1;
	mShadowMapTextureDesc.SampleDesc.Quality = 0;
	mShadowMapTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	mShadowMapTextureDesc.BindFlags = D3D10_BIND_DEPTH_STENCIL | D3D10_BIND_SHADER_RESOURCE; // Indicate we will use texture as a depth buffer and also pass it to shaders
	mShadowMapTextureDesc.CPUAccessFlags = 0;
	mShadowMapTextureDesc.MiscFlags = 0;

	// Create the depth stencil view, i.e. indicate that the texture just created is to be used as a depth buffer
	mShadowMapDsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // See "tech gotcha" above. The depth buffer sees each pixel as a "depth" float
	mShadowMapDsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	mShadowMapDsvDesc.Texture2D.MipSlice = 0;
    mShadowMapDsvDesc.Flags = 0;

	// We also need to send this texture (resource) to the shaders. To do that we must create a shader-resource "view"
	mShadowMapSrvDesc.Format = DXGI_FORMAT_R32_FLOAT; // See "tech gotcha" above. The shaders see textures as colours, so shadow map pixels are not seen as depths
                                           // but rather as "red" floats (one float taken from RGB). Although the shader code will use the value as a depth
	mShadowMapSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	mShadowMapSrvDesc.Texture2D.MostDetailedMip = 0;
	mShadowMapSrvDesc.Texture2D.MipLevels = 1;
	for (unsigned short int i = 0; i < gsNumSpotlights; ++i)
	{
		if (FAILED(gD3DDevice->CreateTexture2D(&mShadowMapTextureDesc, NULL, &mShadowMapSpotlightTexture[i])))
		{
			gLastError = "Error creating shadow map texture";
			return false;
		}
		if (FAILED(gD3DDevice->CreateDepthStencilView(mShadowMapSpotlightTexture[i], &mShadowMapDsvDesc, &mShadowMapSpotlightDepthStencil[i])))
		{
			gLastError = "Error creating shadow map depth stencil view";
			return false;
		}
		if (FAILED(gD3DDevice->CreateShaderResourceView(mShadowMapSpotlightTexture[i], &mShadowMapSrvDesc, &mShadowMapSpotlightSRV[i])))
		{
			gLastError = "Error creating shadow map shader resource view";
			return false;
		}
	}
   //*****************************//


	//**** Create Portal Texture Description ****//

	// Using a helper function to load textures from files above. Here we create the portal texture manually
	// as we are creating a special kind of texture (one that we can render to). Many settings to prepare:
	mPortalDesc.Width = mPortalWidth;  // Size of the portal texture determines its quality
	mPortalDesc.Height = mPortalHeight;
	mPortalDesc.MipLevels = 1; // No mip-maps when rendering to textures (or we would have to render every level)
	mPortalDesc.ArraySize = 1;
	mPortalDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA texture (8-bits each)
	mPortalDesc.SampleDesc.Count = 1;
	mPortalDesc.SampleDesc.Quality = 0;
	mPortalDesc.Usage = D3D11_USAGE_DEFAULT;
	mPortalDesc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE; // IMPORTANT: Indicate we will use texture as render target, and pass it to shaders
	mPortalDesc.CPUAccessFlags = 0;
	mPortalDesc.MiscFlags = 0;

	// We also need to send this texture (resource) to the shaders. To do that we must create a shader-resource "view"
	mPortalSRDesc.Format = mPortalDesc.Format;
	mPortalSRDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	mPortalSRDesc.Texture2D.MostDetailedMip = 0;
	mPortalSRDesc.Texture2D.MipLevels = 1;
	
	//**** Create Portal Depth Buffer ****//

	// We also need a depth buffer to go with our portal
	//**** This depth buffer can be shared with any other portals of the same size
	D3D11_TEXTURE2D_DESC dbPortalDesc = {};
	dbPortalDesc.Width = mPortalWidth;
	dbPortalDesc.Height = mPortalHeight;
	dbPortalDesc.MipLevels = 1;
	dbPortalDesc.ArraySize = 1;
	dbPortalDesc.Format = DXGI_FORMAT_D32_FLOAT; // Depth buffers contain a single float per pixel
	dbPortalDesc.SampleDesc.Count = 1;
	dbPortalDesc.SampleDesc.Quality = 0;
	dbPortalDesc.Usage = D3D11_USAGE_DEFAULT;
	dbPortalDesc.BindFlags = D3D10_BIND_DEPTH_STENCIL;
	dbPortalDesc.CPUAccessFlags = 0;
	dbPortalDesc.MiscFlags = 0;
	if (FAILED(gD3DDevice->CreateTexture2D(&dbPortalDesc, NULL, &mPortalDepthStencil)))
	{
		gLastError = "Error creating portal depth stencil texture";
		return false;
	}

	// Create the depth stencil view, i.e. indicate that the texture just created is to be used as a depth buffer
	D3D11_DEPTH_STENCIL_VIEW_DESC portalDescDSV = {};
	portalDescDSV.Format = dbPortalDesc.Format;
	portalDescDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	portalDescDSV.Texture2D.MipSlice = 0;
	portalDescDSV.Flags = 0;
	if (FAILED(gD3DDevice->CreateDepthStencilView(mPortalDepthStencil, &portalDescDSV, &mPortalDepthStencilView)))
	{
		gLastError = "Error creating portal depth stencil view";
		return false;
	}



  	// Create all filtering modes, blending modes etc. used by the app (see State.cpp/.h)
	if (!CreateStates())
	{
		gLastError = "Error creating states";
		return false;
	}

	return true;
}


// Prepare the scene
// Returns true on success
bool CSceneManager::InitScene()
{
	//// Set up scene ////
	NewModel(Mesh_Teapot, { StoneTexture }, { 15, 0, 0 }, 1, { 0, ToRadians(215.0f), 0 });
	NewModel(Mesh_Crate, { CargoTexture }, { 40, 0, 30 }, 6, { 0.0f, ToRadians(-20.0f), 0.0f });
	NewModel(Mesh_Ground, { GrassTexture }, { -20, 0, -20 });
	NewModel(Mesh_Sphere, { WoodTexture, WoodNormal }, { -20, 12, 20 }, 1, { 0,0,0 }, 6, ps_Wiggle);
	NewModel(Mesh_SphereTangent, { PatternTexture, PatternNormalH }, { -10, 12, -10 }, 1, { 0,0,0 }, 3, ps_WiggleParallax);
	NewModel(Mesh_Cube, { BrickTexture, WoodTexture }, { 40, 5.5f, -30 }, 1, { 0, 0, 0 }, 1, ps_Fade);
	NewModel(Mesh_CubeTangent, { TechTexture, TechNormalH }, { 40, 5.5f, -10 }, 1, { 0,ToRadians(45.0f),0 }, 1, ps_ParallaxMap);
	NewModel(Mesh_CubeTangent, { PatternTexture, PatternNormalH }, { 40, 20.0f, -10 }, 1, { 0,ToRadians(45.0f),0 }, 1, ps_NormalMap);
	NewModel(Mesh_Cube, { GlassTexture }, { 5, 10, 30 }, 1, { 0, ToRadians(180), 0 }, 0, ps_Transparent);

    // Light creation
	NewLight(ELightType::spotlight, mMeshArray[Mesh_Light], { 0.8f, 0.8f, 1.0f }, { 30, 20, 0 },
		10, mModelCollection[ps_PixelLighting].front()->Position()); //The teapot is created first.
	NewLight(ELightType::point, mMeshArray[Mesh_Light], { 1.0f, 0.8f, 0.2f }, { -5, 30, -20 }, 50, { 0,0,0 });

	NewPortal({ 10, 15, 50 }, { 0, ToRadians(180), 0 });

    //// Set up camera ////

    mCamera = new Camera();
    mCamera->SetPosition({ 15, 30,-70 });
    mCamera->SetRotation({ ToRadians(13), 0, 0 });

    return true;
}


// Release the geometry and scene resources created above
void CSceneManager::ReleaseResources()
{
    ReleaseStates();
	
	for (unsigned short int i = 0; i < mLightStackTop.y; ++i)
	{
		if (mShadowMapSpotlightDepthStencil[i])  mShadowMapSpotlightDepthStencil[i]->Release();
		if (mShadowMapSpotlightSRV[i])           mShadowMapSpotlightSRV[i]->Release();
		if (mShadowMapSpotlightTexture[i])       mShadowMapSpotlightTexture[i]->Release();
	}
	for (auto &texture : mTextures)
	{
		texture.Release();
	}

    if (gPerModelConstantBuffer)  gPerModelConstantBuffer->Release();
    if (gPerFrameConstantBuffer)  gPerFrameConstantBuffer->Release();

    ReleaseShaders();

    // See note in InitGeometry about why we're not using unique_ptr and having to manually delete
    for (unsigned short int i = 0; i < mLightStackTop.y; ++i)
	{
		mSpotlights[i]->Release();
		delete mSpotlights[i];
		mSpotlights[i] = nullptr;
	}
	for (unsigned short int i = 0; i < mLightStackTop.x; ++i)
	{
		mPointLights[i]->Release();
		delete mPointLights[i];
		mPointLights[i] = nullptr;
	}
	for (unsigned short int i = 0; i < mLightStackTop.z; ++i)
	{
		mDirectionalLights[i]->Release();
		delete mDirectionalLights[i];
		mDirectionalLights[i] = nullptr;
	}

    delete mCamera;    mCamera    = nullptr;

	for (auto &group : mModelCollection)
	{
		for (auto &model : group)
		{
			delete model; model = nullptr;
		}
	}

	for (auto &shader : mTeapotCollection)
	{
		for (auto &model : shader)
		{
			delete model; model = nullptr;
		}
	}

	for (auto &model : mTransparentModels)
	{
		delete model; model = nullptr;
	}

	for (auto &mesh : mMeshArray)
	{
		delete mesh;     mesh = nullptr;
	}
}



//--------------------------------------------------------------------------------------
// Scene Rendering
//--------------------------------------------------------------------------------------

// Render the scene from the given light's point of view. Only renders depth buffer
void CSceneManager::RenderDepthBufferFromLight(const CSpotlight &light)
{
    // Get camera-like matrices from the spotlight, seet in the constant buffer and send over to GPU
    gPerFrameConstants.viewMatrix           = light.CalculateViewMatrix();
    gPerFrameConstants.projectionMatrix     = light.CalculateProjectionMatrix();
    gPerFrameConstants.viewProjectionMatrix = gPerFrameConstants.viewMatrix * gPerFrameConstants.projectionMatrix;
    UpdateConstantBuffer(gPerFrameConstantBuffer, gPerFrameConstants);

    // Indicate that the constant buffer we just updated is for use in the vertex shader (VS) and pixel shader (PS)
    gD3DContext->VSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer); // First parameter must match constant buffer number in the shader 
    gD3DContext->PSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer);


    //// Only render models that cast shadows ////

    // Use special depth-only rendering shaders
    gD3DContext->VSSetShader(mVertexShaders[vs_BasicTransform], nullptr, 0);
    gD3DContext->PSSetShader(mPixelShaders[ps_DepthOnly],       nullptr, 0);
    
    // States - no blending, normal depth buffer and culling
    gD3DContext->OMSetBlendState(gNoBlendingState, nullptr, 0xffffff);
    gD3DContext->OMSetDepthStencilState(gUseDepthBufferState, 0);
    gD3DContext->RSSetState(gCullBackState);

	// Render models - no state changes required between each object in this situation (no textures used in this step)
	for(auto &pixelShader : mModelCollection)
	{
		for (auto& model : pixelShader)
		{
			model->Render();
		}
	}
	for (auto& portal : mPortalCollection)
	{
		portal->Render();
	}

	gD3DContext->RSSetState(gCullNoneState);
	for (auto &pixelShader : mTeapotCollection)
	{
		for (auto& model : pixelShader)
		{
			model->Render();
		}
	}

	gD3DContext->OMSetBlendState(gMultiplicativeBlending, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(gDepthReadOnlyState, 0);
	for (auto &model : mTransparentModels)
	{
		model->Render();
	}

}



// Render everything in the scene from the given camera
// This code is common between rendering the main scene and rendering the scene in the portal
// See RenderScene function below
void CSceneManager::RenderSceneFromCamera(Camera* camera)
{
    // Set camera matrices in the constant buffer and send over to GPU
    gPerFrameConstants.viewMatrix           = camera->ViewMatrix();
    gPerFrameConstants.projectionMatrix     = camera->ProjectionMatrix();
    gPerFrameConstants.viewProjectionMatrix = camera->ViewProjectionMatrix();
    UpdateConstantBuffer(gPerFrameConstantBuffer, gPerFrameConstants);

    // Indicate that the constant buffer we just updated is for use in the vertex shader (VS) and pixel shader (PS)
    gD3DContext->VSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer); // First parameter must match constant buffer number in the shader 
    gD3DContext->PSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer);


    //// Render lit models ////
	    
    // States - no blending, normal depth buffer and culling
    gD3DContext->OMSetBlendState(gNoBlendingState, nullptr, 0xffffff);
    gD3DContext->OMSetDepthStencilState(gUseDepthBufferState, 0);

    // Select the approriate textures and sampler to use in the pixel shader
    gD3DContext->PSSetSamplers(0, 1, &gAnisotropic4xSampler);
	gD3DContext->RSSetState(gCullNoneState);

	// Render model - it will update the model's world matrix and send it to the GPU in a constant buffer, then it will call
   // the Mesh render function, which will set up vertex & index buffer before finally calling Draw on the GPU
	for (int i = 0; i < gsNumOfModelPS; ++i)
	{
		bool secondTexture = false;
		// Select which shaders to use next
		switch (i)
		{
		case ps_Wiggle:
			gD3DContext->VSSetShader(mVertexShaders[vs_Wiggle], nullptr, 0);
			gD3DContext->PSSetShader(mPixelShaders[ps_Wiggle], nullptr, 0);
			break;
		case ps_NormalMap:
			gD3DContext->VSSetShader(mVertexShaders[vs_NormalMap], nullptr, 0);
			gD3DContext->PSSetShader(mPixelShaders[ps_NormalMap], nullptr, 0);
			secondTexture = true;
			break;
		case ps_ParallaxMap:
			//gD3DContext->VSSetShader(mVertexShaders[vs_NormalMap], nullptr, 0);
			gD3DContext->PSSetShader(mPixelShaders[ps_ParallaxMap], nullptr, 0);
			secondTexture = true;
			break;
		case ps_PixelLighting:
			gD3DContext->VSSetShader(mVertexShaders[vs_PixelLighting], nullptr, 0);
			gD3DContext->PSSetShader(mPixelShaders[ps_PixelLighting], nullptr, 0);
			break;
		case ps_Fade:
			//gD3DContext->VSSetShader(mVertexShaders[vs_PixelLighting], nullptr, 0); //Already set
			gD3DContext->PSSetShader(mPixelShaders[ps_Fade], nullptr, 0);
			secondTexture = true;
			break;
		case ps_WiggleParallax:
			gD3DContext->VSSetShader(mVertexShaders[vs_WiggleTangent], nullptr, 0);
			gD3DContext->PSSetShader(mPixelShaders[ps_ParallaxMap], nullptr, 0);
			break;
		}
		if (!mTeapotCollection[i].empty())
		{
			for (auto &model : mTeapotCollection[i])
			{
				gD3DContext->PSSetShaderResources(0, 1, model->GetTexture()->GetSpecularMapSRV()); // First parameter must match texture slot number in the shader
				if (secondTexture) gD3DContext->PSSetShaderResources(gsNumSpotlights + 1, 1, model->GetTexture(1)->GetSpecularMapSRV());
				model->Render();
			}
		}
	}

    gD3DContext->RSSetState(gCullBackState);
    // Render model - it will update the model's world matrix and send it to the GPU in a constant buffer, then it will call
    // the Mesh render function, which will set up vertex & index buffer before finally calling Draw on the GPU
	for (int i = 0; i < gsNumOfModelPS; ++i)
	{
		bool secondTexture = false;
		// Select which shaders to use next
		switch (i)
		{
		case ps_Wiggle:
			gD3DContext->VSSetShader(mVertexShaders[vs_Wiggle], nullptr, 0);
			gD3DContext->PSSetShader(mPixelShaders[ps_Wiggle], nullptr, 0);
			break;
		case ps_NormalMap:
			gD3DContext->VSSetShader(mVertexShaders[vs_NormalMap], nullptr, 0);
			gD3DContext->PSSetShader(mPixelShaders[ps_NormalMap], nullptr, 0);
			secondTexture = true;
			break;
		case ps_ParallaxMap:
			//gD3DContext->VSSetShader(mVertexShaders[vs_NormalMap], nullptr, 0);
			gD3DContext->PSSetShader(mPixelShaders[ps_ParallaxMap], nullptr, 0);
			secondTexture = true;
			break;
		case ps_PixelLighting:
			gD3DContext->VSSetShader(mVertexShaders[vs_PixelLighting], nullptr, 0);
			gD3DContext->PSSetShader(mPixelShaders[ps_PixelLighting], nullptr, 0);
			break;
		case ps_Fade:
			//gD3DContext->VSSetShader(mVertexShaders[vs_PixelLighting], nullptr, 0); //Already set
			gD3DContext->PSSetShader(mPixelShaders[ps_Fade], nullptr, 0);
			secondTexture = true;
			break;
		case ps_WiggleParallax:
			gD3DContext->VSSetShader(mVertexShaders[vs_WiggleTangent], nullptr, 0);
			gD3DContext->PSSetShader(mPixelShaders[ps_ParallaxMap], nullptr, 0);
			secondTexture = true;
			break;
		}
		if (!mModelCollection[i].empty())
		{
			for (auto &model : mModelCollection[i])
			{
				gD3DContext->PSSetShaderResources(0, 1, model->GetTexture()->GetSpecularMapSRV()); // First parameter must match texture slot number in the shader
				if(secondTexture) gD3DContext->PSSetShaderResources(gsNumSpotlights + 1, 1, model->GetTexture(1)->GetSpecularMapSRV());
				model->Render();
			}
		}
	}
	
	//// Render Portals ////
	gD3DContext->PSSetShader(mPixelShaders[ps_Portal], nullptr, 0);
	gD3DContext->PSSetShaderResources(0, 1, mTextures[TVTexture].GetSpecularMapSRV());

	for (auto &portal : mPortalCollection)
	{
		gD3DContext->PSSetShaderResources(gsNumSpotlights + 1, 1, portal->GetPortalTextureSRV());
	
		portal->Render();
	}

    //// Render lights ////

    // Select which shaders to use next
    gD3DContext->VSSetShader(mVertexShaders[vs_BasicTransform], nullptr, 0);
    gD3DContext->PSSetShader(mPixelShaders[ps_LightModel],      nullptr, 0);

    // Select the texture and sampler to use in the pixel shader
    gD3DContext->PSSetShaderResources(0, 1, mTextures[FlareTexture].GetSpecularMapSRV()); // First parameter must match texture slot number in the shaer
    gD3DContext->PSSetSamplers(0, 1, &gAnisotropic4xSampler);

    // States - additive blending, read-only depth buffer and no culling (standard set-up for blending
    gD3DContext->OMSetBlendState(gAdditiveBlendingState, nullptr, 0xffffff);
    gD3DContext->OMSetDepthStencilState(gDepthReadOnlyState, 0);
    gD3DContext->RSSetState(gCullNoneState);

    // Render all the lights in the array
	for (unsigned short int i = 0; i < mLightStackTop.x; ++i)
	{
		gPerModelConstants.objectColour = mPointLights[i]->GetColour(); // Set any per-model constants apart from the world matrix just before calling render (light colour here)
		mPointLights[i]->Render();
	}
    for (unsigned short int i = 0; i < mLightStackTop.y; ++i)
    {
        gPerModelConstants.objectColour = mSpotlights[i]->GetColour(); // Set any per-model constants apart from the world matrix just before calling render (light colour here)
		mSpotlights[i]->Render();
    }
	for (unsigned short int i = 0; i < mLightStackTop.z; ++i)
	{
		gPerModelConstants.objectColour = mDirectionalLights[i]->GetColour(); // Set any per-model constants apart from the world matrix just before calling render (light colour here)
		mDirectionalLights[i]->Render();
	}

	gD3DContext->VSSetShader(mVertexShaders[vs_BasicTransform], nullptr, 0);
	gD3DContext->PSSetShader(mPixelShaders[ps_Transparent], nullptr, 0);
	gD3DContext->PSSetSamplers(0, 1, &gTrilinearSampler);
	gD3DContext->OMSetBlendState(gMultiplicativeBlending, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(gDepthReadOnlyState, 0);
	gD3DContext->RSSetState(gCullNoneState);
	for (auto &model : mTransparentModels)
	{
		gD3DContext->PSSetShaderResources(0, 1, model->GetTexture()->GetSpecularMapSRV()); // First parameter must match texture slot number in the shader
		model->Render();
	}

}

// Rendering the scene now renders everything twice. First it renders the scene for the portal into a texture.
// Then it renders the main scene using the portal texture on a model.
void CSceneManager::RenderScene()
{
    //// Common settings ////

    // Set up the light information in the constant buffer
    // Don't send to the GPU yet, the function RenderSceneFromCamera will do that
	for (unsigned short int i = 0; i < mLightStackTop.x; ++i)
	{
		gPerFrameConstants.pointLights[i].colour = mPointLights[i]->GetColour() * mPointLights[i]->GetStrength();
		gPerFrameConstants.pointLights[i].position = mPointLights[i]->GetPosition();
	}
	for (unsigned short int i = 0; i < mLightStackTop.y; ++i)
	{
		gPerFrameConstants.spotlights[i].colour = mSpotlights[i]->GetColour() * mSpotlights[i]->GetStrength();
		gPerFrameConstants.spotlights[i].position = mSpotlights[i]->GetPosition();
		gPerFrameConstants.spotlights[i].facing = mSpotlights[i]->GetFacing();    // Additional lighting information for spotlights
		gPerFrameConstants.spotlights[i].cosHalfAngle = mSpotlights[i]->GetCosHalfAngle(); // --"--
		gPerFrameConstants.spotlights[i].viewMatrix = mSpotlights[i]->CalculateViewMatrix();         // Calculate camera-like matrices for...
		gPerFrameConstants.spotlights[i].projectionMatrix = mSpotlights[i]->CalculateProjectionMatrix();   //...lights to support shadow mapping
	}
	gPerFrameConstants.lightStackTops = mLightStackTop;

    gPerFrameConstants.ambientColour  = mAmbientColour;
    gPerFrameConstants.specularPower  = mSpecularPower;
    gPerFrameConstants.cameraPosition = mCamera->Position();

	//***************************************//
    //// Render from light's point of view ////
    
    // Only rendering from light 1 to begin with

    // Setup the viewport to the size of the shadow map texture
    D3D11_VIEWPORT vp;
    vp.Width  = static_cast<FLOAT>(mShadowMapSize);
    vp.Height = static_cast<FLOAT>(mShadowMapSize);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    gD3DContext->RSSetViewports(1, &vp);

	for (unsigned short int i = 0; i < mLightStackTop.y; ++i)
	{
		// Select the shadow map texture as the current depth buffer. We will not be rendering any pixel colours
		// Also clear the the shadow map depth buffer to the far distance
		gD3DContext->OMSetRenderTargets(0, nullptr, mShadowMapSpotlightDepthStencil[i]);
		gD3DContext->ClearDepthStencilView(mShadowMapSpotlightDepthStencil[i], D3D11_CLEAR_DEPTH, 1.0f, 0);

		// Render the scene from the point of view of light 1 (only depth values written)
		RenderDepthBufferFromLight(*mSpotlights[i]);
	}

	//// Portal Scene Rendering ////
	// Set the portal texture and portal depth buffer as the targets for rendering
	// The portal texture will later be used on models in the main scene
	// Setup the viewport for the portal texture size
	vp.Width = static_cast<FLOAT>(mPortalWidth);
	vp.Height = static_cast<FLOAT>(mPortalHeight);
	gD3DContext->RSSetViewports(1, &vp);
	gD3DContext->PSSetSamplers(1, 1, &gPointSampler);
	for (auto &portal : mPortalCollection)
	{
		gD3DContext->OMSetRenderTargets(1, portal->GetPortalRenderTarget(), mPortalDepthStencilView);

		gD3DContext->PSSetShaderResources(1, gsNumSpotlights, mShadowMapSpotlightSRV); //Putting this line here allows shadows to work in portals.

		// Clear the portal texture to a fixed colour and the portal depth buffer to the far distance
		gD3DContext->ClearRenderTargetView(*portal->GetPortalRenderTarget(), &mBackgroundColor.r);
		gD3DContext->ClearDepthStencilView(mPortalDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

		// Render the scene for the portal
		RenderSceneFromCamera(portal->GetCamera());
	}

    //**************************//

	// Set shadow maps in shaders
	// First parameter is the "slot", must match the Texture2D declaration in the HLSL code
	// In this app the diffuse map uses slot 0, the shadow maps use slots 1 onwards. If we were using other maps (e.g. normal map) then
	// we might arrange things differently

    //// Main scene rendering ////

    // Set the back buffer as the target for rendering and select the main depth buffer.
    // When finished the back buffer is sent to the "front buffer" - which is the monitor.
    gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, gDepthStencil);

    // Clear the back buffer to a fixed colour and the depth buffer to the far distance
    gD3DContext->ClearRenderTargetView(gBackBufferRenderTarget, &mBackgroundColor.r);
    gD3DContext->ClearDepthStencilView(gDepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // Setup the viewport to the size of the main window
    vp.Width  = static_cast<FLOAT>(gViewportWidth);
    vp.Height = static_cast<FLOAT>(gViewportHeight);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    gD3DContext->RSSetViewports(1, &vp);


    // Render the scene for the main window
    RenderSceneFromCamera(mCamera);

    // Unbind shadow maps from shaders - prevents warnings from DirectX when we try to render to the shadow maps again next frame
    std::vector<ID3D11ShaderResourceView*> nullView;
	nullView.resize(gsNumSpotlights, nullptr);
	gD3DContext->PSSetShaderResources(1, gsNumSpotlights, nullView.data());


    //*****************************//
    // Temporary demonstration code for visualising the light's view of the scene
    //ColourRGBA white = {1,1,1};
    //gD3DContext->ClearRenderTargetView(gBackBufferRenderTarget, &white.r);
    //RenderDepthBufferFromLight(0);
    //*****************************//


    //// Scene completion ////

    // When drawing to the off-screen back buffer is complete, we "present" the image to the front buffer (the screen)
    gSwapChain->Present(0, 0);
}


//--------------------------------------------------------------------------------------
// Scene Update
//--------------------------------------------------------------------------------------

// Update models and camera. frameTime is the time passed since the last frame
void CSceneManager::UpdateScene(float frameTime)
{
	// Control sphere (will update its world matrix)
	mTeapotCollection[ps_PixelLighting].front()->Control(frameTime, Key_I, Key_K, Key_J, Key_L, Key_U, Key_O, Key_Period, Key_Comma );

    // Orbit the light - a bit of a cheat with the static variable [ask the tutor if you want to know what this is]
	static float rotate = 0.0f;
    static bool go = true;
	mSpotlights[0]->SetPosition(mTeapotCollection[ps_PixelLighting].front()->Position() + CVector3{ cos(rotate) * gLightOrbit, 10, sin(rotate) * gLightOrbit } );
	mSpotlights[0]->FaceTarget(mTeapotCollection[ps_PixelLighting].front()->Position());
    if (go)  rotate -= gLightOrbitSpeed * frameTime;
    if (KeyHit(Key_1))  go = !go;

	// Control camera (will update its view matrix)
	mCamera->Control(frameTime, Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D );


    // Show frame time / FPS in the window title //
    const float fpsUpdateTime = 0.5f; // How long between updates (in seconds)
    static float totalFrameTime = 0;
    static int frameCount = 0;
    totalFrameTime += frameTime;

	gPerFrameConstants.wiggle += frameTime;
    ++frameCount;
    if (totalFrameTime > fpsUpdateTime)
    {
        // Displays FPS rounded to nearest int, and frame time (more useful for developers) in milliseconds to 2 decimal places
        float avgFrameTime = totalFrameTime / frameCount;
        std::ostringstream frameTimeMs;
        frameTimeMs.precision(2);
        frameTimeMs << std::fixed << avgFrameTime * 1000;
        std::string windowTitle = "CO2409 Week 20: Shadow Mapping - Frame Time: " + frameTimeMs.str() +
                                  "ms, FPS: " + std::to_string(static_cast<int>(1 / avgFrameTime + 0.5f));
        SetWindowTextA(gHWnd, windowTitle.c_str());
        totalFrameTime = 0;
        frameCount = 0;
    }
}

void CSceneManager::NewModel(EMeshType meshIndex, std::vector<ETextureType> textureIndexes, CVector3 position, float scale, CVector3 rotation, float wiggleStrength, EPixelShaders shaderType)
{
	std::vector<CTexture*> textures;
	for (auto &index : textureIndexes)
	{
		textures.push_back(&mTextures[index]);
	}
	if (shaderType == ps_Transparent)
	{
		mTransparentModels.push_back(new Model(mMeshArray[meshIndex], textures, position, rotation, scale));
	}
	if (shaderType < gsNumOfModelPS)
	{
		if (meshIndex == EMeshType::Mesh_Teapot)
		{
			mTeapotCollection[shaderType].push_back(new Model(mMeshArray[meshIndex], textures, position, rotation, scale));
			mTeapotCollection[shaderType].back()->SetWiggleStrength(wiggleStrength);
		}
		else if (meshIndex < EMeshType::Mesh_Portal)
		{
			mModelCollection[shaderType].push_back(new Model(mMeshArray[meshIndex], textures, position, rotation, scale));
			mModelCollection[shaderType].back()->SetWiggleStrength(wiggleStrength);
		}
	}
}

void CSceneManager::NewLight(const ELightType & type, Mesh* mesh, const CVector3 &colour, const CVector3 &position, 
							 const float &strength, const CVector3 &facingToward, const float &fov)
{
	switch (type)
	{
	case ELightType::point:
		if (mLightStackTop.x < mPointLights.size())
		{
			mPointLights[mLightStackTop.x] = new CLight(mesh, colour, position, strength, facingToward);
			++mLightStackTop.x;
		}
		return;
	case ELightType::spotlight:
		if (mLightStackTop.y < mSpotlights.size())
		{
			mSpotlights[mLightStackTop.y] = new CSpotlight(mesh, colour, position, strength, facingToward, fov);
			++mLightStackTop.y;
		}
		return;
	case ELightType::directional:
		break;
	}
}

void CSceneManager::NewPortal(CVector3 position, CVector3 rotation)
{
	mPortalCollection.push_back(new CPortal(mMeshArray[Mesh_Portal], position, rotation));
	mPortalCollection.back()->CreateTexture(mPortalDesc, mPortalSRDesc);
}