#include <gtest/gtest.h>

#include "scenes/Scene.hpp"
#include "scenes/Entity.hpp"
#include "scenes/Components.hpp"

TEST(Entity, EntityDefault)
{
	Scene scene{};
	Entity ent = scene.SpawnEntity("1");

	EXPECT_TRUE(ent.HasComponent<TagComponent>()) << "Entity was missing a transform component";
	EXPECT_TRUE(ent.HasComponent<TransformComponent>()) << "Entity was missing a tag component";
}

TEST(Entity, EntityAddComponent)
{
	Scene scene{};
	Entity ent = scene.SpawnEntity("2");

	ent.AddComponent<MeshComponent>();
	ASSERT_TRUE(ent.HasComponent<MeshComponent>()) << "Entity was missing a mesh component";
	ASSERT_DEATH({ ent.AddComponent<MeshComponent>(); }, "Entity already has the component") << "Entity was missing a mesh component";
}

TEST(Entity, EntityRemoveComponent)
{
	Scene scene{};
	Entity ent = scene.SpawnEntity("3");

	ent.AddComponent<PointLightComponent>();
	ASSERT_TRUE(ent.HasComponent<PointLightComponent>()) << "Entity was missing a point light component";

	ent.RemoveComponent<PointLightComponent>();
	ASSERT_FALSE(ent.HasComponent<PointLightComponent>()) << "Entity's point light component wasn't removed";

	ASSERT_DEATH({ ent.RemoveComponent<PointLightComponent>(); }, "Entity doesn't have the component") << "Entity's point light component wasn't removed";
}

TEST(Entity, EntityRemoveRequiredComponents)
{
	Scene scene{};
	Entity ent = scene.SpawnEntity("4");

	ASSERT_DEATH({ ent.RemoveComponent<TransformComponent>(); }, "Can't remove the transform component") << "Entity's transform component was actually removed";
	ASSERT_DEATH({ ent.RemoveComponent<TagComponent>(); }, "Can't remove the tag component") << "Entity's tag component was actually removed";
}