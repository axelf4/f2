#include <vector>
#include <map>
#include <typeinfo>

struct component { virtual ~component() = 0; };

class component_list {
public:
	const component **components;
	const unsigned int length;

	component_list(const component **components, unsigned int length);
	~component_list();
};

#define ADD_COMPONENT(name, type) 
#define ADD_ENTITY_TYPE(name) 

typedef unsigned int entity;

class Updator {
public:
	virtual ~Updator() = 0;

	virtual void update(float dt) = 0;
};

class EntitySystem {
	// TODO make into array to support deletion
	std::map<entity, component_list> entities;
	std::vector<Updator*> updators;

public:

	~EntitySystem();

	entity create_entity(component_list components);

	// void destroy_entity(entity entity);

	const component * get_component(entity entity, std::type_info type);

	void add_updator(Updator *updator);

	void update(float dt);

};