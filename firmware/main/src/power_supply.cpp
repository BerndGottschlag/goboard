#include "power_supply.hpp"

#ifdef CONFIG_BOARD_GOBOARD_NRF52840
// On the real hardware, charge the batteries for 10 seconds at a time.
#define CHARGING_DURATION K_SECONDS(10)
// On the real hardware, wait for 1000ms to let the battery voltage recover
// before measurements.
#define RECOVERY_DURATION K_SECONDS(3)
#else
// For tests, drastically reduce the times above to reduce test durations.
#define CHARGING_DURATION K_MSEC(10)
#define RECOVERY_DURATION K_MSEC(1)
#endif

#define CHARGE_END_VOLTAGE 1380
#define DISCHARGED_VOLTAGE 1100

template<class PowerSupplyPinType>
PowerSupply<PowerSupplyPinType>::PowerSupply(PowerSupplyPinType *pins):
		pins(pins) {
	k_work_init_delayable(&charging_ended, static_on_charging_ended);
	k_work_init_delayable(&recovery_ended, static_on_recovery_ended);

	// Determine the initial power supply state immediately and start the
	// cycle.
	on_recovery_ended();
}

template<class PowerSupplyPinType>
PowerSupply<PowerSupplyPinType>::~PowerSupply() {
	// Cancel the workqueue entries and wait until they have stopped.
	atomic_set(&stop, 1);
	bool stopped;
	do {
		k_work_sync sync;
		k_work_cancel_delayable_sync(&charging_ended, &sync);
		k_work_cancel_delayable_sync(&recovery_ended, &sync);
		k_sched_lock();
		stopped = !k_work_delayable_is_pending(&charging_ended) &&
				!k_work_delayable_is_pending(&recovery_ended);
		k_sched_unlock();
	} while (!stopped);

	pins->configure_charging(false);
	pins->configure_discharging(false, false);
}

template<class PowerSupplyPinType>
PowerSupplyMode PowerSupply<PowerSupplyPinType>::get_mode() {
	return (PowerSupplyMode)atomic_get(&mode);
}

template<class PowerSupplyPinType>
uint8_t PowerSupply<PowerSupplyPinType>::get_battery_charge() {
	return atomic_get(&charge);
}

template<class PowerSupplyPinType>
struct CallbackChange {
	PowerSupply<PowerSupplyPinType> *thisptr;
	k_work work;
	void (*new_callback)();
	k_sem done;
};

template<class PowerSupplyPinType>
void PowerSupply<PowerSupplyPinType>::set_callback(void (*change_callback)()) {
	// The power supply code is executed on the system workqueue. To prevent
	// race conditions while setting the callback and to wait for any
	// invocation of the old callback, we execute the callback change on the
	// system workqueue.
	CallbackChange<PowerSupplyPinType> change = {
		.thisptr = this,
		.new_callback = change_callback,
	};
	k_sem_init(&change.done, 0, 1);
	k_work_init(&change.work, static_set_callback_work);
	k_work_submit(&change.work);

	// Wait until the workqueue entry has been executed.
	// TODO: Do we need the semaphore, or does k_work_flush block while the
	// work is still in the queue?
	k_sem_take(&change.done, K_FOREVER);
	struct k_work_sync sync;
	k_work_flush(&change.work, &sync);
}

template<class PowerSupplyPinType>
void PowerSupply<PowerSupplyPinType>::static_set_callback_work(struct k_work *work) {
	CallbackChange<PowerSupplyPinType> *change =
			CONTAINER_OF(work,
			             CallbackChange<PowerSupplyPinType>,
			             work);
	change->thisptr->change_callback = change->new_callback;
	k_sem_give(&change->done);
}

template<class PowerSupplyPinType>
void PowerSupply<PowerSupplyPinType>::static_on_charging_ended(struct k_work *work) {
	PowerSupply<PowerSupplyPinType> *thisptr =
			CONTAINER_OF(k_work_delayable_from_work(work),
			             PowerSupply<PowerSupplyPinType>,
			             charging_ended);
	thisptr->on_charging_ended();
}

template<class PowerSupplyPinType>
void PowerSupply<PowerSupplyPinType>::on_charging_ended() {
	if (atomic_get(&stop)) {
		return;
	}

	// Disable charging/balancing.
	pins->configure_charging(false);
	pins->configure_discharging(false, false);

	// Start the recovery period.
	k_work_schedule(&recovery_ended, RECOVERY_DURATION);
}

template<class PowerSupplyPinType>
void PowerSupply<PowerSupplyPinType>::static_on_recovery_ended(struct k_work *work) {
	PowerSupply<PowerSupplyPinType> *thisptr =
			CONTAINER_OF(k_work_delayable_from_work(work),
			             PowerSupply<PowerSupplyPinType>,
			             recovery_ended);
	thisptr->on_recovery_ended();
}

