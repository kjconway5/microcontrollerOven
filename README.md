Kye Conway 
kyejconway5@gmail.com
ECE-13, Fall 2023

Summary: 
  In this lab, I used a provided finite state machine and timer interrupts to recreate the functionality of a 
  toaster oven on my PIC 320x microcontrollers. I implemented three oven functionalities, those being bake, broil and 
  toast. In bake mode, the user provided input on the potentiometer to change the time and temperature of the oven. In 
  broil the timer to can be adjusted using the potentiometer while the temperature is kept constant at five hundred 
  degrees. In toast, the timer can be changed with the potentiometer, but the temperature is not changed or printed 
  to the OLED screen. From the finite state machine, I implemented the ability to switch between the cooking modes with 
  the press of button three on the board. A button press allows the ability to change the time and temperature in bake 
  by storing the button press time and checking if it was longer than our set threshold press time of one second. I also 
  implemented the ability to start the cooking feature with each cook mode’s special character from the provided 
  ASCII.h file. The ability to reset the oven if the user started the cooking process and wanted to reset back to the 
  setup mode was also implemented. For extra credit, I implemented the ability for the OLED to alternate between an 
  inverted and a non-inverted state after the timer has run out while in the cooking mode. The key concepts used in 
  this lab were **C programming**, **event driven programming**, **finite state machines**, reading the prvided finite 
  state machine diagrams, and using interrupts to utilize our finite state machine to start certain processes. 


Example Video: [https://drive.google.com/file/d/1ExnI1JbU4NZyMG4CQCu1borJtuADqtFk/view?usp=sharing]

