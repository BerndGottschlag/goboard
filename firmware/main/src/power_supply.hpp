#ifndef POWER_SUPPLY_HPP_INCLUDED
#define POWER_SUPPLY_HPP_INCLUDED

#include <kernel.h>
#include <sys/atomic.h>

#include <stdint.h>
#include <stddef.h>

/// Current mode of the power supply system.
enum PowerSupplyMode {
	/// The batteries are not low, and we are not charging the batteries.
	POWER_SUPPLY_NORMAL,
	/// We are currently charging the batteries.
	POWER_SUPPLY_CHARGING,
	/// We are not charging the batteries, and the voltage is low. In this
	/// state, the system should transition into system-off mode.
	POWER_SUPPLY_LOW
};

/// Power supply and battery management.
///
/// The power supply code performs a number of tasks:
///
/// - It periodically measures the battery voltages. The lower voltage is
///   used to calculate the remaining.
/// - If USB is connected, the code charges the batteries inbetween voltage
///   measurements if both batteries are below a safe maximum voltage. After
///   each charging period, the code waits for some time without charging to let
///   the battery voltage normalize before the following voltage measurement.
/// - If USB is connected and the battery voltages are substantially different,
///   the code actively discharges the battery with the higher voltage to
///   balance the charge.
template<class PowerSupplyPinType> class PowerSupply {
public:
	PowerSupply(PowerSupplyPinType *pins);
	/// Destructor. Similar to `set_callback()`, the destructor waits for
	/// any current callback invocation and must therefore not be called
	/// from the system workqueue.
	~PowerSupply();

	/// Returns the current mode of the power supply.
	PowerSupplyMode get_mode();

	/// Returns the estimated battery charge in percent.
	uint8_t get_battery_charge();

	bool has_usb_connection(void) {
		// TODO: Mutex, the GPIOs area also accessed from another thread.
		return pins->has_usb_connection();
	}

	// Sets a callback which is called whenever the state of the power
	// supply (mode or charge percentage) changes.
	//
	// The callback is called from the system workqueue. If the system
	// workqueue is currently executing the old callback, the call blocks
	// until the callback has returned. The function therefore must never be
	// called from the system workqueue as that situation would result in a
	// deadlock.
	void set_callback(void (*change_callback)());
private:
	static void static_set_callback_work(struct k_work *item);

	static void static_on_charging_ended(struct k_work *item);
	void on_charging_ended();
	static void static_on_recovery_ended(struct k_work *item);
	void on_recovery_ended();


	PowerSupplyPinType *pins;
	/// Callback which is called whenever mode, state of charge, or USB
	/// connection state change.
	///
	/// The variable is only accessed from the system workqueue, therefore
	/// no mutex is required to access it.
	void (*change_callback)() = NULL;

	atomic_t stop = ATOMIC_INIT(0);
	/// Workqueue entry which is executed after the charging period.
	/// At this point, charging shall be stopped and the recovery period
	/// should be initiated by submitting `recovery_ended`.
	struct k_work_delayable charging_ended;
	/// Workqueue entry which is executed after the voltage recovery period
	/// has ended. At this point, the voltages shall be measured and
	/// depending on the voltages charging or balancing (or both) shall be
	/// enabled, and `charging_ended` should be submitted as a timer for the
	/// charging period.
	struct k_work_delayable recovery_ended;

	// The power supply logic is executed in a separate thread, so the
	// variables to transfer information to the rest of the program need to
	// be accessed with atomic instructions. Normal loads/stores should be
	// completely atomic, but if we explicitly use atomic_t, we ensure that
	// the compiler does not do anything stupid.
	atomic_t mode = ATOMIC_INIT(POWER_SUPPLY_NORMAL);
	atomic_t charge = ATOMIC_INIT(100);

	// We need to memorize the USB connection status so that we can invoke
	// the callback when it changes.
	bool usb_was_connected = false;
};

#endif

