#ifndef ECSARCH_HPP
#define ECSARCH_HPP

#include <map>
//#include <list>
#include <string>
#include <vector>
#include <memory>
#include <initializer_list>


struct Component;
class  Entity;
class  EntityManager;
class  System;
class  EntityFactory;


/// It stores state data (fields) and have no behavior (no methods).
struct Component
{
    Component(std::string& type);
    Component(const char* type);
    virtual ~Component();

    const std::string type;
};

/// An ID associated with a set of components.
class Entity
{
    std::vector<std::shared_ptr<Component>> components;

public:
    Entity(uint32_t id, std::initializer_list<Component*>& components);
    ~Entity();

    const std::vector<std::shared_ptr<Component>>& getComponents();
    void addComponent(Component* component);
    Component* getComponentOfType(std::string& type);
    Component* getComponentOfType(const char*  type);

    const uint32_t id;
    const Entity* resourceHandle;
};


/// It has behavior (methods) and have no state data (no fields). To each system corresponds a set of components. The systems iterate through their components performing operations (behavior) on their state.
class System
{
    EntityManager* entityManager;

public:
    System(EntityManager* entityManager) : entityManager(entityManager) { };
    virtual ~System() { };

    virtual void update(float timeStep) = 0;
};


/// Acts as a "database", where you look up entities and get their list of components.
class EntityManager
{
    //std::vector<Component> m_componentPool;
    uint32_t getNewId();
    uint32_t lowestUnassignedId = 1;

    std::map<uint32_t, Entity*> entities;
    std::vector<std::shared_ptr<Component>> sComponents;     // singleton components. Type shared_ptr prevents singleton components to be deleted during entity destruction.
    std::vector<System*> systems;

public:
    EntityManager(std::initializer_list<Entity*> entities, std::initializer_list<Component*> sComponents, std::initializer_list<System*> systems);
    EntityManager();
    ~EntityManager();

    void update(float timeStep);
    void addSingletonComponent(Component* component);
    uint32_t addEntity(std::initializer_list<Component*>& components);
    void removeEntity(uint32_t entityId);
    void addComponentToEntity(uint32_t entityId, Component* component);
    Component* getComponentOfType(std::string& type, uint32_t entityId);
    Component* getComponentOfType(const char*  type, uint32_t entityId);
    void getAllEntitiesPossesingComponentsOfType(std::string& type, std::vector<Entity*> dest); // helper method
    void getAllEntitiesPossesingComponentsOfType(const char*  type, std::vector<Entity*> dest); // helper method
};


class EntityFactory
{
    EntityManager* entityManager;

public:
    EntityFactory(EntityManager* entityManager) : entityManager(entityManager) { };

    //Entity* createHumanPlayer();
    //Entity* createAIPlayer();
    //Entity* createBasicMonster();
};

#endif