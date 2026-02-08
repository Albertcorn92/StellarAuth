#ifndef SOLARAUTH_STELLARAUTH_HPP
#define SOLARAUTH_STELLARAUTH_HPP

#include "StellarAuth/StellarAuthComponentAc.hpp"
#include <cmath> 

namespace SolarAuth {

  class StellarAuth final : public StellarAuthComponentBase {

    public:
      StellarAuth(const char *const compName);
      ~StellarAuth();

    private:
      friend class StellarAuthTester;

      // --- FINITE STATE MACHINE ---
      enum AuthState {
          STATE_LOCKED = 0,
          STATE_ARMED,        // Inside Time Window
          STATE_VERIFYING,    // Checking Persistence
          STATE_AUTHENTICATED,// Success
          STATE_FAULTED       // Sensor Failure
      };

      // --- SECURITY KEYS ---
      // Emergency Bypass Key (Break Glass in Case of Fire)
      static const U32 EMERGENCY_KEY = 0x98765432;

      // --- COMPONENT VARIABLES ---
      F32 m_heading; 
      U32 m_heartbeatCounter;
      U32 m_scrubCount;
      bool m_isStable;

      // --- PASSIVE SHADOW LOGIC ---
      F32 m_currentLightLevel;
      F32 m_prevLightLevel;
      F32 m_last_light_value; 
      U32 m_stuck_sensor_counter;

      static constexpr F32 INGRESS_THRESHOLD = -40.0f; 
      static constexpr F32 STABILITY_THRESHOLD = 5.0f;
      static constexpr F32 LIGHT_NOISE_FLOOR = 1.0f; // Lux

      // --- MISSION PARAMETERS (Shadow Copies) ---
      U32 m_target_window_start;
      U32 m_target_window_end;
      F32 m_target_yaw;
      
      U32 m_last_auth_window_start; // Replay Resistance

      // --- INTERNAL STATE ---
      AuthState m_current_state;

      // --- TMR BLOCK (Triple Modular Redundancy) ---
      // Storing RAW Enum values (0-4), not magic hex, to prevent cast errors.
      U32 m_state_A;
      U32 m_state_B;
      U32 m_state_C;
      
      // TMR for the Persistence Counter (The "Key")
      U32 m_persistence_A;
      U32 m_persistence_B;
      U32 m_persistence_C;

      static const U32 AUTH_THRESHOLD = 3;

      // --- HELPER METHODS ---
      U32 doVoteState() const; 
      U32 doVotePersistence() const;
      void scrubMemory(); 
      void updateState(AuthState newState);
      void updatePersistence(U32 newVal);
      void loadParameters();

      // --- HANDLERS ---
      void schedIn_handler(const FwIndexType portNum, U32 context);
      void AUTH_START_cmdHandler(const FwOpcodeType opCode, const U32 cmdSeq, U32 pattern_key);
      void AUTH_RESET_cmdHandler(const FwOpcodeType opCode, const U32 cmdSeq);
      void LOAD_TRANSIT_SCHEDULE_cmdHandler(const FwOpcodeType opCode, const U32 cmdSeq, U32 starID, U32 seconds, U32 microseconds);
      
      // Port Handler for GNC Attitude
      void navAttitudeIn_handler(const FwIndexType portNum, F32 yaw);
  };
}
#endif
