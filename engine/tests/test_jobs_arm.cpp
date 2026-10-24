// OpenBFME infrastructure tests. GPL-3.0.
// Apple SDK fenv.h: FE_DFL_DISABLE_DENORMS_ENV sets FPCR.FZ (bit 24).
#include "doctest.h"
#include "Common/JobSystem.h"
#include "Common/NumericState.h"
#include <atomic>
#include <cfenv>
#include <stdexcept>
#include <string>

#if defined(__APPLE__) && defined(__aarch64__)
TEST_CASE("Apple ARM jobs: FZ errors name FPCR and retain throwing-chunk details")
{
	struct Restore
	{
		std::fenv_t previous;
		Restore() { REQUIRE(std::fegetenv(&previous) == 0); }
		~Restore() { std::fesetenv(&previous); }
	} restore;
	for (int threads : {1, 4})
	{
		for (bool throwing : {false, true})
		{
			NumericState::setFPMode();
			JobSystem pool(threads, JobSystem::FloatingPoint::Simulation, "arm-diag");
			std::atomic<int> status{-1};
			bool reported = false;
			std::string what;
			try
			{
				pool.parallelFor(8, 1, [&](std::size_t chunk, std::size_t, std::size_t) {
					if (chunk == 3)
					{
						status.store(std::fesetenv(FE_DFL_DISABLE_DENORMS_ENV));
						if (throwing) throw std::runtime_error("chunk failed");
					}
				});
			}
			catch (const std::logic_error &error) { reported = true; what = error.what(); }
			std::fenv_t actual{};
			REQUIRE(std::fegetenv(&actual) == 0);
			const bool restored = (actual.__fpcr & 0x01000000ull) == 0;
			NumericState::setFPMode();
			const std::string expected = "JobSystem 'arm-diag': chunk 3 left the canonical floating-point environment (FPCR 0x01000000, expected FPCR & 0x01C09F00 == 0: nearest rounding, traps disabled, FZ off)";
			CHECK(status.load() == 0);
			CHECK(reported == true);
			CHECK(what == expected + (throwing ? " after throwing: chunk failed" : ""));
			CHECK(restored == true);
			std::atomic<int> completed{0};
			pool.parallelFor(8, 1, [&](std::size_t, std::size_t, std::size_t) { completed.fetch_add(1); });
			CHECK(completed.load() == 8);
		}
	}
}
#endif
