#ifndef _ENGINE_PHYSICS_MANAGER
#define _ENGINE_PHYSICS_MANAGER
#pragma once

#include "..\core\Matrix.h"
#include "..\core\Vector.h"

#include "Singleton.h"

class PhysicsManager : public Singleton<PhysicsManager>
{

public:

        PhysicsManager() { }
		~PhysicsManager() { Shutdown(); }

		//\brief Stubbed out
		void Startup() {};
		void Shutdown() {};
        void Update(float a_dt);

private:

};

#endif //_ENGINE_PHYSICS_MANAGER