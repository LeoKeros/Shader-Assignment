//--------------------------------------------------------------------------------------
// Texture Pixel Shader
//--------------------------------------------------------------------------------------
// Pixel shader simply samples a diffuse texture map and tints with colours from vertex shadeer

#include "Common.hlsli" // Shaders can also use include files - note the extension


//--------------------------------------------------------------------------------------
// Textures (texture maps)
//--------------------------------------------------------------------------------------

// Here we allow the shader access to a texture that has been loaded from the C++ side and stored in GPU memory.
// Note that textures are often called maps (because texture mapping describes wrapping a texture round a mesh).
// Get used to people using the word "texture" and "map" interchangably.
Texture2D DiffuseSpecularMap : register(t0); // Diffuse map (main colour) in rgb and specular map (shininess level) in alpha - C++ must load this into slot 0
Texture2D ShadowMapSpotlights[4] : register(t1); // Texture holding the view of the scene from a light
Texture2D NormalHeightMap          : register(t5); // Normal map in rgb - C++ must load this into slot 1
SamplerState TexSampler : register(s0); // A sampler is a filter for a texture like bilinear, trilinear or anisotropic
SamplerState PointClamp : register(s1); // No filtering for shadow maps (you might think you could use trilinear or similar, but it will filter light depths not the shadows cast...)
SamplerState ShadowSample : register(s2); // No filtering for shadow maps (you might think you could use trilinear or similar, but it will filter light depths not the shadows cast...)

//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

