#pragma once

#include "Components.hpp"

struct Plane
{
	Transform Transform;
	Material  Material;
};

struct Cube
{
	Transform Transform;
	Material  Material;
};

struct LightCube
{
	Transform  Transform;
	Material   Material;
	PointLight Light;
};