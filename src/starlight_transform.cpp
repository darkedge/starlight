#include "starlight_transform.h"
#include "starlight.h"
#include <cassert>

using namespace Vectormath::Aos;

void Transform::SetLocalPosition(float x, float y, float z) {
	SetLocalPosition(Vector3(x, y, z));
}

void Transform::SetLocalPosition(const Vector3 &_position) {
	this->m_localPosition = _position;
	m_dirty = true;
}

void Transform::SetLocalRotation(const Quat &_orientation) {
	this->m_localRotation = _orientation;
	m_dirty = true;
}

// Degrees
void Transform::SetLocalRotation(float pitch, float yaw, float roll) {
	SetLocalRotation(Vector3(pitch, yaw, roll));
}

// Degrees
void Transform::SetLocalRotation(const Vector3 &degrees) {
	assert(false);
	//SetLocalRotation(Quat(degrees * DEG2RAD));
}

void Transform::SetLocalScale(float xyz) {
	SetLocalScale(Vector3(xyz));
}

void Transform::SetLocalScale(float x, float y, float z) {
	SetLocalScale(Vector3(x, y, z));
}

void Transform::SetLocalScale(const Vector3 &_scale) {
	this->m_localScale = _scale;
	m_dirty = true;
}

Matrix4 Transform::GetLocalToWorldMatrix() {
	if (m_dirty) {
		m_localMatrix = Matrix4::translation(m_localPosition)
			*	Matrix4(m_localRotation, Vector3(0.0f))
			*	Matrix4::scale(m_localScale);
		m_dirty = false;
	}

	if (m_parent) {
		return m_parent->GetLocalToWorldMatrix() * m_localMatrix;
	}
	else {
		return m_localMatrix;
	}
}

Matrix4 Transform::GetWorldToLocalMatrix() {
	return inverse(GetLocalToWorldMatrix());
}

Matrix4 Transform::GetViewMatrix()
{
	Matrix4 translate = Matrix4::translation(-GetPosition());
	Matrix4 rotate = transpose(Matrix4(GetRotation(), Vector3(0.0f)));
	return rotate * translate;
}

Vector3 Transform::TransformPoint(const Vector3 &point) {
	return (GetLocalToWorldMatrix() * Vector4(point, 1)).getXYZ();
}

Vector3 Transform::TransformDirection(const Vector3 &direction) const {
	return normalize(rotate(GetRotation(), direction));
}

Vector3 Transform::TransformVector(const Vector3 &vector) {
	return (GetLocalToWorldMatrix() * Vector4(vector, 0)).getXYZ();
}

Vector3 Transform::GetPosition() const {
	if (m_parent) {
		return (m_parent->GetLocalToWorldMatrix() * Vector4(m_localPosition, 1)).getXYZ();
	}
	else {
		return m_localPosition;
	}
}

void Transform::SetPosition(const Vector3 &_position) {
	if (m_parent) {
		m_localPosition = (m_parent->GetWorldToLocalMatrix() * Vector4(_position, 1)).getXYZ();
	}
	else {
		m_localPosition = _position;
	}
	m_dirty = true;
}

void Transform::SetPosition(float x, float y, float z) {
	SetPosition(Vector3(x, y, z));
}

Quat Transform::GetRotation() const {
	if (m_parent) {
		return m_parent->GetRotation() * m_localRotation;
	}
	else {
		return m_localRotation;
	}
}

void Transform::SetRotation(const Quat &_orientation) {
	if (m_parent) {
		assert(false);
		//m_localRotation = glm::inverse(m_parent->GetRotation()) * _orientation;
	}
	else {
		m_localRotation = _orientation;
	}
	m_dirty = true;
}

void Transform::SetRotation(const Vector3 &degrees) {
	assert(false);
	//SetRotation(Quat(degrees * DEG2RAD));
}

void Transform::SetRotation(float pitch, float yaw, float roll) {
	SetRotation(Vector3(pitch, yaw, roll));
}

// TODO: Scale
void Transform::SetParent(Transform *parent, bool worldPositionStays) {
	assert(false);
#if 0 // old Kline code
	assert(parent != this);
	if (worldPositionStays) {
		// Unparent
		if (parent) {
			parent->RemoveChild(this);
		}
		Vector3 pos = GetPosition();
		Quat rot = GetRotation();
		this->m_parent = parent;
		parent->AddChild(this);
		SetPosition(pos);
		SetRotation(rot);
	}
#endif
}

void Transform::AddChild(Transform *transform) {
	assert(false);
}

void Transform::Destroy() {
	assert(false);
}