//***| INFO |*********************************************************************************
// Normal mapping pixel shader function. The lighting part of the shader is the same as the
// per-pixel lighting shader - only the source of the surface normal is different
//
// An extra "Normal Map" texture is used - this contains normal (x,y,z) data in place of
// (r,g,b) data indicating the normal of the surface *per-texel*. This allows the lighting
// to take account of bumps on the texture surface. Using these normals is complex:
//    1. We must store a "tangent" vector as well as a normal for each vertex (the tangent
//       is basically the direction of the texture U axis in model space for each vertex)
//    2. Get the (interpolated) model normal and tangent at this pixel from the vertex
//       shader - these are the X and Z axes of "tangent space"
//    3. Use a "cross-product" to calculate the bi-tangent - the missing Y axis
//    4. Form the "tangent matrix" by combining these axes
//    5. Extract the normal from the normal map texture for this pixel
//    6. Use the tangent matrix to transform the texture normal into model space, then
//       use the world matrix to transform it into world space
//    7. This final world-space normal can be used in the usual lighting calculations, and
//       will show the "bumpiness" of the normal map
//
// Note that all this detail boils down to just five extra lines of code here
//********************************************************************************************
float4 main(NormalMappingPixelShaderInput input) : SV_Target
{
    float3 cameraDirection = normalize(gCameraPosition - input.worldPosition);

	//************************
	// Normal Map Extraction
	//************************

	// Will use the model normal/tangent to calculate matrix for tangent space. The normals for each pixel are *interpolated* from the
	// vertex normals/tangents. This means they will not be length 1, so they need to be renormalised (same as per-pixel lighting issue)
	float3 modelNormal  = normalize(input.modelNormal);
	float3 modelTangent = normalize(input.modelTangent);

	// Calculate bi-tangent to complete the three axes of tangent space - then create the *inverse* tangent matrix to convert *from*
	// tangent space into model space. This is just a matrix built from the three axes (very advanced note - by default shader matrices
	// are stored as columns rather than in rows as in the C++. This means that this matrix is created "transposed" from what we would
	// expect. However, for a 3x3 rotation matrix the transpose is equal to the inverse, which is just what we require)
	float3 modelBiTangent = cross(modelNormal, modelTangent );
	float3x3 invTangentMatrix = float3x3(modelTangent, modelBiTangent, modelNormal);
	
    // Transform camera vector from world into model space. Need *inverse* world matrix for this.
	// Only need 3x3 matrix to transform vectors, to invert a 3x3 matrix we transpose it (flip it about its diagonal)
    float3x3 invWorldMatrix = transpose((float3x3) gWorldMatrix);
    float3 cameraModelDir = normalize(mul(invWorldMatrix, cameraDirection)); // Normalise in case world matrix is scaled

	// Then transform model-space camera vector into tangent space (texture coordinate space) to give the direction to offset texture
	// coordinate, only interested in x and y components. Calculated inverse tangent matrix above, so invert it back for this step
    float3x3 tangentMatrix = transpose(invTangentMatrix);
    float2 textureOffsetDir = mul(cameraModelDir, tangentMatrix).xy;

	// Get the height info from the normal map's alpha channel at the given texture coordinate
	// Rescale from 0->1 range to -x->+x range, x determined by ParallaxDepth setting
    float textureHeight = 0.08f * (NormalHeightMap.Sample(TexSampler, input.uv).a - 0.5f);
	
	// Use the depth of the texture to offset the given texture coordinate - this corrected texture coordinate will be used from here on
    float2 offsetTexCoord = input.uv + textureHeight * textureOffsetDir;

	// Get the texture normal from the normal map. The r,g,b pixel values actually store x,y,z components of a normal. However, r,g,b
	// values are stored in the range 0->1, whereas the x, y & z components should be in the range -1->1. So some scaling is needed
    float3 textureNormal = (2.0f * NormalHeightMap.Sample(TexSampler, offsetTexCoord).rgb - 1.0f); // Scale from 0->1 to -1->1
    textureNormal.z *= 0.05f;

	// Now convert the texture normal into model space using the inverse tangent matrix, and then convert into world space using the world
	// matrix. Normalise, because of the effects of texture filtering and in case the world matrix contains scaling
	float3 worldNormal = normalize( mul( (float3x3)gWorldMatrix, mul(textureNormal, invTangentMatrix) ) );
       
	///////////////////////
	// Calculate lighting

    // Slight adjustment to calculated depth of pixels so they don't shadow themselves
    const float DepthAdjust = 0.0005f;

    // Lighting equations
    
    float3 finalDiffuseLight = 0; // Initialy assume no contribution from this light
    float3 finalSpecularLight = 0;
    
    
    // ****** POINTLIGHTS ******* //  
    for (int x = 0; x < lightStackTops.x; ++x)
    {
	    // Direction from pixel to light
        float3 lightVector = pointLights[x].position - input.worldPosition;
        float3 lightDist = length(lightVector);
        float3 lightDirection = lightVector / lightDist;
        float3 diffuseLight = (pointLights[x].colour * max(dot(worldNormal, lightDirection), 0) / lightDist);
        finalDiffuseLight = finalDiffuseLight + diffuseLight;

        float3 halfway = normalize(lightDirection + cameraDirection);
        finalSpecularLight = finalSpecularLight + (diffuseLight * pow(max(dot(worldNormal, halfway), 0), gSpecularPower));
    }

    // ****** SPOTLIGHTS ******* //  
    for (int i = 0; i < lightStackTops.y; ++i)
    {
	    // Direction from pixel to light
        float3 lightDirection = normalize(spotlights[i].position - input.worldPosition);

	    // Check if pixel is within light cone
        if (dot(spotlights[i].facing, -lightDirection) > spotlights[i].cosHalfAngle) //**** TODO: This condition needs to be written as the first exercise to get spotlights working
           // As well as the variables above, you also will need values from the constant buffers in "common.hlsli"
        {
	        // Using the world position of the current pixel and the matrices of the light (as a camera), find the 2D position of the
	        // pixel *as seen from the light*. Will use this to find which part of the shadow map to look at.
	        // These are the same as the view / projection matrix multiplies in a vertex shader (can improve performance by putting these lines in vertex shader)
            float4 lightViewPosition = mul(spotlights[i].viewMatrix, float4(input.worldPosition, 1.0f));
            float4 lightProjection = mul(spotlights[i].projectionMatrix, lightViewPosition);

		    // Convert 2D pixel position as viewed from light into texture coordinates for shadow map - an advanced topic related to the projection step
		    // Detail: 2D position x & y get perspective divide, then converted from range -1->1 to UV range 0->1. Also flip V axis
            float2 shadowMapUV = 0.5f * lightProjection.xy / lightProjection.w + float2(0.5f, 0.5f);
            shadowMapUV.y = 1.0f - shadowMapUV.y; // Check if pixel is within light cone

		    // Get depth of this pixel if it were visible from the light (another advanced projection step)
            float depthFromLight = lightProjection.z / lightProjection.w - DepthAdjust; //*** Adjustment so polygons don't shadow themselves
		    
		    // Compare pixel depth from light with depth held in shadow map of the light. If shadow map depth is less than something is nearer
		    // to the light than this pixel - so the pixel gets no effect from this light
            if (depthFromLight < ShadowMapSpotlights[i].Sample(PointClamp, shadowMapUV).r)
            {
                float3 lightDist = length(spotlights[i].position - input.worldPosition);
                float3 diffuseLight = (spotlights[i].colour * max(dot(worldNormal, lightDirection), 0) / lightDist); // Equations from lighting lecture
                finalDiffuseLight = finalDiffuseLight + diffuseLight;
                
                float3 halfway = normalize(lightDirection + cameraDirection);
                finalSpecularLight = finalSpecularLight + (diffuseLight * pow(max(dot(worldNormal, halfway), 0), gSpecularPower)); // Multiplying by finalDiffuseLight instead of light colour - my own personal preference
            }
        }
    }

    // Sample diffuse material colour for this pixel from a texture using a given sampler that you set up in the C++ code
    // Ignoring any alpha in the texture, just reading RGB
    float4 textureColour = DiffuseSpecularMap.Sample(TexSampler, offsetTexCoord);
    float3 diffuseMaterialColour = textureColour.rgb;
    float specularMaterialColour = textureColour.a;

    float3 finalColour = (gAmbientColour + finalDiffuseLight) * diffuseMaterialColour + finalSpecularLight * specularMaterialColour;

    return float4(finalColour, 1.0f); // Always use 1.0f for alpha - no alpha blending in this lab
}