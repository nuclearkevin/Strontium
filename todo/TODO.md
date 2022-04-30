- General
  - Rewrite the job system.
    - [ ] Implement the Jolt physics job system which performs calls to engine's job system.
      - [ ] class PhysicsThreadPool final : public JobSystem { ... }
      - [ ] class PhysicsBarrier : public Barrier { ... }
      - [ ] class Semaphore { ... }

- Physics
  - [x] Add Jolt Physics to the engine build system.

  - [x] Physics components for the ECS.
    - Collider components (mapped to some functionality of Jolt shapes somehow).
      - [ ] Convex hull collider
      - [ ] Mesh collider
      - [ ] Height field collider

  - [ ] A constraint abstraction for compound shapes

- Rendering
  - [ ] Bloom pass.
      - [ ] Fix bloom pass resizing issues. On NVIDIA hardware (untested on AMD)
      the bloom texture doesn't get cleared and/or increases in intensity every resize.

  - [ ] Geometry and shadow pass.
      - [ ] De-interleave vertex attributes. Use separate buffers.
      - [ ] Merge positions (x, y, z) and UVs (w, 16 bit unorm per UV component) into one buffer.

  - [ ] Improve the animation system.
    - [ ] Animation and Animator class:
      - [ ] Use unsigned integer keys instead of strings.
      - [ ] Remove recursive traversal of the model hierarchy.

  - [ ] Sky atmosphere pass.
    - [ ] Fix the camera positioning system
    - [ ] Implement the aerial perspective LUT from Hillaire2020.
      - Use 2D array textures and manually interpolate in the z-axis since 3D array
      textures aren't a thing. :(

  - [ ] Point lighting pass.
    - [ ] Add the renderpass class.
    - [ ] Add point light submission to Scene.cpp.
    - [ ] **Abstract 1D textures, notably with red 8 bit integers.**
    - [ ] Rewrite the frustum+AABB and culling shaders to improve readability and
    minimize resource binding.
