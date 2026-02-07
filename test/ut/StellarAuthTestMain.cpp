#include "StellarAuthTester.hpp"

// GROUP 1: MISSION LOGIC
TEST(StellarAuthFinal, MissionSuccess) {
    SolarAuth::StellarAuthTester tester;
    tester.testMissionSuccess();
}

TEST(StellarAuthFinal, TimingViolation) {
    SolarAuth::StellarAuthTester tester;
    tester.testTimingViolation();
}

// GROUP 2: ADVANCED MATH & SECURITY
TEST(StellarAuthFinal, YawWraparound) {
    SolarAuth::StellarAuthTester tester;
    tester.testYawWraparound();
}

TEST(StellarAuthFinal, ReplayResistance) {
    SolarAuth::StellarAuthTester tester;
    tester.testReplayAttack();
}

// GROUP 3: RELIABILITY & FAULTS
TEST(StellarAuthFinal, StuckSensorFault) {
    SolarAuth::StellarAuthTester tester;
    tester.testStuckSensorActive();
}

TEST(StellarAuthFinal, RadiationResilience) {
    SolarAuth::StellarAuthTester tester;
    tester.testTMRRepair();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
