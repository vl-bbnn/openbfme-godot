// OpenBFME infrastructure numerics tests. GPL-3.0.
// Apple SDK fenv.h documents FE_DFL_DISABLE_DENORMS_ENV and FPCR.FZ.
// Hand vector: 2^-127 + 2^-126 = 3*2^-127, exactly binary32 0x00C00000.
#include "doctest.h"
#include "Common/NumericState.h"
#include <cfenv>
#include <cstdint>
#include <cstring>
#include <limits>

#if defined(__APPLE__) && defined(__aarch64__)
namespace
{
struct RestoreEnvironment
{
	std::fenv_t previous;
	RestoreEnvironment() { REQUIRE(std::fegetenv(&previous) == 0); }
	~RestoreEnvironment() { std::fesetenv(&previous); }
};
std::uint32_t bitsOfArm(float value)
{
	std::uint32_t bits;
	std::memcpy(&bits, &value, sizeof(bits));
	return bits;
}
float armSubnormal()
{
	const std::uint32_t bits = 0x00400000u;
	float value;
	std::memcpy(&value, &bits, sizeof(value));
	return value;
}
} // namespace

TEST_CASE("Apple ARM FPCR: FZ is noncanonical and PC24 retains subnormal operands")
{
	RestoreEnvironment restore;
	NumericState::setFPMode();
	const bool baselineCanonical = NumericState::floatingPointEnvironmentIsCanonical();
	volatile float normal = std::numeric_limits<float>::min();
	volatile float small = armSubnormal();
	const auto baseline = bitsOfArm(NumericState::pc24Add(small, normal));
	REQUIRE(std::fesetenv(FE_DFL_DISABLE_DENORMS_ENV) == 0);
	const bool hostileCanonical = NumericState::floatingPointEnvironmentIsCanonical();
	const auto hostileProduct = bitsOfArm(NumericState::sseMul(normal, 0.5f));
	const auto hostile = bitsOfArm(NumericState::pc24Add(small, normal));
	const auto reference = bitsOfArm(NumericState::reference::pc24Add(small, normal));
	NumericState::normalizeFloatingPointEnvironment();
	const bool normalizedCanonical = NumericState::floatingPointEnvironmentIsCanonical();
	const auto normalizedProduct = bitsOfArm(NumericState::sseMul(normal, 0.5f));
	CHECK(baselineCanonical == true);
	CHECK(baseline == 0x00C00000u);
	CHECK(hostileCanonical == false);
	CHECK(hostileProduct == 0u); // premise: the Apple hostile environment really flushes
	CHECK(reference == 0x00C00000u);
	CHECK(hostile == 0x00C00000u);
	CHECK(normalizedCanonical == true);
	CHECK(normalizedProduct == 0x00400000u);
}

TEST_CASE("Apple ARM FPCR: each directed rounding mode leaves the PC24 fast path")
{
	RestoreEnvironment restore;
	volatile float normal = std::numeric_limits<float>::min();
	volatile float small = armSubnormal();
	for (int rounding : {FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO})
	{
		NumericState::setFPMode();
		REQUIRE(std::fesetround(rounding) == 0);
		const bool canonical = NumericState::floatingPointEnvironmentIsCanonical();
		const auto result = bitsOfArm(NumericState::pc24Add(small, normal));
		NumericState::setFPMode();
		CHECK(canonical == false);
		CHECK(result == 0x00C00000u);
		CHECK(NumericState::floatingPointEnvironmentIsCanonical() == true);
	}
}
#endif
