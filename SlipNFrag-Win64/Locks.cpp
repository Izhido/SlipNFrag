#include "Locks.h"

bool Locks::StopEngine;

std::mutex Locks::InputMutex;

std::mutex Locks::RenderMutex;

std::mutex Locks::DirectRectMutex;

std::mutex Locks::SoundMutex;
