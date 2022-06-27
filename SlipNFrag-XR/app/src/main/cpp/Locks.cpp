#include "Locks.h"

std::mutex Locks::ModeChangeMutex;

std::mutex Locks::InputMutex;

std::mutex Locks::RenderInputMutex;

std::mutex Locks::RenderMutex;

std::mutex Locks::DirectRectMutex;

std::mutex Locks::SoundMutex;
