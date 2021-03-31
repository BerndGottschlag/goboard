#include "key_matrix.hpp"

#include "exception.hpp"

#define KEY_MATRIX DT_PATH(keymatrix)

#define SPI DT_PHANDLE(KEY_MATRIX, spi)
#define SPI_LABEL DT_PROP(SPI, label)

#define POWER_LABEL DT_GPIO_LABEL(KEY_MATRIX, power_gpios)
#define POWER_PIN DT_GPIO_PIN(KEY_MATRIX, power_gpios)
#define POWER_FLAGS DT_GPIO_FLAGS(KEY_MATRIX, power_gpios)

#define LOAD_LABEL DT_GPIO_LABEL(KEY_MATRIX, load_gpios)
#define LOAD_PIN DT_GPIO_PIN(KEY_MATRIX, load_gpios)
#define LOAD_FLAGS DT_GPIO_FLAGS(KEY_MATRIX, load_gpios)

#define STORE_LABEL DT_GPIO_LABEL(KEY_MATRIX, store_gpios)
#define STORE_PIN DT_GPIO_PIN(KEY_MATRIX, store_gpios)
#define STORE_FLAGS DT_GPIO_FLAGS(KEY_MATRIX, store_gpios)

KeyMatrix::KeyMatrix() {
	// Initialize the GPIOs.
	power_gpio = init_output_gpio(POWER_LABEL,
	                              POWER_PIN,
	                              POWER_FLAGS);
	gpio_pin_set(power_gpio, POWER_PIN, false);
	load_gpio = init_output_gpio(LOAD_LABEL,
	                             LOAD_PIN,
	                             LOAD_FLAGS);
	gpio_pin_set(load_gpio, LOAD_PIN, false);
	store_gpio = init_output_gpio(STORE_LABEL,
	                             STORE_PIN,
	                             STORE_FLAGS);
	gpio_pin_set(store_gpio, STORE_PIN, false);

	// Initialize SPI.
	spi_dev = device_get_binding(SPI_LABEL);
	printk("SPI: %s\n", SPI_LABEL);
	if (spi_dev == NULL) {
		throw InitializationFailed("SPI not found");
	}
}

KeyMatrix::~KeyMatrix() {
	gpio_pin_set(load_gpio, LOAD_PIN, false);
	gpio_pin_set(store_gpio, STORE_PIN, false);
}


void KeyMatrix::enable() {
	gpio_pin_set(power_gpio, POWER_PIN, true);
	// We need to wait for some time to make sure that all the capacitance
	// is charged.
	// TODO: Correct time constant?
	k_sleep(K_MSEC(2));
}

void KeyMatrix::disable() {
	gpio_pin_set(power_gpio, POWER_PIN, false);
}

void KeyMatrix::select_row() {
	gpio_pin_set(store_gpio, STORE_PIN, true);
	k_sleep(K_USEC(1));
	gpio_pin_set(store_gpio, STORE_PIN, false);
	k_sleep(K_USEC(1));
}

void KeyMatrix::load_input() {
	gpio_pin_set(load_gpio, LOAD_PIN, true);
	k_sleep(K_USEC(1));
	gpio_pin_set(load_gpio, LOAD_PIN, false);
	k_sleep(K_USEC(1));
}

uint16_t KeyMatrix::transfer(uint8_t out) {
	uint16_t in = 0;
	struct spi_buf buf = {
		.buf = &in,
		.len = sizeof(in),
	};
	struct spi_buf_set buf_set = {
		.buffers = &buf,
		.count = 1,
	};
	if (spi_read(spi_dev, &READ_SPI_CFG, &buf_set) != 0) {
		throw HardwareError("spi_read failed");
	}
	// The row selection lines are active-low.
	out = ~out << 1;
	buf.buf = &out;
	buf.len = sizeof(out);
	if (spi_write(spi_dev, &WRITE_SPI_CFG, &buf_set) != 0) {
		throw HardwareError("spi_write failed");
	}
	// The keys are active-low.
	in = ~in;
	return (in >> 8) | (in << 8);
}

// TODO: This code should be deduplicated, a copy can be found in
// power_supply_pins.cpp.
const struct device *KeyMatrix::init_output_gpio(const char *label,
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

const struct spi_config KeyMatrix::READ_SPI_CFG = {
	.frequency = 2000000,
	.operation = SPI_OP_MODE_MASTER |
	             SPI_MODE_CPOL |
	             SPI_TRANSFER_MSB |
	             SPI_WORD_SET(8),
	.slave = 0,
	.cs = NULL,
};

const struct spi_config KeyMatrix::WRITE_SPI_CFG = {
	.frequency = 2000000,
	.operation = SPI_OP_MODE_MASTER |
	             SPI_MODE_CPOL |
	             SPI_MODE_CPHA |
	             SPI_TRANSFER_MSB |
	             SPI_WORD_SET(8),
	.slave = 0,
	.cs = NULL,
};

