//--------------------------------------------------------------------------------------
// Per-Pixel Lighting Pixel Shader
//--------------------------------------------------------------------------------------
// Pixel shader receives position and normal from the vertex shader and uses them to calculate
// lighting per pixel. Also samples a samples a diffuse + specular texture map and combines with light colour.

#include "Common.hlsli" // Shaders can also use include files - note the extension


//--------------------------------------------------------------------------------------
// Textures (texture maps)
//--------------------------------------------------------------------------------------

// Here we allow the shader access to a texture that has been loaded from the C++ side and stored in GPU memory.
// Note that textures are often called maps (because texture mapping describes wrapping a texture round a mesh).
// Get used to people using the word "texture" and "map" interchangably.
Texture2D DiffuseSpecularMap : register(t0); // Textures here can contain a diffuse map (main colour) in their rgb channels and a specular map (shininess) in the a channel
Texture2D ShadowMapSpotlights[4] : register(t1); // Texture holding the view of the scene from a light
Texture2D PortalTexture      : register(t5);

SamplerState TexSampler      : register(s0); // A sampler is a filter for a texture like bilinear, trilinear or anisotropic - this is the sampler used for the texture above
SamplerState PointClamp : register(s1); // No filtering for shadow maps (you might think you could use trilinear or similar, but it will filter light depths not the shadows cast...)
SamplerState ShadowSample : register(s2); // No filtering for shadow maps (you might think you could use trilinear or similar, but it will filter light depths not the shadows cast...)


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

// Pixel shader entry point - each shader has a "main" function
// This shader just samples a diffuse texture map
float4 main(LightingPixelShaderInput input) : SV_Target
{
	// Slight adjustment to calculated depth of pixels so they don't shadow themselves
	const float DepthAdjust = 0.0005f;

    // Normal might have been scaled by model scaling or interpolation so renormalise
    input.worldNormal = normalize(input.worldNormal); 

	///////////////////////
	// Calculate lighting
    
    // Direction from pixel to camera
    float3 cameraDirection = normalize(gCameraPosition - input.worldPosition);

	float3 finalDiffuseLight  = 0; // Initialy assume no contribution from this light
	float3 finalSpecularLight = 0;
     
    // ****** POINTLIGHTS ******* //  
    for (int i = 0; i < lightStackTops.x; ++i)
    {
	    // Direction from pixel to light
        float3 lightVector = pointLights[i].position - input.worldPosition;
        float3 lightDist = length(lightVector);
        float3 lightDirection = lightVector / lightDist;
        float3 diffuseLight = (pointLights[i].colour * max(dot(input.worldNormal, lightDirection), 0) / lightDist);
        finalDiffuseLight = finalDiffuseLight + diffuseLight;

        float3 halfway = normalize(lightDirection + cameraDirection);
        finalSpecularLight = finalSpecularLight + (diffuseLight * pow(max(dot(input.worldNormal, halfway), 0), gSpecularPower));
    }

    // ****** SPOTLIGHTS ******* //  
    for (i = 0; i < lightStackTops.y; ++i)
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
                float3 diffuseLight = (spotlights[i].colour * max(dot(input.worldNormal, lightDirection), 0) / lightDist); // Equations from lighting lecture
                finalDiffuseLight = finalDiffuseLight + diffuseLight;
                
                float3 halfway = normalize(lightDirection + cameraDirection);
                finalSpecularLight = finalSpecularLight + (diffuseLight * pow(max(dot(input.worldNormal, halfway), 0), gSpecularPower)); // Multiplying by finalDiffuseLight instead of light colour - my own personal preference
            }
        }
    }
    
    float4 textureColour = DiffuseSpecularMap.Sample(TexSampler, input.uv); //Reuse for sampling.
 
    input.uv.x = 1 - input.uv.x;
    float3 portalColour = PortalTexture.Sample(TexSampler, input.uv).rbg;
    portalColour = (portalColour.r + portalColour.g + portalColour.b) / 3; // Diffuse material colour in texture RGB (base colour of model), shifted to greyscale;

    portalColour.r = (textureColour.r * textureColour.a) + (portalColour.r * (1 - textureColour.a));
    portalColour.b = (textureColour.b * textureColour.a) + (portalColour.b * (1 - textureColour.a));
    portalColour.g = (textureColour.g * textureColour.a) + (portalColour.g * (1 - textureColour.a));

	////////////////////
	// Combine lighting and textures

    // Sample diffuse material and specular material colour for this pixel from a texture using a given sampler that you set up in the C++ code
    
    // Combine lighting with texture colours
    float3 finalColour = (gAmbientColour + finalDiffuseLight) * portalColour + finalSpecularLight * textureColour.a;

    return float4(finalColour, 1.0f); // Always use 1.0f for output alpha - no alpha blending in this lab
}