#include "starlight_transform.h"
#include <glm/gtx/transform.hpp>
#include <cassert>

void Transform::SetLocalPosition(float x, float y, float z) {
	SetLocalPosition(glm::vec3(x, y, z));
}

void Transform::SetLocalPosition(const glm::vec3 &_position) {
	this->m_localPosition = _position;
	m_dirty = true;
}

void Transform::SetLocalRotation(const glm::quat &_orientation) {
	this->m_localRotation = _orientation;
	m_dirty = true;
}

// Degrees
void Transform::SetLocalRotation(float pitch, float yaw, float roll) {
	SetLocalRotation(glm::vec3(pitch, yaw, roll));
}

// Degrees
void Transform::SetLocalRotation(const glm::vec3 &degrees) {
	SetLocalRotation(glm::quat(glm::radians(degrees)));
}

void Transform::SetLocalScale(float xyz) {
	SetLocalScale(glm::vec3(xyz));
}

void Transform::SetLocalScale(float x, float y, float z) {
	SetLocalScale(glm::vec3(x, y, z));
}

void Transform::SetLocalScale(const glm::vec3 &_scale) {
	this->m_localScale = _scale;
	m_dirty = true;
}

glm::mat4 Transform::GetLocalToWorldMatrix() {
	if (m_dirty) {
		m_localMatrix = glm::translate(m_localPosition)
			*	glm::mat4(m_localRotation)
			*	glm::scale(m_localScale);
		m_dirty = false;
	}

	if (m_parent) {
		return m_parent->GetLocalToWorldMatrix() * m_localMatrix;
	}
	else {
		return m_localMatrix;
	}
}

glm::mat4 Transform::GetWorldToLocalMatrix() {
	return glm::inverse(GetLocalToWorldMatrix());
}

glm::vec3 Transform::TransformPoint(const glm::vec3 &point) {
	return glm::vec3(GetLocalToWorldMatrix() * glm::vec4(point, 1));
}

glm::vec3 Transform::TransformDirection(const glm::vec3 &direction) const {
	return glm::normalize(GetRotation() * direction);
}

glm::vec3 Transform::TransformVector(const glm::vec3 &vector) {
	return glm::vec3(GetLocalToWorldMatrix() * glm::vec4(vector, 0));
}

glm::vec3 Transform::GetPosition() const {
	if (m_parent) {
		return glm::vec3(m_parent->GetLocalToWorldMatrix() * glm::vec4(m_localPosition, 1));
	}
	else {
		return m_localPosition;
	}
}

void Transform::SetPosition(const glm::vec3 &_position) {
	if (m_parent) {
		m_localPosition = glm::vec3(m_parent->GetWorldToLocalMatrix() * glm::vec4(_position, 1));
	}
	else {
		m_localPosition = _position;
	}
	m_dirty = true;
}

void Transform::SetPosition(float x, float y, float z) {
	SetPosition(glm::vec3(x, y, z));
}

glm::quat Transform::GetRotation() const {
	if (m_parent) {
		return m_parent->GetRotation() * m_localRotation;
	}
	else {
		return m_localRotation;
	}
}

void Transform::SetRotation(const glm::quat &_orientation) {
	if (m_parent) {
		m_localRotation = glm::inverse(m_parent->GetRotation()) * _orientation;
	}
	else {
		m_localRotation = _orientation;
	}
	m_dirty = true;
}

void Transform::SetRotation(const glm::vec3 &degrees) {
	SetRotation(glm::quat(glm::radians(degrees)));
}

void Transform::SetRotation(float pitch, float yaw, float roll) {
	SetRotation(glm::vec3(pitch, yaw, roll));
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
		glm::vec3 pos = GetPosition();
		glm::quat rot = GetRotation();
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
