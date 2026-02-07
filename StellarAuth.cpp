#include <StellarAuth/StellarAuth.hpp>

namespace SolarAuth {

  StellarAuth::StellarAuth(const char *const compName) : 
    StellarAuthComponentBase(compName), 
    m_heading(0), 
    m_prevHeading(0), 
    m_heartbeatCounter(0),
    m_currentTargetStar(0), 
    m_scrubCount(0), 
    m_persistenceCounter(0), 
    m_isStable(true),
    m_currentLightLevel(100.0f), 
    m_prevLightLevel(100.0f),
    m_expectedStarID(0), 
    m_expectedTime(Fw::Time(0,0)),
    m_state_A(STATE_LOCKED), 
    m_state_B(STATE_LOCKED), 
    m_state_C(STATE_LOCKED)
  {}

  StellarAuth::~StellarAuth() {}

  U32 StellarAuth::doVote() {
    if (m_state_A == m_state_B) return m_state_A;
    if (m_state_A == m_state_C) return m_state_A;
    if (m_state_B == m_state_C) return m_state_B;
    return 0;
  }

  void StellarAuth::scrubMemory() {
    U32 validState = doVote();
    F32 health = 100.0f;
    if (validState == 0) {
        this->log_FATAL_SA_TMR_Failure();
        updateState(STATE_LOCKED);
        health = 0.0f;
    } else {
        if (m_state_A != validState) { m_state_A = validState; m_scrubCount++; log_WARNING_LO_SA_SEU_Scrubbed(1); health -= 33.3f; }
        if (m_state_B != validState) { m_state_B = validState; m_scrubCount++; log_WARNING_LO_SA_SEU_Scrubbed(2); health -= 33.3f; }
        if (m_state_C != validState) { m_state_C = validState; m_scrubCount++; log_WARNING_LO_SA_SEU_Scrubbed(3); health -= 33.3f; }
    }
    this->tlmWrite_SA_HealthStatus(health);
  }

  bool StellarAuth::detectShadowEdge() {
    F32 slope = m_currentLightLevel - m_prevLightLevel;
    bool detected = (slope <= INGRESS_THRESHOLD);
    if (detected) {
        this->log_ACTIVITY_HI_SA_IngressDetected(slope);
    }
    m_prevLightLevel = m_currentLightLevel; // Update AFTER check
    return detected;
  }

  void StellarAuth::updateState(U32 newState) {
    m_state_A = newState; m_state_B = newState; m_state_C = newState;
  }

  void StellarAuth::schedIn_handler(const FwIndexType portNum, U32 context) {
    this->m_heartbeatCounter++;
    scrubMemory();

    // STABILITY CHECK
    U32 headingDelta = (m_heading > m_prevHeading) ? (m_heading - m_prevHeading) : (m_prevHeading - m_heading);
    if (headingDelta > STABILITY_THRESHOLD) {
        if (m_isStable) this->log_WARNING_HI_SA_StabilityAlert();
        m_isStable = false;
        m_persistenceCounter = 0; 
    } else {
        m_isStable = true;
    }
    m_prevHeading = m_heading;

    Fw::Time currentTime = this->getTime();
    U32 foundStar = identifyStar(this->m_heading);
    bool shadowDetected = detectShadowEdge(); 
    
    bool timeValid = false;
    if (m_expectedStarID != 0) {
        I32 diff = static_cast<I32>(currentTime.getSeconds()) - static_cast<I32>(m_expectedTime.getSeconds());
        if (diff >= -TIME_WINDOW_SEC && diff <= TIME_WINDOW_SEC) timeValid = true;
    }

    // AUTH ENGINE
    if (foundStar != 0 && foundStar == m_expectedStarID && timeValid && shadowDetected && m_isStable) {
        if (m_persistenceCounter < AUTH_THRESHOLD) m_persistenceCounter++;
        m_currentTargetStar = foundStar;
        if (m_persistenceCounter >= AUTH_THRESHOLD) {
            updateState(STATE_UNLOCKED);
            this->log_ACTIVITY_HI_SA_AuthSuccess();
        }
    } else {
        // Reset if we have a target but conditions fail
        if (m_expectedStarID != 0) m_persistenceCounter = 0;
    }
    
    this->tlmWrite_Heartbeat(this->m_heartbeatCounter);
    this->tlmWrite_SA_PersistenceCounter(this->m_persistenceCounter);
    this->tlmWrite_SA_AuthState(doVote());
    this->tlmWrite_SA_IsStable(m_isStable ? 1 : 0);
  }

  U32 StellarAuth::identifyStar(U32 heading) {
    if (heading >= 355 || heading <= 5) return 101; 
    if (heading >= 85 && heading <= 95) return 202; 
    if (heading >= 175 && heading <= 185) return 303;
    return 0;
  }

  void StellarAuth::LOAD_TRANSIT_SCHEDULE_cmdHandler(const FwOpcodeType opCode, const U32 cmdSeq, U32 starID, U32 seconds, U32 microseconds) {
    this->m_expectedStarID = starID;
    this->m_expectedTime.set(seconds, microseconds); 
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  void StellarAuth::AUTH_START_cmdHandler(const FwOpcodeType opCode, const U32 cmdSeq, U32 pattern_key) {
    if (doVote() == STATE_UNLOCKED && pattern_key == this->m_currentTargetStar) {
        this->log_ACTIVITY_HI_SA_AuthSuccess();
    } else {
        this->log_WARNING_HI_SA_AuthFailed(this->m_currentTargetStar, pattern_key);
    }
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  void StellarAuth::AUTH_RESET_cmdHandler(const FwOpcodeType opCode, const U32 cmdSeq) {
    updateState(STATE_LOCKED);
    m_persistenceCounter = 0;
    m_expectedStarID = 0;
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }
}
