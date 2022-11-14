#pragma once
#ifndef INCLUDE_IMPLUSPLUS_ERROR_HPP
#define INCLUDE_IMPLUSPLUS_ERROR_HPP
#include <functional>
#include <stdexcept>
#include <string>

namespace impp
{
	namespace error
	{
		using error_handler = std::function<void(const std::runtime_error&)>;
		namespace detail
		{
			struct error_handling
			{
				static void default_throw_wrapper(const std::runtime_error& err){ throw err; }
				static inline error_handler _handler = error_handling::default_throw_wrapper;
			};

			void on_error(const std::runtime_error& err)
			{
				if(error_handling::_handler)
					error_handling::_handler(err);
			}
		}

		//method used to set an error handler - it can be an empty function used to silence errors
		void set_error_handler(error_handler func)
		{
			detail::error_handling::_handler = func;
		}
	}
}

#endif //INCLUDE_IMPLUSPLUS_ERROR_HPP
