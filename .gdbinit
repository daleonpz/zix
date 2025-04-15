# Connect to OpenOCD
target remote localhost:3333
monitor reset halt

# Define a custom dashboard-like command
define dashboard
    printf "\n--- Source ---\n"
    list
    printf "\n--- Registers ---\n"
    info registers
    printf "\n--- Stack ---\n"
    backtrace
    printf "\n--- Breakpoints ---\n"
    info breakpoints
end

# Run dashboard on every stop
define hook-stop
    dashboard
end

# Set breakpoint at main
break main
