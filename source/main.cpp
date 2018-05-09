#include <iostream>

#include "FornaxApp.h"
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
		std::getchar();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}