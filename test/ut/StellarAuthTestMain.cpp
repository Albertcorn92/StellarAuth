#include "StellarAuthTester.hpp"

TEST(StellarAuthFinal, MissionAuth) {
    SolarAuth::StellarAuthTester tester;
    tester.testSouthernCrossAuth();
}

TEST(StellarAuthFinal, SecurityHardening) {
    SolarAuth::StellarAuthTester tester;
    tester.testTumblingRejection();
    tester.testTimingViolation();
}

TEST(StellarAuthFinal, RadiationResilience) {
    SolarAuth::StellarAuthTester tester;
    tester.testRadiationSelfHealing();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
