#include "Runtime/EntryPoint.h"
#include "Runtime/Application.h"
#include "Core/Log/Logger.h"

using namespace Span;

class PlaygroundApp : public Application
{
public:

	void OnStart() override
	{
		
	}
};

Application* Span::CreateApplication()
{
	return new PlaygroundApp();
}