# Smart Streetlight System in C (LPC2148)

This project implements a **Smart Streetlight System** using the **LPC2148 microcontroller** in Embedded C.  
It automatically controls streetlights based on **time** and **ambient light intensity**, with an **RTC** for real-time scheduling and a **keypad interface** for manual RTC editing.

---

##  Features
- Real-time clock (RTC) display on LCD  
- Time and date editing using keypad (4x4 matrix)  
- LDR-based automatic streetlight control  
- LEDs simulate streetlights  
- Interrupt-driven RTC edit function  
- 8-bit LCD interface for display  

---

## Hardware Used
- LPC2148 Microcontroller  
- 16x2 LCD  
- LDR sensor (connected to ADC channel)  
- 8 LEDs (connected to Port 1 pins)  
- 4x4 Matrix Keypad  
- Push button (External Interrupt 1 for RTC edit)

---

##  How to Run (Keil uVision / Proteus)
1. Open the source code in **Keil uVision**.  
2. Compile and build the project.  
3. Flash the hex file to LPC2148 (or run in Proteus simulation).  
4. Use the keypad to edit time/date.  
5. Observe automatic LED control based on LDR input and time.

---

Developed by **Sivakumar Naik Bukke**
