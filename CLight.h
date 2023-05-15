#pragma once
#include "Model.h"
#include "CVector3.h"
#include "CMatrix4x4.h"
#include "GraphicsHelpers.h"

enum class ELightType : char
{
	point,
	spotlight,
	directional
};

class CLight
{
protected:
	ELightType mType;
	Model* mpBody;
	CVector3 mColour;
	float mStrength;
public:
	CLight(Mesh* mesh, const CVector3 &colour, const CVector3 &position, const float &strength, const CVector3 &facingToward = { 0.0f, 0.0f, 0.0f });
	void Rotate(const CVector3 &rotation);
	void FaceTarget(const CVector3 &target);
	void Render();
	void Release();

	void SetPosition(const CVector3 &pos);

	const CVector3& GetFacing() const;
	const CVector3& GetPosition() const;
	const CVector3& GetColour() const;
	float GetStrength() const;
	~CLight();
};

class CSpotlight : public CLight
{
private:
	float mSpotlightConeAngle = 0;
public:
	CSpotlight(Mesh* mesh, const CVector3 &colour, const CVector3 &position, const float &strength, const CVector3 &facingToward = { 0.0f, 0.0f, 0.0f }, const float &fov = 90.0f);
	const float& GetConeAngle() const;
	float GetCosHalfAngle() const;
	CMatrix4x4 CalculateViewMatrix() const;
	CMatrix4x4 CalculateProjectionMatrix() const;

};
