#ifndef FLATBED_SCANNER_H
#define FLATBED_SCANNER_H

#ifdef FLATBED_SCANNER_H
#define FLATBED_SCANNER_API __declspec(dllexport)
#else
#define FLATBED_SCANNER_API __declspec(dllimport)
#endif

#include <system_error>
#include <string>
#include <accuree_define.hpp>
#include <functional>

typedef std::function<void(const std::string&)> flatbed_hanlder;

class FLATBED_SCANNER_API flatbed_scanner {
public:
	flatbed_scanner();
	~flatbed_scanner();

	bool is_open();
	std::error_code open(const std::string& device, std::error_code& ec);
	std::error_code close(std::error_code& ec);

	void start(const flatbed_hanlder& handler);
	std::error_code reset(std::error_code& ec);
	std::error_code enable(std::error_code& ec);
	std::error_code disable(std::error_code& ec);
	std::string get_barcode(std::error_code& ec);

	void open(const std::string& device);
	void close();
	void reset();
	void enable();
	void disable();

private:
	struct Impl;
	Impl* pImpl;
};


#endif

