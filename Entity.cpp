#include "Entity.h"

Entity::Entity(std::shared_ptr<Mesh> mesh)
    : mesh(mesh)
{
    transform = std::make_shared<Transform>(Transform()); // The entity starts at (0, 0, 0) by default
}

Entity::~Entity()
{
}

std::shared_ptr<Mesh> Entity::GetMesh()
{
    return mesh;
}

std::shared_ptr<Transform> Entity::GetTransform()
{
    return transform;
}

void Entity::SetMesh(std::shared_ptr<Mesh> newMesh)
{
    mesh = newMesh;
}