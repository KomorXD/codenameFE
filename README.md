# f\\(.\*\\)\\.e\\(.\*\\)
A project of mine with the sole purpose of creating a sandbox to play with various 3D graphic effects, implementations and architecture of an app that aims to be a very poor game engine.

## Motivation
I've been interested in 3D graphics for quite some time - my engineering thesis was basically a very basic renderer that just supported spheres (and details like skyboxes) with an editor just for their purpose. But I want to learn all the cool stuff that there is with 3D graphics, and this project is my gateway into that. I want to build an application with such an architecture, that adding new features will be as smooth of a process for me as possible. I want to explore different approaches and build something, that I could use to create a pretty scene, maybe even a playable one if I decide to do scripting. The project will also contain an editor, heavily inspired by standard game engine editors, but obviously with less functionality, with things like entity panel with components, entity hierarchy panel, serialization, asset manager, gizmos.

## What I plan the app to feature
### Renderer
 - [x] Managing meshes to optimize draw calls
 - [x] Easy way of adding new primitive meshes
 - [x] Color picking
 - [x] Highlighting objects using stencil
 - [x] Instancing
 - [x] Post-processing (just blur now, but the setup is there)
 - [x] ECS approach
 - [x] Normal mapping, tangent spaces
 - [x] HDR
 - [x] Material management
 - [x] Utilizing UBOs to pass light sources' and materials' data
 - [x] PBR
 - [x] Parallax mapping
 - [x] Environment maps + Diffuse and Specular IBL
 - [x] Bloom
 - [x] Spotlight shadow maps
 - [x] Point light shadow maps
 - [x] Directional light shadow maps
 - [x] Cascaded shadow maps
 - [x] Deffered rendering
 - [ ] Volumetric point lights for deferred pipeline
 - [ ] Bloom with compute shaders
 - [ ] Uploading custom models (+ making sure they have normals and tangents, or calculating them)
 - [ ] SSAO
 - [ ] Managing textures to optimize draw calls, probably texture atlas
 - [ ] Point light volumetric lights
 - [ ] Forward+ rendering
 - [ ] Whatever comes to my mind next

### Shadows :OOO (will need to optimize those for sure)
Point light | Spotlight
:-----------------:|:-----------------:
![Point light shadows](https://github.com/KomorXD/codenameFE/assets/51238441/1070bae0-bbcb-414c-af8c-b433f221630f)|![Spotlight shadows](https://github.com/KomorXD/codenameFE/assets/51238441/4bd5de54-cc33-43a2-aecf-ba8106f44769)

### Bloom (unoptimized, but not slow either ig)!!!!!!!!!!!!
![blum](https://github.com/KomorXD/codenameFE/assets/51238441/14f108f6-d7dc-4968-af2f-2b3d98a84057)

#### IBL Diffuse and specular environment maps :OOOOOOOOOOOOOO
![IBL showcase](https://github.com/KomorXD/codenameFE/assets/51238441/57e9b1e9-b2b6-4d20-b870-c311d9acd714)

Balls 1 | Balls 2
:-----------------:|:-----------------:
![Balls 1](https://github.com/KomorXD/codenameFE/assets/51238441/efc0444e-5460-4531-9db1-9c4906cdd7c8)|![Balls 2](https://github.com/KomorXD/codenameFE/assets/51238441/bb86d354-d171-4dbb-89b2-d9e533e11633)

#### Parallax mapping!!!!!!!!!!!!!
Without height map | With height map
:-----------------:|:-----------------:
![No parallax](https://github.com/KomorXD/codenameFE/assets/51238441/34d75101-f604-4057-a7b0-e5270e2d729e)|![Parallax](https://github.com/KomorXD/codenameFE/assets/51238441/e7d36372-0315-437a-8b6b-70f3c1a29fc9)

### Might happen, we will see
 - Mesh editor
 - Texture editor
 - Scripting

## What rules do I follow during development
This project is a way for me to let myself loose when it comes to code, since this project is just my side - by that I mean I don't follow strict rules like SOLID. I try to make things in a ways that make sense to me, are simple, with little layers of abstraction and no hidden control flows, while trying to keep it as readable and maintainable as I can (which obviously is hard for me to judge as a solo author, but I try to put thought to my code).

## Installation and building
The project is built using CMake, is developed under MSVC++ tools, and at the point of writing this README it is not adjusted for GCC / Clang (requires to add headers in some locations, that were included in other headers in VC++).

    git clone --recursive https://github.com/KomorXD/codenameFE
Pick CMake configuration (Windows/Linux - Debug/Release/Prod) and build it. The project also contains tests written using googletest. All libraries are pulled as submodules, hence `--recursive`.
