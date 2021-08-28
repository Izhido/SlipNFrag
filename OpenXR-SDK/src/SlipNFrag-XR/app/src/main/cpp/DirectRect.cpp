#include "DirectRect.h"

std::mutex DirectRect::DirectRectMutex;
std::vector<DirectRect> DirectRect::directRects;
