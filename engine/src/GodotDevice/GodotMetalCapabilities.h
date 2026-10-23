// OpenBFME. GPL-3.0.
// Presentation-only capability query. The native Metal device is borrowed from
// Godot's active RenderingDevice; this helper never retains or releases it.
#pragma once
#include <cstdint>

namespace diagnostics
{
struct MetalSamplerCapabilities
{
	bool deviceAvailable = false;
	bool argumentBuffersTier2 = false;
	bool metal3Family = false;
};
#ifdef __APPLE__
MetalSamplerCapabilities metalSamplerCapabilities(std::uint64_t nativeDevice);
#else
inline MetalSamplerCapabilities metalSamplerCapabilities(std::uint64_t) { return {}; }
#endif
// Mirrors the direct argument-buffer eligibility of the pinned Godot 4.7.2
// Metal backend (metal_device_properties.h/.cpp). The debug override is also
// sampled at startup, just as in Godot. Neither the OS name nor MSL version
// string alone establishes this capability.
inline bool canUseDirectMetalSamplers(const MetalSamplerCapabilities &caps, bool disabled)
{
	return caps.deviceAvailable && caps.argumentBuffersTier2 && caps.metal3Family && !disabled;
}
} // namespace diagnostics
