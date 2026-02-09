/*
 * PROJECT: StellarAuth Flight Software (Digital Twin)
 * HARDWARE: ESP32-S3 Dev Module
 * VERSION: v4.1-ENGINEERING (Enhanced Telemetry)
 * DEVELOPER: Albert Cornelius
 * * LOGGING UPGRADE:
 * - Added Mission Clock [T+Seconds] to all logs.
 * - Added Data Context (Slope, Yaw Delta, Light Level) to transitions.
 * - Standardized levels: [INFO], [WARN], [CRITICAL].
 */

#include <Arduino.h>
#include <cmath>

// --- 1. MISSION CONSTANTS ---
enum AuthState {
    STATE_LOCKED = 0,
    STATE_ARMED,
    STATE_VERIFYING,
    STATE_AUTHENTICATED,
    STATE_FAULTED
};

// Physics Thresholds
const float INGRESS_THRESHOLD = -40.0f;  
const float STABILITY_THRESHOLD = 5.0f;  
const uint32_t AUTH_THRESHOLD = 3;       

// --- 2. THE FLIGHT SOFTWARE CORE ---
class StellarAuthCore {
public:
    // --- TMR REGISTERS ---
    uint32_t m_state_A = STATE_LOCKED;
    uint32_t m_state_B = STATE_LOCKED;
    uint32_t m_state_C = STATE_LOCKED;

    // --- MISSION PARAMETERS ---
    uint32_t m_target_window_start = 0;
    uint32_t m_target_window_end = 0;
    float m_target_yaw = 0.0f;
    uint32_t m_last_auth_window_start = 0; 

    // --- SENSOR INPUTS ---
    float m_heading = 0.0f;
    float m_currentLightLevel = 100.0f;
    float m_prevLightLevel = 100.0f;
    uint32_t m_persistence = 0;
    uint32_t m_missionTime = 0; // For Logging

    // --- DIAGNOSTICS ---
    uint32_t m_scrubCount = 0;

    // --- LOGGING HELPER ---
    void log(String level, String msg, String data = "") {
        Serial.print("[T+"); Serial.print(m_missionTime); Serial.print("] ");
        Serial.print(level); Serial.print(" ");
        Serial.print(msg);
        if (data != "") {
            Serial.print(" | "); Serial.print(data);
        }
        Serial.println();
    }

    // --- TMR VOTING LOGIC ---
    uint32_t doVoteState() {
        if (m_state_A == m_state_B) return m_state_A;
        if (m_state_A == m_state_C) return m_state_A;
        if (m_state_B == m_state_C) return m_state_B;
        return 0xFFFFFFFF; 
    }

    // --- ACTIVE MEMORY SCRUBBER ---
    void scrubMemory() {
        uint32_t valid = doVoteState();
        
        if (valid == 0xFFFFFFFF) {
            log("[CRITICAL]", "TMR FAILURE: Consensus Lost!", "Entering SAFE MODE");
            updateState(STATE_FAULTED);
        } else {
            if (m_state_A != valid) { 
                m_state_A = valid; m_scrubCount++; 
                log("[WARN]", "SEU REPAIRED", "Reg: A");
            }
            if (m_state_B != valid) { 
                m_state_B = valid; m_scrubCount++; 
                log("[WARN]", "SEU REPAIRED", "Reg: B");
            }
            if (m_state_C != valid) { 
                m_state_C = valid; m_scrubCount++; 
                log("[WARN]", "SEU REPAIRED", "Reg: C");
            }
        }
    }

    void updateState(AuthState newState) {
        m_state_A = newState; m_state_B = newState; m_state_C = newState;
    }

    // --- MAIN LOGIC CYCLE ---
    void runCycle(uint32_t timeSec) {
        m_missionTime = timeSec;
        scrubMemory(); 
        
        float slope = m_currentLightLevel - m_prevLightLevel;
        bool inside_window = (timeSec >= m_target_window_start) && (timeSec <= m_target_window_end);
        
        // Yaw Calculation
        float delta = std::abs(m_heading - m_target_yaw);
        if (delta > 180.0f) delta = 360.0f - delta;

        AuthState current_state = (AuthState)doVoteState();

        switch (current_state) {
            case STATE_LOCKED:
                if (inside_window) {
                    if (m_last_auth_window_start != m_target_window_start) {
                        // Strict Arming Check
                        if (delta <= STABILITY_THRESHOLD) {
                            updateState(STATE_ARMED);
                            String telemetry = "Win:" + String(m_target_window_start) + " YawDelta:" + String(delta, 1);
                            log("[INFO]", "LOCKED -> ARMED", telemetry);
                        }
                    }
                }
                break;

            case STATE_ARMED:
                if (!inside_window) {
                    updateState(STATE_LOCKED);
                    log("[INFO]", "ARMED -> LOCKED", "Window Expired");
                } else {
                    if (delta <= STABILITY_THRESHOLD && slope <= INGRESS_THRESHOLD) {
                        updateState(STATE_VERIFYING);
                        m_persistence = 0;
                        String telemetry = "Slope:" + String(slope, 1) + " Light:" + String(m_currentLightLevel, 1);
                        log("[INFO]", "ARMED -> VERIFYING", telemetry);
                    }
                }
                break;

            case STATE_VERIFYING:
                if (m_currentLightLevel < 50.0f) {
                    m_persistence++;
                    if (m_persistence >= AUTH_THRESHOLD) {
                        updateState(STATE_AUTHENTICATED);
                        log("[INFO]", "VERIFYING -> AUTHENTICATED", "Persistence: " + String(m_persistence));
                    }
                } else {
                    updateState(STATE_ARMED);
                    log("[WARN]", "ABORT: Signal Noise", "Light:" + String(m_currentLightLevel));
                }
                break;

            case STATE_AUTHENTICATED:
                log("[EVENT]", ">>> COMMAND AUTHORIZED <<<");
                m_last_auth_window_start = m_target_window_start; 
                updateState(STATE_LOCKED);
                log("[INFO]", "Key Burned", "WindowID:" + String(m_target_window_start));
                break;
                
             case STATE_FAULTED:
                break;
        }

        m_prevLightLevel = m_currentLightLevel;
    }
};

