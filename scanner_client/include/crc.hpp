#ifndef CRC_HPP
#define CRC_HPP

#include <array>
#include <accuree_define.hpp>

namespace accuree {
	class crc {
	public:
		std::array<byte, 2> operator()(const byte* pbuf, std::size_t len) {
			unsigned short crc_reg = 0;
			unsigned short to_xor = 0;
			byte index = 0;

			for (std::size_t i = 0; i != len; i++) {
				index = (crc_reg ^ pbuf[i]) & 0xFF;
				to_xor = index;
				for (unsigned int j = 0; j < 8; j++) {
					if (to_xor & 0x0001)
						to_xor = (to_xor >> 1) ^ 0x8408;
					else
						to_xor >>= 1;
				}
				crc_reg = (crc_reg >> 8) ^ to_xor;
			}
			std::array<byte, 2> out = { 0 };
			out[1] = (byte)(crc_reg >> 8);
			out[0] = (byte)((byte)(crc_reg & 0xFF));
			return out;
		}

		// ccitt
		std::array<byte, 2> ccitt(const byte* pbuf, std::size_t len) {
			return operator()(pbuf, len);
		}

		bool ccitt_verify(const byte* pbuf, std::size_t len) {
			std::array<byte, 2> out = ccitt(pbuf, len);
			if (out[0] == 0x00 && out[1] == 0x00) {
				return true;
			}

			return false;
		}

		// coin
		byte for_coin(const byte* pbuf, std::size_t len) {
			byte out = 0x00;
			for (std::size_t i = 0; i != len; ++i) {
				out += pbuf[0];
			}

			out = 0xFF - out + 1;
			return out;
		}

		bool for_coin_verify(const byte* pbuf, std::size_t len) {
			if (for_coin(pbuf, len) == 0x00) {
				return true;
			}
			return false;
		}

		// scales
		ushort for_scales(const byte* pbuf, std::size_t len) {
			const ushort polinomio = 0xA001;
			ushort result = 0xFFFF;
			for (std::size_t i = 0; i != len; ++i) {
				byte tmp = pbuf[i];
				result = (ushort)(tmp ^ result);
				for (int j = 0; j < 8; j++) {
					if ((result & 0x0001) == 1) {
						result = (ushort)((result >> 1) ^ polinomio);
					}
					else {
						result = (ushort)(result >> 1);
					}
				}
			}
			return result;
		}

		// scales verify
		bool for_scales_verify(const byte* pbuf, std::size_t len) {
			return for_scales(pbuf, len) == 0 ? true : false;
		}

		// light
		byte for_light(const byte* pbuf, std::size_t len) {
			byte result = 0x00;
			for (std::size_t i = 0; i != len; ++i) {
				result ^= pbuf[i];
			}
			return result;
		}

		bool for_light_verify(const byte* pbuf, std::size_t len) {
			return for_light(pbuf, len) == 0x00 ? true : false;
		}
	};
}

#endif
