/*
 * Copyright (c) 2021 Chris Camacho (codifies -  http://bedroomcoders.co.uk/)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "raylib.h"
#include "raymath.h"

#include "ode/ode.h"
#include "ode/objects.h"

typedef struct Timer {
  double startTime;   // Start time (seconds)
  double lifeTime;    // Lifetime (seconds)
} Timer;

void StartTimer(Timer *timer, double lifetime);
bool TimerDone(Timer timer);
double GetElapsed(Timer timer);

inline float rndf(float min, float max);
//macro candidate ? marcro's? eek!

float rndf(float min, float max) {
  return (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * (max - min) + min;
}

// TODO extern for now - need to add function set these and keep them here...
extern Model ball;
extern Model box;
extern Model cylinder;

// 0 chassis / 1-4 wheel / 5 anti roll counter weight
typedef struct vehicle {
    dBodyID bodies[6];
    dGeomID geoms[6];
    dJointID joints[6];
} vehicle;

typedef struct geomInfo {

    bool collidable;
} geomInfo ;

typedef enum GAME_STATE {
  PAUSED = 0,
  UNPAUSED,
} GAME_STATE;

void drawXboxOverlay(int gamepad, Texture2D &texXboxPad);
void rayToOdeMat(Matrix* mat, dReal* R);
void odeToRayMat(const dReal* R, Matrix* matrix);
void drawAllSpaceGeoms(dSpaceID space);
void drawGeom(dGeomID geom);
vehicle* CreateVehicle(dSpaceID space, dWorldID world);
void updateVehicle(vehicle *car, float accel, float maxAccelForce,
                    float steer, float steerFactor);
void unflipVehicle (vehicle *car);
bool checkColliding(dGeomID g);
void teleportVehicle(vehicle *car, dReal *position);
dBodyID* createObjects(int &numObj, dWorldID &world, dBodyID *obj, dSpaceID &space);
