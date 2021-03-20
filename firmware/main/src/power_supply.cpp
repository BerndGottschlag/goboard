#include "power_supply.hpp"

#ifdef CONFIG_BOARD_GOBOARD_NRF52840
// On the real hardware, charge the batteries for 10 seconds at a time.
#define CHARGING_DURATION K_SECONDS(10)
// On the real hardware, wait for 1000ms to let the battery voltage recover
// before measurements.
#define RECOVERY_DURATION K_SECONDS(1)
#else
// For tests, drastically reduce the times above to reduce test durations.
#define CHARGING_DURATION K_MSEC(10)
#define RECOVERY_DURATION K_MSEC(1)
#endif

template<class PowerSupplyPinType>
PowerSupply<PowerSupplyPinType>::PowerSupply(PowerSupplyPinType *pins):
		pins(pins) {
	// TODO
}

template<class PowerSupplyPinType>
PowerSupply<PowerSupplyPinType>::~PowerSupply() {
	// TODO
}

template<class PowerSupplyPinType>
PowerSupplyMode PowerSupply<PowerSupplyPinType>::get_mode() {
	// TODO
}

template<class PowerSupplyPinType>
uint8_t PowerSupply<PowerSupplyPinType>::get_battery_charge() {
	// TODO
}

template<class PowerSupplyPinType>
void PowerSupply<PowerSupplyPinType>::set_callback(void (*change_callback)()) {
	// TODO
}

#ifdef CONFIG_BOARD_GOBOARD_NRF52840
#include "power_supply_pins.hpp"
template class PowerSupply<PowerSupplyPins>;
#endif

#ifndef CONFIG_BOARD_GOBOARD_NRF52840
#include "tests.hpp"
#include <ztest.h>
namespace tests {
	/// Artificial power supply pins for tests.
	class MockPowerSupplyPins {
	public:
		void set_input(uint32_t low, uint32_t high, bool usb) {
			low_voltage = low;
			high_voltage = high;
			usb_connected = usb;
		}

		void reset_output() {
			charged = false;
			discharged_low = false;
			discharged_high = false;
		}

		// Returns whether any of the outputs has been active since the
		// last call to `reset_output()`.
		void get_output(bool *charging,
		                bool *discharging_low,
		                bool *discharging_high) {
			*charging = charged;
			*discharging_low = discharged_low;
			*discharging_high = discharged_high;
		}

		void measure_battery_voltage(uint32_t *low, uint32_t *high) {
			// Make sure that the voltage is never measured while we
			// are either charging or discharging the
			// batteries.
			zassert_false(currently_charging,
			              "measuring voltage while charging");
			zassert_false(currently_discharging_low,
			              "measuring voltage while discharging");
			zassert_false(currently_discharging_high,
			              "measuring voltage while discharging");

			*low = low_voltage;
			*high = high_voltage;
		}

		void configure_charging(bool active) {
			currently_charging = active;
			charged |= active;
		}

		void configure_discharging(bool low, bool high) {
			currently_discharging_low = low;
			discharged_low |= low;
			currently_discharging_high = high;
			discharged_high |= high;
		}

		bool has_usb_connection(void) {
			return usb_connected;
		}
	private:
		uint32_t low_voltage = 0;
		uint32_t high_voltage = 0;
		bool usb_connected = false;

		bool currently_charging = false;
		bool currently_discharging_low = false;
		bool currently_discharging_high = false;

		bool charged = false;
		bool discharged_low = false;
		bool discharged_high = false;
	};

	struct ChargingTest {
		uint32_t low_voltage;
		uint32_t high_voltage;
		bool usb_connected;

		bool should_charge;
		bool should_discharge_low;
		bool should_discharge_high;
		PowerSupplyMode expected_mode;
	};

	static void single_charging_test(ChargingTest test,
	                                 MockPowerSupplyPins *pins,
	                                 PowerSupply<MockPowerSupplyPins> *ps) {
		pins->set_input(test.low_voltage,
		                test.high_voltage,
		                test.usb_connected);
		// Wait for a complete power management cycle and a bit longer.
		k_sleep(CHARGING_DURATION);
		k_sleep(RECOVERY_DURATION);
		k_sleep(CHARGING_DURATION);
		k_sleep(RECOVERY_DURATION);
		// Reset the outputs, we only want to test whether outputs are
		// currently used.
		pins->reset_output();
		k_sleep(CHARGING_DURATION);
		k_sleep(RECOVERY_DURATION);
		k_sleep(CHARGING_DURATION);
		k_sleep(RECOVERY_DURATION);
		bool charging, discharging_low, discharging_high;
		pins->get_output(&charging, &discharging_low, &discharging_high);
		zassert_true(charging == test.should_charge,
		             "wrong charging state");
		zassert_true(discharging_low == test.should_discharge_low,
		             "wrong discharging state");
		zassert_true(discharging_high == test.should_discharge_high,
		             "wrong discharging state");
		zassert_true(ps->get_mode() == test.expected_mode,
		             "wrong power supply mode");
	}

	static void charging_test(void) {
		ChargingTest tests[] = {
			// Both batteries low.
			{ 1050, 1050, false, false, false, false, POWER_SUPPLY_LOW },
			// One battery low.
			{ 1050, 1350, false, false, false, false, POWER_SUPPLY_LOW },
			{ 1350, 1050, false, false, false, false, POWER_SUPPLY_LOW },
			// Both batteries full.
			{ 1350, 1350, false, false, false, false, POWER_SUPPLY_NORMAL },
			// Both batteries full, USB connected.
			{ 1350, 1350, true, false, false, false, POWER_SUPPLY_NORMAL },
			// One battery full, USB connected (counts as "charging"
			// as we are balancing the batteries).
			{ 1100, 1350, true, false, false, true, POWER_SUPPLY_CHARGING },
			{ 1350, 1100, true, false, true, false, POWER_SUPPLY_CHARGING },
			// USB connected, charging without balancing.
			{ 1100, 1100, true, true, false, false, POWER_SUPPLY_CHARGING },
			// USB connected, charging with balancing.
			{ 1100, 1200, true, true, false, true, POWER_SUPPLY_CHARGING },
			{ 1200, 1100, true, true, true, false, POWER_SUPPLY_CHARGING },
			// USB disconnected again - we do not want to discharge
			// if USB is not connected.
			{ 1200, 1100, true, false, false, false, POWER_SUPPLY_NORMAL },
		};

		MockPowerSupplyPins pins;
		PowerSupply<MockPowerSupplyPins> ps(&pins);

		for (size_t i = 0; i < ARRAY_SIZE(tests); i++) {
			single_charging_test(tests[i], &pins, &ps);
		}
	}

	static void soc_test(void) {
		// TODO: Test the state-of-charge report.
	}

	void power_supply_tests() {
		ztest_test_suite(power_supply,
			ztest_unit_test(charging_test),
			ztest_unit_test(soc_test)
		);
		ztest_run_test_suite(power_supply);
	}
	RegisterTests power_supply_tests_(power_supply_tests);
}
#endif

