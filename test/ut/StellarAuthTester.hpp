#ifndef SolarAuth_StellarAuthTester_HPP
#define SolarAuth_StellarAuthTester_HPP

#include <Fw/Types/BasicTypes.hpp>
#include "StellarAuthGTestBase.hpp"
#include <StellarAuth/StellarAuth.hpp>

namespace SolarAuth {

  class StellarAuthTester : public StellarAuthGTestBase {

    public:
      static const I32 MAX_HISTORY_SIZE = 20;

      StellarAuthTester();
      ~StellarAuthTester();

      // --- FULL MISSION TEST SUITE ---
      void testPolarisAuthSequence();    // Standard North Heading Auth
      void testSouthernCrossAuth();      // South Heading + Shadow Ingress
      void testTumblingRejection();     // Anti-Spoofing / Stability Check
      void testTimingViolation();       // Predictive Window Validation
      void testRadiationSelfHealing();   // TMR Bit-Flip Repair
      void testTotalMemoryCorruption(); // Safe-Mode on Fatal TMR Failure

    private:
      StellarAuth component;
      void dispatchAll();
  };
}
#endif
