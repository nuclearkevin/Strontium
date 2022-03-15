- General
  - **Add thread-safe data structures.**
    - [x] Queue.
    - [x] Vector.
    - [ ] Static array.
    - [ ] Unordered map.

  - [x] Update logs to use static functions instead of a singleton.
    - [x] Add log dumping to files.

  - Rewrite the job system.
    - [x] Use a concurrent queue (https://github.com/cameron314/concurrentqueue) OR a
    threadsafe non-concurrent queue that minimizes locking.
    - [x] Implement the Jolt physics job system which performs calls to engine's job system.
      - [x] class PhysicsThreadPool final : public JobSystem { ... }
      - [x] class PhysicsBarrier : public Barrier { ... }
      - [x] class Semaphore { ... }

  - [x] Add a scene copying system. Prevents physics (and later scripting) updates
  from changing the scene permanently.  
    - Requested renderer handles are a headache. Deal with those separately.

- Physics
  - [x] Add Jolt Physics to the engine build system.

  - [x] PhysicsWorld functions(?) / class.
    - [x] init(): Sets up the resources required for physics.
    - [x] shutdown(): Frees resources used by Jolt
    - [x] addBody(Entity entity), etc: Add physics bodies.
    - [x] onSimulationBegin(): Creates the Jolt physics world. Creates all the physics
    actors using components sent over and initializes the physics scene.
    - [x] onUpdate(float dtFrame, other physics params...): Steps the physics system
    (https://gafferongames.com/post/fix_your_timestep/).
    - [x] onSimulationEnd(): Deletes the physics world and all of the actors created.

  - [x] PhysicsActor class.
    - Maps EnTT entities to Jolt actors.

  - [x] PhysicsUtils functions.
    - Convert between Jolt types and GLM types.

  - [x] Physics components for the ECS.
    - [x] 3D rigidbody component.
    - Collider components (mapped to some functionality of Jolt shapes somehow).
      - [x] Box collider
      - [x] Sphere collider
      - [ ] Capsule collider
      - [ ] Tapered-capsule collider
      - [ ] Cylinder collider
      - [ ] Convex hull collider
      - [ ] Mesh collider
      - [ ] Height field collider

  - [ ] A constraint abstraction for compound shapes

- Rendering
  - [ ] Debug pass (editor only).
    - [ ] Visualize colliders.
    - [ ] Visualize camera frustums
    
  - [ ] Sky atmosphere pass.
    - [ ] Fix the camera positioning system
    - [ ] Implement the aerial perspective LUT from Hillaire2020.
      - [ ] Use 2D array textures and manually interpolate in the z-axis since 3D array
      textures aren't a thing. :(

  - [ ] Point lighting pass.
    - [ ] Add the renderpass class.
    - [ ] Add point light submission to Scene.cpp.
    - [ ] **Abstract 1D textures, notably with red 8 bit integers.**
    - [ ] Rewrite the frustum+AABB and culling shaders to improve readability and
    minimize resource binding.

  - [ ] Spot lighting pass. Do after the point lighting pass.
    - [ ] Add the renderpass class.
    - [ ] Add spot light submission to Scene.cpp.
    - [ ] Write the culling shaders. Cone AABB, cone frustum testing vs bounding sphere testing.
