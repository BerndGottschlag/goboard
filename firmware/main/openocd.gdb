set history save on
set confirm off
target extended-remote :3333
monitor arm semihosting enable
monitor reset halt

# print demangled symbols
set print asm-demangle on

# set backtrace limit to not have infinite backtrace loops
set backtrace limit 32

# detect unhandled exceptions, hard faults and panics
break z_arm_fault

#break main

load
continue
# monitor verify
# monitor reset
# quit

