// OpenBFME. GPL-3.0. See GodotDevice/GodotProcessMemory.h.
#include "GodotDevice/GodotProcessMemory.h"
#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/task_info.h>
#endif

namespace diagnostics
{
ProcessMemory processMemory()
{
	ProcessMemory out;
#ifdef __APPLE__
	// Apple's public task_info.h defines byte-valued TASK_VM_INFO fields. Rev1
	// includes phys_footprint; Rev3 adds its ledger peak. Check returned counts
	// before reading either revision, even with a newer build SDK.
	// https://github.com/apple-oss-distributions/xnu/blob/main/osfmk/mach/task_info.h
	out.backend = "mach_task_vm_info";
	out.requiredWords = TASK_VM_INFO_REV1_COUNT;
	task_vm_info_data_t info{};
	mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
	const kern_return_t result = task_info(mach_task_self(), TASK_VM_INFO, reinterpret_cast<task_info_t>(&info), &count);
	out.queryResult = result;
	out.returnedWords = count;
	if (result != KERN_SUCCESS)
	{
		out.status = "query_failed";
		return out;
	}
	if (count < TASK_VM_INFO_REV1_COUNT)
	{
		out.status = "short_response";
		return out;
	}
	out.residentBytes = static_cast<std::int64_t>(info.resident_size);
	out.residentPeakBytes = static_cast<std::int64_t>(info.resident_size_peak);
	out.physicalFootprintBytes = static_cast<std::int64_t>(info.phys_footprint);
	out.compressedBytes = static_cast<std::int64_t>(info.compressed);
	out.pageSizeBytes = info.page_size;
	if (count >= TASK_VM_INFO_REV3_COUNT && info.ledger_phys_footprint_peak >= 0)
	{
		out.physicalFootprintPeakBytes = info.ledger_phys_footprint_peak;
	}
	out.status = "ok";
	out.available = true;
#endif
	return out;
}
} // namespace diagnostics
