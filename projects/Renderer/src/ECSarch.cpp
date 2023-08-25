
#include <iostream>

#include "ECSarch.hpp"


Component::Component(std::string& type) : type(type) { }
Component::Component(const char*  type) : type(type) { }
Component::~Component() { }

Entity::Entity(uint32_t id, std::initializer_list<Component*>& components)
	: id(id), resourceHandle(this) 
{ 
	for (std::initializer_list<Component*>::iterator it = components.begin(); it != components.end(); it++)
		this->components.push_back(std::shared_ptr<Component>(*it));
};

Entity::~Entity() { }

const std::vector<std::shared_ptr<Component>>& Entity::getComponents() { return components; };

void Entity::addComponent(Component* component) { components.push_back(std::shared_ptr<Component>(component)); };

Component* Entity::getComponentOfType(std::string& type)
{ 
	for (unsigned i = 0; i < components.size(); i++)
		if (type == components[i]->type) return components[i].get();

	return nullptr;
}

Component* Entity::getComponentOfType(const char* type)
{
	for (unsigned i = 0; i < components.size(); i++)
		if (type == components[i]->type) return components[i].get();

	return nullptr;
}

EntityManager::EntityManager(std::initializer_list<Entity*> entities, std::initializer_list<Component*> sComponents, std::initializer_list<System*> systems)
{
	for (std::initializer_list<Entity*>::iterator it = entities.begin(); it != entities.end(); it++)
		this->entities[(*it)->id] = * it;

	for (std::initializer_list<Component*>::iterator it = sComponents.begin(); it != sComponents.end(); it++)
		this->sComponents.push_back(std::shared_ptr<Component>(*it));

	this->systems = systems;
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

void EntityManager::addSingletonComponent(Component* component)
{
	sComponents.push_back(std::shared_ptr<Component>(component));
}

uint32_t EntityManager::addEntity(std::initializer_list<Component*>& components)
{
	uint32_t newId = getNewId();
	entities[newId] = new Entity(newId, components);
	return newId;
}

void EntityManager::removeEntity(uint32_t entityId)
{
	if (entities.find(entityId) != entities.end())
	{
		delete entities[entityId];
		entities.erase(entityId);
	}
}

void EntityManager::addComponentToEntity(uint32_t entityId, Component* component)
{
	entities[entityId]->addComponent(component);
}

Component* EntityManager::getComponentOfType(std::string& type, uint32_t entityId)
{
	return entities[entityId]->getComponentOfType(type);
}

Component* EntityManager::getComponentOfType(const char* type, uint32_t entityId)
{
	return entities[entityId]->getComponentOfType(type);
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
