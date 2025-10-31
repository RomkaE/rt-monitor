# rt-monitor

**rt-monitor** is a **run-time / real-time task statistics monitor** for **FreeRTOS**.  
It extends the built-in FreeRTOS runtime statistics with real-time analysis,  
overflow-safe counters, and formatted console output via UART or USB CDC — all without additional tools.

---

## Key Features

- **Real-time statistics mode**  
  A modified FreeRTOS version clears per-task runtime counters after each read.  
  This prevents counter overflow and provides up-to-date values.  
  It allows monitoring short-term peaks instead of long-term averages.

- **Uses built-in FreeRTOS runtime statistics**  
  Based on the kernel API for task runtime measurement, fully compatible with standard FreeRTOS behavior.

- **Own formatted output functions**  
  Provides a simple text output layer that can be directed to **UART** or **USB CDC**.

- **Console-oriented output**  
  Output is formatted for display in a serial console or terminal window.

- **Correct handling of timer overflow**  
  Supports proper calculation of task runtime when the statistics timer wraps once between measurements.

---

## Examples

Integration examples are provided in a separate repository:  
➡️ [rt-monitor-examples](https://github.com/RomkaE/rt-monitor-examples)

Examples are located in a separate repository to keep the main `rt-monitor` project simple and independent.  
This approach also reflects how the module is intended to be used — as an external component integrated into other projects.

Separating examples from the main code avoids the common situation where demo projects become part of the library itself,  
making it harder to reuse or include in other systems.  
Here, examples are focused only on showing how to integrate `rt-monitor` into different projects.
