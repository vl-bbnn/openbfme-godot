// OpenBFME. GPL-3.0.
// Standard CRC-32 (IEEE 802.3, reflected, poly 0xEDB88320), the same value as Python's zlib.crc32.
// Used only to compare whole arrays against an independent oracle; it is not a SAGE checksum.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#if defined(__aarch64__) && defined(__ARM_FEATURE_CRC32) && defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#include <arm_acle.h>
#endif

// the table, built once by a thread-safe static initialisation (lane PERF-2: map chunks are parsed on several threads at once; the table used to be filled
// lazily through a plain `built` flag, a data race ThreadSanitizer found)
inline const std::array<std::uint32_t, 256> &crc32Table()
{
	static const std::array<std::uint32_t, 256> table = [] {
		std::array<std::uint32_t, 256> t{};
		for (std::uint32_t i = 0; i < 256; ++i)
		{
			std::uint32_t c = i;
			for (int k = 0; k < 8; ++k)
			{
				c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
			}
			t[i] = c;
		}
		return t;
	}();
	return table;
}

inline std::uint32_t crc32Bytes(const void *data, size_t size, std::uint32_t crc = 0)
{
	const std::uint8_t *p = (const std::uint8_t *)data;
	crc = ~crc;
#if defined(__aarch64__) && defined(__ARM_FEATURE_CRC32) && defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	// ARM IEEE CRC32 instructions preserve the reflected polynomial and complement
	// contract. memcpy keeps unaligned inputs legal and never reads past the span.
	while (size >= sizeof(std::uint64_t))
	{
		std::uint64_t word;
		std::memcpy(&word, p, sizeof(word));
		crc = __crc32d(crc, word);
		p += sizeof(word);
		size -= sizeof(word);
	}
	for (size_t i = 0; i < size; ++i)
	{
		crc = __crc32b(crc, p[i]);
	}
#else
	const std::array<std::uint32_t, 256> &table = crc32Table();
	for (size_t i = 0; i < size; ++i)
	{
		crc = table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
	}
#endif
	return ~crc;
}
