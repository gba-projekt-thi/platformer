#pragma once

#include "bn_vector.h"
#include "cfg.h"
#include "resettable.h"

class ResetRegistry {
   public:
    static ResetRegistry& instance();

    void register_resettable(Resettable* obj);
    void unregister_resettable(Resettable* obj);

    void reset_all();

   private:
    bn::vector<Resettable*, Cfg::Level::Limits::RESETTABLES> _objects;
};