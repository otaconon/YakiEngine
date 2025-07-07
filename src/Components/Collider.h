#pragma once

struct SphereCollider
{
	SphereCollider(const float radius, const bool isTrigger = false)
		: radius{radius},
		isTrigger{isTrigger}
	{}

	float radius;
	bool isTrigger;
};

struct BoxCollider
{
	BoxCollider(const glm::vec3 dims, const bool isTrigger = false)
		: dims(dims),
		isTrigger(isTrigger)
	{}

	glm::vec3 dims;
	bool isTrigger;
};


