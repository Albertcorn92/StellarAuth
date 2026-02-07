// ======================================================================
// \title  StellarAuthTester.cpp
// \author albertcornelius
// \brief  cpp file for StellarAuth component test harness implementation class
// ======================================================================

#include "StellarAuthTester.hpp"

namespace SolarAuth {

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

StellarAuthTester ::StellarAuthTester()
    : StellarAuthGTestBase("StellarAuthTester", StellarAuthTester::MAX_HISTORY_SIZE), component("StellarAuth") {
    this->initComponents();
    this->connectPorts();
}

StellarAuthTester ::~StellarAuthTester() {}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void StellarAuthTester ::toDo() {
    // TODO
}

}  // namespace SolarAuth
