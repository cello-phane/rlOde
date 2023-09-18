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
#include <array>
#include <assert.h>
/* #include <signal.h> */
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <utility>
#define XBOX360_NAME_ID     "Xbox 360 Controller"
#ifdef _WIN32
#ifdef _MSC_VER
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif // MSVC
#define APP_NAME_STR_LEN 80
#endif // _WIN32


bool in_callback = false;
#define ERR_EXIT(err_msg, err_class)                \
  do {                                              \
    if (!demo->suppress_popups)                     \
      MessageBox(NULL, err_msg, err_class, MB_OK);	\
    exit(1);                                        \
  } while (0)
void DbgMsg(char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  vprintf(fmt, va);
  va_end(va);
  fflush(stdout);
}

#include <raylib.h>
#define RLIGHTS_IMPLEMENTATION
#include <rlights.h>

#include <raymath.h>

#include <raylibODE.h>

/*
 * get ODE from https://bitbucket.org/odedevs/ode/downloads/
 *
 * clone ode into the main directory of this project
 *
 * cd into it
 *
 * I'd suggest building it with this configuration
 * ./configure --enable-double-precision --enable-ou --enable-libccd
 * --with-box-cylinder=libccd --with-drawstuff=none --disable-demos
 * --with-libccd=internal
 *
 *

 * and run make, you should then be set to compile this project
 */

// globals in use by nearCallback
dWorldID world;
dJointGroupID contactgroup;

Model box;
Model ball;
Model cylinder;

inline float rndf(float min, float max);
//macro candidate ? marcro's? eek!

