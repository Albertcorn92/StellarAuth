#ifndef SolarAuth_StellarAuthTester_HPP
#define SolarAuth_StellarAuthTester_HPP

#include <Fw/Types/BasicTypes.hpp>
#include "StellarAuthGTestBase.hpp"
#include <StellarAuth/StellarAuth.hpp>

namespace SolarAuth {

  class StellarAuthTester : public StellarAuthGTestBase {

    public:
      // INCREASED SIZE TO PREVENT OVERFLOW IN LONG TESTS
      static const I32 MAX_HISTORY_SIZE = 1000;

      StellarAuthTester();
      ~StellarAuthTester();

      // --- TEST SCENARIOS ---
      void testMissionSuccess();         
      void testTimingViolation();        
      void testYawWraparound();          
      void testReplayAttack();           
      void testStuckSensorActive();      
      void testTMRRepair();              
      void testEmergencyBypass();        

    private:
      StellarAuth component;
      void dispatchAll();
      void stepCycles(U32 cycles);
      void setInputs(F32 lightLevel, F32 yaw, U32 timeSec);
      void setupParameters(U32 start, U32 end, F32 yaw);
  };
}
#endif
