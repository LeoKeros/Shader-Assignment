#include "CLight.h"
#include "Shader.h"

//Create a new light.
CLight::CLight(Mesh* mesh, const CVector3 &colour, const CVector3 &position, const float &strength, const CVector3 &facingToward)
{
	mType = ELightType::point;
	mpBody = new Model(mesh);
	mColour = colour;
	mStrength = strength;
	mpBody->SetPosition(position);
	mpBody->SetScale(pow(strength, 0.7f));
	mpBody->FaceTarget(facingToward);
}

CSpotlight::CSpotlight(Mesh* mesh, const CVector3 &colour, const CVector3 &position, const float &strength, const CVector3 &facingToward, const float &fov) : CLight(mesh, colour, position, strength, facingToward )
{
	mType = ELightType::spotlight;
	mSpotlightConeAngle = fov;
}

const float& CSpotlight::GetConeAngle() const
{
	return mSpotlightConeAngle;
}

CMatrix4x4 CSpotlight::CalculateViewMatrix() const
{
	return InverseAffine(mpBody->WorldMatrix());
}

CMatrix4x4 CSpotlight::CalculateProjectionMatrix() const
{
	return MakeProjectionMatrix(1.0f, ToRadians(mSpotlightConeAngle));
}

float CSpotlight::GetCosHalfAngle() const
{
	return cos(ToRadians(mSpotlightConeAngle / 2));
}

//Rotate the light in each axis by the specified amount.
void CLight::Rotate(const CVector3 &rotation)
{
	mpBody->SetRotation(rotation);
}

//Rotate the light to face a target.
void CLight::FaceTarget(const CVector3 &target)
{
	mpBody->FaceTarget(target);
}

void CLight::SetPosition(const CVector3 &pos)
{
	mpBody->SetPosition(pos);
}

const CVector3& CLight::GetFacing() const
{
	return Normalise(mpBody->WorldMatrix().GetZAxis());
}

const CVector3& CLight::GetPosition() const
{
	return mpBody->Position();
}

//Return a constant reference to the colour.
const CVector3& CLight::GetColour() const
{
	return mColour;
}

//Return the value of the light strength.
float CLight::GetStrength() const
{
	return mStrength;
}

void CLight::Render()
{
	mpBody->Render();
}

void CLight::Release()
{
	delete mpBody;
	mpBody = nullptr;
}

CLight::~CLight()
{
}
