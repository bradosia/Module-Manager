# Plugin Manager
A simple dynamic library plugin manager and loader. Easily define plugins and load them from a plugin directory.

# Dependencies
* boost
* c++17

# Example Usage
main.cpp
```cpp
#include "PluginManager.hpp"
#include "PluginInterface.hpp"
using namespace bradosia;

int main(){
  PluginManager pluginManagerObj;
  pluginManagerObj.addPluginInterface<PluginInterface>("pluginName");
  pluginManagerObj.loadPlugins("plugins_directory");
  boost::shared_ptr<PluginInterface> plugin = pluginManagerObj.getPlugin<PluginInterface>("pluginName");
  plugin->testPlugin();
}
```

PluginInterface.hpp
```cpp
class PluginInterface {
public:
  virtual void testPlugin() = 0;
};
```

plugin.hpp source for "plugins_directory/plugin.dll"
```cpp
#include <boost/config.hpp> // for BOOST_SYMBOL_EXPORT
#include "pluginInterface.hpp"

namespace PluginNamespace {
class PluginClass : public PluginInterface {
  void testPlugin(){
    printf("Hello World!");
  }
}

extern "C" BOOST_SYMBOL_EXPORT PluginClass pluginName;
PluginClass pluginName;
}
```

