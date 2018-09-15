#include "PCH.hpp"
#include "PhysicsWorld.hpp"
#include "Engine.hpp"
#include "World.hpp"
#include "Camera.hpp"
#include "Model.hpp"
#include "Renderer.hpp"
#include "AssetStore.hpp"
#include "Window.hpp"
#include "Threading.hpp"

void PhysicsWorld::addRigidBody(PhysicsObject * body)
{
	Engine::threading->physObjectAddMutex.lock();
	objectsToAdd.push_back(body);
	Engine::threading->physObjectAddMutex.unlock();
}

void PhysicsWorld::updateAddedObjects()
{
	Engine::threading->physObjectAddMutex.lock();
	for (auto object : objectsToAdd)
	{
		dynamicsWorld->addRigidBody(object->rigidBody);
		objects.push_back(object);
	}
	objectsToAdd.clear();
	Engine::threading->physObjectAddMutex.unlock();
}

void PhysicsWorld::updateModels()
{
	auto tIndex = ModelInstance::toEngineTransformIndex;

	btTransform t;
	for (auto o : objects)
	{
		o->rigidBody->getMotionState()->getWorldTransform(t);
		btQuaternion q = t.getRotation();
		btVector3 p = t.getOrigin();
		o->instance->transform[tIndex].setTranslation(glm::fvec3(p.x(), p.y(), p.z()));
		o->instance->transform[tIndex].setQuat(glm::fquat(q.w(), q.x(), q.y(), q.z()));
		o->instance->transform[tIndex].updateMatrix();
	}

	ModelInstance::toEngineTransformIndex = tIndex == 0 ? 1 : 0;
}

btVector3 PhysicsWorld::getRayTo(int x, int y)
{
	float ndcx = ((2.0f * x) / float(Engine::window->resX)) - 1.0f;
	float ndcy = ((2.0f * y) / float(Engine::window->resY)) - 1.0f;

	glm::fvec4 ray_clip(ndcx, ndcy, -1.f, 1.f);

	glm::fvec4 ray_eye;

	ray_eye = Engine::camera.getInverseProj() * ray_clip;
	ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.f, 0.0);

	glm::fvec4 w = (glm::inverse(Engine::camera.getView()) * ray_eye);
	glm::fvec3 ray_wor(w.x, w.y, w.z);

	ray_wor = glm::normalize(ray_wor) * 10000.f;

	return btVector3(ray_wor.x, ray_wor.y, ray_wor.z);
}

bool PhysicsWorld::pickBody(btVector3& rayFromWorld, btVector3& rayToWorld)
{
	if (dynamicsWorld == 0)
		return false;

	static int i = 0;

	rayToWorld = rayFromWorld + (rayToWorld*10000.f);

	btCollisionWorld::ClosestRayResultCallback rayCallback(rayFromWorld, rayToWorld);
	dynamicsWorld->rayTest(rayFromWorld, rayToWorld, rayCallback);
	if (rayCallback.hasHit()) {
		btVector3 pickPos = rayCallback.m_hitPointWorld;
		btRigidBody* body = (btRigidBody*)btRigidBody::upcast(rayCallback.m_collisionObject);
		if (body) {
			if (!(body->isStaticObject() || body->isKinematicObject()))
			{
				pickedBody = body;
				savedState = pickedBody->getActivationState();
				pickedBody->setActivationState(DISABLE_DEACTIVATION);

				btVector3 localPivot = body->getCenterOfMassTransform().inverse() * pickPos;
				
				p2p = new btPoint2PointConstraint(*body, localPivot);
				dynamicsWorld->addConstraint(p2p, true);
				pickedConstraint = p2p;
				p2p->m_setting.m_impulseClamp = 3000.f;
				p2p->m_setting.m_tau = 1.2;
				p2p->m_setting.m_damping = 1.8;
			}
		}
		oldPickingPos = rayToWorld;
		hitPos = pickPos;
		oldPickingDist = (pickPos - rayFromWorld).length();
	}
}

void PhysicsWorld::removePickingConstraint()
{
	if (pickedConstraint) {
		pickedBody->forceActivationState(savedState);
		pickedBody->activate();
		dynamicsWorld->removeConstraint(pickedConstraint);
		delete pickedConstraint;
		pickedConstraint = 0;
		pickedBody = 0;
	}
}

bool PhysicsWorld::movePickedBody(btVector3 & rayFromWorld, btVector3 & rayToWorld)
{
	if (pickedBody  && pickedConstraint) {
		btPoint2PointConstraint* pickCon = static_cast<btPoint2PointConstraint*>(pickedConstraint);
		if (pickCon) {
			btVector3 newPivotB;
			btVector3 dir = rayToWorld - rayFromWorld;
			dir.normalize();
			dir *= oldPickingDist;
			newPivotB = rayFromWorld + dir;
			pickCon->setPivotB(newPivotB);
			return true;
		}
	}
	return false;
}

bool PhysicsWorld::mouseMoveCallback(int x, int y)
{
	if (pickedBody && pickedConstraint)
	{
		btVector3 rayTo = getRayTo(x, y);
		glm::fvec3 p = Engine::camera.getPosition();
		btVector3 rayFrom(p.x, p.y, p.z);

		movePickedBody(rayFrom, rayTo);
	}
	return false;
}