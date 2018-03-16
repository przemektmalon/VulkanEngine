#include "PCH.hpp"
#include "Camera.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"

Camera::Camera()
{
	yaw = 0; pitch = 0; roll = 0;
	targetYaw = 0; targetPitch = 0; targetRoll = 0;
	pos = glm::fvec3(0, 0, 0);
	targetPos = glm::fvec3(0, 0, 0);
	turnLerpFactor = 1.15;
	moveLerpFactor = 3.f;
}

void Camera::initialiseProj(float pAspect, float pFOV, float pNear, float pFar)
{
	nearClip = pNear;
	farClip = pFar;
	fov = pFOV;
	aspect = pAspect;

	recalculateProj();
}

void Camera::recalculateProj()
{
	proj = glm::perspective(fov, aspect, nearClip, farClip);
	inverseProj = glm::inverse(proj);
}

void Camera::initaliseOrtho(int sizeX, int sizeY, float pNear, float pFar)
{
	proj = glm::ortho(0.f, float(sizeX), 0.f, float(sizeY), pNear, pFar);
}

glm::fvec3 Camera::getDirectionVector()
{
	return glm::fvec3(glm::sin(yaw) * glm::cos(pitch), -glm::sin(pitch), -glm::cos(yaw) * glm::cos(pitch));
}

void Camera::update(Time & dt)
{
	targetPitch = glm::clamp(targetPitch, -glm::pi<float>() / 2.f, glm::pi<float>() / 2.f);
	pitch = ler(pitch, targetPitch, turnLerpFactor);
	yaw = ler(yaw, targetYaw, turnLerpFactor);

	matRoll = glm::rotate(glm::fmat4(), roll, glm::vec3(0.0f, 0.0f, 1.0f));
	matPitch = glm::rotate(glm::fmat4(), pitch, glm::vec3(1.0f, 0.0f, 0.0f));
	matYaw = glm::rotate(glm::fmat4(), yaw, glm::vec3(0.0f, 1.0f, 0.0f));

	pos.x = ler(pos.x, targetPos.x, moveLerpFactor);
	pos.y = ler(pos.y, targetPos.y, moveLerpFactor);
	pos.z = ler(pos.z, targetPos.z, moveLerpFactor);

	glm::fmat4 translate = glm::fmat4(1.0f);
	translate = glm::translate(translate, -pos);

	qRot = glm::angleAxis(targetPitch, glm::vec3(1.0f, 0.0f, 0.0f));
	qRot *= glm::angleAxis(targetYaw, glm::vec3(0.0f, 1.0f, 0.0f));

	rotation = glm::fmat4(qRot);
	view = rotation * translate;
	inverseView = glm::inverse(view);

	projView = proj * view;

	viewRays[0] = glm::fvec4(-1.f, -1.f, -1.f, 1.f);//BL
	viewRays[1] = glm::fvec4(1.f, -1.f, -1.f, 1.f);//BR
	viewRays[2] = glm::fvec4(1.f, 1.f, -1.f, 1.f);//TR
	viewRays[3] = glm::fvec4(-1.f, 1.f, -1.f, 1.f);//TL

	for (int i = 0; i < 4; ++i)
	{
		viewRays[i] = inverseProj * viewRays[i];
		viewRays[i] /= viewRays[i].w;
		viewRays[i] /= viewRays[i].z;
	}
}

void Camera::setFOV(float pFOV)
{
	fov = glm::radians(pFOV);
	recalculateProj();
	//Engine::renderer->cameraProjUpdated();
}
