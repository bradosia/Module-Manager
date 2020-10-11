/*
 * @name Module Manager
 * @author Branden Lee
 * @version 1.00
 * @license GNU LGPL v3
 * @brief Manages shared library modules
 */

#ifndef BRADOSIA_MODULE_MANAGER_H
#define BRADOSIA_MODULE_MANAGER_H

// c++
#include <iostream>
#include <memory>
#include <string>

/* boost 1.72.0
 * License: Boost Software License (similar to BSD and MIT)
 */
#include "boost/filesystem.hpp"
#include <boost/dll/import.hpp> // for import_alias
#include <boost/shared_ptr.hpp>
#include <boost/signals2.hpp>

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
  virtual int addPath(boost::filesystem::path lib_path) = 0;
  virtual int callLoadedSignal() = 0;
};

template <class T> class InterfaceMethods : public InterfaceMethodsBase {
public:
  std::shared_ptr<boost::signals2::signal<int(std::shared_ptr<T>)>>
      loadedSignal;

  InterfaceMethods(std::string s) : InterfaceMethodsBase(s) {}
  ~InterfaceMethods() {}
  std::vector<boost::shared_ptr<T>> modulePtrs;
  int addPath(boost::filesystem::path lib_path) {
    std::cout << "PLUGIN: Loading " << lib_path << "\n";
    boost::shared_ptr<T> module;
    try {
      module = boost::dll::import<T>(lib_path, moduleName,
                                     boost::dll::load_mode::default_mode);
    } catch (...) {
      std::cout << "PLUGIN: Loading FAILED " << lib_path << "\n";
      return -1;
    }
    std::cout << "PLUGIN: Loading SUCCESS " << lib_path << "\n";
    modulePtrs.push_back(module);
    return 0;
  }
  int callLoadedSignal() {
    (*loadedSignal)(to_std(modulePtrs.front()));
    return 0;
  }
};

class ModuleManager {
private:
  std::unordered_map<std::string, std::shared_ptr<InterfaceMethodsBase>>
      interfaceMap;
  unsigned int modulesLoadedNum = 0;

public:
  boost::signals2::signal<int()> callbackLoadAllSignal;

  template <class T> void addModule(std::string moduleName) {
    std::shared_ptr<InterfaceMethods<T>> interface =
        std::make_shared<InterfaceMethods<T>>(moduleName);
    std::shared_ptr<InterfaceMethodsBase> interfaceBase =
        std::dynamic_pointer_cast<InterfaceMethodsBase>(interface);
    interfaceMap.insert({moduleName, interface});
    interface->loadedSignal =
        std::make_shared<boost::signals2::signal<int(std::shared_ptr<T>)>>();
  }

  void loadModules(std::string directoryPathStr) {
    int rc;
    std::string moduleName;
    for (auto &p :
         boost::filesystem::recursive_directory_iterator(directoryPathStr)) {
      std::cout << "MODULE: File Found " << p.path() << "\n";
      std::cout << "path extension " << p.path().extension().string() << "\n";
      std::string pathStr = p.path().string();
      /* On linux the dynamic library extensions are libMyLibrary.so.1.0.0
       * Thus, we use find instead of substr matching
       */
      if (boost::filesystem::is_regular_file(p) &&
          (pathStr.find(".dll") != std::string::npos ||
           pathStr.find(".dylib") != std::string::npos ||
           pathStr.find(".so") != std::string::npos)) {
        for (auto pairs : interfaceMap) {
          moduleName = pairs.first;
          rc = pairs.second->addPath(p.path());
          if (rc == 0) {
            pairs.second->callLoadedSignal();
            modulesLoadedNum++;
            if (modulesLoadedNum == interfaceMap.size()) {
              callbackLoadAllSignal();
            }
          }
        }
      }
    }
  }

  template <class T> std::shared_ptr<T> getModule(std::string moduleName) {
    std::shared_ptr<InterfaceMethods<T>> interface =
        std::dynamic_pointer_cast<std::shared_ptr<InterfaceMethods<T>>>(
            interfaceMap.at(moduleName));
    if (interface->modulePtrs.empty()) {
      return nullptr;
    }
    return to_std(interface->modulePtrs.front());
  }

  template <class T>
  std::shared_ptr<boost::signals2::signal<int(std::shared_ptr<T>)>>
  getCallbackLoadSignal(std::string moduleName) {
    std::shared_ptr<InterfaceMethods<T>> interface =
        std::dynamic_pointer_cast<InterfaceMethods<T>>(
            interfaceMap.at(moduleName));
    return interface->loadedSignal;
  }
};

} // namespace bradosia

#endif
// end BRADOSIA_MODULE_MANAGER_H
