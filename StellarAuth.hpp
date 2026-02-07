#ifndef SOLARAUTH_STELLARAUTH_HPP
#define SOLARAUTH_STELLARAUTH_HPP

#include "StellarAuth/StellarAuthComponentAc.hpp"
#include <cmath> // For std::abs and std::fmod

namespace SolarAuth {

  class StellarAuth final : public StellarAuthComponentBase {

    public:
      StellarAuth(const char *const compName);
      ~StellarAuth();

    private:
      friend class StellarAuthTester;

      // --- NEW: FINITE STATE MACHINE ---
      enum AuthState {
          STATE_LOCKED = 0,
          STATE_ARMED,        // Inside Time Window
          STATE_VERIFYING,    // Checking Persistence
          STATE_AUTHENTICATED,// Success
          STATE_FAULTED       // Sensor Failure
      };

      // --- COMPONENT VARIABLES ---
      U32 m_heading; 
      U32 m_prevHeading;
      U32 m_heartbeatCounter;
      U32 m_scrubCount;
      U32 m_persistenceCounter;
      bool m_isStable;

      // --- PASSIVE SHADOW LOGIC ---
      F32 m_currentLightLevel;
      F32 m_prevLightLevel;
      F32 m_last_light_value; // For stuck sensor check
      U32 m_stuck_sensor_counter;

      static constexpr F32 INGRESS_THRESHOLD = -40.0f; 
      static constexpr F32 STABILITY_THRESHOLD = 5.0f;

      // --- MISSION PLAN (Configurable) ---
      U32 m_target_window_start;
      U32 m_target_window_end;
      F32 m_target_yaw;
      U32 m_mission_cycle;
      U32 m_last_auth_window_start; // Replay Resistance

      // --- INTERNAL STATE ---
      AuthState m_current_state;

      // --- TMR BLOCK ---
      static const U32 TMR_STATE_LOCKED   = 0xDEADBEEF;
      static const U32 TMR_STATE_UNLOCKED = 0xCAFEBABE;
      static const U32 AUTH_THRESHOLD = 3;

      U32 m_state_A;
      U32 m_state_B;
      U32 m_state_C;

      // --- HELPER METHODS ---
      U32 doVote(); 
      void scrubMemory(); 
      void updateState(U32 newState);

      // --- HANDLERS ---
      void schedIn_handler(const FwIndexType portNum, U32 context);
      void AUTH_START_cmdHandler(const FwOpcodeType opCode, const U32 cmdSeq, U32 pattern_key);
      void AUTH_RESET_cmdHandler(const FwOpcodeType opCode, const U32 cmdSeq);
      void LOAD_TRANSIT_SCHEDULE_cmdHandler(const FwOpcodeType opCode, const U32 cmdSeq, U32 starID, U32 seconds, U32 microseconds);
      
      // [NEW]
      void UPDATE_EPHEMERIS_cmdHandler(const FwOpcodeType opCode, const U32 cmdSeq, U32 window_start, U32 window_end, F32 target_yaw);
  };
}
#endif
