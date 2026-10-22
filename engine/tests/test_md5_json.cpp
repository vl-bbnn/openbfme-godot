// OpenBFME unit tests: MD5, JSON reader, built-in archive policies. GPL-3.0.

#include "doctest.h"

#include "Common/MD5.h"
#include "Common/MiniJson.h"
#include "Common/RetailArchivePolicy.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

TEST_CASE("MD5 matches RFC 1321 test vectors")
{
	CHECK(MD5::ofBytes("", 0) == "d41d8cd98f00b204e9800998ecf8427e");
	CHECK(MD5::ofBytes("abc", 3) == "900150983cd24fb0d6963f7d28e17f72");
	std::string digits = "12345678901234567890123456789012345678901234567890123456789012345678901234567890";
	CHECK(MD5::ofBytes(digits.data(), digits.size()) == "57edf4a22be3c955ac49da2e2107b67a");
}

TEST_CASE("file MD5 matches the portable digest across padding and read boundaries")
{
	const auto directory = std::filesystem::temp_directory_path() / ("openbfme-md5-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	REQUIRE(std::filesystem::create_directory(directory));
	struct Cleanup
	{
		std::filesystem::path path;
		~Cleanup() { std::error_code error; std::filesystem::remove_all(path, error); }
	} cleanup{ directory };
	const auto file = directory / std::filesystem::u8path(u8"hash input-\u0442\u0435\u0441\u0442.bin");
	const size_t readSize = 1 << 20;
	std::vector<unsigned char> bytes(readSize * 2 + 17);
	for (size_t i = 0; i < bytes.size(); ++i) bytes[i] = static_cast<unsigned char>((i * 13 + 7) % 256);
	for (size_t length : { size_t(0), size_t(1), size_t(55), size_t(56), size_t(63), size_t(64), size_t(65), size_t(127), size_t(128), size_t(129), readSize - 1, readSize, readSize + 1, bytes.size() })
	{
		std::ofstream out(file, std::ios::binary | std::ios::trunc);
		REQUIRE(out.is_open());
		out.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(length));
		out.close();
		REQUIRE_FALSE(out.fail());
		std::string error = "unchanged";
		CHECK(MD5::ofFile(file.u8string(), &error) == MD5::ofBytes(bytes.data(), length));
		CHECK(error == "unchanged");
	}
	{
		std::ofstream out(file, std::ios::binary | std::ios::trunc);
		out << "abc";
		out.close();
		REQUIRE_FALSE(out.fail());
	}
	CHECK(MD5::ofFile(file.u8string(), nullptr) == "900150983cd24fb0d6963f7d28e17f72");
	const auto originalTime = std::filesystem::last_write_time(file);
	{
		std::ofstream out(file, std::ios::binary | std::ios::trunc);
		out << "abd";
		out.close();
		REQUIRE_FALSE(out.fail());
	}
	std::filesystem::last_write_time(file, originalTime);
	CHECK(std::filesystem::file_size(file) == 3);
	CHECK(std::filesystem::last_write_time(file) == originalTime);
	CHECK(MD5::ofFile(file.u8string(), nullptr) == MD5::ofBytes("abd", 3));
	const std::string missing = (directory / "missing.bin").u8string();
	std::string error;
	CHECK(MD5::ofFile(missing, &error).empty());
	CHECK(error == "could not open " + missing + " for hashing");
	CHECK(MD5::ofFile(missing, nullptr).empty());
}

TEST_CASE("JSON reader parses what the policies use and rejects malformed input")
{
	JsonValue v;
	std::string error;
	REQUIRE(JsonValue::parse("{\"a\": [1, 2.5, -3e2], \"b\": \"x\\\"y\\u0041\", \"c\": true, \"d\": null}", v, &error));
	REQUIRE(v.get("a"));
	CHECK(v.get("a")->array.size() == 3);
	CHECK(v.get("a")->array[2].number == -300.0);
	CHECK(v.get("b")->string == "x\"yA");
	CHECK(v.get("c")->boolean);
	CHECK(v.get("d")->type == JsonValue::NUL);
	CHECK_FALSE(JsonValue::parse("{\"a\": 1,}", v, &error));
	CHECK_FALSE(JsonValue::parse("{\"a\": 1} x", v, &error));
	CHECK_FALSE(JsonValue::parse("{\"a\": 1, \"a\": 2}", v, &error));
}

TEST_CASE("built-in policies: RotWK 2.01 and BFME2 1.06 English")
{
	RetailArchivePolicy rotwk;
	RetailArchivePolicy bfme2;
	std::string error;
	REQUIRE_MESSAGE(RetailArchivePolicy::loadBuiltin("rotwk-201", rotwk, &error), error);
	REQUIRE_MESSAGE(RetailArchivePolicy::loadBuiltin("bfme2-106", bfme2, &error), error);
	CHECK(rotwk.game == "rotwk");
	CHECK(rotwk.patch == "2.01");
	CHECK(rotwk.archives.size() == 106);
	CHECK(bfme2.archives.size() == 107);
	CHECK(rotwk.excluded.size() == 4);
	CHECK(bfme2.excluded.empty());
	bool hasPatch = false;
	for (const RetailArchiveEntry &e : rotwk.archives)
	{
		CHECK(e.md5.size() == 32);
		CHECK(e.path.find("202") == std::string::npos); // no 2.02 archive in the pure list
		if (e.path == "_patch201.big")
		{
			hasPatch = true;
			CHECK(e.size == 19777872u);
		}
	}
	CHECK(hasPatch);
	CHECK_FALSE(RetailArchivePolicy::loadBuiltin("rotwk-202", rotwk, &error));
}
