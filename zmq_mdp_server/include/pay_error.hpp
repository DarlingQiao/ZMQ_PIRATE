#ifndef PAY_ERROR_HPP
#define PAY_ERROR_HPP

#include <system_error>
enum class pay_error {
	success,
	device_not_open,
	device_already_open,
	invalid_command,
	start_bit_not_match,
	crc_not_match,
	wrong_ack
};

class pay_error_category : public std::error_category {
public:
	virtual const char* name() const {
		return "pay_error";
	}
	
	virtual std::string message(int ErrVal) const {
		switch (ErrVal) {
		case static_cast<int>(pay_error::success) :
			return "success";
		case static_cast<int>(pay_error::device_not_open) :
			return "device not open";
		case static_cast<int>(pay_error::device_already_open) :
			return "device already open";
		case static_cast<int>(pay_error::invalid_command) :
			return "invalid command";
		case static_cast<int>(pay_error::start_bit_not_match) :
			return "start bit not match";
		case static_cast<int>(pay_error::crc_not_match) :
			return "crc not match";
		default:
			return "unknown error";
		}
	}
};

const std::error_category& pay_category() {
	static const pay_error_category pay_category_const;
	return pay_category_const;
}

namespace std {
	template<> struct is_error_code_enum<pay_error> : public true_type {};
	template<> struct is_error_condition_enum<pay_error> : public true_type{};
}

std::error_code make_error_code(pay_error e) {
	return std::error_code(static_cast<int>(e), pay_category());
}

#endif
