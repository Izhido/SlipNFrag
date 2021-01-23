#include "ConsolePerImage.h"
#include "AppState.h"

void ConsolePerImage::Reset(AppState& appState)
{
	vertices.Reset(appState);
	indices.Reset(appState);
	stagingBuffers.Reset(appState);
}