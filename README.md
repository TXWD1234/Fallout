# **TXCompute**

TXCompute is an IDE device of the TXCSL language.
It consist of a keyboard and a terminal-like text editor.
It allow you to write, edit and execute TXCSL program.

---
This project consist of 3 main parts:
- The Hardware Assembly
- Terminal Engine
- TXCSL Languge Processor

# The Hardware Assembly

The TXCompute device, or the TXComputer consists of 4 parts:
- The CPU / MCU
- The LCD Screen
- The keyboard
- The shell

## Appearence

The TXComputer appears like a Apple 2 / IBM old computer, since it's size and meaning matches that era perfectly: *Small screen, DOS-like terminal based interface, Pixel font ...*
The Screen has a size of `96.52mm(Width) x 57.26mm(Height)`, with a resolution of `480px(Width) x 320px(Height)`.
It's stabled to the shell in a `100` degree angle, facing the keyboard and the user behind the keyboard.
The keyboard is a 16 key keyboard, laying flat but with a small angle of `10` degree to the ground, also stabled on to the shell.
The shell has it's main color being beige. it's round cornered, and have smooth surface. It hides all the cables and PCBs from the user, leaving only the keyboard and screen interface.

## The CPU/MPU

TXComputer uses ESP32-S3-WROOM-1 Devboard as it's MPU.
It's the conclusion after consideration of:
- **Prize:** ESP32 is much cheaper then Raspberry Pi; 
- **Performance:** ESP32 have much more RAM and flash memory then Arduino
- **Pragrammability:** ESP32 have a complete tool chain: ESP-IDF, which is completely based on CMake and C, which is what I am familiar with

## The LCD Screen

TXComputer uses a `480px(Width) x 320px(Height)` LCD screen pro

## The Keyboard


## The Power supply

## The PCB board


# Terminal Engine



# TXCSL






# The story of TXCompute - Lower, lower, and lower....