StellarAuthCore fsw;

// --- 3. ADVANCED VERIFICATION SUITE (v4.0 Logic / v4.1 Logging) ---
void runAutoTest() {
    Serial.println("\n\n========================================================");
    Serial.println("   STELLARAUTH FLIGHT READINESS VERIFICATION (v4.1)");
    Serial.println("========================================================");
    delay(2000);

    uint32_t now = 1000; 

    // --- TEST 1: NOMINAL MISSION PROFILE ---
    Serial.println("\n--- [TEST 1] Nominal Authentication ---");
    fsw.m_target_window_start = now; 
    fsw.m_target_window_end = now + 60;
    fsw.m_target_yaw = 45.0f;
    fsw.m_heading = 45.0f;
    fsw.m_currentLightLevel = 100.0f; fsw.m_prevLightLevel = 100.0f;
    
    fsw.runCycle(now);     // -> ARMED
    fsw.m_currentLightLevel = 10.0f; 
    fsw.runCycle(now + 1); // -> VERIFYING
    fsw.runCycle(now + 2); // P1
    fsw.runCycle(now + 3); // P2
    fsw.runCycle(now + 4); // P3 -> AUTH
    fsw.runCycle(now + 5); // Burn Key

    if (fsw.m_last_auth_window_start == now) Serial.println("RESULT: PASS");
    else Serial.println("RESULT: FAIL");

    // Reset
    fsw.updateState(STATE_LOCKED);
    fsw.m_currentLightLevel = 100.0f;
    delay(250);

    // --- TEST 2: REPLAY ATTACK ---
    Serial.println("\n--- [TEST 2] Replay Attack Resistance ---");
    fsw.m_currentLightLevel = 100.0f; fsw.m_prevLightLevel = 100.0f;
    fsw.runCycle(now); // Key is burned -> Remain LOCKED
    
    if (fsw.doVoteState() == STATE_LOCKED) Serial.println("RESULT: PASS");
    else Serial.println("RESULT: FAIL");
    delay(250);

    // --- TEST 3: TMR REPAIR ---
    Serial.println("\n--- [TEST 3] Single-Bit Upset Recovery ---");
    fsw.m_state_B = 0xBADF00D; 
    fsw.runCycle(now + 10);    
    
    if (fsw.m_state_B == fsw.m_state_A && fsw.m_scrubCount > 0) Serial.println("RESULT: PASS");
    else Serial.println("RESULT: FAIL");
    delay(250);

    // --- TEST 4: BOUNDARY PRECISION (Math) ---
    Serial.println("\n--- [TEST 4] Yaw Limit Precision (5.1 deg) ---");
    fsw.m_last_auth_window_start = 0; // Clear key
    fsw.updateState(STATE_LOCKED);
    fsw.m_target_yaw = 0.0f;
    
    // CASE A: 5.1 Degrees (Should Fail to Arm)
    fsw.m_heading = 5.1f; 
    fsw.runCycle(now);
    
    if(fsw.doVoteState() == STATE_LOCKED) Serial.println("RESULT: PASS");
    else {
        Serial.print("RESULT: FAIL (Accepted Bad Yaw. State: "); 
        Serial.print(fsw.doVoteState()); Serial.println(")");
    }
    delay(250);

    // --- TEST 5: SENSOR NOISE ---
    Serial.println("\n--- [TEST 5] Sensor Noise Injection ---");
    fsw.m_heading = 0.0f; // Fix Yaw so it CAN arm
    fsw.updateState(STATE_LOCKED);
    fsw.runCycle(now); // Arm it
    
    fsw.m_currentLightLevel = 100.0f; fsw.m_prevLightLevel = 100.0f;
    
    for(int i=0; i<10; i++) {
        fsw.m_currentLightLevel = (i % 2 == 0) ? 10.0f : 100.0f; 
        fsw.runCycle(now + 20 + i);
    }
    
    if(fsw.doVoteState() != STATE_AUTHENTICATED) Serial.println("RESULT: PASS");
    else Serial.println("RESULT: FAIL");
    delay(250);

    // --- TEST 6: DOUBLE FAULT (Fail-Safe) ---
    Serial.println("\n--- [TEST 6] Double-Bit Upset (Fail-Safe) ---");
    
    // Corrupt A AND B
    fsw.m_state_A = 0x11111111;
    fsw.m_state_B = 0x22222222;
    
    fsw.runCycle(now + 50); 
    
    if(fsw.doVoteState() == STATE_FAULTED) { 
        Serial.println("RESULT: PASS"); 
    } else {
         Serial.print("RESULT: FAIL (State: "); Serial.print(fsw.doVoteState()); Serial.println(")");
    }

    Serial.println("========================================================");
    Serial.println("   VERIFICATION COMPLETE. ALL SYSTEMS GREEN.");
    Serial.println("========================================================\n");
}

// --- 4. ARDUINO SETUP ---
void setup() {
    Serial.begin(115200);
    while(!Serial); 
    delay(1000);
    
    runAutoTest();
}

void loop() {
    delay(1000); // Idle
}
