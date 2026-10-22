// OpenBFME. GPL-3.0.
// Explicit device-layer diagnostics, never simulation input. Values are bytes;
// -1 means unavailable, not zero usage. No sampling happens unless called.
#pragma once
#include <cstdint>

namespace diagnostics
{
struct ProcessMemory
{
	const char *backend = "unsupported";
	const char *status = "unsupported";
	std::int64_t queryResult = -1;
	std::int64_t returnedWords = 0;
	std::int64_t requiredWords = 0;
	std::int64_t residentBytes = -1;
	std::int64_t residentPeakBytes = -1;
	std::int64_t physicalFootprintBytes = -1;
	std::int64_t physicalFootprintPeakBytes = -1;
	std::int64_t compressedBytes = -1;
	std::int64_t pageSizeBytes = -1;
	bool available = false;
};
ProcessMemory processMemory();
} // namespace diagnostics
