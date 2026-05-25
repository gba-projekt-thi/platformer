#include "reset_registry.h"

ResetRegistry& ResetRegistry::instance() {
    static ResetRegistry registry;
    return registry;
}

void ResetRegistry::register_resettable(Resettable* obj) {
    _objects.push_back(obj);
}

void ResetRegistry::unregister_resettable(Resettable* obj) {
    for (int i = 0; i < _objects.size(); ++i) {
        if (_objects[i] == obj) {
            _objects.erase(_objects.begin() + i);
            break;
        }
    }
}

void ResetRegistry::reset_all() {
    for (Resettable* obj : _objects) {
        obj->reset();
    }
}