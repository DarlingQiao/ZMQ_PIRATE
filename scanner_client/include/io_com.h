#ifndef IO_COM_H
#define IO_COM_H

#define DLL_EXPORT __declspec(dllexport)

#include <accuree_define.hpp>
#include <string>
#include <system_error>
#include <buffer.hpp>

namespace accuree {
	class DLL_EXPORT io_com {
	public:
		io_com();
		~io_com();
		bool is_open();
		std::error_code open(const std::string& device, std::error_code& ec);
		std::error_code close(std::error_code& ec);
		std::error_code set_option(unsigned nbaudrate, byte bytesize, byte parity,
			byte stopbits, std::error_code& ec);
		std::error_code cancel(std::error_code& ec);

		std::size_t write(const const_buffer& buffer, std::error_code& ec);
		std::size_t read(const mutable_buffer& buffer, ulong ntimeout,
			std::error_code& ec);
		std::size_t read(const mutable_buffer& buffer, std::size_t num_bytes_read,
			ulong ntimeout, std::error_code& ec);
	private:
		struct impl;
		impl* pImpl;
	};
}


#endif
