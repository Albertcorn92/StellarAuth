module SolarAuth {

    # --- Port Definitions ---
    # Custom port to receive Attitude data (Yaw) from GNC
    port NavAttitude(yaw: F32)

    @ Finalized Mission-Ready Stellar Authentication Component.
    active component StellarAuth {

        # --- Parameters ---
        # These generate commands automatically starting at Opcode 0
        
        @ Start of the valid authentication window (UTC Seconds)
        param TargetWindowStart: U32 default 0 id 0

        @ End of the valid authentication window (UTC Seconds)
        param TargetWindowEnd: U32 default 0 id 1

        @ Expected Star Alignment (0.0 - 360.0 degrees)
        param TargetYaw: F32 default 0.0 id 2

        # --- Commands ---
        # STARTING AT OPCODE 10 TO AVOID COLLISION WITH PARAMETERS
        
        @ Standard commands
        async command AUTH_START(pattern_key: U32) opcode 10
        async command AUTH_RESET opcode 11
        
        @ Deprecated Command - Returns Validation Error
        async command LOAD_TRANSIT_SCHEDULE(starID: U32, seconds: U32, 
microseconds: U32) opcode 12

        # --- Telemetry ---
        # Optimized for Bandwidth (U8 instead of U32) and Human Factors
        
        telemetry SA_AuthState: U32 id 1
        telemetry Heartbeat: U32 id 2
        telemetry SA_PersistenceCounter: U32 id 3
        telemetry SA_ScrubCount: U32 id 4
        
        @ Boolean Status (0=Unstable, 1=Stable)
        telemetry SA_IsStable: U8 id 5
        
        @ System Health with Limits and Formatting
        telemetry SA_HealthStatus: F32 id 6 \
            format "{.1f} %%" \
            low { red 50.0, orange 90.0 }
        
        telemetry SA_LightLevel: F32 id 7 format "{.2f} Lux"
        telemetry AuthFSM: U32 id 8
        telemetry CurrentYaw: F32 id 9 format "{.1f} deg"

        # --- Events ---
        event SA_AuthSuccess(bypass: bool) \
            severity activity high id 0 \
            format "Stellar Authentication Successful (Bypass: {})"

        event SA_AuthFailed(expected: U32, actual: U32) \
            severity warning high id 1 \
            format "Auth Failed: Expected {}, got {}"

        event SA_SEU_Scrubbed(slot: U32) \
            severity warning low id 2 \
            format "Radiation Bit-Flip Repaired in Slot {}" \
            throttle 5

        event SA_StabilityAlert() \
            severity warning high id 3 \
            format "Auth Rejected: Satellite Tumbling" \
            throttle 1

        event SA_PersistenceReset() \
            severity warning low id 4 \
            format "Auth Sequence Reset: Signal Noise Detected" \
            throttle 5
    
        event SA_TMR_Failure() \
            severity fatal id 5 \
            format "TMR Critical Failure: Memory Integrity Lost"

        event SA_IngressDetected(slope: F32) \
            severity activity high id 6 \
            format "Shadow Ingress Detected: Slope {}" \
            throttle 5

        event SA_InitComplete(version: string) \
            severity activity low id 7 \
            format "StellarAuth Initialized. Version: {}"

        # --- Ports ---
        sync input port schedIn: Svc.Sched
        sync input port navAttitudeIn: NavAttitude
        
        time get port timeCaller
        output port watchdogOut: Svc.WatchDog

        # --- Parameter Ports ---
        param get port prmGetOut
        param set port prmSetOut

        # --- Standard F´ Framework Ports ---
        command recv port cmdIn
        command reg port cmdRegOut
        command resp port cmdResponseOut
        event port eventOut
        text event port textEventOut
        telemetry port tlmOut
    }
}
