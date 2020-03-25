/*
 * @name Module Manager
 * @author Branden Lee
 * @version 1.00
 * @license GNU LGPL v3
 * @brief Manages shared library modules
 */

#ifndef BRADOSIA_MODULE_MANAGER_H
#define BRADOSIA_MODULE_MANAGER_H

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
  std::string moduleName;
  InterfaceMethodsBase(std::string s) { moduleName = s; }
  ~InterfaceMethodsBase() {}
  virtual void addPath(std::filesystem::path p) = 0;
};

template <class T> class InterfaceMethods : public InterfaceMethodsBase {
public:
  InterfaceMethods(std::string s) : InterfaceMethodsBase(s) {}
  ~InterfaceMethods() {}
  std::vector<boost::shared_ptr<T>> modulePtrs;
  void addPath(std::filesystem::path p) {
    boost::filesystem::path lib_path(p.string().c_str());
    std::cout << "PLUGIN: Loading " << p << "\n";
    boost::shared_ptr<T> module;
    try {
      module = boost::dll::import<T>(lib_path, moduleName,
                                     boost::dll::load_mode::default_mode);
    } catch (...) {
      std::cout << "PLUGIN: Loading FAILED " << p << "\n";
    }
    if (module) {
      std::cout << "PLUGIN: Loading SUCCESS " << p << "\n";
      modulePtrs.push_back(module);
    }
  }
};

class ModuleManager {
private:
  std::unordered_map<std::string, InterfaceMethodsBase *> interfaceMap;

public:
  template <class T> void addModuleInterface(std::string moduleName) {
    InterfaceMethods<T> *interface = new InterfaceMethods<T>(moduleName);
    InterfaceMethodsBase *interfaceBase = (InterfaceMethodsBase *)interface;
    interfaceMap.insert({moduleName, interface});
  }

  void loadModules(std::string directoryPathStr) {
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

  template <class T> std::shared_ptr<T> getModule(std::string moduleName) {
    InterfaceMethods<T> *interface =
        dynamic_cast<InterfaceMethods<T> *>(interfaceMap.at(moduleName));
    if (interface->modulePtrs.empty()) {
      return nullptr;
    }
    return to_std(interface->modulePtrs.front());
  }
};

} // namespace bradosia

#endif
// end BRADOSIA_MODULE_MANAGER_H
