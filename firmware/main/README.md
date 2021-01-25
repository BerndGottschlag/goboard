# Compiling

Ensure that the NRF5 SDK (version 17) is placed at `../3rdparty/nrf5-sdk`, then
execute the following:

    mkdir build
    cmake .. -DCMAKE_TOOLCHAIN_FILE=../Toolchain.cmake
    make -j`nproc`

