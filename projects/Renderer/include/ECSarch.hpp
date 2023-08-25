#ifndef ECSARCH_HPP
#define ECSARCH_HPP

#include <map>
#include <string>
#include <vector>
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
    std::vector<Component*> components;

public:
    Entity(uint32_t id, std::initializer_list<Component*> components);
    const std::vector<Component*>& getComponents();
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
class EntityManager0
{
    //NSMutableArray* entities;
    //NSMutableDictionary* componentsByClass;
    //uint32_t lowestUnassignedId = 1;

public:
    //EntityManager(NSMutableArray* entities, NSMutableDictionary* componentsByClass);
    //void generateNewId(uint32_t newId);
    //void createEntity(Entity* newEntity);
    //void addComponentToEntity(Entity* entity, Component* component);
    //Component* getComponentOfClass(Class class, Entity* entity);
    //void removeEntity(Entity* entity);
    //NSArray* getAllEntitiesPossesingComponentsOfClass(Class class); // helper method
};


class EntityManager
{
    //std::vector<Component> m_componentPool;
    uint32_t getNewId();
    uint32_t lowestUnassignedId = 1;

public:
    EntityManager();
    ~EntityManager();

    std::map<uint32_t, Entity*> entities;
    std::vector<Component*> components;
    std::vector<System*> systems;

    void update(float timeStep);
    //EntityManager(NSMutableArray* entities, NSMutableDictionary* componentsByClass);
    void addEntity(Entity* newEntity);
    void removeEntity(Entity* entity);
    void addComponentToEntity(Entity* entity, Component* component);
    Component* getComponentOfType(std::string& type, Entity* entity);
    Component* getComponentOfType(const char*  type, Entity* entity);
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