//--------------------------------------------------------------------------------------
// Scene geometry and layout preparation
// Scene rendering & update
//--------------------------------------------------------------------------------------


#include "Mesh.h"
#include "Model.h"
#include "Camera.h"
#include "Shader.h"
#include "Input.h"
#include "Common.h"
#include "CLight.h"
#include "CTexture.h"
#include "CPortal.h"

#include "CVector2.h" 
#include "CVector3.h" 
#include "CMatrix4x4.h"
#include "MathHelpers.h"     // Helper functions for maths
#include "GraphicsHelpers.h" // Helper functions to unclutter the code here

#include "ColourRGBA.h" 

#include <sstream>
#include <memory>
#include <vector>
#include <array>

#ifndef _SCENE_H_INCLUDED_
#define _SCENE_H_INCLUDED_
//--------------------------------------------------------------------------------------
// Scene Geometry and Layout
//--------------------------------------------------------------------------------------


class CSceneManager
{
private:
	enum EMeshType
	{
		Mesh_Troll,
		Mesh_Cube,
		Mesh_CubeTangent, //Cube mesh with tangets. Loading with tangents normally causes problems for other shaders.
		Mesh_Decal,
		Mesh_Crate,
		Mesh_Sphere,
		Mesh_SphereTangent, //Sphere mesh with tangets. Loading with tangents normally causes problems for other shaders.
		Mesh_Ground,
		Mesh_Light,
		Mesh_Teapot,
		Mesh_Portal,
	};
	enum ETextureType
	{
		TrollTexture,
		StoneTexture,
		BrickTexture,
		MoogleTexture,
		CargoTexture,
		WoodTexture,
		WoodNormal,
		GrassTexture,
		MetalTexture,
		MetalNormal,
		PatternTexture,
		PatternNormalH,
		BrainTexture,
		BrainNormalH,
		CobbleTexture,
		CobbleNormalH,
		TechTexture,
		TechNormalH,
		WallTexture,
		WallNormalH,
		TVTexture,
		FlareTexture,
		GlassTexture,
		NumOfTextures
	};
	enum EVertexShaders
	{
		//Pixel lit models
		vs_Wiggle,
		vs_NormalMap,
		vs_PixelLighting, //Generic shader, so is the last so that unique models can use it without having to set it themselves.
		//Unique usages
		vs_BasicTransform,
		vs_WiggleTangent,
		NumVertexShaders,
	};
	enum EPixelShaders
	{
		//Pixel lit models
		ps_Wiggle,
		ps_NormalMap,
		ps_ParallaxMap,
		ps_PixelLighting, //Generic shader, placed to match it's vertex shader
		ps_Fade, //Uses the generic vertex shader
		//Unique usages
		ps_Portal,
		ps_Transparent, //For moedels with alpha, like glass.
		ps_LightModel,
		ps_DepthOnly,
		NumPixelShaders,
		//Mixed shaders using alternative vertex shader / pixel shader combinations
		ps_WiggleParallax = ps_Portal, //Uses the same number because doesn't refer to a real pixel shader, and portal is used specifically.
		NumPSInCollections
	};
	enum EMaxGroupSizes //Used to declare constant sizes of groups that don't match the number of type enums for that group (EG, mesh).
	{
		gsNumOfMesh = Mesh_Portal + 1,
		gsNumOfModelPS = NumPSInCollections, //Not including portals.
		gsNumSpotlights = 4,
		gsNumPointLights = 3,
		gsNumDirectionalLights = 1,

	};

	// Constants controlling speed of movement/rotation (measured in units per second because we're using frame time)
	//Meshes for all models.
	Mesh* mMeshArray[gsNumOfMesh];

	//Collections of objects.
	std::vector<Model*> mModelCollection[gsNumOfModelPS]; //Jagged array. Not including portals (Handled seperately). Sorted according to shaders used.
	std::vector<Model*> mTeapotCollection[gsNumOfModelPS]; //Teapots handled seperately, because they require different culling. Sorted according to shaders used.
	std::vector<Model*> mTransparentModels; //Models that use no culling
	std::vector<CPortal*> mPortalCollection;

	//Light stack
	std::array<CLight*, gsNumPointLights> mPointLights;
	std::array<CSpotlight*, gsNumSpotlights> mSpotlights;
	std::array<CLight*, gsNumDirectionalLights> mDirectionalLights;
	CVector3 mLightStackTop = { 0, 0, 0 }; //Point, spot, direction

	// Textures
	CTexture mTextures[NumOfTextures]; //The default textures for each mesh. The portal texture is the texture around the portal.

	//Texture data
	int mPortalWidth = 1024;
	int mPortalHeight = 1024;

	//Shadow mapping
	int mShadowMapSize = 1024;

	// Vertex and pixel shader DirectX objects
	std::array<ID3D11VertexShader*, NumVertexShaders> mVertexShaders;
	std::array<ID3D11PixelShader*, NumPixelShaders> mPixelShaders;

