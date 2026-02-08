# StellarAuth v1.0-GOLD: Spatio-Temporal Physical Interlock

![Verification Status](https://github.com/Albertcorn92/StellarAuth/actions/workflows/verify.yml/badge.svg)

## Overview
StellarAuth is a mission-critical F´ (F Prime) flight software component designed for **Zero-Knowledge Physical Authentication**. Unlike standard systems that rely on static keys, StellarAuth requires a precise alignment of **Time** (Orbital Schedule), **Space** (Star Vector), and **Physics** (Shadow Ingress) to authorize commands.

## Key Features
* **Spatio-Temporal Lock:** Implements a "One of One" physical gate. Authentication is only possible when the satellite is pointing at a specific star *and* inside a ground-uploaded time window (`UPDATE_EPHEMERIS`).
* **Finite State Machine (FSM):** Deterministic logic flow (`LOCKED` $\to$ `ARMED` $\to$ `VERIFYING` $\to$ `AUTHENTICATED`) preventing accidental unlocks.
* **Replay Resistance:** "Burns" the authentication window ID after use, preventing hackers from recording and replaying a valid shadow signal.
* **Triple Modular Redundancy (TMR):** Critical state variables use majority-vote logic to self-heal from radiation-induced bit flips (SEUs).
* **Context-Aware Fault Detection:** Smart filtering prevents "Stuck Sensor" false positives during long-duration deep space cruise phases.

## Verified Performance
Achieved 100% pass rates on the formal GTest suite (6/6 Scenarios):
1. **Mission Success:** Nominal Spatio-Temporal Lock.
2. **Timing Violation:** Rejection of shadows outside the Ephemeris window (Debris filtering).
3. **Spatial Error:** Rejection of shadows from the wrong star vector (Yaw Math verified).
4. **Replay Attack:** Successful blocking of reused signal data.
5. **Stuck Sensor:** Detection of hardware failure without false alarms.
6. **Radiation Resilience:** Confirmed TMR self-healing during memory corruption.

## 🛰️ Hardware-in-the-Loop (HiL) Verification
The StellarAuth v4.1 Flight Software was validated on **ESP32-S3 silicon** to prove fault tolerance in a physical environment.

* **Target:** ESP32-S3 Dev Module (Xtensa LX7)
* **Test Suite:** v4.1-PLATINUM (Stress Testing)
* **Results:** [View Full HiL Logs](docs/verification_ESP32_HiL.txt)

| ID | Test Case | Outcome | Engineering Note |
| :--- | :--- | :--- | :--- |
| **TC-01** | Nominal Mission Profile | ✅ **PASS** | Full FSM transition from `LOCKED` to `Key Burn` verified. |
| **TC-02** | Replay Attack Injection | ✅ **PASS** | System correctly rejected reuse of Window ID `1000`. |
| **TC-03** | Single-Event Upset (SEU) | ✅ **PASS** | TMR Scrubbing repaired Register B in <1ms. |
| **TC-04** | Yaw Boundary Precision | ✅ **PASS** | Rejected arming at 5.1° deviation (Limit: 5.0°). |
| **TC-05** | Sensor Noise Rejection | ✅ **PASS** | 5/5 false positives aborted due to signal bounce. |
| **TC-06** | Double-Bit Upset | ✅ **PASS** | **Safe Mode Triggered.** Multi-bit failure correctly identified. |

## About the Developer
**Albert Cornelius** is an Engineering student at Embry-Riddle Aeronautical University and an aspiring Flight Software Engineer. His research focuses on **Resilient Space Systems**, **Cyber-Physical Security**, and **Fault-Tolerant Avionics**.

StellarAuth represents a synthesis of aerospace engineering principles and rigorous software verification, designed to meet the high-assurance standards required for NASA Class B safety-critical software.

---
*Built with NASA's F´ (F Prime) Framework | Verified with GoogleTest | Hardened for LEO/GEO Environments*
