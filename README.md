# Modifications-TS-520


I Started reparing and adjusting my Kenwood transceiver TS-520, but I saw some articles about how to create a device to read the frequency of operation. I decide to include it. I continued including new features as: s-meter, band indicator, spectrum frequency, mode of operation and some others indicators.
As appeared in the picture I use two microcontrollers: one for display and interface, and the second one for the FHT.

This device is inspired in the work of Stephen Leander KV6O, but I changed everything.

On the final PCBs you can see that there are terminals coded from JP1 to JP8. Once the PCB components are assembled, these terminals must be shorted each two with a drop of solder. The grounds of the different circuits are isolated from each other and are only connected to the general ground at these points (star ground).
The VCCs are configured in the same way.

You can use all the documentation that appears in the repository as long as it is not used for commercial purposes.

![WhatsApp Image 2023-10-10 at 12 25 15-3](https://github.com/joanperelopez/Modifications-TS-520/assets/73885181/7f15d40e-f390-4a47-bfcb-a38b76726800)



The experiments for the design of the electronic circuits and the microcontroller began with a breadboard and the RF circuits mounted directly on a copper plate.

![Top_Assbly](https://github.com/joanperelopez/Modifications-TS-520/assets/73885181/8c674a67-83ea-4e70-82e7-e750e3be8978)

This is the final appearance of the circuit in the TOP side.


![Bottom_Assbly](https://github.com/joanperelopez/Modifications-TS-520/assets/73885181/a4aa84f0-b00c-4b37-8cf3-96827e404165)


This is the final appearance of the circuit in the BOTTOM side.

The way to calculate the frequency at which the TS-520 emits and receives is a mathematical operation between three different frequencies: CAR which is the frequency of the beat oscillator. VFO which, as its name indicates, is the variable frequency oscillator signal and HET which is a fixed signal from a quartz oscillator and which is different in each band.
Each of these frequencies are amplified and applied to an ATmega328P that makes the readings, the mathematical operations and finally displays the frequency on a graphic screen. 

The band is automatically displayed using the HET information.

The operating mode (USB, LSB and CW) is identified by the frequency of the CAR input and is displayed on the corresponding indicator.

The information from the red LEDs that the TS-520 has is input into the ATmega328P and shown on the display.

The AGC (Automatic Gain Control) signal is conditioned and input into the ATmega328P to display the signal intensity (s-meter).

The RF signal before the SSB and CW filters enters a RF receiver of 3,395MHz that incorporates a Tayloe detector. Once detected, the baseband signal in I/Q format enters the second ATmega328P where it is processed and the FHT is done. This frequency information is transmitted to the first ATmega328P that will display it on the screen. The FHT is like FFT but it uses only the real part, so the CPU load and the complexity is the half.
It is not possible to do the entire process on a single ATmega328P since the FHT consumes almost all the processor resources.


![Schematic](https://github.com/joanperelopez/Modifications-TS-520/assets/73885181/55cd9e32-1d0c-4cf3-96bf-c9d6cb068cca)


Two shields are provided for the RF sections of the receiver. These should be fitted only after both sections are tested and working correctly.
Connectors J7 and J12 are used to program ATmega328P microcontrollers.
Connectors J5 and J6 connect to the IL9341 2.4" display.