	//These are populated as each type of light is created.
	ID3D11Texture2D*          mShadowMapSpotlightTexture[gsNumSpotlights]; // This object represents the memory used by the texture on the GPU
	ID3D11DepthStencilView*   mShadowMapSpotlightDepthStencil[gsNumSpotlights]; // This object is used when we want to render to the texture above **as a depth buffer**
	ID3D11ShaderResourceView* mShadowMapSpotlightSRV[gsNumSpotlights]; // This object is used to give shaders access to the texture above (SRV = shader resource view)
	D3D11_TEXTURE2D_DESC mShadowMapTextureDesc = {};
	D3D11_DEPTH_STENCIL_VIEW_DESC mShadowMapDsvDesc = {};
	D3D11_SHADER_RESOURCE_VIEW_DESC mShadowMapSrvDesc = {};

	//Portals
	ID3D11Texture2D*        mPortalDepthStencil = nullptr; // This object represents the memory used by the texture on the GPU
	ID3D11DepthStencilView* mPortalDepthStencilView = nullptr; // This object is used when we want to use the texture above as the depth buffer
	D3D11_TEXTURE2D_DESC mPortalDesc = {};
	D3D11_SHADER_RESOURCE_VIEW_DESC mPortalSRDesc = {};

	//Main camera for the scene
	Camera* mCamera;

	// Additional light information
	CVector3 mAmbientColour = { 0.2f, 0.2f, 0.3f }; // Background level of light (slightly bluish to match the far background, which is dark blue)
	float    mSpecularPower = 256; // Specular power controls shininess - same for all models in this app

	ColourRGBA mBackgroundColor = { 0.2f, 0.2f, 0.3f, 1.0f };

	// Variables controlling light1's orbiting of the cube
	const float gLightOrbit = 20.0f;
	const float gLightOrbitSpeed = 0.7f;

	ID3D11SamplerState* gPointSampler = nullptr;
	ID3D11SamplerState* gTrilinearSampler = nullptr;
	ID3D11SamplerState* gAnisotropic4xSampler = nullptr;

	ID3D11BlendState* gNoBlendingState = nullptr;
	ID3D11BlendState* gAdditiveBlendingState = nullptr;
	ID3D11BlendState* gMultiplicativeBlending = nullptr;

	ID3D11RasterizerState*   gCullBackState = nullptr;
	ID3D11RasterizerState*   gCullFrontState = nullptr;
	ID3D11RasterizerState*   gCullNoneState = nullptr;

	ID3D11DepthStencilState* gUseDepthBufferState = nullptr;
	ID3D11DepthStencilState* gDepthReadOnlyState = nullptr;
	ID3D11DepthStencilState* gNoDepthBufferState = nullptr;
public:
	//--------------------------------------------------------------------------------------
	// Scenery Management
	//--------------------------------------------------------------------------------------
	//Adds a new model to the model collection. The model collection is used for generic, unimportant models.
	void NewModel(EMeshType meshIndex, std::vector<ETextureType> textureIndexes, CVector3 position = { 0,0,0 }, float scale = 1, 
				  CVector3 rotation = { 0,0,0 }, float wiggleStrength = 0, EPixelShaders shaderType = ps_PixelLighting);
	void NewPortal(CVector3 position = { 0,0,0 }, CVector3 rotation = { 0,0,0 });

	//Light factory
	void NewLight(const ELightType & type, Mesh* mesh, const CVector3 &colour, const CVector3 &position, const float &strength, 
				  const CVector3 &facingToward = { 0.0f, 0.0f, 0.0f }, const float &fov = 90);
	


	//---------------------------------------------------------------------------------------
	// Initialisation and Release
	//---------------------------------------------------------------------------------------

	// Prepare the geometry required for the scene
	// Returns true on success
	bool InitGeometry();

	// Layout the scene
	// Returns true on success
	bool InitScene();

	// Release the geometry resources created above
	void ReleaseResources();


	//--------------------------------------------------------------------------------------
	// Scene Render and Update
	//--------------------------------------------------------------------------------------
	void RenderDepthBufferFromLight(const CSpotlight &light);
	void RenderSceneFromCamera(Camera* camera);
	void RenderScene();

	// frameTime is the time passed since the last frame
	void UpdateScene(float frameTime);


	//--------------------------------------------------------------------------------------
	// Shader creation / destruction
	//--------------------------------------------------------------------------------------
	// Load shaders required for this app, returns true on success
	bool LoadShaders();
	// Release shaders used by the app
	void ReleaseShaders();

	//--------------------------------------------------------------------------------------
	// State creation / destruction
	//--------------------------------------------------------------------------------------
	
	// Create all the states used in this app, returns true on success
	bool CreateStates();

	// Release DirectX state objects
	void ReleaseStates();

};

#endif //_SCENE_H_INCLUDED_
