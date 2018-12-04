#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <array>

// reference from boost.asio

namespace accuree {
	class mutable_buffer;
	class const_buffer;

	void* buffer_cast_helper(const mutable_buffer&);
	const void* buffer_cast_helper(const const_buffer&);
	std::size_t buffer_size_helper(const mutable_buffer&);
	std::size_t buffer_size_helper(const const_buffer&);

	class mutable_buffer {
	public:
		mutable_buffer() : data_(0), size_(0) {}
		mutable_buffer(void* data, std::size_t size) : data_(data), size_(size) {}

		void* data() { return data_; }
		void* data() const { return data_; }
		std::size_t size() const { return size_; }
	private:
		friend void* buffer_cast_helper(const mutable_buffer& b);
		friend std::size_t buffer_size_helper(const mutable_buffer& b);
		void* data_;
		std::size_t size_;
	};

	inline void* buffer_cast_helper(const mutable_buffer& b) {
		return b.data_;
	}

	inline std::size_t buffer_size_helper(const mutable_buffer& b) {
		return b.size_;
	}

	class const_buffer {
	public:
		const_buffer() : data_(0), size_(0) {}
		const_buffer(const void* data, std::size_t size) : data_(data), size_(size) {}

		const void* data() { return data_; }
		const void* data() const { return data_; }
		std::size_t size() const { return size_; }
	private:
		friend const void* buffer_cast_helper(const const_buffer& b);
		friend std::size_t buffer_size_helper(const const_buffer& b);
		const void* data_;
		std::size_t size_;
	};

	inline const void* buffer_cast_helper(const const_buffer& b) {
		return b.data_;
	}

	inline std::size_t buffer_size_helper(const const_buffer& b) {
		return b.size_;
	}


	inline std::size_t buffer_size(const mutable_buffer& b) {
		return buffer_size_helper(b);
	}

	inline std::size_t buffer_size(const const_buffer& b) {
		return buffer_size_helper(b);
	}


	inline mutable_buffer buffer(void* data, std::size_t size) {
		return mutable_buffer(data, size);
	}

	inline const_buffer buffer(const void* data, std::size_t size) {
		return const_buffer(data, size);
	}

	// for std::array
	template<typename PodType, std::size_t N>
	inline const_buffer buffer(const std::array<PodType, N>& data) {
		return const_buffer(data.data(), data.size() * sizeof(PodType));
	}

	template<typename PodType, std::size_t N>
	inline mutable_buffer buffer(std::array<PodType, N>& data) {
		return mutable_buffer(data.data(), data.size() * sizeof(PodType));
	}


	template<typename PodType, std::size_t N>
	inline const_buffer buffer(PodType(&data)[N]) {
		return const_buffer(data, N * sizeof(PodType));
	}
}

#endif
