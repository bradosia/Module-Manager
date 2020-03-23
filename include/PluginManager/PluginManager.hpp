/*
 * @name Plugin Manager
 * @author Branden Lee
 * @version 1.00
 * @license GNU LGPL v3
 * @brief Manages shared library plugins
 */

#ifndef BRADOSIA_PLUGIN_MANAGER_H
#define BRADOSIA_PLUGIN_MANAGER_H

// c++17
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

/* boost
 */
#include <boost/dll/import.hpp> // for import_alias
#include <boost/shared_ptr.hpp>

namespace bradosia {

template <class T> std::shared_ptr<T> to_std(const boost::shared_ptr<T> &p) {
  return std::shared_ptr<T>(p.get(), [p](T *) mutable {
    // p.reset();
  });
}

template <class T> boost::shared_ptr<T> to_boost(const std::shared_ptr<T> &p) {
  return boost::shared_ptr<T>(p.get(), [p](T *) mutable { p.reset(); });
}

class InterfaceMethodsBase {
public:
  std::string pluginName;
  InterfaceMethodsBase(std::string s) { pluginName = s; }
  ~InterfaceMethodsBase() {}
  virtual void addPath(std::filesystem::path p) = 0;
};

template <class T> class InterfaceMethods : public InterfaceMethodsBase {
public:
  InterfaceMethods(std::string s) : InterfaceMethodsBase(s) {}
  ~InterfaceMethods() {}
  std::vector<boost::shared_ptr<T>> pluginPtrs;
  void addPath(std::filesystem::path p) {
    boost::filesystem::path lib_path(p.string().c_str());
    std::cout << "PLUGIN: Loading " << p << "\n";
    boost::shared_ptr<T> plugin;
    try {
      plugin = boost::dll::import<T>(lib_path, pluginName,
                                     boost::dll::load_mode::default_mode);
    } catch (...) {
      std::cout << "PLUGIN: Loading FAILED " << p << "\n";
    }
    if (plugin) {
      std::cout << "PLUGIN: Loading SUCCESS " << p << "\n";
      pluginPtrs.push_back(plugin);
    }
  }
};

class PluginManager {
private:
  std::unordered_map<std::string, InterfaceMethodsBase *> interfaceMap;

public:
  template <class T> void addPluginInterface(std::string pluginName) {
    InterfaceMethods<T> *interface = new InterfaceMethods<T>(pluginName);
    InterfaceMethodsBase *interfaceBase = (InterfaceMethodsBase *)interface;
    interfaceMap.insert({pluginName, interface});
  }

  void loadPlugins(std::string directoryPathStr) {
    for (auto &p :
         std::filesystem::recursive_directory_iterator(directoryPathStr)) {
      std::cout << "PLUGIN: File Found " << p.path() << "\n";
      if (p.is_regular_file() &&
          (p.path().extension() == ".dll" || p.path().extension() == ".dylib" ||
           p.path().extension() == ".so")) {
        for (auto pairs : interfaceMap) {
          pairs.second->addPath(p.path());
        }
      }
    }
  }

  template <class T> std::shared_ptr<T> getPlugin(std::string pluginName) {
    InterfaceMethods<T> *interface =
        dynamic_cast<InterfaceMethods<T> *>(interfaceMap.at(pluginName));
    if (interface->pluginPtrs.empty()) {
      return nullptr;
    }
    return to_std(interface->pluginPtrs.front());
  }
};

} // namespace bradosia

#endif
// end BRADOSIA_PLUGIN_MANAGER_H
