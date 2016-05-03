/*
Project: FLuaG
File: threading.hpp

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <thread>
#include <future>

namespace Threading{
	// Allows function executions in same thread
	template<typename Ret>
	class Context_Base : protected std::thread{
		protected:
			// Host & client thread communication objects
			std::promise<bool> host_promise;
			std::future<bool> host_future = host_promise.get_future();
			std::promise<Ret> client_promise;
			std::future<Ret> client_future = client_promise.get_future();
			// Function from host to call by client
			std::function<Ret()> host_func;
		public:
			// Ctor
			Context_Base(const std::function<void()>& client_func){std::thread::operator=(std::thread(client_func));}
			// No copy
			Context_Base(const Context_Base&) = delete;
			Context_Base& operator=(const Context_Base&) = delete;
			// No move
			Context_Base(Context_Base&& other) = delete;
			Context_Base& operator=(Context_Base&& other) = delete;
			// Dtor
			~Context_Base() noexcept{
				if(this->joinable()){
					this->host_promise.set_value(false);
					this->join();
				}
			}
			// Call function in client thread
			virtual Ret operator()(const std::function<Ret()>& host_func) = 0;
	};

	template<typename Ret>
	class Context : public Context_Base<Ret>{
		public:
			Context() : Context_Base<Ret>([this](){
				while(this->host_future.get()){
					this->host_promise = std::promise<bool>();
					this->host_future = this->host_promise.get_future();
					this->client_promise.set_value(this->host_func());
				}
			}){}
			Ret operator()(const std::function<Ret()>& host_func) override{
				if(!this->joinable())
					throw std::future_error(std::future_errc::no_state);
				this->host_func = host_func;
				this->host_promise.set_value(true);
				Ret result = this->client_future.get();
				this->client_promise = std::promise<Ret>();
				this->client_future = this->client_promise.get_future();
				return result;
			}
	};

	template<>
	class Context<void> : public Context_Base<void>{
		public:
			Context() : Context_Base<void>([this](){
				while(this->host_future.get()){
					this->host_promise = std::promise<bool>();
					this->host_future = this->host_promise.get_future();
					this->host_func();
					this->client_promise.set_value();
				}
			}){}
			void operator()(const std::function<void()>& host_func) override{
				if(!this->joinable())
					throw std::future_error(std::future_errc::no_state);
				this->host_func = host_func;
				this->host_promise.set_value(true);
				this->client_future.get();
				this->client_promise = std::promise<void>();
				this->client_future = this->client_promise.get_future();
			}
	};
}

