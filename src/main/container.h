//
// Created by vert9 on 11/13/24.
//

#ifndef CONTAINER_H
#define CONTAINER_H

#include <any>
#include <format>
#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>

/**
 * @brief A Simple Inversion of Control (IoC) Container implementation
 *
 * Copied from: https://www.youtube.com/watch?v=Ne-zLg6d9TM
 */
class Container {
	public:
		~Container() {
			factoryMap.clear();
		};

		// TODO how to make this argument list dynamic?!
		template <typename T>
		using Generator = std::function<std::shared_ptr<T>(std::any& arg1, std::any& arg2, std::any& arg3)>;

		template <typename T>
		void RegisterFactory(Generator<T> gen) {
			factoryMap[typeid(T)] = gen;
		}

		// TODO how to make this argument list dynamic?!
		template <class T>
		std::shared_ptr<T> Resolve(std::any arg1 = nullptr, std::any arg2 = nullptr, std::any arg3 = nullptr) {
			if (const auto i = factoryMap.find(typeid(T)); i != factoryMap.end()) {
				return std::any_cast<Generator<T>>(i->second)(arg1, arg2, arg3);
			}

			throw std::runtime_error(std::format("Could not resolve factory for type [{}]", typeid(T).name()));
		}
		static Container& Instance() {
			static Container c;
			return c;
		}
	private:
		std::unordered_map<std::type_index, std::any> factoryMap;
};

#endif //CONTAINER_H
