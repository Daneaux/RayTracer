#pragma once

#include "renderer/IRenderer.h"
#include "renderer/SoftwareRenderer.h"
#include "renderer/GPURenderer.h"
#include "renderer/SceneOverviewRenderer.h"
#include "renderer/SoftwareOverviewRenderer.h"
#include <Windows.h>

enum RendererType
{
	GPU,
	Software,
	SceneOverview,
	SoftwareOverview
};

class RendererFactory
{
public:
	static std::unique_ptr<IRenderer> CreateRenderer(RendererType type)
	{
		switch (type)
		{
		case RendererType::GPU:
			return std::make_unique<GPURenderer>();
		case RendererType::Software:
			return std::make_unique<SoftwareRenderer>();
		case RendererType::SceneOverview:
			return std::make_unique<SceneOverviewRenderer>();
		case RendererType::SoftwareOverview:
			return std::make_unique<SoftwareOverviewRenderer>();
		default:
			return nullptr;
		}
	}
};