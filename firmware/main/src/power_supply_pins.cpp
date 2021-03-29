#include "power_supply_pins.hpp"
#include "exception.hpp"

#include <drivers/adc.h>

#ifndef CONFIG_ADC_NRFX_SAADC
#error SAADC needs to be enabled!
#endif

#define POWERSUPPLY DT_PATH(powersupply)

#if !DT_NODE_HAS_STATUS(POWERSUPPLY, okay)
#error Device tree has no power supply!
#endif

#define ADC DEVICE_DT_GET(DT_IO_CHANNELS_CTLR(POWERSUPPLY)) // Only one ADC.
#define VMID DT_IO_CHANNELS_INPUT_BY_IDX(POWERSUPPLY, 0)
#define VBATT DT_IO_CHANNELS_INPUT_BY_IDX(POWERSUPPLY, 1)

#define VBATT_POWER_LABEL DT_GPIO_LABEL(POWERSUPPLY, vbatt_power_gpios)
#define VBATT_POWER_PIN DT_GPIO_PIN(POWERSUPPLY, vbatt_power_gpios)
#define VBATT_POWER_FLAGS DT_GPIO_FLAGS(POWERSUPPLY, vbatt_power_gpios)

#define VBATT_OUTPUT_OHM DT_PROP(POWERSUPPLY, vbatt_output_ohms)
#define VBATT_FULL_OHM DT_PROP(POWERSUPPLY, vbatt_full_ohms)

#define CHARGE_LABEL DT_GPIO_LABEL(POWERSUPPLY, charge_gpios)
#define CHARGE_PIN DT_GPIO_PIN(POWERSUPPLY, charge_gpios)
#define CHARGE_FLAGS DT_GPIO_FLAGS(POWERSUPPLY, charge_gpios)

#define DISCHARGE_LOW_LABEL DT_GPIO_LABEL(POWERSUPPLY, discharge_low_gpios)
#define DISCHARGE_LOW_PIN DT_GPIO_PIN(POWERSUPPLY, discharge_low_gpios)
#define DISCHARGE_LOW_FLAGS DT_GPIO_FLAGS(POWERSUPPLY, discharge_low_gpios)

#define DISCHARGE_HIGH_LABEL DT_GPIO_LABEL(POWERSUPPLY, discharge_high_gpios)
#define DISCHARGE_HIGH_PIN DT_GPIO_PIN(POWERSUPPLY, discharge_high_gpios)
#define DISCHARGE_HIGH_FLAGS DT_GPIO_FLAGS(POWERSUPPLY, discharge_high_gpios)

#define USB_CONNECTED_LABEL DT_GPIO_LABEL(POWERSUPPLY, usb_connected_gpios)
#define USB_CONNECTED_PIN DT_GPIO_PIN(POWERSUPPLY, usb_connected_gpios)
#define USB_CONNECTED_FLAGS DT_GPIO_FLAGS(POWERSUPPLY, usb_connected_gpios)

#define ADC_CHANNEL_VMID 0
#define ADC_CHANNEL_VBATT 1

PowerSupplyPins::PowerSupplyPins() {
	if (!device_is_ready(ADC)) {
		throw InitializationFailed("ADC is not ready");
	}

	// Configure the GPIOs.
	// TODO: Should the GPIOs be encapsulated in a class to ensure proper
	// deinitialization in case of exceptions?
	vbatt_power_gpio = init_output_gpio(VBATT_POWER_LABEL,
	                                    VBATT_POWER_PIN,
	                                    VBATT_POWER_FLAGS);
	gpio_pin_set(vbatt_power_gpio, VBATT_POWER_PIN, false);
	charge_gpio = init_output_gpio(CHARGE_LABEL,
	                               CHARGE_PIN,
	                               CHARGE_FLAGS);
	gpio_pin_set(charge_gpio, CHARGE_PIN, false);
	discharge_low_gpio = init_output_gpio(DISCHARGE_LOW_LABEL,
	                                      DISCHARGE_LOW_PIN,
	                                      DISCHARGE_LOW_FLAGS);
	gpio_pin_set(discharge_low_gpio, DISCHARGE_LOW_PIN, false);
	discharge_high_gpio = init_output_gpio(DISCHARGE_HIGH_LABEL,
	                                       DISCHARGE_HIGH_PIN,
	                                       DISCHARGE_HIGH_FLAGS);
	gpio_pin_set(discharge_high_gpio, DISCHARGE_HIGH_PIN, false);

	usb_connected_gpio = device_get_binding(USB_CONNECTED_LABEL);
	if (usb_connected_gpio == NULL) {
		throw InitializationFailed("USB GPIO not found");
	}
	if (gpio_pin_configure(usb_connected_gpio,
	                       USB_CONNECTED_PIN,
	                       GPIO_INPUT | USB_CONNECTED_FLAGS) != 0) {
		throw InitializationFailed("USB gpio_pin_configure failed");
	}

	// Configure the ADC channels.
	struct adc_channel_cfg vmid_adc_cfg = {
		.gain = ADC_GAIN_1_6,
		.reference = ADC_REF_INTERNAL,
		.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
		.channel_id = ADC_CHANNEL_VMID,
		.input_positive = SAADC_CH_PSELP_PSELP_AnalogInput0 + VMID,
	};
	if (adc_channel_setup(ADC, &vmid_adc_cfg) != 0) {
		throw InitializationFailed("failed to setup ADC channel 0");
	}
	struct adc_channel_cfg vbatt_adc_cfg = {
		.gain = ADC_GAIN_1_6,
		.reference = ADC_REF_INTERNAL,
		.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
		.channel_id = ADC_CHANNEL_VBATT,
		.input_positive = SAADC_CH_PSELP_PSELP_AnalogInput0 + VBATT,
	};
	if (adc_channel_setup(ADC, &vbatt_adc_cfg) != 0) {
		throw InitializationFailed("failed to setup ADC channel 1");
	}

	// Read the voltage once to calibrate the ADC.
	for (unsigned int channel = ADC_CHANNEL_VMID; channel <= ADC_CHANNEL_VBATT; channel++) {
		int16_t raw_value;
		struct adc_sequence adc_seq = {
			.channels = BIT(channel),
			.buffer = &raw_value,
			.buffer_size = sizeof(raw_value),
			.resolution = 14,
			.oversampling = 4,
			.calibrate = true,
		};
		if (adc_read(ADC, &adc_seq) != 0) {
			throw HardwareError("failed to calibrate ADC");
		}
		printk("battery %d: %d\n", channel, raw_value);
	}
}

