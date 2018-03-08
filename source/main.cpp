#include <iostream>

#include "render/FornaxApp.h"
#include "render/VkRenderBackend.h"

int main()
{
	FornaxApp app;
	try
	{
		app.CreateWindow();
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	app.Run();
	app.Cleanup();

	return EXIT_SUCCESS;
}