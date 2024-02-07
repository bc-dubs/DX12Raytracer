#pragma once
// Ben Coukos-Wiley
// 2/6/2024
// A class to represent an object in the game engine

#include "Transform.h"
#include "Mesh.h"
#include <memory>

class Entity {
public:
	Entity(std::shared_ptr<Mesh> mesh); // add constructor that takes transform or one that takes parent?
	~Entity();

	// Getters
	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<Transform> GetTransform();

	// Setters
	void SetMesh(std::shared_ptr<Mesh> newMesh);

private:
	std::shared_ptr <Transform> transform;
	std::shared_ptr<Mesh> mesh;
};