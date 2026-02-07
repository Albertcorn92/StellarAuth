# StellarAuth v1.0-GOLD: Spatio-Temporal Physical Interlock

![Verification Status](https://github.com/Albertcorn92/StellarAuth/actions/workflows/verify.yml/badge.svg)

## Overview
StellarAuth is a mission-critical F´ (F Prime) flight software component designed for **Zero-Knowledge Physical Authentication**. Unlike standard systems 
that rely on static keys, StellarAuth requires a precise alignment of **Time** (Orbital Schedule), **Space** (Star Vector), and **Physics** (Shadow 
Ingress) to authorize commands.

## Key Features
* **Spatio-Temporal Lock:** Implements a "One of One" physical gate. Authentication is only possible when the satellite is pointing at a specific star 
*and* inside a ground-uploaded time window (`UPDATE_EPHEMERIS`).
* **Finite State Machine (FSM):** deterministic logic flow (`LOCKED` $\to$ `ARMED` $\to$ `VERIFYING` $\to$ `AUTHENTICATED`) preventing accidental unlocks.
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

## About the Author
Developed by **Albert Cornelius**, Engineering student at Embry-Riddle Aeronautical University.
