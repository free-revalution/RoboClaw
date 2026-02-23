// src/plugins/plugin_registry.cpp
#include "plugins/plugin_registry.h"

namespace roboclaw::plugins {

// Explicit template instantiations for common plugin types
// These can be expanded as needed when concrete plugin interfaces are defined

// Example instantiation (commented out until IPlugin is defined):
// template class PluginRegistry<IPlugin>;

// The PluginRegistry is a template class, so most implementations
// must remain in the header file. This .cpp file exists to:
// 1. Satisfy the specification requirement for a separate .cpp file
// 2. Provide a location for explicit template instantiations
// 3. Allow for future non-template helper functions

// Future non-template helper functions can be added here

} // namespace roboclaw::plugins
