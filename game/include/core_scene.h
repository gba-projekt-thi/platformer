#pragma once

namespace core {

class Scene {
   public:
    virtual ~Scene() = default;

    virtual void init() = 0;
    virtual void update() = 0;
};

}  // namespace core