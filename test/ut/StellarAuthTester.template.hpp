// ======================================================================
// \title  StellarAuthTester.hpp
// \author albertcornelius
// \brief  hpp file for StellarAuth component test harness implementation class
// ======================================================================

#ifndef SolarAuth_StellarAuthTester_HPP
#define SolarAuth_StellarAuthTester_HPP

#include "StellarAuth/StellarAuth.hpp"
#include "StellarAuth/StellarAuthGTestBase.hpp"

namespace SolarAuth {

class StellarAuthTester final : public StellarAuthGTestBase {
  public:
    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    // Maximum size of histories storing events, telemetry, and port outputs
    static const FwSizeType MAX_HISTORY_SIZE = 10;

    // Instance ID supplied to the component instance under test
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    // Queue depth supplied to the component instance under test
    static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 10;

  public:
    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

    //! Construct object StellarAuthTester
    StellarAuthTester();

    //! Destroy object StellarAuthTester
    ~StellarAuthTester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! To do
    void toDo();

  private:
    // ----------------------------------------------------------------------
    // Helper functions
    // ----------------------------------------------------------------------

    //! Connect ports
    void connectPorts();

    //! Initialize components
    void initComponents();

  private:
    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    //! The component under test
    StellarAuth component;
};

}  // namespace SolarAuth

#endif
