#pragma once
#include "PCH.hpp"
#include "Time.hpp"

class Camera
{
public:
	Camera();
	~Camera() {}

	void initialiseProj(float pAspect, float pFOV = glm::pi<float>() / 2.5f, float pNear = 0.1f, float pFar = 1000000.f);

	void recalculateProj();

	void initaliseOrtho(int sizeX, int sizeY, float pNear = 0.99f, float pFar = 100.f);

	void update();

	void setFOV(float pFOV);

	void move(glm::fvec3 move) { targetPos += move; }
	void rotate(float roll, float pitch, float yaw) { targetRoll += roll; targetPitch += pitch; targetYaw += yaw; }

	float getFOV() { return fov; }
	float getAspect() { return aspect; }
	float getNearPlane() { return nearClip; }
	float getFarPlane() { return farClip; }
	glm::fvec3 getDirectionVector();
	glm::fvec3 getPosition() { return pos; }
	glm::quat getQuaternion() { return qRot; }
	glm::fmat4 getProj() { return proj; }
	glm::fmat4 getView() { return view; }
	glm::fmat4 getInverseProj() { return inverseProj; }
	glm::fmat4 getInverseView() { return inverseView; }

	glm::fmat4 getMatYaw() { return matYaw; }
	glm::fmat4 getMatPitch() { return matPitch; }
	glm::fmat4 getMatRoll() { return matRoll; }

	glm::fvec4 getViewRays() { return glm::fvec4(viewRays[2].x, viewRays[2].y, viewRays[0].x, viewRays[0].y); }

private:

	float ler(float a, float b, float N)
	{
		float v = ((a * (N - 1)) + b) / N;
		return v;
	}

	glm::fvec3 pos;
	glm::fvec3 targetPos;
	float yaw, pitch, roll;
	float targetYaw, targetPitch, targetRoll;
	glm::fmat4 rotation;
	glm::quat qRot;

	glm::fmat4 proj, view, projView;
	glm::fmat4 inverseProj;
	glm::fmat4 inverseView;

	glm::fmat4 matRoll, matPitch, matYaw;

	float nearClip, farClip;
	float fov, aspect;

	float turnLerpFactor;
	float moveLerpFactor;

	glm::fvec4 viewRays[4];
};