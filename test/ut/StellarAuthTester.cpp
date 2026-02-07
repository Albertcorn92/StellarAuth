#include "StellarAuthTester.hpp"
#include <Fw/Test/UnitTest.hpp>

namespace SolarAuth {

  StellarAuthTester::StellarAuthTester() :
      StellarAuthGTestBase("Tester", StellarAuthTester::MAX_HISTORY_SIZE),
      component("StellarAuth")
  {
      this->component.init(100);
      this->init();
      this->connect_to_schedIn(0, this->component.get_schedIn_InputPort(0));
      this->connect_to_cmdIn(0, this->component.get_cmdIn_InputPort(0));
      this->component.set_cmdResponseOut_OutputPort(0, this->get_from_cmdResponseOut(0));
      this->component.set_tlmOut_OutputPort(0, this->get_from_tlmOut(0));
      this->component.set_timeCaller_OutputPort(0, this->get_from_timeCaller(0));
      this->component.set_eventOut_OutputPort(0, this->get_from_eventOut(0));
      this->component.set_textEventOut_OutputPort(0, this->get_from_textEventOut(0));
  }

  StellarAuthTester::~StellarAuthTester() {}

  void StellarAuthTester::dispatchAll() {
    while (this->component.m_queue.getMessagesAvailable() > 0) {
        this->component.doDispatch();
    }
  }

  void StellarAuthTester::stepCycles(U32 cycles) {
      for (U32 i = 0; i < cycles; i++) {
          this->invoke_to_schedIn(0, 0);
          this->dispatchAll();
      }
  }

  // --- TEST 1: SPATIO-TEMPORAL LOCK (Happy Path) ---
  void StellarAuthTester::testMissionSuccess() {
      this->clearHistory();
      
      this->sendCmd_UPDATE_EPHEMERIS(0, 10, 100, 120, 45.0f);
      this->dispatchAll();

      // Cycle 1: Enter Window (LOCKED -> ARMED)
      this->component.m_mission_cycle = 99;
      this->component.m_heading = 45.0f; 
      this->stepCycles(1); 
      ASSERT_TLM_AuthFSM(0, 1); // ARMED
      
      this->clearHistory(); 

      // Cycle 2: Inject Shadow (ARMED -> VERIFYING)
      this->component.m_prevLightLevel = 100.0f;
      this->component.m_currentLightLevel = 10.0f;
      this->stepCycles(1); 
      ASSERT_TLM_AuthFSM(0, 2); // VERIFYING

      this->clearHistory();

      // Cycle 3-6: Persistence (VERIFYING -> AUTHENTICATED -> LOCKED)
      this->component.m_prevLightLevel = 10.0f; 
      this->stepCycles(4); 

      // Verify Success Event
      ASSERT_EVENTS_SA_AuthSuccess_SIZE(1);

      // --- FIX: CLEAR HISTORY BEFORE CHECKING FINAL STATE ---
      // The history currently contains [2, 2, 3, 0]. We want to verify the 0.
      this->clearHistory();
      this->stepCycles(1); // Step one more time in the new LOCKED state
      ASSERT_TLM_AuthFSM(0, 0); // Now Index 0 is definitely 0.
  }

  // --- TEST 2: TIMING VIOLATION ---
  void StellarAuthTester::testTimingViolation() {
      this->clearHistory();
      this->sendCmd_UPDATE_EPHEMERIS(0, 10, 500, 520, 90.0f);
      this->dispatchAll();

      this->component.m_mission_cycle = 200;
      this->component.m_heading = 90.0f;
      this->component.m_prevLightLevel = 100.0f;
      this->component.m_currentLightLevel = 10.0f;
      
      this->stepCycles(1);
      ASSERT_TLM_AuthFSM(0, 0); 
  }

  // --- TEST 3: YAW WRAPAROUND ---
  void StellarAuthTester::testYawWraparound() {
      this->clearHistory();
      this->sendCmd_UPDATE_EPHEMERIS(0, 10, 100, 120, 1.0f);
      this->dispatchAll();

      this->component.m_mission_cycle = 99;
      this->component.m_heading = 359.0f; 
      this->stepCycles(1); 
      ASSERT_TLM_AuthFSM(0, 1); 

      this->clearHistory();

      this->component.m_prevLightLevel = 100.0f;
      this->component.m_currentLightLevel = 10.0f;
      this->stepCycles(1);

      ASSERT_TLM_AuthFSM(0, 2); 
  }

  // --- TEST 4: REPLAY RESISTANCE ---
  void StellarAuthTester::testReplayAttack() {
      this->clearHistory();
      this->sendCmd_UPDATE_EPHEMERIS(0, 10, 100, 120, 45.0f);
      this->dispatchAll();
      
      // Perform valid auth
      this->component.m_mission_cycle = 99;
      this->component.m_heading = 45.0f;
      this->stepCycles(1); 

      this->component.m_prevLightLevel = 100.0f;
      this->component.m_currentLightLevel = 10.0f;
      this->stepCycles(1); 

      this->component.m_prevLightLevel = 10.0f;
      this->stepCycles(4); 
      ASSERT_EVENTS_SA_AuthSuccess_SIZE(1);

      // --- REPLAY ATTACK ---
      this->clearHistory();
      
      this->component.m_prevLightLevel = 100.0f;
      this->component.m_currentLightLevel = 10.0f; // Shadow again
      this->stepCycles(1);

      // Verify Blocked (Stays LOCKED)
      ASSERT_TLM_AuthFSM(0, 0); 
      ASSERT_EVENTS_SA_AuthSuccess_SIZE(0);
  }

  // --- TEST 5: CONTEXT-AWARE FAULTS ---
  void StellarAuthTester::testStuckSensorActive() {
      this->clearHistory();
      this->sendCmd_UPDATE_EPHEMERIS(0, 10, 100, 120, 45.0f);
      this->dispatchAll();
      
      this->component.m_mission_cycle = 99;
      this->stepCycles(1); 
      this->clearHistory(); 

      this->component.m_currentLightLevel = 50.0f;
      this->component.m_last_light_value = 50.0f;
      this->component.m_stuck_sensor_counter = 100;
      
      this->stepCycles(1);
      ASSERT_TLM_AuthFSM(0, 4); 
  }

  // --- TEST 6: TMR SELF-HEALING ---
  void StellarAuthTester::testTMRRepair() {
      this->clearHistory();
      this->component.m_state_A = 0xDEADBEEF;
      this->component.m_state_B = 0xBADF00D; 
      this->component.m_state_C = 0xDEADBEEF;

      this->stepCycles(1);

      ASSERT_EVENTS_SA_SEU_Scrubbed_SIZE(1); 
      ASSERT_EQ(this->component.m_state_B, static_cast<U32>(0xDEADBEEF)); 
  }
}
