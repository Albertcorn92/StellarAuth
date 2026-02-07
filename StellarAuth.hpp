#ifndef SOLARAUTH_STELLARAUTH_HPP
#define SOLARAUTH_STELLARAUTH_HPP

#include "StellarAuth/StellarAuthComponentAc.hpp"

namespace SolarAuth {

  class StellarAuth final : public StellarAuthComponentBase {

    public:
      StellarAuth(const char *const compName);
      ~StellarAuth();

    private:
      friend class StellarAuthTester;

      U32 m_heading;
      U32 m_prevHeading;
      U32 m_heartbeatCounter;
      U32 m_currentTargetStar;
      U32 m_scrubCount;
      U32 m_persistenceCounter;
      bool m_isStable;

      // --- PASSIVE SHADOW LOGIC ---
      F32 m_currentLightLevel;
      F32 m_prevLightLevel;
      static constexpr F32 INGRESS_THRESHOLD = -40.0f; 
      static constexpr U32 STABILITY_THRESHOLD = 5; // Max 5 deg change/tick

      U32 m_expectedStarID;
      Fw::Time m_expectedTime;
      static const I32 TIME_WINDOW_SEC = 2;

      // --- TMR BLOCK ---
      static const U32 STATE_LOCKED   = 0xDEADBEEF;
      static const U32 STATE_UNLOCKED = 0xCAFEBABE;
      static const U32 AUTH_THRESHOLD = 3;

      U32 m_state_A;
      U32 m_state_B;
      U32 m_state_C;

      U32 identifyStar(U32 heading);
      U32 doVote(); 
      void scrubMemory(); 
      void updateState(U32 newState);
      bool detectShadowEdge(); 

      void schedIn_handler(const FwIndexType portNum, U32 context);
      void AUTH_START_cmdHandler(const FwOpcodeType opCode, const U32 cmdSeq, U32 pattern_key);
      void AUTH_RESET_cmdHandler(const FwOpcodeType opCode, const U32 cmdSeq);
      void LOAD_TRANSIT_SCHEDULE_cmdHandler(const FwOpcodeType opCode, const U32 cmdSeq, U32 starID, U32 seconds, U32 microseconds);
  };
}
#endif
