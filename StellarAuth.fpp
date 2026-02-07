module SolarAuth {
    @ Finalized Mission-Ready Stellar Authentication Component.
    active component StellarAuth {

        # --- Commands ---
        @ Standard commands
        async command AUTH_START(pattern_key: U32) opcode 0
        async command AUTH_RESET opcode 1
        
        @ Legacy command
        async command LOAD_TRANSIT_SCHEDULE(starID: U32, seconds: U32, microseconds: U32) opcode 2

        @ [NEW] Updates the Spatio-Temporal Lock parameters from Ground
        async command UPDATE_EPHEMERIS(
            window_start: U32 @< Start cycle of valid window
            window_end: U32   @< End cycle of valid window
            target_yaw: F32   @< Expected star alignment (0-360)
        ) opcode 3

        # --- Telemetry ---
        telemetry SA_AuthState: U32 id 1
        telemetry Heartbeat: U32 id 2
        telemetry TargetStarID: U32 id 3
        telemetry SA_PersistenceCounter: U32 id 6
        telemetry SA_ScrubCount: U32 id 4
        telemetry SA_IsStable: U32 id 5
        telemetry SA_ExpectedStarID: U32 id 7
        telemetry SA_HealthStatus: F32 id 10
        telemetry SA_LightLevel: F32 id 11
        telemetry AuthFSM: U32 id 12

        # --- Events ---
        # Ensure these format strings do not break onto a new line!
        event SA_AuthSuccess() severity activity high id 0 format "Stellar Authentication Successful"
        event SA_AuthFailed(expected: U32, actual: U32) severity warning high id 1 format "Auth Failed: Expected {}, got {}"
        event SA_SEU_Scrubbed(slot: U32) severity warning low id 2 format "Radiation Bit-Flip Repaired in Slot {}"
        event SA_StabilityAlert() severity warning high id 3 format "Auth Rejected: Satellite Tumbling"
        event SA_PersistenceReset() severity warning low id 4 format "Auth Sequence Reset: Signal Noise Detected"
        event SA_TMR_Failure() severity fatal id 6 format "TMR Critical Failure: Memory Integrity Lost"
        event SA_IngressDetected(slope: F32) severity activity high id 7 format "Shadow Ingress Detected: Slope {}"
        
        @ [NEW] Logged when a new occultation target is uploaded
        event EphemerisUpdated(
            start: U32
            end: U32
            yaw: F32
        ) severity activity high id 8 format "Mission Plan Updated: Window [{} - {}], Target Yaw {}"

        # --- Ports ---
        sync input port schedIn: Svc.Sched
        time get port timeCaller
        output port watchdogOut: Svc.WatchDog

        # --- Standard F´ Framework Ports ---
        command recv port cmdIn
        command reg port cmdRegOut
        command resp port cmdResponseOut
        event port eventOut
        text event port textEventOut
        telemetry port tlmOut
    }
}
