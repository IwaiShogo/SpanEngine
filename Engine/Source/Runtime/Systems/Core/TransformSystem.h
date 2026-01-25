#pragma once
#include "ECS/Kernel/System.h"
#include "ECS/Kernel/World.h"
#include "Components/Core/Transform.h"
#include "Components/Core/LocalToWorld.h"

namespace Span
{
	class TransformSystem : public System
	{
	public:
		void OnUpdate() override
		{
			GetWorld()->ForEach<Transform, LocalToWorld>(
				[](Entity, Transform& t, LocalToWorld& ltw)
				{
					// çsóÒåvéZ
					ltw.Value = Matrix4x4::TRS(t.Position, t.Rotation, t.Scale);
				}
			);
		}
	};
}
