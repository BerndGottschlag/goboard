# Compiling

Install the nRF Connect SDK as described in the manual:

https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/gs_installing.html

At the time of writing, you need to install the `master` branch of the SDK. Once
the SDK is installed, the following can be used to compile the firmware:

```
source <ZEPHYR_PATH>/env.sh
./compile.sh
```

Executing `test.sh` instead compiles a debug version of the firmware and
executes unit tests. Executing `openocd` in a different terminal and then
executing `program.sh` flashes the firmware onto the target.

