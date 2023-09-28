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
#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

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
dBodyID *objInGameInitd = nullptr;
int totalObjCount = 0;
int newObjCount = 0;

//when objects potentially collide this callback is called
//you can rule out certain collisions or use different surface parameters
//depending what object types collide.... lots of flexibility and power here!
#define MAX_CONTACTS 16

static void nearCallback([[maybe_unused]] void *data, dGeomID o1, dGeomID o2) {
  data = nullptr;
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
    contact[i].surface.mu = 2000;
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

double stackTime = 0;
int main(int argc, char *argv[]) {
  int IsDrawingInfo = 1; // toggle with this boolean integer
  int DrawInfoState = 0; //0, 1, 2 represent [none, all, mph+fps]

  bool IsDrawingXboxOverlay = false;
  if (IsGamepadAvailable(0)) {
#define XBOX360_NAME_ID     "Xbox 360 Controller"
  }
  int gamepad = 0; // which gamepad to display

  assert(sizeof(dReal) == sizeof(double));
  srand(time(NULL));

  // Initialization
  //-------------------------------------------------------------------------
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
  camera.fovy = 65.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  box = LoadModelFromMesh(GenMeshCube(1, 1, 1));
  ball  = LoadModelFromMesh(GenMeshSphere(.5, 32, 32));
  // alas gen cylinder is wrong orientation for ODE...
  // so rather than muck about at render time just make one the right
  // orientation
  cylinder = LoadModel("data/cylinder.obj");

  Model ground = LoadModel("data/ground_2x.obj");

  Texture2D texXboxPad = LoadTexture("data/xbox.png");
  // texture the models
  Texture planetTx = LoadTexture("data/planet.png");
  Texture crateTx = LoadTexture("data/crate.png");
  Texture drumTx = LoadTexture("data/drum.png");
  Texture grassTx = LoadTexture("data/grass.png");

  box.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = crateTx;
  ball.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = planetTx;
  cylinder.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = drumTx;
  ground.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = grassTx;
  SetTextureFilter(grassTx, TEXTURE_FILTER_TRILINEAR);
  Shader shader = LoadShader("data/simpleLight.vs", "data/simpleLight.fs");
  // load a shader and set up some uniforms
  shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(shader, "matModel");
  shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");

  // ambient light level
  int amb = GetShaderLocation(shader, "ambient");
  std::array<float,4> shader_vert = {0.2, 0.2, 0.2, 1.0};
  SetShaderValue(shader, amb, &shader_vert,  SHADER_UNIFORM_VEC4);

  // models share the same shader
  box.materials[0].shader = shader;
  ball.materials[0].shader = shader;
  cylinder.materials[0].shader = shader;
  ground.materials[0].shader = shader;

  // using 4 point lights, white, red, green and blue
  Light lights[MAX_LIGHTS];
  Vector3 lightpos1 = {-25, 25,  25};
  Vector3 lightpos2 = {-25, 25, -25};
  Vector3 lightpos3 = {-25, 25, -25};
  Vector3 lightpos4 = {-25, 25,  25};
  Vector3 light_dir_target = {10, 10, 10};
  Color color1 = {128, 128, 128, 255};
  Color color2 = {64,   64,  64, 255};
  Color color3 = {0,   255,   0, 255};
  Color color4 = {0,     0, 255, 255};
  lights[0] = CreateLight(LIGHT_POINT, lightpos1,
                          Vector3Zero(),    color1, shader);
  lights[1] = CreateLight(LIGHT_POINT, lightpos2,
                          Vector3Zero(),    color2, shader);
  lights[2] = CreateLight(LIGHT_DIRECTIONAL, lightpos3,
                          light_dir_target, color3, shader);
  lights[3] = CreateLight(LIGHT_POINT, lightpos4,
                          Vector3Zero(),    color4, shader);

  dInitODE2(0); // initialise and create the physics
  dAllocateODEDataForThread(dAllocateMaskAll);

  world = dWorldCreate();
  // printf("phys iterations per step %i\n",
  // dWorldGetQuickStepNumIterations(world));
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
  for (int i = 0; i < nV; i++)
    groundInd[i] = i;

  // static tri mesh data to geom
  dTriMeshDataID triData = dGeomTriMeshDataCreate();
  dGeomTriMeshDataBuildSingle(triData, ground.meshes[0].vertices,
                              3 * sizeof(float), nV, groundInd, nV,
                              3 * sizeof(int));
  dCreateTriMesh(space, triData, NULL, NULL, NULL);
  // This used to be dBodyID obj[numObj];
  // We now use the function included in this source file
  dBodyID *obj = createObjects(numObj, world, obj, space);
  totalObjCount += numObj;

  float   paused_accel = 0.0f;
  float   paused_steer = 0.0f;
  double  paused_roll  = 0.0;

  float accel = 0;
  float steer = 0;
  Vector3 debug = {0};
  bool antiSway = true;
  bool teleporting = false;
  // keep the physics fixed time in step with the render frame
  // rate which we don't know in advance
  double frameTime = 0;
  Timer *gameTimer = new Timer;
  auto game_lifetime = -1;
  const double physSlice = 1.0 / 240.0;
  const int maxPsteps = 6;
  int carFlipped = 0; // number of frames car roll is >90

  //-------------------------------------------------------------------------
  //
  // Main game loop
  //
  //-------------------------------------------------------------------------
  GAME_STATE gs = UNPAUSED;
  DisableCursor(); // Disable mouse cursor
  double elapsedTime;

  while (!WindowShouldClose()) { // Detect window close button or ESC key
    //-----------------------------------------------------------------------
    // Update
    //-----------------------------------------------------------------------

    // extract just the roll of the car
    // count how many frames its >90 degrees either way
    const dReal *q = dBodyGetQuaternion(car->bodies[0]);
    double z0 = 2.0f * (q[0] * q[3] + q[1] * q[2]);
    double z1 = 1.0f - 2.0f * (q[1] * q[1] + q[3] * q[3]);
    double roll = atan2f(z0, z1);
    
    const dReal *cp = dBodyGetPosition(car->bodies[0]);
    // π/2 = 90 degrees = approximately 1.570796 radians
    // If the car is on its side - and roll is approx 1.0 radians, it is flipped
    if (fabs(roll) > (M_PI_2 - 0.8)) {
      carFlipped++;
    } else {
      carFlipped = 0;
    }
    Vector3 cam_target = {static_cast<float>(cp[0]), static_cast<float>(cp[1]),
                          static_cast<float>(cp[2]) };

    updateVehicle(car, accel, 800.0, steer, 10.0);

    camera.target = cam_target;

    float lerp = 0.1f;

    dVector3 co;
    dBodyGetRelPointPos(car->bodies[0], -8, 3, 0, co);

    camera.position.x -= (camera.position.x - co[0]) * lerp; // * (1/ft);
    camera.position.y -= (camera.position.y - co[1]) * lerp; // * (1/ft);
    camera.position.z -= (camera.position.z - co[2]) * lerp; // * (1/ft);
    UpdateCamera(&camera, 0);

    // update the light shader with the camera view position
    SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW],
                   &camera.position.x, SHADER_UNIFORM_VEC3);

    frameTime += GetFrameTime();
    int pSteps = 0;
    
    StartTimer(gameTimer, game_lifetime);
    elapsedTime += 10000 * (GetElapsed(*gameTimer));

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
    
    //physTime = GetTime() - physTime;
    
    if (!IsGamepadAvailable(gamepad)) //gamepad is off, stop drawing the overlay
      IsDrawingXboxOverlay = false;

    /*-----------------------------------------------------------------------
    // If Game is Unpaused/Playing
    // -------------------------------------------------------------------- */
    if (gs == UNPAUSED) {
      // if the car roll >90 degrees, for 150 frames, is relatively on ground
      if (carFlipped > 150 && cp[1] < 4.0f && !teleporting)
        unflipVehicle(car);

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

      if (IsGamepadAvailable(gamepad)) {
        float lstick_lr = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X);
        float lstick_ud = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y);
        float rstick_lr = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_X);
        float rstick_ud = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y);
        if (lstick_ud < 0)
          accel += 10;
        if (lstick_ud > 0)
          accel -= 10;
        if (accel > 50)
          accel = 50;
        if (accel < -15)
          accel = -15;
        if (rstick_lr > 0)
          steer -= .05;
        if (rstick_lr < 0)
          steer += .05;
        if (rstick_lr == 0)
          steer *= .5;
        if (steer > .5)
          steer = .5;
        if (steer < -.5)
          steer = -.5;
      }

      // Levitate the objects
      if (obj != nullptr) {
        for (int i = 0; i < numObj; i++) {
          const dReal *pos = dBodyGetPosition(obj[i]);
          if (IsKeyDown(KEY_SPACE) ||
              (IsGamepadAvailable(gamepad) &&
               IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_2)))
            {
              // apply force if the key Spacebar is held down
              const dReal *v = dBodyGetLinearVel(obj[i]);
              if (v[1] < 10 && pos[1] < 10) { // cap upwards velocity
                if (!dBodyIsEnabled(obj[i])) {
                  dBodyEnable(obj[i]); // case its gone to sleep
                }
                dMass mass;
                dBodyGetMass(obj[i], &mass);
                // give some object more force than others
                float f = (6 + (static_cast<float>(i / numObj) * 4)) * mass.mass;
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
      }

      // Levitate the car if Key Left Shift is held down
      if (IsKeyDown(KEY_LEFT_SHIFT) ||
          (IsGamepadAvailable(gamepad) &&
           IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_TRIGGER_2))) {
        if (!teleporting) { //if not already offgrid/teleporting
          for (int i = 0; i < 6; i++) {
            dMass cm;
            dBodyGetMass(car->bodies[i], &cm);
            float f = (6 + (static_cast<float>(i / 6) * 4)) * cm.mass;
            if (i == 3)
              f += f*0.5;
            dBodyAddForce(car->bodies[i], 0, f * 10, 0);
          }
        }
      }

      // Fallen off the ledge or flown too high
      if (cp[1] < -10.0 || cp[1] > 30.0) {
        stackTime = elapsedTime + 0.2;
        teleporting = true;
        double init_position_z = abs(cp[2]) > 240.0f ? 60.0f : cp[2];
        double init_position_x = abs(cp[0]) > 240.0f ? 8.0f : cp[0];
        // TODO: The init_pos y should be set relative to ground mesh Y below car
        dVector3 init_position = {init_position_x, 3.6, init_position_z};
        // init_position becomes the exit position if still on ground mesh
        teleportVehicle(car, init_position);
        // to enable the movement after landing after a teleport
        for (int i = 0; i < 2 && i > 2 && i < 6; i++) {
          dBodyAddForce(car->bodies[i], 0.0f, -80.0f, 0.0f);
          dBodyEnable(car->bodies[i]);
        }
        if (carFlipped)
          unflipVehicle(car);
      }
      if (elapsedTime > stackTime) {
          teleporting = false;
        }
      // Spawn new objects 10 at a time(For now..)
      if (IsKeyPressed(KEY_F2)) {
        newObjCount = 10;
        totalObjCount += newObjCount;
        objInGameInitd = createObjects(newObjCount, world, obj, space);
      }
    }

    // After Paused or Unpaused is initiated
    if (IsKeyPressed(KEY_F10) ||
        IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_MIDDLE_RIGHT))
      {
      if (gs == UNPAUSED) { // Pause it
        gs = PAUSED;
        paused_accel = accel;
        paused_steer = steer;
        paused_roll = roll;
        for (int i = 0; i < numObj; i++) {
          if(dBodyIsEnabled(obj[i]))
            dBodyDisable(obj[i]);
        }
        for (int i = 0; i < 6; i++)
          dBodyDisable(car->bodies[i]);
      } else { // Unpause it
        gs = UNPAUSED;
        accel = paused_accel;
        steer = paused_steer;
        roll  = paused_roll;
        for (int i = 0; i < numObj; i++) {
          if(!dBodyIsEnabled(obj[i]))
            dBodyEnable(obj[i]);
        }
        for (int i = 0; i < 6; i++) {
          dBodyEnable(car->bodies[i]);
        }
      }
    }

    /*-----------------------------------------------------------------------
    // If Game is Paused
    // -------------------------------------------------------------------- */
    if (gs == PAUSED) {
      if (IsGamepadAvailable(gamepad) // If gamepad is on and connected
          && IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_MIDDLE_LEFT))
      {
        // "select" is pressed -wait one sec- to delay the over-reactive xbox
        // ctrls alternate [on : off] to draw overlay
        _sleep(1);
        IsDrawingXboxOverlay = !IsDrawingXboxOverlay;
      }
      if (IsKeyPressed(KEY_O) ||      // If O is pressed
          (IsGamepadAvailable(gamepad) && // or if xbox Y pressed while paused
           IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_UP)))
        // Toggle between the states to draw info
        DrawInfoState = IsDrawingInfo ? (DrawInfoState + 1) % 3 : DrawInfoState;

      // Lights and Xbox overlay buttons
      if (IsKeyPressed(KEY_L) ||
          (IsGamepadAvailable(gamepad) &&
           IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)))
      {
        lights[0].enabled = !lights[0].enabled;
        UpdateLightValues(shader, lights[0]);
      }
    }

    //-----------------------------------------------------------------------
    // Draw
    //-----------------------------------------------------------------------
    BeginDrawing();

    ClearBackground(BLACK);

    BeginMode3D(camera);
    Vector3 ground_vec = {0.0,0.0,0.0,};
    DrawModel(ground, ground_vec, 1.0, Color{GREEN});

    // NB normally you wouldn't be drawing the collision meshes
    // instead you'd iterate all the bodies get a user data pointer
    // from the body you'd previously set and use that to look up
    // what you are rendering oriented and positioned as per the
    // body
    drawAllSpaceGeoms(space);
    // Tile grid on the z-plane
    // DrawGrid(501, 1.f);

    EndMode3D();

    if (DrawInfoState > 0) {
      const double *cv = dBodyGetLinearVel(car->bodies[0]);
      Vector3 cvs = {
      static_cast<float>(cv[0]),
      static_cast<float>(cv[1]),
      static_cast<float>(cv[2])};
      float vel = Vector3Length(cvs) * 2.23693629f;
      DrawText(TextFormat("%2i FPS", GetFPS()), 10, 12,
               20, GREEN);
      DrawText(TextFormat("mph %.2f", vel), 10, 35,
               35, ORANGE);
      if (DrawInfoState > 1) {
        if (pSteps > maxPsteps)
          DrawText("WARNING CPU overloaded lagging real time", 200, 20,
                   20, RED);
        DrawText(TextFormat("accel %2.2f", accel), 10, 70,
                 20, WHITE);
        DrawText(TextFormat("steer %4.4f", steer), 10, 105,
                 15, WHITE);
        // if (antiSway) {
        //   DrawText("Anti sway bars ON", 10, 88, 15, WHITE);
        // }
        // else if (!antiSway) {
        //   DrawText("Anti sway bars OFF", 10, 88, 15, PINK);
        // }
        DrawText(TextFormat("objects %i", totalObjCount), 10, 27,
                 10, WHITE);
        DrawText(TextFormat("roll %.4f", fabs(roll)), 10, 120,
                 15, WHITE);
        DrawText(TextFormat("car x: %.2f\n \t\t y: %.2f\n \t\t z: %.2f\n",
                            cp[0], cp[1], cp[2]), 10, 137, 15, WHITE);
        // DrawText(TextFormat("Timer %.6f", elapsedTime*10000), 10, 210,
                 // 20, WHITE);
      }
    }
    if (IsDrawingXboxOverlay)
      drawXboxOverlay(gamepad, texXboxPad);

    if (gs == PAUSED) {
      DrawText(TextFormat("PAUSED"), (screenWidth / 2) - 120,
               (screenHeight / 2), 60, RED);
      if(!IsGamepadAvailable(gamepad)) {
        DrawText(TextFormat("O \t -> top left info"), (screenWidth / 2) - 100,
                 (screenHeight / 2) + 50, 20, YELLOW);
        DrawText(TextFormat("L \t -> on/off lights"), (screenWidth / 2) - 100,
                 (screenHeight / 2) + 70, 20, YELLOW);
      }
      else if(IsGamepadAvailable(gamepad)) {
        DrawText(TextFormat("Y \t -> top left info"), (screenWidth / 2) - 100,
                 (screenHeight / 2) + 50, 20, YELLOW);
        DrawText(TextFormat("X \t -> on/off lights"), (screenWidth / 2) - 100,
               (screenHeight / 2) + 70, 20, YELLOW);
        DrawText(TextFormat("[SELECT] \t -> controller overlay"),
                 (screenWidth / 2) - 183, (screenHeight / 2) + 90, 20, YELLOW);
      }
    }
    
    EndDrawing();
  }//End While WindowShouldClose
  // printf("%i %i\n",pSteps, numObj);

  //-----------------------------------------------------------------------
  // Free-memory-and-terminate---------------------------------------------
  //-----------------------------------------------------------------------
  UnloadModel(box);
  UnloadModel(ball);
  UnloadModel(cylinder);
  UnloadModel(ground);
  UnloadTexture(drumTx);
  UnloadTexture(planetTx);
  UnloadTexture(crateTx);
  UnloadTexture(grassTx);
  UnloadShader(shader);
  if(objInGameInitd != nullptr)
    RL_FREE(objInGameInitd);
  if (obj != nullptr)
    RL_FREE(obj);
  RL_FREE(car);
  RL_FREE(groundInd);
  dGeomTriMeshDataDestroy(triData);
  delete gameTimer;
  dJointGroupEmpty(contactgroup);
  dJointGroupDestroy(contactgroup);
  dSpaceDestroy(space);
  dWorldDestroy(world);
  dCloseODE();
  CloseWindow(); // Close window and OpenGL context
  //-------------------------------------------------------------------------

  return 0;
}