float rndf(float min, float max) {  
  return (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * (max - min) + min;
}

//when objects potentially collide this callback is called
//you can rule out certain collisions or use different surface parameters
//depending what object types collide.... lots of flexibility and power here!
#define MAX_CONTACTS 16

static void nearCallback(void *data, dGeomID o1, dGeomID o2) {
  (void)data;
  int i;//to iterate MAX_CONTACTS

  // exit without doing anything if the two bodies are connected by a joint
  dBodyID b1 = dGeomGetBody(o1);
  dBodyID b2 = dGeomGetBody(o2);
  if (b1 == b2)
    return;
  if (b1 && b2 && dAreConnectedExcluding(b1, b2, dJointTypeContact))
    return;

  if (!checkColliding(o1))
    return;
  if (!checkColliding(o2))
    return;
  
  // getting these just so can sometimes be a little bit of a black art!
  dContact contact[MAX_CONTACTS]; // up to MAX_CONTACTS contacts per body-body
  for (i = 0; i < MAX_CONTACTS; i++) {
    contact[i].surface.mode = dContactSlip1 | dContactSlip2 | dContactSoftERP |
      dContactSoftCFM | dContactApprox1;
    contact[i].surface.mu = 1000;
    contact[i].surface.slip1 = 0.0001;
    contact[i].surface.slip2 = 0.001;
    contact[i].surface.soft_erp = 0.5;
    contact[i].surface.soft_cfm = 0.0003;
  
    contact[i].surface.bounce = 0.1;
    contact[i].surface.bounce_vel = 0.1;
  }
  int numc = dCollide(o1, o2, MAX_CONTACTS, &contact[0].geom, sizeof(dContact));
  if (numc) {
    dMatrix3 RI;
    dRSetIdentity(RI);
    for (i = 0; i < numc; i++) {
      dJointID c = dJointCreateContact(world, contactgroup, contact + i);
      dJointAttach(c, b1, b2);
    }
  }
}

int main(int argc, char* argv[]) {
  assert(sizeof(dReal) == sizeof(double));
  srand(time(NULL));
  
  // Initialization
  //-------------------------------------------------------------------------------------
  // Default values
  int numObj = 100;        // Default number of bodies
  int screenWidth = 1920;  // Default screen width
  int screenHeight = 1080; // Default screen height
  
  // Check if command-line arguments are provided
  if (argc >= 2) 
    numObj = atoi(argv[1]);
  if (argc >= 3) 
    screenWidth = atoi(argv[2]);
  if (argc >= 4) 
    screenHeight = atoi(argv[3]);

  // a space can have multiple "worlds" for example you might have different
  // sub levels that never interact, or the inside and outside of a building
  dSpaceID space;

  // create an array of bodies
  dBodyID obj[100];

  SetWindowState(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
  InitWindow(screenWidth, screenHeight, "raylib ODE and a car!");

  // Define the camera to look into our 3d world
  Camera3D camera = {0};
  // Camera position
  camera.position = {25.0f, 15.0f, 0.0f};
  // Camera start looking at point
  camera.target = {0.0f, 0.5f, 1.0f};
  // Camera up vector (rotation towards target)
  camera.up = { 0.0f, 1.0f, 0.0f };
  // Camera field-of-view Y
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;
  
  box = LoadModelFromMesh(GenMeshCube(1, 1, 1));
  ball  = LoadModelFromMesh(GenMeshSphere(.5, 32, 32));
  // alas gen cylinder is wrong orientation for ODE...
  // so rather than muck about at render time just make one the right
  // orientation
  cylinder = LoadModel("data/cylinder.obj");

  Model ground = LoadModel("data/ground_2x.obj");

  // texture the models
  Texture planetTx = LoadTexture("data/planet.png");
  Texture crateTx = LoadTexture("data/crate.png");
  Texture drumTx = LoadTexture("data/drum.png");
  Texture grassTx = LoadTexture("data/grass.png");

  box.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = crateTx;
  ball.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = planetTx;
  cylinder.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = drumTx;
  ground.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = grassTx;
  Shader shader = LoadShader("data/simpleLight.vs", "data/simpleLight.fs");
  // load a shader and set up some uniforms
  shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(shader, "matModel");
  shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");

  // ambient light level
  int amb = GetShaderLocation(shader, "ambient");
  std::array<float,4> shader_vert = {0.2, 0.2, 0.2, 1.0};
  SetShaderValue(shader, amb, &shader_vert,
                 SHADER_UNIFORM_VEC4);

  // models share the same shader
  box.materials[0].shader = shader;
  ball.materials[0].shader = shader;
  cylinder.materials[0].shader = shader;
  ground.materials[0].shader = shader;

  // using 4 point lights, white, red, green and blue
  Light lights[MAX_LIGHTS];
  Vector3 lightpos1 = {-25, 25, 25};
  Vector3 lightpos2 = {-25, 25, -25};
  Vector3 lightpos3 = {-25, 25, -25};
  Vector3 lightpos4 = {-25, 25, 25};
  Color color1 = {128, 128, 128, 255};
  Color color2 = {64, 64, 64, 255};
  Vector3 light_dir_target = {10, 10, 10};
  Color color3 = {0, 255, 0, 255};
  Color color4 = {0, 0, 255, 255};
  lights[0] = CreateLight(LIGHT_POINT, lightpos1, Vector3Zero(), color1, shader);
  lights[1] = CreateLight(LIGHT_POINT, lightpos2, Vector3Zero(), color2, shader);
  lights[2] = CreateLight(LIGHT_DIRECTIONAL, lightpos3, light_dir_target, color3, shader);
  lights[3] = CreateLight(LIGHT_POINT, lightpos4, Vector3Zero(), color4, shader);

  dInitODE2(0); // initialise and create the physics
  dAllocateODEDataForThread(dAllocateMaskAll);

  world = dWorldCreate();
  printf("phys iterations per step %i\n",
         dWorldGetQuickStepNumIterations(world));
  space = dHashSpaceCreate(NULL);
  contactgroup = dJointGroupCreate(0);
  dWorldSetGravity(world, 0, -9.8, 0); // gravity

  dWorldSetAutoDisableFlag(world, 1);
  dWorldSetAutoDisableLinearThreshold(world, 0.05);
  dWorldSetAutoDisableAngularThreshold(world, 0.05);
  dWorldSetAutoDisableSteps(world, 4);

  vehicle *car = CreateVehicle(space, world);
    
  // create some decidedly sub optimal indices!
  // for the ground trimesh
  int nV = ground.meshes[0].vertexCount;
  int *groundInd = static_cast<int*>(RL_MALLOC(nV * sizeof(int)));
  for (int i = 0; i < nV; i++) {
    groundInd[i] = i;
  }

  // static tri mesh data to geom
  dTriMeshDataID triData = dGeomTriMeshDataCreate();
  dGeomTriMeshDataBuildSingle(triData, ground.meshes[0].vertices,
                              3 * sizeof(float), nV, groundInd, nV,
                              3 * sizeof(int));
  dCreateTriMesh(space, triData, NULL, NULL, NULL);

  // create the physics bodies
  for (int i = 0; i < numObj; i++) {
    obj[i] = dBodyCreate(world);
    dGeomID geom;
    dMatrix3 R;
    dMass m;
    float typ = rndf(0, 1);
    if (typ < .25) { //  box
      Vector3 s = {rndf(0.5, 2), rndf(0.5, 2), rndf(0.5, 2)};
      geom = dCreateBox(space, s.x, s.y, s.z);
      dMassSetBox(&m, 1, s.x, s.y, s.z);
    } else if (typ < .5) { //  sphere
      float r = rndf(0.5, 1);
      geom = dCreateSphere(space, r);
      dMassSetSphere(&m, 1, r);
    } else if (typ < .75) { //  cylinder
      float l = rndf(0.5, 2);
      float r = rndf(0.25, 1);
      geom = dCreateCylinder(space, r, l);
      dMassSetCylinder(&m, 1, 3, r, l);
    } else { //  composite of cylinder with 2 spheres
      double l = rndf(.9, 1.5);
      dGeomID geom3 = dCreateSphere(space, l / 2);
      geom = dCreateCylinder(space, 0.25, l);
      dGeomID geom2 = dCreateSphere(space, l / 2);
      dMass m2, m3;
      dMassSetSphere(&m2, 1, l / 2);
      dMassTranslate(&m2, 0, 0, l - 0.25);
      dMassSetSphere(&m3, 1, l / 2);
      dMassTranslate(&m3, 0, 0, -l + 0.25);
      dMassSetCylinder(&m, 1, 3, .25, l);
      dMassAdd(&m2, &m3);
      dMassAdd(&m, &m2);

      dGeomSetBody(geom2, obj[i]);
      dGeomSetBody(geom3, obj[i]);
      dGeomSetOffsetPosition(geom2, 0, 0, l - 0.25);
      dGeomSetOffsetPosition(geom3, 0, 0, -l + 0.25);
    }

    // give the body a random position and rotation
    dBodySetPosition(obj[i], dRandReal() * 10 - 5, 4 + (i / 10), dRandReal() * 10 - 5);
    dRFromAxisAndAngle(R, dRandReal() * 2.0 - 1.0, dRandReal() * 2.0 - 1.0,
                       dRandReal() * 2.0 - 1.0, dRandReal() * M_PI * 2 - M_PI);
    dBodySetRotation(obj[i], R);
    // set the bodies mass and the newly created geometry
    dGeomSetBody(geom, obj[i]);
    dBodySetMass(obj[i], &m);
  }

  float accel = 0;
  float steer = 0;
  /* Vector3 debug = {0}; */
  bool antiSway = true;

  // keep the physics fixed time in step with the render frame
  // rate which we don't know in advance
  double frameTime = 0;
  double physTime = 0;
  const double physSlice = 1.0 / 240.0;
  const int maxPsteps = 6;
  int carFlipped = 0; // number of frames car roll is >90
  //-------------------------------------------------------------------------------------
  //
  // Main game loop
  //
  //-------------------------------------------------------------------------------------
  while (!WindowShouldClose()) {// Detect window close button or ESC key
      //---------------------------------------------------------------------------------
      // Update
      //---------------------------------------------------------------------------------

      // extract just the roll of the car
      // count how many frames its >90 degrees either way
      const dReal *q = dBodyGetQuaternion(car->bodies[0]);
      double z0 = 2.0f * (q[0] * q[3] + q[1] * q[2]);
      double z1 = 1.0f - 2.0f * (q[1] * q[1] + q[3] * q[3]);
      double roll = atan2f(z0, z1);
      // assert(M_PI_2 == M_PI/2);//M_PI_2 is half of M_PI
      // If the car is flipped, it's in a halfway rotated state?
      if (fabs(roll) > (M_PI_2 - 0.001)) {
        carFlipped++;
      } else {
        carFlipped = 0;
      }

      // if the car roll >90 degrees for 100 frames then flip it
      if (carFlipped > 100) {
        unflipVehicle(car);
      }

      accel *= .99;
      if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
        accel += 10;
      if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
        accel -= 10;
      if (accel > 50)
        accel = 50;
      if (accel < -15)
        accel = -15;

      if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
        steer -= .1;
      if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
        steer += .1;
      if ((!IsKeyDown(KEY_RIGHT) || !IsKeyDown(KEY_D)) &&
          (!IsKeyDown(KEY_LEFT) || !IsKeyDown(KEY_A)))
        steer *= .5;
      if (steer > .5)
        steer = .5;
      if (steer < -.5)
        steer = -.5;

      updateVehicle(car, accel, 800.0, steer, 10.0);

      const dReal *cp = dBodyGetPosition(car->bodies[0]);
      double cp_x = cp[0];
      double cp_y = cp[1];
      double cp_z = cp[2];
      Vector3 cam_target = {static_cast<float>(cp_x), static_cast<float>(cp_y), static_cast<float>(cp_z)};
      camera.target = cam_target;

      float lerp = 0.1f;

      dVector3 co;
      dBodyGetRelPointPos(car->bodies[0], -8, 3, 0, co);

      camera.position.x -= (camera.position.x - co[0]) * lerp; // * (1/ft);
      camera.position.y -= (camera.position.y - co[1]) * lerp; // * (1/ft);
      camera.position.z -= (camera.position.z - co[2]) * lerp; // * (1/ft);
      UpdateCamera(&camera, 0);
      
      
      // Levitate the objects
      for (int i = 0; i < numObj; i++) {
        const dReal *pos = dBodyGetPosition(obj[i]);
        if (IsKeyDown(KEY_SPACE)) {
          // apply force if the key Spacebar is held down
          const dReal *v = dBodyGetLinearVel(obj[i]);
          if (v[1] < 10 && pos[1] < 10) { // cap upwards velocity and don't let it get too high
            if (!dBodyIsEnabled(obj[i])) {
              dBodyEnable(obj[i]); // case its gone to sleep
            }
            dMass mass;
            dBodyGetMass(obj[i], &mass);
            // give some object more force than others
            // ??? cast to float both i and numbObj at this divsion
            float f = (6 + ((float)(i / numObj) * 4)) * mass.mass;
            dBodyAddForce(obj[i], rndf(-f, f), f * 10, rndf(-f, f));
          }
        }
        //Fall back down?
        if (pos[1] < -10) {
          dBodySetPosition(obj[i], dRandReal() * 10 - 5, 12 + rndf(1, 2),
                           dRandReal() * 10 - 5);
          dBodySetLinearVel(obj[i], 5, 5, 5);
          dBodySetAngularVel(obj[i], 5, 5, 5);
        }
      }

      // Levitate the car if Key Left Shift is held down
      if (IsKeyDown(KEY_LEFT_SHIFT)) {
        for (int i = 0; i < 6; i++) {
          dMass cm;
          dBodyGetMass(car->bodies[i], &cm);
          float f = (6 + ((float)(i / 6) * 4)) * cm.mass;
          if (i == 3) {
            f += f*0.5;
          }
          dBodyAddForce(car->bodies[i], 0, f * 10, 0);
        }
      }
      
      if (IsKeyPressed(KEY_L)) {
        lights[0].enabled = !lights[0].enabled;
        UpdateLightValues(shader, lights[0]);
      }

      // update the light shader with the camera view position
      SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW],
                     &camera.position.x, SHADER_UNIFORM_VEC3);

      frameTime += GetFrameTime();
      int pSteps = 0;
      physTime = GetTime();

      while (frameTime > physSlice) {
        // check for collisions
        // TODO use 2nd param data to pass custom structure with
        // world and space ID's to avoid use of globals...
        dSpaceCollide(space, 0, &nearCallback);

        // step the world
        dWorldQuickStep(world, physSlice); // NB fixed time step is important
        dJointGroupEmpty(contactgroup);

        frameTime -= physSlice;
        pSteps++;
        if (pSteps > maxPsteps) {
          frameTime = 0;
          break;
        }
      }

      physTime = GetTime() - physTime;

      //---------------------------------------------------------------------------------
      // Draw
      //---------------------------------------------------------------------------------
    Texture2D texXboxPad = LoadTexture("data/xbox.png");

    //--------------------------------------------------------------------------------------

    int gamepad = 0; // which gamepad to display
    BeginDrawing();

    ClearBackground(BLACK);

    BeginMode3D(camera);
    Vector3 ground_vec = {0.0,0.0,0.0,};
    DrawModel(ground, ground_vec, 1, GREEN);

    // NB normally you wouldn't be drawing the collision meshes
    // instead you'd iterrate all the bodies get a user data pointer
    // from the body you'd previously set and use that to look up
    // what you are rendering oriented and positioned as per the
    // body
    drawAllSpaceGeoms(space);
    DrawGrid(400, 1.0f);

    EndMode3D();

    //DrawFPS(10, 20); // can't see it in lime green half the time!!

    if (pSteps > maxPsteps)
      DrawText("WARNING CPU overloaded lagging real time", 200, 20, 20, RED);
    DrawText(TextFormat("%2i FPS", GetFPS()), 10, 20, 20, GREEN);
    DrawText(TextFormat("accel %2.2f", accel), 10, 80, 20, WHITE);
    DrawText(TextFormat("steer %4.4f", steer), 10, 115, 15, WHITE);
    if (!antiSway)
      DrawText("Anti sway bars OFF", 10, 75, 15, RED);
    /* DrawText(TextFormat("debug %4.4f %4.4f %4.4f",debug.x,debug.y,debug.z),
     * 10, 100, 20, WHITE); */
    // DrawText(TextFormat("Phys steps per frame %i", pSteps), 10, 120, 20, WHITE);
    // DrawText(TextFormat("Phys time per frame %i", physTime), 10, 140, 20,
             // WHITE);
    // DrawText(TextFormat("total time per frame %i", frameTime), 10, 160, 20,
             // WHITE);
    DrawText(TextFormat("objects %i", numObj), 10, 35, 10, WHITE);

    DrawText(TextFormat("roll %.4f", fabs(roll)), 10, 130, 15, WHITE);

    const double *cv = dBodyGetLinearVel(car->bodies[0]);
    Vector3 cvs = {
      static_cast<float>(cv[0]),
      static_cast<float>(cv[1]),
      static_cast<float>(cv[2])};
    float vel = Vector3Length(cvs) * 2.23693629f;
    DrawText(TextFormat("mph %.2f", vel), 10, 60, 20, ORANGE);
    DrawText(TextFormat("car x: %.2f\n \t\t y: %.2f\n \t\t z: %.2f\n", cp[0], cp[1], cp[2]), 5, 140, 20, WHITE);
    // printf("%i %i\n",pSteps, numObj);


    // if (IsKeyPressed(KEY_LEFT) && gamepad > 0) gamepad--;
    // if (IsKeyPressed(KEY_RIGHT)) gamepad++;

    // if (IsGamepadAvailable(gamepad))
    //   {
    //     DrawText(TextFormat("GP%d: %s", gamepad, GetGamepadName(gamepad)), 10, 10, 10, BLACK);

    //     if (true)
    //       {
    //         DrawTexture(texXboxPad, 0, 0, DARKGRAY);

    //         // Draw buttons: xbox home
    //         if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_MIDDLE)) DrawCircle(394, 89, 19, RED);

    //         // Draw buttons: basic
    //         if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_MIDDLE_RIGHT)) DrawCircle(436, 150, 9, RED);
    //         if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_MIDDLE_LEFT)) DrawCircle(352, 150, 9, RED);
    //         if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) DrawCircle(501, 151, 15, BLUE);
    //         if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) DrawCircle(536, 187, 15, LIME);
    //         if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) DrawCircle(572, 151, 15, MAROON);
    //         if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_UP)) DrawCircle(536, 115, 15, GOLD);

    //         // Draw buttons: d-pad
    //         DrawRectangle(317, 202, 19, 71, BLACK);
    //         DrawRectangle(293, 228, 69, 19, BLACK);
    //         if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_UP)) DrawRectangle(317, 202, 19, 26, RED);
    //         if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) DrawRectangle(317, 202 + 45, 19, 26, RED);
    //         if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) DrawRectangle(292, 228, 25, 19, RED);
    //         if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) DrawRectangle(292 + 44, 228, 26, 19, RED);

    //         // Draw buttons: left-right back
    //         if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) DrawCircle(259, 61, 20, RED);
    //         if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) DrawCircle(536, 61, 20, RED);

    //         // Draw axis: left joystick

    //         Color leftGamepadColor = BLACK;
    //         if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_THUMB)) leftGamepadColor = RED;
    //         DrawCircle(259, 152, 39, BLACK);
    //         DrawCircle(259, 152, 34, LIGHTGRAY);
    //         DrawCircle(259 + (int)(GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X)*20),
    //                    152 + (int)(GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y)*20), 25, leftGamepadColor);

    //         // Draw axis: right joystick
    //         Color rightGamepadColor = BLACK;
    //         if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_THUMB)) rightGamepadColor = RED;
    //         DrawCircle(461, 237, 38, BLACK);
    //         DrawCircle(461, 237, 33, LIGHTGRAY);
    //         DrawCircle(461 + (int)(GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_X)*20),
    //                    237 + (int)(GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_Y)*20), 25, rightGamepadColor);

    //         // Draw axis: left-right triggers
    //         DrawRectangle(170, 30, 15, 70, GRAY);
    //         DrawRectangle(604, 30, 15, 70, GRAY);
    //         DrawRectangle(170, 30, 15, (int)(((1 + GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_TRIGGER))/2)*70), RED);
    //         DrawRectangle(604, 30, 15, (int)(((1 + GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_TRIGGER))/2)*70), RED);

    //         //DrawText(TextFormat("Xbox axis LT: %02.02f", GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_TRIGGER)), 10, 40, 10, BLACK);
    //         //DrawText(TextFormat("Xbox axis RT: %02.02f", GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_TRIGGER)), 10, 60, 10, BLACK);
    //       }
    EndDrawing();

  }//End While WindowShouldClose

  //----------------------------------------------------------------------------------

  //-------------------------------------------------------------------------------------
  // De-Initialization
  //-------------------------------------------------------------------------------------
  UnloadModel(box);
  UnloadModel(ball);
  UnloadModel(cylinder);
  UnloadModel(ground);
  UnloadTexture(drumTx);
  UnloadTexture(planetTx);
  UnloadTexture(crateTx);
  UnloadTexture(grassTx);
  UnloadShader(shader);

  RL_FREE(car);

  RL_FREE(groundInd);
  dGeomTriMeshDataDestroy(triData);

  dJointGroupEmpty(contactgroup);
  dJointGroupDestroy(contactgroup);
  dSpaceDestroy(space);
  dWorldDestroy(world);
  dCloseODE();
  CloseWindow(); // Close window and OpenGL context
  //-------------------------------------------------------------------------------------
    
  return 0;
}
