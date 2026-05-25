#pragma once

class Resettable {
   public:
    virtual ~Resettable() = default;
    virtual void reset() = 0;
};