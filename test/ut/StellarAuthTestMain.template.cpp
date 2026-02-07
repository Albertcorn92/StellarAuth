// ======================================================================
// \title  StellarAuthTestMain.cpp
// \author albertcornelius
// \brief  cpp file for StellarAuth component test main function
// ======================================================================

#include "StellarAuthTester.hpp"

TEST(Nominal, toDo) {
    SolarAuth::StellarAuthTester tester;
    tester.toDo();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