template<class PowerSupplyPinType>
void PowerSupply<PowerSupplyPinType>::on_recovery_ended() {
	if (atomic_get(&stop)) {
		return;
	}

	bool usb_connected = has_usb_connection();

	// Measure the voltage.
	uint32_t low_voltage, high_voltage;
	pins->measure_battery_voltage(&low_voltage, &high_voltage);
#ifdef CONFIG_BOARD_GOBOARD_NRF52840
	printk("battery voltage: %dmV, %dmV\n", low_voltage, high_voltage);
	printk("usb: %d\n", usb_connected);
#endif
	// TODO: Derive the state of charge from the lower voltage.

	// Start charging or balancing.
	PowerSupplyMode new_mode = POWER_SUPPLY_NORMAL;
	if (usb_connected) {
		if (low_voltage < CHARGE_END_VOLTAGE &&
				high_voltage < CHARGE_END_VOLTAGE) {
#ifdef CONFIG_BOARD_GOBOARD_NRF52840
			printk("charging.\n");
#endif
			pins->configure_charging(true);
			new_mode = POWER_SUPPLY_CHARGING;
		}
		// Balancing counts as "charging", even if one battery is
		// already full (the other will be charged when possible).
		if (((int)high_voltage - (int)low_voltage) > 20) {
#ifdef CONFIG_BOARD_GOBOARD_NRF52840
			printk("discharging 1.\n");
#endif
			pins->configure_discharging(false, true);
			new_mode = POWER_SUPPLY_CHARGING;
		}
		if (((int)low_voltage - (int)high_voltage) > 20) {
#ifdef CONFIG_BOARD_GOBOARD_NRF52840
			printk("discharging 2.\n");
#endif
			pins->configure_discharging(true, false);
			new_mode = POWER_SUPPLY_CHARGING;
		}
	} else {
		if (low_voltage < DISCHARGED_VOLTAGE ||
				high_voltage < DISCHARGED_VOLTAGE) {
			new_mode = POWER_SUPPLY_LOW;
		}
	}
	int old_mode = __atomic_exchange_n(&mode, new_mode, __ATOMIC_SEQ_CST);
	bool usb_changed = usb_was_connected != usb_connected;
	usb_was_connected = usb_connected;
	if (old_mode != new_mode || usb_changed) {
		// TODO: Also check the state of charge.
		if (change_callback) {
			change_callback();
		}
	}

	// Start the charging period.
	k_work_schedule(&charging_ended, CHARGING_DURATION);
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
		bool expect_callback;
	};

	static atomic_t callback_called = ATOMIC_INIT(0);

	static void power_supply_callback() {
		atomic_set(&callback_called, 1);
	}

	static void single_charging_test(ChargingTest test,
	                                 MockPowerSupplyPins *pins,
	                                 PowerSupply<MockPowerSupplyPins> *ps) {
		atomic_set(&callback_called, 0);
		pins->set_input(test.low_voltage,
		                test.high_voltage,
		                test.usb_connected);
		// Wait for a complete power management cycle and a bit longer.
		k_sleep(CHARGING_DURATION);
		k_sleep(RECOVERY_DURATION);
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
		k_sleep(CHARGING_DURATION);
		k_sleep(RECOVERY_DURATION);
		bool charging, discharging_low, discharging_high;
		pins->get_output(&charging, &discharging_low, &discharging_high);
		zassert_true(charging == test.should_charge,
		             "wrong charging state (expected %d, got %d for %d/%d/%d)",
		             test.should_charge, charging,
		             test.low_voltage, test.high_voltage, test.usb_connected);
		zassert_true(discharging_low == test.should_discharge_low,
		             "wrong discharging state (expected %d, got %d for %d/%d/%d)",
		             test.should_discharge_low, discharging_low,
		             test.low_voltage, test.high_voltage, test.usb_connected);
		zassert_true(discharging_high == test.should_discharge_high,
		             "wrong discharging state (expected %d, got %d for %d/%d/%d)",
		             test.should_discharge_high, discharging_high,
		             test.low_voltage, test.high_voltage, test.usb_connected);
		zassert_true(ps->get_mode() == test.expected_mode,
		             "wrong power supply mode (expected %d, got %d for %d/%d/%d)",
		             test.expected_mode, ps->get_mode(),
		             test.low_voltage, test.high_voltage, test.usb_connected);
		int callback = atomic_get(&callback_called);
		zassert_true(callback == (int)test.expect_callback,
		             "callback was called: %d, should be %d for %d/%d/%d",
		             callback, test.expect_callback,
		             test.low_voltage, test.high_voltage, test.usb_connected);
	}

	static void charging_test(void) {
		ChargingTest tests[] = {
			// Both batteries low.
			{ 1050, 1050, false, false, false, false, POWER_SUPPLY_LOW, true },
			// One battery low.
			{ 1050, 1400, false, false, false, false, POWER_SUPPLY_LOW, false },
			{ 1400, 1050, false, false, false, false, POWER_SUPPLY_LOW, false },
			// Both batteries full.
			{ 1400, 1400, false, false, false, false, POWER_SUPPLY_NORMAL, true },
			// Both batteries full, USB connected.
			{ 1400, 1400, true, false, false, false, POWER_SUPPLY_NORMAL, true },
			// One battery full, USB connected (counts as "charging"
			// as we are balancing the batteries).
			{ 1100, 1400, true, false, false, true, POWER_SUPPLY_CHARGING, true },
			{ 1400, 1100, true, false, true, false, POWER_SUPPLY_CHARGING, false },
			// USB connected, charging without balancing.
			{ 1100, 1100, true, true, false, false, POWER_SUPPLY_CHARGING, false },
			// USB connected, charging with balancing.
			{ 1100, 1200, true, true, false, true, POWER_SUPPLY_CHARGING, false },
			{ 1200, 1100, true, true, true, false, POWER_SUPPLY_CHARGING, false },
			// USB disconnected again - we do not want to discharge
			// if USB is not connected.
			{ 1200, 1100, false, false, false, false, POWER_SUPPLY_NORMAL, true },
		};

		MockPowerSupplyPins pins;
		pins.set_input(1200, 1200, false);
		PowerSupply<MockPowerSupplyPins> ps(&pins);
		ps.set_callback(power_supply_callback);

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

