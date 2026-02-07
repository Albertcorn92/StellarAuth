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
  }

  StellarAuthTester::~StellarAuthTester() {}

  void StellarAuthTester::dispatchAll() {
    while (this->component.m_queue.getMessagesAvailable() > 0) {
        this->component.doDispatch();
    }
  }

  void StellarAuthTester::testSouthernCrossAuth() {
      this->clearHistory();
      this->sendCmd_LOAD_TRANSIT_SCHEDULE(0, 0, 303, 500, 0);
      this->dispatchAll();
      this->setTestTime(Fw::Time(500, 0));
      
      // Force reset component internal state for clean test
      this->component.m_heading = 180;
      this->component.m_prevHeading = 180;
      this->component.m_persistenceCounter = 0;

      for (U32 i = 1; i <= 3; i++) {
          this->component.m_prevLightLevel = 100.0f;
          this->component.m_currentLightLevel = 40.0f;
          this->invoke_to_schedIn(0, 0);
          this->dispatchAll();
          ASSERT_TLM_SA_PersistenceCounter(i-1, i);
      }
      ASSERT_EQ(this->component.doVote(), static_cast<U32>(0xCAFEBABE));
  }

  void StellarAuthTester::testTumblingRejection() {
      this->clearHistory();
      this->sendCmd_LOAD_TRANSIT_SCHEDULE(0, 0, 101, 100, 0);
      this->dispatchAll();
      this->setTestTime(Fw::Time(100, 0));

      // Ensure component starts stable at 0 degrees
      this->component.m_heading = 0;
      this->component.m_prevHeading = 0;
      this->component.m_prevLightLevel = 100.0f;
      this->component.m_currentLightLevel = 40.0f;
      this->component.m_persistenceCounter = 0;
      this->component.updateState(0xDEADBEEF);

      // Tick 1: Stable & Valid -> Persistence 1
      this->invoke_to_schedIn(0, 0);
      this->dispatchAll();
      ASSERT_TLM_SA_PersistenceCounter(0, 1);

      // Tick 2: TUMBLE -> Heading jump 50 degrees
      this->component.m_heading = 50; 
      this->invoke_to_schedIn(0, 0);
      this->dispatchAll();

      ASSERT_TLM_SA_PersistenceCounter(1, 0); // Reset verified
      ASSERT_EVENTS_SA_StabilityAlert_SIZE(1);
  }

  void StellarAuthTester::testRadiationSelfHealing() {
      this->clearHistory();
      this->component.updateState(0xCAFEBABE);
      this->component.m_state_B = 0x0;
      this->invoke_to_schedIn(0, 0);
      this->dispatchAll();
      ASSERT_EQ(this->component.m_state_B, static_cast<U32>(0xCAFEBABE));
  }

  void StellarAuthTester::testTimingViolation() {
      this->clearHistory();
      this->sendCmd_LOAD_TRANSIT_SCHEDULE(0, 0, 101, 100, 0);
      this->dispatchAll();
      this->setTestTime(Fw::Time(200, 0)); 
      this->component.m_heading = 0;
      this->component.m_prevHeading = 0;
      this->invoke_to_schedIn(0, 0);
      this->dispatchAll();
      ASSERT_TLM_SA_PersistenceCounter(0, 0);
  }
}