PowerSupplyPins::~PowerSupplyPins() {
	// Disable charging/discharging.
	gpio_pin_set(vbatt_power_gpio, VBATT_POWER_PIN, false);
	gpio_pin_set(charge_gpio, CHARGE_PIN, false);
	gpio_pin_set(discharge_low_gpio, DISCHARGE_LOW_PIN, false);
	gpio_pin_set(discharge_high_gpio, DISCHARGE_HIGH_PIN, false);

	// Enable a wakeup interrupt when USB is connected.
	// TODO
}

void PowerSupplyPins::measure_battery_voltage(uint32_t *low, uint32_t *high) {
	gpio_pin_set(vbatt_power_gpio, VBATT_POWER_PIN, true);

	// Sample VMID first to allow the voltage to stabilize.
	// TODO: One call to adc_read for both channels should be sufficient.
	int32_t voltages[2];
	for (unsigned int channel = ADC_CHANNEL_VMID; channel <= ADC_CHANNEL_VBATT; channel++) {
		int16_t raw_value;
		struct adc_sequence adc_seq = {
			.channels = BIT(channel),
			.buffer = &raw_value,
			.buffer_size = sizeof(raw_value),
			.resolution = 14,
			.oversampling = 4,
			.calibrate = false,
		};
		if (adc_read(ADC, &adc_seq) != 0) {
			throw HardwareError("failed to read ADC");
		}
		voltages[channel] = raw_value;
		adc_raw_to_millivolts(adc_ref_internal(ADC),
				      ADC_GAIN_1_6,
				      adc_seq.resolution,
				      &voltages[channel]);
	}
	voltages[ADC_CHANNEL_VBATT] = (uint64_t)voltages[ADC_CHANNEL_VBATT] *
	                              VBATT_FULL_OHM /
	                              VBATT_OUTPUT_OHM;
	gpio_pin_set(vbatt_power_gpio, VBATT_POWER_PIN, false);

	*low = voltages[ADC_CHANNEL_VMID];
	if (voltages[ADC_CHANNEL_VBATT] < voltages[ADC_CHANNEL_VMID]) {
		*high = 0;
	} else {
		*high = voltages[ADC_CHANNEL_VBATT] - voltages[ADC_CHANNEL_VMID];
	}
}

void PowerSupplyPins::configure_charging(bool active) {
	gpio_pin_set(charge_gpio, CHARGE_PIN, active);
}

void PowerSupplyPins::configure_discharging(bool low, bool high) {
	gpio_pin_set(discharge_low_gpio, DISCHARGE_LOW_PIN, low);
	gpio_pin_set(discharge_high_gpio, DISCHARGE_HIGH_PIN, high);
}

bool PowerSupplyPins::has_usb_connection(void) {
	int status = gpio_pin_get(usb_connected_gpio, USB_CONNECTED_PIN);
	if (status < 0) {
		throw HardwareError("failed to get USB connection GPIO status");
	}
	return status;
}

const struct device *PowerSupplyPins::init_output_gpio(const char *label,
                                                       gpio_pin_t pin,
                                                       gpio_flags_t flags) {
	const struct device *gpio = device_get_binding(label);
	if (gpio == NULL) {
		throw InitializationFailed("GPIO not found");
	}
	if (gpio_pin_configure(gpio, pin,
	                       GPIO_OUTPUT_INACTIVE | flags) != 0) {
		throw InitializationFailed("gpio_pin_configure failed");
	}

	return gpio;
}

