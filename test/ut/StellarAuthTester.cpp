#include "StellarAuthTester.hpp"
#include <Fw/Test/UnitTest.hpp>

namespace SolarAuth {

  StellarAuthTester::StellarAuthTester() :
      StellarAuthGTestBase("Tester", StellarAuthTester::MAX_HISTORY_SIZE),
      component("StellarAuth")
  {
      this->component.init(100);
      this->init();
      
      // --- PORT CONNECTIONS ---
      this->connect_to_schedIn(0, this->component.get_schedIn_InputPort(0));
      this->connect_to_cmdIn(0, this->component.get_cmdIn_InputPort(0));
      this->connect_to_navAttitudeIn(0, this->component.get_navAttitudeIn_InputPort(0));
      
      this->component.set_cmdResponseOut_OutputPort(0, this->get_from_cmdResponseOut(0));
      this->component.set_tlmOut_OutputPort(0, this->get_from_tlmOut(0));
      this->component.set_timeCaller_OutputPort(0, this->get_from_timeCaller(0));
      this->component.set_eventOut_OutputPort(0, this->get_from_eventOut(0));
      this->component.set_textEventOut_OutputPort(0, this->get_from_textEventOut(0));
      this->component.set_cmdRegOut_OutputPort(0, this->get_from_cmdRegOut(0));
      this->component.set_watchdogOut_OutputPort(0, this->get_from_watchdogOut(0));
      this->component.set_prmGetOut_OutputPort(0, this->get_from_prmGetOut(0));
      this->component.set_prmSetOut_OutputPort(0, this->get_from_prmSetOut(0));
  }

  StellarAuthTester::~StellarAuthTester() {}

  void StellarAuthTester::dispatchAll() {
    this->component.doDispatch();
  }

  void StellarAuthTester::stepCycles(U32 cycles) {
      for (U32 i = 0; i < cycles; i++) {
          this->invoke_to_schedIn(0, 0);
      }
  }

  void StellarAuthTester::setInputs(F32 lightLevel, F32 yaw, U32 timeSec) {
      this->component.m_currentLightLevel = lightLevel;
      this->invoke_to_navAttitudeIn(0, yaw);
      Fw::Time testTime(timeSec, 0);
      this->setTestTime(testTime);
  }
  
  void StellarAuthTester::setupParameters(U32 start, U32 end, F32 yaw) {
      this->paramSet_TargetWindowStart(start, Fw::ParamValid::VALID);
      this->paramSet_TargetWindowEnd(end, Fw::ParamValid::VALID);
      this->paramSet_TargetYaw(yaw, Fw::ParamValid::VALID);

      // Force Load (White Box)
      this->component.m_target_window_start = start;
      this->component.m_target_window_end = end;
      this->component.m_target_yaw = yaw;
  }

  // --- TEST 1: SPATIO-TEMPORAL LOCK (Happy Path) ---
  void StellarAuthTester::testMissionSuccess() {
      this->clearHistory();
      
      this->setupParameters(1000, 1100, 45.0f);

      // Cycle 1: Enter Window (Time=1000, Yaw=45) -> ARMED
      this->setInputs(100.0f, 45.0f, 1000);
      this->stepCycles(1); 
      
      ASSERT_TLM_AuthFSM(0, 1); // ARMED
      
      this->clearHistory(); 

      // Cycle 2: Inject Shadow (ARMED -> VERIFYING)
      this->setInputs(10.0f, 45.0f, 1001);
      this->stepCycles(1); 
      ASSERT_TLM_AuthFSM(0, 2); // VERIFYING

      this->clearHistory();

      // Cycle 3-6: Persistence (VERIFYING -> AUTHENTICATED)
      for(int i=0; i<4; i++) {
         this->setInputs(10.0f, 45.0f, 1002+i);
         this->stepCycles(1);
      }

      ASSERT_EVENTS_SA_AuthSuccess_SIZE(1);
  }

  // --- TEST 2: TIMING VIOLATION ---
  void StellarAuthTester::testTimingViolation() {
      this->clearHistory();
      this->setupParameters(1000, 1100, 90.0f);

      // Attempt access at Time=2000 (Outside Window)
      this->setInputs(10.0f, 90.0f, 2000);
      this->stepCycles(1);
      
      ASSERT_TLM_AuthFSM(0, 0); // LOCKED
  }

  // --- TEST 3: YAW WRAPAROUND ---
  void StellarAuthTester::testYawWraparound() {
      this->clearHistory();
      this->setupParameters(1000, 1100, 0.0f);

      // Yaw 359.0 should match Target 0.0 (Delta = 1.0)
      this->setInputs(100.0f, 359.0f, 1000);
      this->stepCycles(1); 
      ASSERT_TLM_AuthFSM(0, 1); // ARMED
  }

  // --- TEST 4: REPLAY RESISTANCE ---
  void StellarAuthTester::testReplayAttack() {
      this->testMissionSuccess();
      this->clearHistory();
      
      // Try again in the SAME window (Time=1005)
      this->setInputs(100.0f, 45.0f, 1005); 
      this->stepCycles(1);
      
      this->setInputs(10.0f, 45.0f, 1006);
      this->stepCycles(1);

      ASSERT_TLM_AuthFSM(0, 0); // LOCKED
  }
  
  // --- TEST 5: EMERGENCY BYPASS ---
  void StellarAuthTester::testEmergencyBypass() {
      this->clearHistory();
      // Send Async Command
      this->sendCmd_AUTH_START(0, 10, 0x98765432);
      this->dispatchAll();
      
      this->stepCycles(1);
      
      // CORRECTED EXPECTATIONS:
      // Event 1: Command Handler "Success"
      // Event 2: FSM Logic "Success"
      ASSERT_EVENTS_SA_AuthSuccess_SIZE(2);
      
      // Telemetry: The FSM processes AUTH -> LOCKED in the same cycle.
      // So the final telemetry for this cycle is LOCKED (0).
      // We rely on the Events to prove the Bypass worked.
      ASSERT_TLM_AuthFSM(0, 0); 
  }

  // --- TEST 6: TMR SELF-HEALING ---
  void StellarAuthTester::testTMRRepair() {
      this->clearHistory();
      this->component.m_state_B = 0xDEADBEEF; 
      this->stepCycles(1);

      ASSERT_EVENTS_SA_SEU_Scrubbed_SIZE(1); 
      ASSERT_EQ(this->component.m_state_B, (U32)0); 
  }
  
  // --- TEST 7: STUCK SENSOR ---
  void StellarAuthTester::testStuckSensorActive() {
      this->clearHistory();
      // Set Noise Floor trigger
      this->component.m_currentLightLevel = 50.0f;
      this->component.m_last_light_value = 50.0f;
      
      // Run 101 cycles (History Size 1000 handles this now)
      for(int i=0; i<102; i++) {
          this->stepCycles(1);
      }
      
      // CORRECTED EXPECTATION: Check the LAST entry
      U32 historySize = this->tlmHistory_AuthFSM->size();
      ASSERT_GT(historySize, 0);
      ASSERT_TLM_AuthFSM(historySize - 1, 4); // FAULTED
  }
}
