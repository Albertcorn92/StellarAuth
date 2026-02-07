StellarAuth Triple Modular Redundancy (TMR) Architecture
Design Philosophy: To ensure command integrity in high-radiation 
environments (LEO/GEO) where Single Event Upsets (SEUs) can flip memory 
bits.

1. Memory Redundancy Layer:

Triple State Vectors: The component maintains three independent internal 
registers: m_state_A, m_state_B, and m_state_C.

Synchronization: Every state transition is mirrored across all three 
registers simultaneously.

2. The Voting Engine (doVote):

On every execution cycle, the logic compares all three registers.

If a discrepancy is found (e.g., A=1, B=1, C=0), the system identifies the 
majority (1) and designates it as the "Truth".

3. Self-Healing Layer (scrubMemory):

Once a corrupted register is identified, the system immediately overwrites 
the faulty bit with the majority value.

The component emits a SA_HealthStatus telemetry channel to notify Ground 
Control of the repair.
