#pragma once

#include "renderer/IRenderer.h"
#include "renderer/SoftwareRenderer.h"
#include "renderer/GPURenderer.h"
#include <Windows.h>

enum RendererType
{
	GPU,
	Software
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
		default:
			return nullptr;
		}
	}
};