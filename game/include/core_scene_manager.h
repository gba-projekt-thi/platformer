#pragma once

#include "bn_unique_ptr.h"

#include "core_scene.h"

namespace core {

class SceneManager {
   public:
    static SceneManager& instance() {
        static SceneManager manager;
        return manager;
    }

    void set_next_scene(bn::unique_ptr<Scene> next_scene) {
        _next_scene = bn::move(next_scene);
    }

    void clear() {
        _current_scene.reset();
        _next_scene.reset();
    }

    bool update() {
        if (_next_scene) {
            _current_scene = bn::move(_next_scene);
            _current_scene->init();
        }

        if (! _current_scene) {
            return false;
        }

        _current_scene->update();
        return true;
    }

   private:
    bn::unique_ptr<Scene> _current_scene;
    bn::unique_ptr<Scene> _next_scene;
};

}  // namespace core