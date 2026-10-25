# Arduino Uno - 10-LED Light Effects Experiment

This project is a 10-LED light animation application developed on the Arduino Uno (ATmega328P) to practice fundamental embedded systems principles and serial communication control.

It builds upon a basic LED control experiment by implementing non-blocking timing without `delay()`, software brightness control (Soft-PWM) on pins lacking hardware PWM support, and software-based current limiting for hardware safety.

## Hardware & Circuit Setup

* **Microcontroller:** Arduino Uno
* **Components:** 10x LEDs, 10x 330 Ω resistors (connected in series to the anode/+ legs of the LEDs)
* **Pin Layout:** Connected sequentially starting from the 1st LED on **Pin 13** down to the 10th LED on **Pin 4** (`13, 12, 11, 10, 9, 8, 7, 6, 5, 4`).

## Key Technical Highlights

* **Non-Blocking Architecture (Zero-Delay):** All animation transitions are driven by a `millis()` and `micros()` based Finite State Machine (FSM). Commands sent via the serial port are processed instantaneously without waiting for the active animation step to finish.
* **Software PWM & Port Manipulation:** All outputs—including pins without hardware PWM support (Pins 13, 12, 8, 7, 4)—are driven via phase-shifted (interleaved) software PWM using direct `PORTB` and `PORTD` register manipulation.
* **Current Budget Protection:** In modes where all LEDs are illuminated simultaneously, total current draw is dynamically scaled in software (~45 mA safe budget) to prevent USB voltage droop and Brown-Out Resets.

## Mode List (Serial Monitor Control)

You can switch between modes in real time by sending the following commands through the Serial Monitor (**9600 baud**):

| Command | Mode Name | Description |
| :---: | :--- | :--- |
| `1` | **Police / Ambulance Strobe** | Triple-burst strobe on left and right LED banks followed by alternating cross-flashes |
| `2` | **Audi / VW Dynamic Turn Signal** | Sequential sweeping indicator filling from the 1st LED to the 10th LED |
| `3` | **Comet Trail & Collision** | Bi-directional scanner leaving a fading trail, followed by a center collision burst |
| `4` | **Knight Rider Scanner** | Classic bi-directional scanner with soft-edged transitions |
| `5` | **Symmetric Bounce (Ping-Pong)** | Synchronized pulses moving simultaneously from both ends toward the center |
| `6` | **Sinusoidal Breathing Wave** | Smooth brightness fade-in and fade-out across the entire LED array |
| `7` | **Random Star Sparkle** | Instantaneous flashes on randomly selected LEDs across the strip |
| `8` | **Binary Counter** | 10-bit binary counting visualization running sequentially from 0 to 1023 |
| `9` | **Dual-Sided Compression** | Synchronized inward compression and outward expansion |
| `10` | **Fireplace Flame Effect** | Flickering flame simulation using randomized duty-cycle variations |
| `0` | **Safe Standby** | Turns off all physical outputs and reduces current consumption to zero |
| `m` | **Menu** | Reprints the control menu to the serial terminal |

## Setup & Usage

1. Open the `.ino` project file in the **Arduino IDE** and upload it to your **Arduino Uno** board.
2. Open the **Serial Monitor** in the IDE.
3. Set the baud rate to **9600 baud**.
4. Type a mode number between `0` and `10` into the input box and press **Enter**.
