#include <StellarAuth/StellarAuth.hpp>
#include <cmath>
#include <Fw/Types/String.hpp>

namespace SolarAuth {

  StellarAuth::StellarAuth(const char *const compName) : 
    StellarAuthComponentBase(compName), 
    m_heading(0.0f), 
    m_heartbeatCounter(0), 
    m_scrubCount(0), 
    m_isStable(true), 
    m_currentLightLevel(100.0f), 
    m_prevLightLevel(100.0f),
    m_last_light_value(100.0f),
    m_stuck_sensor_counter(0),
    m_last_auth_window_start(0), 
    m_current_state(STATE_LOCKED),
    m_state_A((U32)STATE_LOCKED), 
    m_state_B((U32)STATE_LOCKED), 
    m_state_C((U32)STATE_LOCKED),
    m_persistence_A(0),
    m_persistence_B(0),
    m_persistence_C(0)
  {
      // Safe Initialization (Inert)
      m_target_window_start = 0;
      m_target_window_end = 0;
      m_target_yaw = 0.0f;
      
      // FIX: Event logging removed from constructor to prevent 
      // "Port Not Connected" crashes during static initialization.
  }

  StellarAuth::~StellarAuth() {}

  // --- TMR VOTING LOGIC ---
  U32 StellarAuth::doVoteState() const {
    if (m_state_A == m_state_B) return m_state_A;
    if (m_state_A == m_state_C) return m_state_A;
    if (m_state_B == m_state_C) return m_state_B;
    return 0xFFFFFFFF; // Critical Failure
  }
  
  U32 StellarAuth::doVotePersistence() const {
    if (m_persistence_A == m_persistence_B) return m_persistence_A;
    if (m_persistence_A == m_persistence_C) return m_persistence_A;
    if (m_persistence_B == m_persistence_C) return m_persistence_B;
    return 0; // Default to safe (0) if corrupted
  }

  void StellarAuth::scrubMemory() {
    // 1. Scrub State
    U32 validState = doVoteState();
    F32 health = 100.0f;
    
    if (validState == 0xFFFFFFFF) {
        this->log_FATAL_SA_TMR_Failure();
        updateState(STATE_FAULTED);
        health = 0.0f;
    } else {
        if (m_state_A != validState) { m_state_A = validState; m_scrubCount++; log_WARNING_LO_SA_SEU_Scrubbed(1); }
        if (m_state_B != validState) { m_state_B = validState; m_scrubCount++; log_WARNING_LO_SA_SEU_Scrubbed(2); }
        if (m_state_C != validState) { m_state_C = validState; m_scrubCount++; log_WARNING_LO_SA_SEU_Scrubbed(3); }
    }
    
    // 2. Scrub Persistence
    U32 validPersist = doVotePersistence();
    if (m_persistence_A != validPersist) { m_persistence_A = validPersist; m_scrubCount++; }
    if (m_persistence_B != validPersist) { m_persistence_B = validPersist; m_scrubCount++; }
    if (m_persistence_C != validPersist) { m_persistence_C = validPersist; m_scrubCount++; }

    // Only penalize health if scrub count is abnormally high (>1000 per cycle)
    if (m_scrubCount > 1000) health = 50.0f; 

    this->tlmWrite_SA_HealthStatus(health);
  }

  void StellarAuth::updateState(AuthState newState) {
    m_state_A = (U32)newState; 
    m_state_B = (U32)newState; 
    m_state_C = (U32)newState;
    m_current_state = newState;
  }
  
  void StellarAuth::updatePersistence(U32 newVal) {
    m_persistence_A = newVal;
    m_persistence_B = newVal;
    m_persistence_C = newVal;
  }

  // --- PARAMETER LOADING ---
  void StellarAuth::loadParameters() {
      Fw::ParamValid valid;
      U32 valU32;
      F32 valF32;

      valU32 = paramGet_TargetWindowStart(valid);
      if (valid == Fw::ParamValid::VALID) m_target_window_start = valU32;

      valU32 = paramGet_TargetWindowEnd(valid);
      if (valid == Fw::ParamValid::VALID) m_target_window_end = valU32;

      valF32 = paramGet_TargetYaw(valid);
      if (valid == Fw::ParamValid::VALID) m_target_yaw = valF32;
  }

  // --- GNC INPUT HANDLER ---
  void StellarAuth::navAttitudeIn_handler(const FwIndexType portNum, F32 yaw) {
      this->m_heading = yaw;
      this->tlmWrite_CurrentYaw(yaw);
  }

