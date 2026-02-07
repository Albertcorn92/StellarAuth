#include <StellarAuth/StellarAuth.hpp>
#include <cmath>

namespace SolarAuth {

  StellarAuth::StellarAuth(const char *const compName) : 
    StellarAuthComponentBase(compName), 
    m_heading(0.0f), 
    m_prevHeading(0.0f), 
    m_heartbeatCounter(0),
    m_scrubCount(0), 
    m_persistenceCounter(0), 
    m_isStable(true),
    m_currentLightLevel(100.0f), 
    m_prevLightLevel(100.0f),
    m_last_light_value(100.0f),
    m_stuck_sensor_counter(0),
    m_mission_cycle(0),
    m_last_auth_window_start(0xFFFFFFFF), 
    m_current_state(STATE_LOCKED),
    m_state_A(TMR_STATE_LOCKED), 
    m_state_B(TMR_STATE_LOCKED), 
    m_state_C(TMR_STATE_LOCKED)
  {
      // LOAD MOCK MISSION PLAN
      m_target_window_start = 490;
      m_target_window_end = 510;
      m_target_yaw = 45.0f; 
  }

  StellarAuth::~StellarAuth() {}

  U32 StellarAuth::doVote() {
    if (m_state_A == m_state_B) return m_state_A;
    if (m_state_A == m_state_C) return m_state_A;
    if (m_state_B == m_state_C) return m_state_B;
    return 0xFFFFFFFF; 
  }

  void StellarAuth::scrubMemory() {
    U32 validState = doVote();
    F32 health = 100.0f;
    if (validState == 0xFFFFFFFF) {
        this->log_FATAL_SA_TMR_Failure();
        updateState(TMR_STATE_LOCKED);
        health = 0.0f;
    } else {
        if (m_state_A != validState) { m_state_A = validState; m_scrubCount++; log_WARNING_LO_SA_SEU_Scrubbed(1); health -= 33.3f; }
        if (m_state_B != validState) { m_state_B = validState; m_scrubCount++; log_WARNING_LO_SA_SEU_Scrubbed(2); health -= 33.3f; }
        if (m_state_C != validState) { m_state_C = validState; m_scrubCount++; log_WARNING_LO_SA_SEU_Scrubbed(3); health -= 33.3f; }
    }
    this->tlmWrite_SA_HealthStatus(health);
  }

  void StellarAuth::updateState(U32 newState) {
    m_state_A = newState; m_state_B = newState; m_state_C = newState;
  }

  void StellarAuth::schedIn_handler(const FwIndexType portNum, U32 context) {
    this->m_heartbeatCounter++;
    this->m_mission_cycle++;
    scrubMemory(); 

    F32 current_yaw = m_heading; 
    F32 slope = m_currentLightLevel - m_prevLightLevel;

    bool inside_window = (m_mission_cycle >= m_target_window_start) && 
                         (m_mission_cycle <= m_target_window_end);

    if (m_current_state == STATE_ARMED || m_current_state == STATE_VERIFYING) {
        if (std::abs(m_currentLightLevel - m_last_light_value) < 0.001f) {
            m_stuck_sensor_counter++;
            if (m_stuck_sensor_counter > 100) m_current_state = STATE_FAULTED;
        } else {
            m_stuck_sensor_counter = 0;
        }
    }
    m_last_light_value = m_currentLightLevel;

    switch (m_current_state) {
        case STATE_LOCKED:
            if (inside_window) {
                if (m_last_auth_window_start != m_target_window_start) {
                    m_current_state = STATE_ARMED;
                }
            }
            break;

        case STATE_ARMED:
            if (!inside_window) {
                m_current_state = STATE_LOCKED; 
            } else {
                F32 delta = std::fmod(std::abs(current_yaw - m_target_yaw), 360.0f);
                if (delta > 180.0f) delta = 360.0f - delta;

                if (delta <= STABILITY_THRESHOLD && slope <= INGRESS_THRESHOLD) {
                    m_current_state = STATE_VERIFYING;
                    m_persistenceCounter = 0;
                    this->log_ACTIVITY_HI_SA_IngressDetected(slope);
                }
            }
            break;

        case STATE_VERIFYING:
            // FIX: Check if light stays DARK (< 50), not if slope stays negative.
            if (m_currentLightLevel < 50.0f) {
                m_persistenceCounter++;
                if (m_persistenceCounter >= AUTH_THRESHOLD) {
                     m_current_state = STATE_AUTHENTICATED;
                }
            } else {
                m_current_state = STATE_ARMED; 
                this->log_WARNING_LO_SA_PersistenceReset();
            }
            break;

        case STATE_AUTHENTICATED:
            updateState(TMR_STATE_UNLOCKED);
            this->log_ACTIVITY_HI_SA_AuthSuccess();
            m_last_auth_window_start = m_target_window_start; 
            m_current_state = STATE_LOCKED; 
            break;
            
        case STATE_FAULTED:
            this->log_WARNING_HI_SA_StabilityAlert();
            break;
    }

    m_prevLightLevel = m_currentLightLevel;
    this->tlmWrite_Heartbeat(m_heartbeatCounter);
    this->tlmWrite_AuthFSM(m_current_state);
    this->tlmWrite_SA_AuthState(doVote());
    this->tlmWrite_SA_IsStable(m_isStable ? 1 : 0);
  }

  void StellarAuth::UPDATE_EPHEMERIS_cmdHandler(const FwOpcodeType opCode, const U32 cmdSeq, U32 window_start, U32 window_end, F32 target_yaw) {
      this->m_target_window_start = window_start;
      this->m_target_window_end = window_end;
      this->m_target_yaw = target_yaw;
      this->log_ACTIVITY_HI_EphemerisUpdated(window_start, window_end, target_yaw);
      this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  void StellarAuth::AUTH_START_cmdHandler(const FwOpcodeType opCode, const U32 cmdSeq, U32 pattern_key) {
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  void StellarAuth::AUTH_RESET_cmdHandler(const FwOpcodeType opCode, const U32 cmdSeq) {
    updateState(TMR_STATE_LOCKED);
    m_current_state = STATE_LOCKED;
    m_persistenceCounter = 0;
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  void StellarAuth::LOAD_TRANSIT_SCHEDULE_cmdHandler(const FwOpcodeType opCode, const U32 cmdSeq, U32 starID, U32 seconds, U32 microseconds) {
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }
}
