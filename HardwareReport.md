# 2026-04-08

## 2026-04-08 19:39:15:<br>Category: KiCad<br>Topic: The absolute beginning steps
### 1. Controls:
- `A`: add symbol
- `V`: set value
- `F`: set footprint (The pysical size, usually use `Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P2.50mm`)
- `W`: wiring tool - can also be done by just clicking on the nodes. *(Just like Endfield)*
### 2. The Microprocessor
Microprocessor is also called MCU, which is the core of the entire hardware project.
*The MCU I am currently using is `ESP32`, the specific version is `ESP32-S3-WROOM-1`.*
There are 5 core parts of the MCU as far as i undertsand:
- 3V3: the power source, that has 3.3V of voltage
- GND: the ground
- EN
- TXD0 / RXD0
- IO ports
### where to start
1. press A, and search esp32, find `ESP32-S3-WROOM-1`, double click and put it somewhere in the schematic
2. press A again and search `3V3`, find it and place it at the top of the `esp32`. don't wire yet. do the same thing with `GND` but at the bottom.
3. Connet the `3v3` symbol to the 3v3 port of `esp32`, and do the same thing for `GND`.

## 2026-04-08 20:06:45:<br>Category: PCB Structure<br>Topic: Decoupling capacitors
Decoupling capacitors minimize the vibration of the eletric current. (since them change every nanosecond)
It's two capacitors in parallel that connects the 3v3 line and the GND line.
One of them is set to `0.1uf` for handling small vibration, and another is set to `10.0uf` to handle large, slow power changes.
*how to change value: hover you mouse on the symbol, and press F.*

## 2026-04-08 21:37:44:<br>Category: KiCad<br>Topic: Symbol, Reference and Value
*Symbol is like a class or type in C++.* it's a component to create a project - It is the component that compose of a project.
Symbols are the representation of real world hardware components.
Reference is the name / unique identifier of a symbol. Each individual symbol will have a reference. *It's like the variable name in C++.*
Value is the attributes of a symbol object, that defines the behavior of that particular object.

# 2026-04-09

## 2026-04-09 15:31:05:<br>Category: PCB Structure<br>Topic: I2C
I2C("I square C" / Inter-Integrated Circuit / IIC) is the protocol of hardware components communicating.
I2C defines that there will be 2 lines in the communication system: SDA (Data line / Signal Data) and SCL (Clock line / Signal Clock).
By the default state, each line will be in high state.
When a device attempt to send a signal (a message), first it pull the state of both lines into low. That indicates a signal is trying to be sent.
Then, for the rest of the message, the SCL will keep doing a high-low-high-low loop. Each state change of the SCL indicates that a bit on the SDA is being sent.
In sync with SCL, the SDA will send one bit for each clock state change.
In the beginning of the message, the SDA will have a stride of data reserved for the metadata (attributes) of the message. Things like the receiver will be contained in there.
Then proceed will be the actuall bit data in the message. For each 8 bit (one byte), a ACK (Acknowledge) bit will be sent by the receiver, indicating that it received the data correctly. A low bit indicates `ok`, and a high bit indicates `error` or `eof`.

### Note for ESP32
In ESP32, which pin will be associating with which need to be explicitly setted in code, by calling a function.

