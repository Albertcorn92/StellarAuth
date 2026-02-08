#include "StellarAuthTester.hpp"
#include <Fw/Test/UnitTest.hpp>

TEST(StellarAuthFinal, MissionSuccess) {
    SolarAuth::StellarAuthTester tester;
    tester.testMissionSuccess();
}

TEST(StellarAuthFinal, TimingViolation) {
    SolarAuth::StellarAuthTester tester;
    tester.testTimingViolation();
}

TEST(StellarAuthFinal, YawWraparound) {
    SolarAuth::StellarAuthTester tester;
    tester.testYawWraparound();
}

TEST(StellarAuthFinal, ReplayResistance) {
    SolarAuth::StellarAuthTester tester;
    tester.testReplayAttack();
}

TEST(StellarAuthFinal, EmergencyBypass) {
    SolarAuth::StellarAuthTester tester;
    tester.testEmergencyBypass();
}

TEST(StellarAuthFinal, TMRRepair) {
    SolarAuth::StellarAuthTester tester;
    tester.testTMRRepair();
}

TEST(StellarAuthFinal, StuckSensorFault) {
    SolarAuth::StellarAuthTester tester;
    tester.testStuckSensorActive();
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
