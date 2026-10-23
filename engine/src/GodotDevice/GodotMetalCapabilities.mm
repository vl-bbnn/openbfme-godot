// OpenBFME. GPL-3.0.
#include "GodotDevice/GodotMetalCapabilities.h"
#import <Metal/Metal.h>
#include <TargetConditionals.h>

namespace diagnostics
{
MetalSamplerCapabilities metalSamplerCapabilities(std::uint64_t nativeDevice)
{
	MetalSamplerCapabilities caps;
#if TARGET_OS_OSX
	if (nativeDevice == 0) return caps;
	// Godot's Metal driver returns its MTLDevice for LOGICAL_DEVICE. Borrow it
	// only for these const feature queries; no retain/release or command work.
	id<MTLDevice> device = (__bridge id<MTLDevice>)(reinterpret_cast<void *>(static_cast<std::uintptr_t>(nativeDevice)));
	caps.deviceAvailable = true;
	caps.argumentBuffersTier2 = [device argumentBuffersSupport] == MTLArgumentBuffersTier2;
	if (@available(macOS 13.0, *))
	{
		// Godot can encode the argument-buffer layout directly only on this
		// family with tier 2, otherwise its Metal16 slot path still applies.
		caps.metal3Family = [device supportsFamily:MTLGPUFamilyMetal3];
	}
#endif
	return caps;
}
} // namespace diagnostics