## 2026-04-09 15:23:43:<br>Category: PCB Structure<br>Topic: Pull up
Signals in hardware are just high and low voltages. 0V is low (representing 0), and anything above that is high (representing 1).
When not powered, a wire is low by default (because obviously it don't have any power, so it don;t have any electricity). But in I2C, the default state should be high.
Because when the wire is low, it would catch any signals that are higher then 0, and that will cause a state change. Which, would happen a lot since the hardware have a lot of noise.
Therefore, we need to set the wire to have a default high voltage, and then whatever voltage change expect droping to 0, will still result a high (1) state.

### Implementing Pull up
Pull up is essentially powering the two signal lines: SDA and SCL. And to power a line, all the action need to be done is just connect it to the power source: VCC / 3V3.
To merely achieve Pull up, just connect the power source with the signal lines will do it.
But 3V3 have too many power, not only the noise will be blocked, it would also block the signals from the ESP32. And the signal lines will always stay high.
Therefore, we need to add resistors to reduce the voltage of the 3V3, such that the voltage would be strong enough to block the noises, while week enough to let the signals from the ESP32.
*And that specific number of resistance is 4.7k Ohms.*

## 2026-04-09 16:07:09:<br>Category: KiCad<br>Topic: Net Labels
Net labels in KiCad is like the teleporting points. It's endpoint of lines, and each net label object is connected.
Net labels can be created by pressing `L` key, and enter the name. net labels with same name will be connected.
By using net labels, the schemantic can be organized very cleanly.

## 2026-04-09 19:41:14:<br>Category: KiCad<br>Topic: The nature of Power symbols
The power symbol you place, such as `VCC` and `GND` are not repersenting physical components, instead they are purely just a net label;
What they actually are have almost no difference then the net labels we placed.
They also connect together like teleporting.
The only difference of them between Net Labels is that they are global, which means they work everywhere, even on different pages they still are connected together.

### VCC vs GND
Usually `VCC` is created uninitialized. You need to initialized it by yourself.
But GND usually don't require an initialization.

## 2026-04-09 19:10:13:<br>Category: PCB Structure<br>Topic: LDO (Linear Regulator)
Because USB provide the power of 5V, and ESP32 accept 3.3V, connecting the USB directly on ESP32 will destory the chip.
Therefore, a voltage converter need to be placed in the middle.
And that's what called a LDO (Linear Regulator).
The common type of LDO, *and what I used* is `AMS1117-3.3`.

### **Important Notes:**
**Both the input and the output of the LDO need a capacitor of `10uf`.**

## 2026-04-09 19:11:46:<br>Category: Personal Journal<br>Topic: The first refactor
As I LDOs, and since I organized my schemantic into boxes and areas, I need to refactor the VCC area.
And what that means is that my first Hardware refactor had came!
THE PAIN, HAD ONLY BEGAN....

## 2026-04-09 19:17:20:<br>Category: PCB Struture<br>Topic: VBUS - USB Interface
*This is a big thing, get ready. —— TX_Jerry*
Remember the VCC initialization thing we memtioned before (in `# The nature of Power symbols`)? well this is the thing that initializes it.
It's basically allowing using USB cable connection to power the PCB.

### VBUS
VBUS (USB Bus Voltage) is the same concept as a power label: it's a Net Label, that represent a concept instead of a actual physical component.
VBUS is basically the power supply from the USB.

### USB-C Input Receptacle
When you have an USE-C cable and you want to use it to power something, the first thing you are looking for is definitely the plug slot or socket for the cable, right?
*That "plug slot" or "socket" is called **"Receptacle** in Hardware.*
And to use USB to power our own PCB, we need one of those receptacle too.
And that's the `USB_C_Receptacle`. *(or simpler version, which is more recommanded for beginners: `USB_C_Receptacle_USB2.0_0_14P`)*
These are the pins on the `USB_C_Receptacle_USB2.0_0_14P`:
- `SHIELD`: connects to `GND`
- `GND`: connects to `GND`
- `D+` / `D-`: connects to `D+` / `D-` on the `ESP32`. **Note that there are 2 `D+` / `D-` each? connect the 2 `D+` together; same for `D-`. This exist because USB-C works for both ways you plug it in, so you need to enable both of them to ensure that machnisum works.**
- `CC1` / `CC2`: connects to GND, with a `5.1k` resistor each
- `VBUS`: connects to the `VBUS` label

### Linear Regulator
Linear Regulator / Voltage Regulator (LDO) is... wait I've already talked about this before. Go read `LDO (Linear Regulator)` if you haven't already.

### The whole picture
First it's `USE_C_Receptable`, which have a `GND` connecting to `GND` Label, 2 `CC` connecting to `GND` Label, each with a `5.1k` resistor, and a `VBUS` connecting to the `VBUS` label; and then there's `AMS1117-3.3`, which have `VBUS` Label as it's input, with a `10uf` capacitior, and then output to `VCC` Label with another `10uf` capacitor, and also a `GND` that connects to `GND` Label. 

## 2026-04-09 19:53:24:<br>Category: Practical Knowledge<br>Topic: The Capacitors
Capacitors are like buffers, not the memory buffer, but literally buffers that buff the overflowing or vibrating electricity.
As you know, hardware and electricity is unstable. They always change. Therefore to get the stable source of electricity, we need buffers to handle those overflowing IO.
Hense, that's why capacitors need to exist everywhere - because they are the "door holders" in the electricity world.

# 2026-04-10

## 2026-04-09 22:31:28:<br>Category: Bug Report<br>Topic: Capacitor placement
**Capacitors need to be connected in parallel, but not in serie!**
The top of the capacitor need to connect with the line it's attaching with, and the bottom need to connect with the `GND`.

## 2026-04-10 16:01:49:<br>Category: KiCad<br>Topic: Buttons and switches
In KiCad, a switch have a `SW_` prefix.
One common type of switches is: `SW_Push`. It's just a simple push button that makes a complete circuit when pressed.

## 2026-04-10 16:09:43:<br>Category: PCB Structure<br>Topic: The boot setup - EN and IO0
The booting setup of the ESP have 2 things: EN - the overall power toggle, and IO0 - the debug/release toggle.

### EN
`EN` is the overall power toggle. When EN is low (powered off), the entire MCU will be powered down.
Switching EN from high to low then to high is called a *"reset"* operation - basically just power off then on. It would clear out the RAM, and run the boot process again.

### IO0
`IO0` is the toggle between `Download` mode and `Running` mode.
When `IO0` is low, the ESP32 is in the `Download` mode, which the data from the computer, connecting to the ESP32 with an USB wire to the USB socket on the ESP32, will be able to be *flashed* into the hard drive of the ESP32.
When `IO0` is high, the ESP32 will be in `Running` mode, where the program you wrote and uploaded will be directly executed as soon as it turned high.

### Implementation
1. connect the `EN` pin from the `ESP32` to `VCC` with a `10k` resistor between.
2. connect the `EN` pin to the `GND`, with a push button (`SW_Push`) between it, that allow you to toggle the state.
3. do the same thing for `IO0`

### **Note for using the dev board**
When using the dev board, the implementation of this part is already included in the board.
It appears as 2 buttons on the dev board labeled `EN` and `BOOT`, that acts exactly like the push buttons you placed.
Therefore, no need for double implementation, you can just use the buttons they provided.

## 2026-04-10 18:24:23:<br>Category: Development Knowledge<br>Topic: How the program in ESP32 (embeded system) is executed
Inside ESP32, there's a big flash memory.
That flash memory is the "Hard drive" of the ESP32.
And the program you wrote, and compiled is store in this memory.

---
When you powered up your ESP32: high your `EN`, it would first look at the `IO0` and check if it's high.
If `IO0` is low, it would jump to the download mode handler (it's a built-in firmware) and wait for you data to be transfered.
if `IO0` is high, it would jump directly to the binary program you uploaded, and start execute it from it's entry point.
*In normal desktop applications, running a program would require it to be load into the RAM,* But in embedded system, it would be executed directly from the flashed disk, and instruction by instruction until the final return of the main function.

---
ESP32 do have a heap, but it's size is limited, it's only 512KB. Which means, you have to care about fragmentation and memory managament more now, since if you exceed the memory limit, it will not only be a segmentation fault, but a straight crash of the program.

## 2026-04-10 19:06:59:<br>Category: Personal Journal<br>Topic: The "dev board or not" struggle
I was once had the idea to make a calculator from the logic gates. And that was the original idea of this project.
But at that point i have completely no idea what everything looks like, so i was just dreaming.
When i actually started researching, i found out very soon that the time and money only allow a project that have the complexity starting from MCUs.
So i decided to use MCU, and now im deciding weither i should use dev board or not.
*Building from scratch* was always what i am going for, and i enjoyed it. But cleaerly we don't have the time for that.
Especially with the high cost of testing and developing for hardware, it's a very harsh development environment, where a tiny mistake can burn the entire thing down, I don't have the time to build from scratch this time.
Therefore, Even though I want to, I don't *dare* to try it.
> *"Let's stick with the project rather then the rabit holes."*

# 2026-04-11

## 2026-04-11 13:26:42:<br>Category: Development report<br>Topic: Installing ESP-IDF - the ESP IDE
In order to program the ESP32, some specific operations need to be done.
First, you need to compile you C++ code with a specific cross-compiler for the embedded system of ESP32.
Second, you need to copy you built binary to the specific location inside the ESP32 chip. The bootloader will only look at there for binary instructions to execute.
And there's an entire IDE to do that, and that's **ESP_IDF**.

### Installation
Install the extension ESP-IDF in your VSCode, and follow it's notification to the EIM (ESP-IDF Installation Manager), and click install. *(at least that's how i did it)*

---
Note: after installation, the the default path of it will be inside `~/.espressif`.
The ESP_IDF can be found in `~/.espressif/v6.0/esp-idf`.
The EIM executable could be found in `~/.espressif/eim_gui/eim` in case you want to reinstall.

## 2026-04-11 16:52:22:<br>Category: Hardware Programming<br>Topic: ESP-IDF project setup
### Create a ESP-IDF Project
The ESP-IDF extension serves a specific type of project: "The ESP-IDF project".
The extension will only be activated once it detected that it's a ESP-IDF project.
The one attribute a project have that makes it qualify as a ESP-IDF project is that:
**It has a `CMakeLists.txt`, and inside that `CMakeLists.txt` there is the presence of this line:**
**`include($ENV{IDF_PATH}/tools/cmake/project.cmake)`**

---
To create a ESP-IDF project, press Ctrl+Shift+P and search: `ESP-IDF: Create New Empty Project`.
Enter the name of the project and choose it's path.

#### check if the extension is activated
if there's a long row of options at the bottom bar, including "ESP-IDF v6.0.0"(or whatever version you have), and some kind of a chip model such as "exp32", then the extension is activated.

### Configure the project
*"Configure" here means literally configuring it, not the CMake Configure state.*
There are a few setups need to be done before the project is eligible to build.

#### The Device Target (The chip model)
On the bottom bar, there is a chip model displaying.
The default model is `esp32`. But you might want to set it as a different model.
Click on it to set the model.
*select the first configuration(`USB_JTAG`) if it asked after you selected the model.* 

#### The CMakeLists.txt
In the most outer CMakeLists.txt (the one in the project directory), there is the magic line that identified this project is a ESP_IDF project:
`include($ENV{IDF_PATH}/tools/cmake/project.cmake)`
But `$ENV{IDF_PATH}` remains empty and uninitializaed. *For some reason that value is not setted in the config files or by their configuration tools.*
So you have to manually initiallize the variable `$ENV{IDF_PATH}`. For example:
`set(ENV{IDF_PATH} /home/TX_Jerry/.espressif/v6.0/esp-idf)`

### Use CMake to build the project
For the ESP-IDF project to successfully build, it requires some environment variables.
And if you directly build from VSCode, it won't have those variables, since VSCode is just calling `cmake --build ./build`, it won't know to set the variables.
*You could try edit the config files to make VSCode set those variables, but i just never got them to work.*
Therefore, we need to use the command line to build the project first **once**, and then in the future we would rely on incremental build, and won't have to build the already built files that requires environment variavbles again.

#### grant command line the env vars
Inside the ESP_IDF dir (usually in `~/.espressif/v6.0/esp-idf`), there are 2 sets of multi-platform scripts: `install` and `export`.
*use `install` to ensure you have everything installed.*
And `export` does the exact task of giving the env vars to the command line.
**But because directly running the script will create a new process, and the variable will be given to that process, and the process you ran the script on will not have the env vars, you must run the script with a `.` before it, to make the env vars apply to current process.**
**Example:**
```bash
. ./"export.sh"
```

#### Build with CMake
After you have the evnv vars, you can create a `build` dir in your project files, and do the classic CMake build operations:
```bash
mkdir build; cd build
cmake -G Ninja -S ..
cmake --build .
```
And after it's completed with no errors, you should see a `<youProjectName>.elf` and a `<youProjectName>.bin` file in the build directory.

---
And now, you can go back to VSCode. and if you press F7 or click build now, it would not have problem with the environment variables.

# 2026-04-12

## 2026-04-12 13:42:32:<br>Category: Hardware Programming<br>Topic: ESP-IDF's CMake
*ESP_IDF wrappes CMake so heavily, it's almost a completely different system.*
*The ESP-IDF's CMake system rely heavily on hardcoding file names and dir names, and they propose "convention more then configuration", therefore you cannot really customize the workflow.*
*The ESP-IDF is built targeting C, though it 100% supports C++, it still requires some extra setup for it to work properly.*
ESP-IDF have a unique "component based" system. It treat every folder with a CMakeLists.txt with the line:
```c
idf_component_register(...)
```
as a "component". The concept of a component can just be analogized as target in CMake.
The registered name of a component is not defined in the function `idf_component_register`. Instead, it's defined as the folder / dir name that the CMakeLists.txt exists(the parent dir name of the CMakeLists.txt).

#### `idf_component_register(SRCS <srcN> ... INCLUDE_DIRS <dirN> ... REQUIRES <componentN> ...)`
The basic syntax of the function `idf_component_register` has 3 parameters:
- `SRCS`: The source files contained in this component. CMake Analogy: the `<src>` part of `add_executable(name <src>)`
- `INCLUDE_DIRS`: The internal AND external include dir of the component. CMake Analogy: `target_include_directories(name PUBLIC <dirs>)`
- `REQUIRES`: The prerequisite or dependency of the current component. write component name with `""` here. CMake Analogy: `target_link_libraries(name PUBLIC <libs>)`

### The main entry
Unfortunatly, the ESP-IDF's CMake force you to hae a `main` component, and it would ONLY look for a `main` component for entry point.
*So like, this thing is all just hard coding...*
Further, since ESP-IDF have it's own compiler, it make the entry function from `mian()` to `app_main()`.
And be because it's targeting to C functions, you have to write `extern "C"`:
```cpp
extern "C" void app_main(void) {}
```