  // --- MAIN SCHEDULER ---
  void StellarAuth::schedIn_handler(const FwIndexType portNum, U32 context) {
    // 1. SAFETY: Stroke Watchdog
    this->watchdogOut_out(0, 0);

    // 2. TIME: Get Absolute Time
    Fw::Time timeStamp = this->getTime(); 
    U32 current_time_sec = timeStamp.getSeconds();

    this->m_heartbeatCounter++;
    scrubMemory(); 
    loadParameters();

    // 3. LOGIC: Calculate Inputs
    F32 current_yaw = m_heading; 
    F32 slope = m_currentLightLevel - m_prevLightLevel;
    U32 current_persistence = doVotePersistence();

    bool inside_window = (current_time_sec >= m_target_window_start) && 
                         (current_time_sec <= m_target_window_end);

    // 4. LOGIC: Sensor Health (Stuck Check)
    if (m_currentLightLevel > LIGHT_NOISE_FLOOR) {
        if (std::abs(m_currentLightLevel - m_last_light_value) < 0.001f) {
            m_stuck_sensor_counter++;
            if (m_stuck_sensor_counter > 100) {
                 updateState(STATE_FAULTED);
            }
        } else {
            m_stuck_sensor_counter = 0;
        }
    }
    m_last_light_value = m_currentLightLevel;

    // 5. FSM EXECUTION
    U32 votedStateRaw = doVoteState();
    if (votedStateRaw == 0xFFFFFFFF) {
        m_current_state = STATE_FAULTED;
    } else {
        m_current_state = (AuthState)votedStateRaw;
    }

    switch (m_current_state) {
        case STATE_LOCKED:
            if (inside_window) {
                if (m_last_auth_window_start != m_target_window_start) {
                    updateState(STATE_ARMED);
                }
            }
            break;

        case STATE_ARMED:
            if (!inside_window) {
                updateState(STATE_LOCKED); 
            } else {
                F32 delta = std::fmod(std::abs(current_yaw - m_target_yaw), 360.0f);
                if (delta > 180.0f) delta = 360.0f - delta;

                if (delta <= STABILITY_THRESHOLD && slope <= INGRESS_THRESHOLD) {
                    updateState(STATE_VERIFYING);
                    updatePersistence(0);
                    this->log_ACTIVITY_HI_SA_IngressDetected(slope);
                }
            }
            break;

        case STATE_VERIFYING:
            if (m_currentLightLevel < 50.0f) {
                current_persistence++;
                updatePersistence(current_persistence);
                
                if (current_persistence >= AUTH_THRESHOLD) {
                     updateState(STATE_AUTHENTICATED);
                }
            } else {
                updateState(STATE_ARMED); 
                updatePersistence(0);
                this->log_WARNING_LO_SA_PersistenceReset();
            }
            break;

        case STATE_AUTHENTICATED:
            this->log_ACTIVITY_HI_SA_AuthSuccess(false); // Not a bypass
            m_last_auth_window_start = m_target_window_start;
            updateState(STATE_LOCKED); 
            break;
            
        case STATE_FAULTED:
            if (m_isStable) { 
                this->log_WARNING_HI_SA_StabilityAlert();
                m_isStable = false; 
            }
            break;
            
        default:
            this->log_FATAL_SA_TMR_Failure();
            updateState(STATE_FAULTED);
            break;
    }

    m_prevLightLevel = m_currentLightLevel;
    
    // 6. TELEMETRY
    this->tlmWrite_Heartbeat(m_heartbeatCounter);
    this->tlmWrite_AuthFSM((U32)m_current_state);
    this->tlmWrite_SA_AuthState(votedStateRaw);
    this->tlmWrite_SA_PersistenceCounter(current_persistence);
}

  // --- COMMAND HANDLERS ---

  void StellarAuth::AUTH_START_cmdHandler(const FwOpcodeType opCode, const U32 cmdSeq, U32 pattern_key) {
    if (pattern_key == EMERGENCY_KEY) {
        this->log_ACTIVITY_HI_SA_AuthSuccess(true); // Bypass
        updateState(STATE_AUTHENTICATED);
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    } else {
        this->log_WARNING_HI_SA_AuthFailed(EMERGENCY_KEY, pattern_key);
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
    }
  }

  void StellarAuth::AUTH_RESET_cmdHandler(const FwOpcodeType opCode, const U32 cmdSeq) {
    updateState(STATE_LOCKED);
    updatePersistence(0);
    m_isStable = true; 
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  void StellarAuth::LOAD_TRANSIT_SCHEDULE_cmdHandler(const FwOpcodeType opCode, const U32 cmdSeq, U32 starID, U32 seconds, U32 microseconds) {
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
  }
}
