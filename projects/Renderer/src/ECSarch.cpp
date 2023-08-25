
#include <iostream>

#include "ECSarch.hpp"


Component::Component(std::string& type) : type(type) { }
Component::Component(const char*  type) : type(type) { }
Component::~Component() { }

Entity::Entity(uint32_t id, std::initializer_list<Component*> components)
	: id(id), components(components), resourceHandle(this) { };

const std::vector<Component*>& Entity::getComponents() { return components; };

void Entity::addComponent(Component* component) { components.push_back(component); };

Component* Entity::getComponentOfType(std::string& type)
{ 
	for (unsigned i = 0; i < components.size(); i++)
		if (type == components[i]->type) return components[i];

	return nullptr;
}

Component* Entity::getComponentOfType(const char* type)
{
	for (unsigned i = 0; i < components.size(); i++)
		if (type == components[i]->type) return components[i];

	return nullptr;
}

EntityManager::EntityManager() { }

EntityManager::~EntityManager() { };

uint32_t EntityManager::getNewId()
{
	if (lowestUnassignedId < UINT32_MAX) return lowestUnassignedId++;
	
	for (uint32_t i = 1; i < UINT32_MAX; ++i)
		if (entities.find(i) == entities.end())
			return i;

	std::cout << "ERROR: No available IDs!" << std::endl;
	return 0;
}

void EntityManager::update(float timeStep)
{
	for (System* s : systems)
		s->update(timeStep);
}

void EntityManager::addEntity(Entity* newEntity)
{
	entities[getNewId()] = newEntity;
}

void EntityManager::removeEntity(Entity* entity)
{
	if (entities.find(entity->id) != entities.end())
	{
		delete entities[entity->id];
		entities.erase(entity->id);
	}
}

void EntityManager::addComponentToEntity(Entity* entity, Component* component)
{
	entity->addComponent(component);
}

Component* EntityManager::getComponentOfType(std::string& type, Entity* entity)
{
	return entity->getComponentOfType(type);
}

Component* EntityManager::getComponentOfType(const char* type, Entity* entity)
{
	return entity->getComponentOfType(type);
}

void EntityManager::getAllEntitiesPossesingComponentsOfType(std::string& type, std::vector<Entity*> dest)
{
	for (unsigned i = 0; i < entities.size(); i++)
		if (entities[i]->getComponentOfType(type))
			dest.push_back(entities[i]);
}

void EntityManager::getAllEntitiesPossesingComponentsOfType(const char* type, std::vector<Entity*> dest)
{
	for (unsigned i = 0; i < entities.size(); i++)
		if (entities[i]->getComponentOfType(type))
			dest.push_back(entities[i]);
}
