#ifndef SolarAuth_StellarAuthTester_HPP
#define SolarAuth_StellarAuthTester_HPP

#include <Fw/Types/BasicTypes.hpp>
#include "StellarAuthGTestBase.hpp"
#include <StellarAuth/StellarAuth.hpp>

namespace SolarAuth {

  class StellarAuthTester : public StellarAuthGTestBase {

    public:
      static const I32 MAX_HISTORY_SIZE = 100;

      StellarAuthTester();
      ~StellarAuthTester();

      // --- THE "FINAL 5" FLIGHT READINESS TESTS ---
      
      // 1. Spatio-Temporal Lock
      void testMissionSuccess();         // Happy Path: Right Time + Right Star
      void testTimingViolation();        // Shadow at Wrong Time -> LOCKED
      
      // 2. Spatial Math
      void testYawWraparound();          // Shadow at 359 deg -> UNLOCKED (Math Fix)
      
      // 3. Replay Resistance
      void testReplayAttack();           // Reuse same valid shadow -> BLOCKED
      
      // 4. Context-Aware Faults
      void testStuckSensorCruise();      // Stuck at 0 while LOCKED -> NO FAULT
      void testStuckSensorActive();      // Stuck at 0 while ARMED -> FAULTED
      
      // 5. Reliability
      void testTMRRepair();              // Bit flip -> Self-Healed

    private:
      StellarAuth component;
      void dispatchAll();
      
      // Helper to simulate "Time Passing"
      void stepCycles(U32 cycles);
  };
}
#endif
