#pragma once
#include "PCH.hpp"

class Transform
{
public:
	Transform() : translation(glm::fvec3(0, 0, 0)), origin(glm::fvec3(0.f, 0.f, 0.f)), scalem(glm::fvec3(1.f, 1.f, 1.f)), needUpdate(true) {}
	Transform(glm::fmat4& mat) : translation(glm::fvec3(0, 0, 0)), origin(glm::fvec3(0.f, 0.f, 0.f)), scalem(glm::fvec3(1.f, 1.f, 1.f)), needUpdate(true), transform(mat) {}
	~Transform() {}

	void setTransformMat(glm::fmat4 pT)
	{
		transform = pT;
		needUpdate = false;
	}

	inline glm::fmat4 getTransformMat()
	{
		return transform;
	}

	inline glm::fmat4 getInverseTransformMat()
	{
		return glm::inverse(transform);
	}

	void operator*=(Transform& other)
	{
		translation += other.getTranslation();
		qRot *= other.qRot;
		scalem *= other.scalem;
		origin = glm::mix(origin, other.origin, 0.5);
	}

	inline Transform& setTranslation(glm::fvec3 p) { translation = p; needUpdate = true; return *this; }
	inline Transform& translate(glm::fvec3 p) { 
		translation += p; 
		needUpdate = true;
		return *this; 
	}
	inline glm::fvec3 getTranslation() { return translation; }
	inline glm::quat getQuat() { return qRot; }

	inline Transform& setQuat(glm::fquat p) { qRot = p; return *this; }

	inline Transform& setScale(glm::fvec3 p) { scalem = p; needUpdate = true; return *this; }
	inline Transform& scale(glm::fvec3 p) { scalem *= p; needUpdate = true; return *this; }
	inline Transform& scale(float p) { scalem = scalem * p; needUpdate = true; return *this; }
	inline glm::fvec3 getScale() { return scalem; }

	inline Transform& setOrigin(glm::fvec3 p) { origin = p; return *this; }
	inline glm::fvec3 getOrigin() { return origin; }

	inline void updateMatrix()
	{
		needUpdate = false;

		glm::fmat4 ttranslate = glm::translate(glm::fmat4(), translation - origin);
		glm::fmat4 sscale = glm::scale(glm::fmat4(), glm::fvec3(scalem.x, scalem.y, scalem.z));

		glm::fmat4 rrotate = glm::fmat4(qRot);
		glm::fmat4 tttranslate = glm::translate(glm::fmat4(), origin);

		transform = ttranslate * rrotate * sscale * tttranslate;
	}

private:

	glm::fvec3 translation;
	glm::fvec3 origin;
	glm::fvec3 scalem;

	glm::quat qRot;

	bool needUpdate;
	glm::fmat4 transform;
};
