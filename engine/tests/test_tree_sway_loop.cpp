// OpenBFME. GPL-3.0. Presentation regression: the last sway sample must join the first without a zero-vector flash.
#include "doctest.h"
#include "GameEngineDevice/W3DDevice/GameClient/W3DVegetationSway.h"
#include "GameEngineDevice/W3DDevice/GameClient/Drawable/Draw/W3DDrawServices.h"

namespace
{
struct SwayRandom : W3DDrawRandom
{
	int draws = 0;
	int value(int lo, int) override { ++draws; return lo; }
	float real(float lo, float hi) override { ++draws; return (lo + hi) * 0.5f; }
};
}

TEST_CASE("tree sway presentation: the loop joins its endpoints and preserves the retail sampler and phase")
{
	W3DVegetationSway::Breeze breeze;
	breeze.directionX = 1.0f;
	breeze.intensity = 0.035f;
	breeze.lean = 0.02f;
	SwayRandom random;
	W3DVegetationSway sway(100.0f); // period 1, no randomness: exactly one table entry per client frame
	sway.update(breeze, random, 99.5);
	W3DVegetationSway::Vec3 retail[11], smooth[11];
	sway.swayOffsets(retail);
	sway.swayOffsets(smooth, true);
	CHECK(retail[1].x == 0.0f); // retail's documented gap, kept for callers that ask for it
	CHECK(retail[1].z == 0.0f);
	CHECK(smooth[0].x == 0.0f);
	CHECK(smooth[0].z == 0.0f);
	CHECK(smooth[1].x == (sway.tableEntry(99).x + sway.tableEntry(0).x) * 0.5f);
	CHECK(smooth[1].z == (sway.tableEntry(99).z + sway.tableEntry(0).z) * 0.5f);
	CHECK(sway.offset(0) == 99.5f);
	CHECK(random.draws == 20);
	sway.update(breeze, random, 0.5);
	sway.swayOffsets(smooth, true);
	CHECK(sway.offset(0) == 100.0f);
	CHECK(smooth[1].x == sway.tableEntry(0).x);
	CHECK(smooth[1].z == sway.tableEntry(0).z);
	sway.update(breeze, random, 0.5);
	sway.swayOffsets(smooth, true);
	CHECK(sway.offset(0) == 0.5f);
	CHECK(smooth[1].x == (sway.tableEntry(0).x + sway.tableEntry(1).x) * 0.5f);
	CHECK(smooth[1].z == (sway.tableEntry(0).z + sway.tableEntry(1).z) * 0.5f);
	CHECK(random.draws == 20);
}
