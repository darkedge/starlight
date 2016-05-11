#pragma once
#include <vectormath/scalar/cpp/vectormath_aos.h>

class Transform
{
public:
	// Transforms position from local space to world space.
	Vectormath::Aos::Vector3 TransformPoint(const Vectormath::Aos::Vector3 &point);
	// Transforms direction from local space to world space.
	Vectormath::Aos::Vector3 TransformDirection(const Vectormath::Aos::Vector3 &direction) const;
	// Transforms vector from local space to world space.
	Vectormath::Aos::Vector3 TransformVector(const Vectormath::Aos::Vector3 &vector);

	// Returns the matrix that transforms a point from local space into world space.
	Vectormath::Aos::Matrix4 GetLocalToWorldMatrix();
	// Returns the matrix that transforms a point from world space into local space.
	Vectormath::Aos::Matrix4 GetWorldToLocalMatrix();

	// Create a view matrix.
	Vectormath::Aos::Matrix4 GetViewMatrix();

	// Returns the position of the transform relative to the parent transform.
	Vectormath::Aos::Vector3 GetLocalPosition() const { return m_localPosition; }
	// Returns the rotation of the transform relative to the parent transform's rotation.
	Vectormath::Aos::Quat GetLocalRotation() const { return m_localRotation; }
	// Returns the scale of the transform relative to the parent.
	Vectormath::Aos::Vector3 GetLocalScale() const { return m_localScale; }

	// Returns the position of the transform in world space.
	Vectormath::Aos::Vector3 GetPosition() const;
	// Returns the rotation of the transform in world space stored as a Quaternion.
	Vectormath::Aos::Quat GetRotation() const;

	// http://docs.unity3d.com/ScriptReference/Transform-lossyScale.html
	//Vectormath::Aos::Vector3 GetLossyScale() const;

	// Returns the rotation as Euler angles in radians relative to the parent transform's rotation.
	//Vectormath::Aos::Vector3 GetLocalEulerAngles() const { return glm::eulerAngles(m_localRotation); }

	Vectormath::Aos::Vector3 Forward() const {
		return Vectormath::Aos::rotate(m_localRotation, Vectormath::Aos::Vector3(0, 0, 1));
	}

	Vectormath::Aos::Vector3 Up() const {
		return Vectormath::Aos::rotate(m_localRotation, Vectormath::Aos::Vector3(0, 1, 0));
	}

	Vectormath::Aos::Vector3 Right() const {
		return Vectormath::Aos::rotate(m_localRotation, Vectormath::Aos::Vector3(1, 0, 0));
	}

	// Sets the position of the transform relative to the parent transform.
	void SetLocalPosition(float x, float y, float z);
	void SetLocalPosition(const Vectormath::Aos::Vector3 &_position);

	// Sets the rotation of the transform relative to the parent transform's rotation.
	void SetLocalRotation(float pitch, float yaw, float roll);
	void SetLocalRotation(const Vectormath::Aos::Vector3 &degrees);
	void SetLocalRotation(const Vectormath::Aos::Quat &_orientation);

	// Sets the scale of the transform relative to the parent.
	void SetLocalScale(float xyz);
	void SetLocalScale(float x, float y, float z);
	void SetLocalScale(const Vectormath::Aos::Vector3 &_scale);

	// Sets the position of the transform in world space.
	void SetPosition(float x, float y, float z);
	void SetPosition(const Vectormath::Aos::Vector3 &_position);

	// Sets the rotation of the transform in world space.
	void SetRotation(float pitch, float yaw, float roll);
	void SetRotation(const Vectormath::Aos::Vector3 &degrees);
	void SetRotation(const Vectormath::Aos::Quat &_orientation);

	// Hierarchy
	Transform *GetParent() const { return m_parent; }
	void SetParent(Transform *parent, bool worldPositionStays = true);
	void RemoveChild(Transform *transform); // constness?
	void AddChild(Transform *transform); // constness?
	void Destroy();

protected:
	Transform *m_parent = nullptr;
	Transform *m_firstChild = nullptr;
	Transform *m_nextSibling = nullptr;

	Vectormath::Aos::Matrix4 m_localMatrix;
	Vectormath::Aos::Vector3 m_localPosition = Vectormath::Aos::Vector3(0);
	Vectormath::Aos::Quat m_localRotation = Vectormath::Aos::Quat(1, 0, 0, 0); // ctor = wxyz; layout = xyzw
	Vectormath::Aos::Vector3 m_localScale = Vectormath::Aos::Vector3(1);

	bool m_dirty = true;
};

