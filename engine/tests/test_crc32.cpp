// OpenBFME. GPL-3.0.
// IEEE CRC32 compatibility: independent bitwise oracle, not SAGE's checksum.
#include "doctest.h"
#include "Common/Crc32.h"
#include <vector>

namespace
{
std::uint32_t bitwiseCrc(const void *data, size_t size, std::uint32_t seed)
{
	const auto *p = static_cast<const std::uint8_t *>(data);
	std::uint32_t crc = ~seed;
	for (size_t i = 0; i < size; ++i)
	{
		crc ^= p[i];
		for (int k = 0; k < 8; ++k)
		{
			crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
		}
	}
	return ~crc;
}
std::uint32_t nextCrcInput(std::uint32_t &state)
{
	state ^= state << 13; state ^= state >> 17; state ^= state << 5;
	return state;
}
}

TEST_CASE("IEEE CRC32: every byte pair and seed preserves bitwise polynomial")
{
	CHECK(crc32Bytes(nullptr, 0) == 0u);
	CHECK(crc32Bytes("123456789", 9) == 0xCBF43926u);
	const std::uint32_t seeds[] = {0u, 1u, 0xFFFFFFFFu, 0x12345678u, 0x80000000u, 0xDEADBEEFu};
	for (auto seed : seeds)
	{
		CHECK(crc32Bytes(nullptr, 0, seed) == seed);
		for (unsigned a = 0; a < 256; ++a)
		{
			std::uint8_t bytes[] = {static_cast<std::uint8_t>(a), 0};
			CHECK(crc32Bytes(bytes, 1, seed) == bitwiseCrc(bytes, 1, seed));
			for (unsigned b = 0; b < 256; ++b)
			{
				bytes[1] = static_cast<std::uint8_t>(b);
				CHECK(crc32Bytes(bytes, 2, seed) == bitwiseCrc(bytes, 2, seed));
			}
		}
	}
}

TEST_CASE("IEEE CRC32: unaligned tails long spans and streamed seeds remain exact")
{
	std::uint32_t rng = 0x43524332u;
	std::vector<std::uint8_t> bytes(16449);
	for (size_t trial = 0; trial < 2048; ++trial)
	{
		const size_t size = nextCrcInput(rng) % 16385, offset = nextCrcInput(rng) % 64;
		for (auto &b : bytes) b = static_cast<std::uint8_t>(nextCrcInput(rng));
		const auto seed = nextCrcInput(rng);
		const auto *p = bytes.data() + offset;
		const auto expected = bitwiseCrc(p, size, seed);
		CHECK(crc32Bytes(p, size, seed) == expected);
		const size_t split = nextCrcInput(rng) % (size + 1);
		CHECK(crc32Bytes(p + split, size - split, crc32Bytes(p, split, seed)) == expected);
	}
	for (size_t offset = 0; offset < 64; ++offset)
	{
		for (size_t size = 0; size < 160; ++size)
		{
			const auto *p = bytes.data() + offset;
			CHECK(crc32Bytes(p, size, 0x87654321u) == bitwiseCrc(p, size, 0x87654321u));
		}
	}
	for (size_t size = 0; size < 160; ++size)
	{
		for (size_t split = 0; split <= size; ++split)
		{
			CHECK(crc32Bytes(bytes.data() + split, size - split, crc32Bytes(bytes.data(), split, 0x87654321u)) == bitwiseCrc(bytes.data(), size, 0x87654321u));
		}
	}
}
