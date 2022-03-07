- General
  - **Add thread-safe data structures.**
    - Queue.
    - Vector.
    - Static array.
    - Unordered map.

  - Rewrite the job system.
    - Use a concurrent queue (https://github.com/cameron314/concurrentqueue) OR a
    threadsafe non-concurrent queue that minimizes locking.
    - Implement the Jolt physics job system.

  - Add a scene copying system. Prevents physics (and later scripting) updates
  from changing the scene permanently.  
    - Requested renderer handles are a headache. Deal with those separately.

- Physics
  - Add Jolt Physics to the engine build system. Looks like it's just the items
  in the "Jolt" folder.

  - PhysicsWorld functions(?) / class.
    - init(): Sets up the resources required for physics.
    - shutdown(): Frees resources used by Jolt
    - add(Body components, Collider components, etc): Add in components
    - onSimulationBegin(): Creates the Jolt physics world. Creates all the physics
    actors using components sent over and initializes the physics scene.
    - onUpdate(float dtFrame, other physics params...): Steps the physics system
    (https://gafferongames.com/post/fix_your_timestep/).
    - updateEntities(Scene): Update scene entities using information from the physics simulation.
    - onSimulationEnd(): Deletes the physics world and all of the actors created.

  - PhysicsActor class.
    - Maps EnTT entities to Jolt actors.

  - PhysicsUtils functions.
    - Convert between Jolt types and GLM types.

  - Physics components for the ECS.
    - Rigid body components (mapped to Jolt shapes somehow).
    - Collider components (mapped to some functionality of Jolt shapes somehow).

- Rendering
  - Sky atmosphere pass.
    - Fix the camera positioning system
    - Implement the aerial perspective LUT from Hillaire2020.
      - Use 2D array textures and manually interpolate in the z-axis since 3D array
      textures aren't a thing. :(

  - Point lighting pass.
    - Add the renderpass class.
    - Add point light submission to Scene.cpp.
    - **Abstract 1D textures, notably with red 8 bit integers.**
    - Rewrite the frustum+AABB and culling shaders to improve readability and
    minimize resource binding.

  - Spot lighting pass. Do after the point lighting pass.
    - Add the renderpass class.
    - Add spot light sibmission to Scene.cpp.
    - Write the culling shaders. Cone AABB, cone frustum testing vs bounding sphere testing.
