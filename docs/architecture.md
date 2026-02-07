# StellarAuth Architecture: The Spatio-Temporal Gate

## 1. The "One of One" Logic (FSM)
StellarAuth moves beyond boolean flags, utilizing a strict Finite State Machine to govern command authority:
* **STATE_LOCKED (0):** Default state. All signals ignored.
* **STATE_ARMED (1):** Transition occurs *only* when `MissionCycle` is inside the `target_window` AND `Heading` matches `target_yaw`.
* **STATE_VERIFYING (2):** Transition occurs when a shadow ingress (slope < -40) is detected while ARMED.
* **STATE_AUTHENTICATED (3):** Reached only after persistence checks pass. Emits `AuthSuccess` and immediately resets to LOCKED.

## 2. Programmable Ephemeris Interface
Ground Control manages the "Key" via the `UPDATE_EPHEMERIS` command, which uploads:
* `window_start` / `window_end`: The precise orbital timeframe for the event.
* `target_yaw`: The expected star vector.
This allows the component to distinguish between a valid "Messenger Satellite" occultation and random space debris.

## 3. Triple Modular Redundancy (TMR)
To ensure integrity in LEO/GEO radiation environments:
* **Storage:** Three independent registers (`m_state_A`, `m_state_B`, `m_state_C`) hold the FSM state.
* **Voting:** Every cycle, `doVote()` enforces the majority value.
* **Scrubbing:** If a bit-flip is detected, `scrubMemory()` repairs it instantly and logs an SEU alert.
