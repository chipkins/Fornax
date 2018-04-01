#include <iostream>

#include "render/FornaxApp.h"
#include "render/VkRenderBackend.h"

int main()
{
	FornaxApp app;
	try
	{
		app.Run();
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}