#include "EntitySystem.h"

component_list::component_list(const component **components, unsigned int length) : components(components), length(length) {}

component_list::~component_list() {
	delete[] components;
}

EntitySystem::~EntitySystem() {
	for (auto iterator : entities) {
		for (int i = 0; i < iterator.second.length; i++) {
			const component *component = iterator.second.components[i];
			delete component;
			delete[] iterator.second.components;
		}
	}
	for (Updator *updator : updators) {
		delete updator;
	}
}

entity EntitySystem::create_entity(component_list components) {
	entity entity = entities.size();
	entities.insert(std::pair<unsigned int, component_list>(entity, components));
	return entity;
}

const component * EntitySystem::get_component(entity entity, std::type_info type) {
	component_list components = entities.find(entity)->second;
	for (size_t i = 0; i < components.length; i++) {
		const component *component = components.components[i];
		if (typeid(component) == type) return component;
	}
}

void EntitySystem::add_updator(Updator *updator) {
	updators.push_back(updator);
}

void EntitySystem::update(float dt) {
	for (auto updator : updators) {
		updator->update(dt);
	}
}