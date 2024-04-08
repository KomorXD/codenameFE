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
 - [x] PBR (Blinn-Phong also available)
 - [ ] Environment maps + IBL
 - [ ] Uploading custom models (+ making sure they have normals and tangents, or calculating them)
 - [ ] Bloom (+ compute shaders)
 - [ ] SSAO
 - [ ] Managing textures to optimize draw calls, probably texture atlas
 - [ ] Directional light shadow maps
 - [ ] Cascaded shadow maps
 - [ ] Spotlight shadow maps
 - [ ] Point light volumetric lights (to try to avoid shadow mapping with those)
 - [ ] Forward+ rendering
 - [ ] Whatever comes to my mind next

### Editor - basically poor Unity-based styled editor

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
