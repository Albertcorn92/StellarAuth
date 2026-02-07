# StellarAuth Security & Threat Analysis

## 1. Temporal Layer: Replay Attacks
* **Threat:** An attacker records a valid "Shadow Signal" from a previous mission and replays it to the sensor bus to unlock the system later.
* **Mitigation:** **Replay Resistance Logic.** The component tracks `m_last_auth_window_start`. Once a window is used, it is "burned." Any subsequent 
signal in that same window is mathematically rejected.

## 2. Spatial Layer: Signal Spoofing
* **Threat:** A hacker shines a laser or casts a shadow at the sensor at the wrong time (e.g., creating a fake occultation).
* **Mitigation:** **Ephemeris Gating.** The system enforces a strict Time-Window and Heading check. Signals received outside the uploaded 
`UPDATE_EPHEMERIS` parameters are ignored (treated as debris).

## 3. Environmental Layer: Radiation/SEU
* **Threat:** High-energy particles flip a bit in memory, changing `is_locked = true` to `false`.
* **Mitigation:** **TMR Voting.** The state is stored in triplicate. A single bit-flip is mathematically overruled by the other two registers and 
self-healed in the next clock cycle.
