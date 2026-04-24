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

## 2026-04-12 19:05:56:<br>Category: Development report<br>Topic: How to find a suitable part
*https://www.lcsc.com/ is the professional website for purchasing electricity parts.*
First, determind the requirements.
For example, today when I was trying to find a model for my screen, I first determinded the dimension of the screen, and then which protocol it will use.
When I had a clear idea about what I would buy, I went to searching. Through research, I found the website `ICSC`, which is the professional platform to buy parts.
During the process of searching on it, I found out that electricity parts are actually cheaper then I thought.
And a also found a very useful stratagy:
**Look at the datasheet provided.**
It always contains very useful informations, and often helps a lot.
*(why did i feel like this is the most boring report i ever wrote?)*

## 2026-04-12 19:16:48:<br>Category: PCB Structure<br>Topic: The Graphics Pipeline
Since in hardware, or specificly, in embeded system such as ESP32, we don't get a GPU or a very big RAM.
Therefore, we have to shrink the size of the screen to fit it in the tiny computation power and memory capacity of ESP32.
The most common size of screens are 320x240 and 480x320. *which is absolute tiny comparing to modern technologies.*

### the Framebuffer layout
Since we don't have a GPU,*we are no longer drawing triangles,* we have to create the final frame buffer (raw screen pixel data) by ourself, or more accurately, by the CPU.
Further, since we don't have a very big RAM, the pixels cannot be fully 32bit RGBA colors.
The most common why to store the raw pixels is the RGB565 protocol (aka 16bit RGB).
It have 5 bit each for Red and Blue, and 6 bit for Green *because science tells you that human eyes are more sensitive for color green then the other 2 colors.*
you will be managing you own buffer, and use an API to draw.

### the PCB Connection
Between the screen and the ESP32, there are exactly 13 connections / lines for the 8-bit parallel data bus, which is the most optimal solution for a screen on ESP32.
8 of them (D0 ~ D7) are for direct data transfer. Which each of them is one bit, and together they send 1 byte each pulse, and each pixel will take 2 pulse.
and the rest 5 are controls:
- CS (Chip select) is the *"Attention Pin"*. It indicates the specific device (in this case, the screen) is listening if it's low. If CS is high, it will **ignore** all the data on the data lines.
- RSDC (Register Select / Draw Command) or DC (Data / Command) is the *"mode selector"*. It indicates the next byte is an instruction if it's low, and the next byte is a pixel data if it's high. *But this pin cannot be high for all the times. during specific phases of rendering, it need to be low and some commands need to be sent.*
- WR (Write) is the write clock for the 8 bit data bus. it will go high and low very frequently. In fact, the faster it swaps between high and low will determind how fase the data bus is. Because each pulse (it goes low and back high: 2 state changes) will triger the device to fetch the data once.
- RD (Read) is the read clock. Basically, since we never read from the screen, it's always tied high (Pulled up).
- RST (Reset) will reset the screen from hardware. It's usually used before everything, duing init time.
And there are also the VCC and GND pins, which results a total of 15 pins from the screen, and the allocation of 12 GPIO pins(VCC, GND and RD are excluded).

### The full pipeline
To draw a pixel on the screen: (a draw call1)
1. CS goes LOW (Hey screen, listen up!)
2. RS/DC goes HIGH (I'm sending pixel data, not a command.)
3. D0-D7 bits are set to the first half of the green color.
4. WR pulses LOW → HIGH (Screen grabs the first 8 bits).
5. D0-D7 bits are set to the second half of the green color.
6. WR pulses LOW → HIGH (Screen grabs the second 8 bits).
7. CS goes HIGH (I'm done for now).

## 2026-04-12 19:57:22:<br>Category: Personal Journal<br>Topic: Search term and common prize for a screen
Search term: `"320x240 2.8 LCD, OLED, Graphic RoHS"`
Prize: $3 ~ $8. pay attention at the protocol and whether it have touch pad or not. avoid the ones with `I2C` and `touch pads`.

# 2026-04-13

## 2026-04-13 21:24:36:<br>Category: Hardware Programming<br>Topic: Fixing the damn ESP-IDF CMake system - make it compatible with standard CMake libraries
*Because ESP-IDF have it's own CMake wrapper system, it's not compatible with other CMake libraries. And I HATE that so much.*
In order to fix this problem, the easiest way is to use the fundamental standard CMake system to link the libraries.

### The fundational structure of ESP-IDF's CMake system
In ESP-IDF's CMake system, targets are abstarted into "components".
And each component, basically wraps a CMake library target.
And at build time, all of those libraries will be linked to an internal executable, including the `main` component.
*Actually, it's the `main` component will be linked to the internal executable, and everything will be linked to the `main` component.*
And since a component contains a project, we can just make a component that links to the library. And then the entire thing will be connected with the ESP-IDF's CMake system.

### Making the "Umbrella Component"
*It's actually pretty staright forward.*
Since a ESP-IDF component have an internal CMake library target, we can just use the standard CMake way to link our libraries to the underlying CMake target directly.

#### `${COMPONENT_LIB}`
Huge thanks to who ever decide to add this entry, it's a direct access to the underlying target of a component in the component's CMakeLists.txt.
Yes, this variable literally represents the underlying target of a ESP-IDF component.

#### Linking the library
Since we have access to a CMake target, everything become so much easier, we can just do this:
```c
add_subdirectory("${TXLib}")
target_link_libraries(${COMPONENT_LIB} INTERFACE
	TXMath
)
```
But also, we need to register the component in order to access it's underlying target (basically defining the target).
We usually do that after the `add_subdirectory()`:
```c
add_subdirectory("${TXLib}")
idf_component_register()
target_link_libraries(${COMPONENT_LIB} INTERFACE
	TXMath
)
```

#### The Script phase pitfall
The ESP-IDF CMake system runs the CMakeLists.txt multiple times.
Sometimes it would be some specific mode, that forbidden some operations, such as the `SCRIPT_MODE`.
So make sure to wrap the include code in `if (!isScriptMode)`, but in CMake... hehe.
```c
if(NOT CMAKE_SCRIPT_MODE_FILE)
	add_subdirectory("${TXLib}")
endif()

idf_component_register()

if(NOT CMAKE_SCRIPT_MODE_FILE)
	target_link_libraries(${COMPONENT_LIB} INTERFACE
		TXMath
	)
endif()
```

# 2026-04-14 21:55:50

## 2026-04-14 21:55:54:<br>Category: Personal Report<br>Topic: Stage report
When I get more into the ESP-IDF development system, I got less passionated about the hardware programming part.
Purely becauset the in convinency and the lack of discipline for the IDE.
For something like OpenGL, even it do have very bad API design, it's wrappable, I can make beautiful API on top of it.
But ESP-IDF is the IDE itself, and I got no way to wrap or fix it.
The only thing i can do it to work around it or cheese or hack it.
Whatever way, it's not the most elegant way of doing it.
I would put the API design of ESP-IDF as a bad design.
But still, i am relying on it, since it's almost the only thing i can use to create ESP projects.
I've thought of switching to someting like Raspberry Pi, but it's way too complicated and **expensive**.
As to today, I've started learning hardware for 6 days. And all of them are centered around ESP32.
I don't want to change it mid way, not only because the reason above, more importantly:
**I just need to make the project. and how i can achieve it is not in consideration.**
Hardware is not like software, where i can build libraries and learn libraries and use it over and over again.
In hardware, one chip is one chip, you cannot reuse a chip in different projects.
**It's not reproducable.**
So the chance for me to change the chip type for next time will be very high, almost certain, but for this time, there's no time and necessarity to change it.

---
And these two days, I've been dealing with CMake and ESP-IDF to make the development environment "usable".
I have some problem remaining, waiting for me to fix, but the major problems are conquered.
I think I can be back to production by the end of tomorrow.
And then, it's just learning the Software APIs, and wrap them —— which I am pretty good at.
But for now, I'll say: Good night! 

# 2026-04-15

## 2026-04-15 20:15:23:<br>Category: Hardware Programming<br>Topic: The ESP-IDF include system
*In old tutorials, it often says to use the `driver` component. But in modern ESP, we have splited the giant driver component into multiple components.*
*Since CMake don't have Intellisense or suggestions (damn it!), you can see the components list in `"~/.espressif/v6.0/esp-idf/components/"`*

In ESP-IDF, components are like CMake libraries, you include, or `REQUIRES` them when declaring a component (via `idf_component_register`).
After `REQUIRES`ed the component, you then can include the headers inside them.
**Attension, when including in source code, remember to put the component name before the path, as a parent directory.**

### Some common components and headers
| Component | Headers |
|-|-|
`esp_driver_gpio` | `"driver/gpio.h"`
`esp_timer`       | `"esp_timer.h"`
`esp_system`      | `"esp_heap_caps.h"`

*Notice: ESP-IDF also have the recursive inclusion thing, since everything is public, the child component will have everything the parent component have. Therefore even if you didn't include some component explicitly, you might still be able to use them.*

# 2026-04-16

## 2026-04-16 21:09:27:<br>Category: Hardware Programming<br>Topic: The General Pipeline Overview
The ESP32 API set have 3 major "Component" *(not the actual ESP-IDF component, but literally components)*:
- GPIO control
- Hardware Peripherals
- Heap / Memory management
- Communication / Protocols
And a bunch of utility components, such as `esp_timer`.

### GPIO control
It's just controlling the GPIO pins to switch between 0 and 1, and also read what it is currently.
It's the fundamental backbone of the ESP32's communication system.
The GPIO is the most low level software control over the silicon signals.
It's often wrapped heavily by other components and high level APIs such as `esp_lcd`.
Therefore normally for common usage you won't need to touch GPIO APIs.
But as if you want any specifc or customized protocol that's not a common protocol, you would need to use the fundational APIs - the GPIO APIs.

### Hardware Peripherals
**Silicon Magic.** They are some specifc hardcoded curcuits in the chip that perform specific tasks.
They consist:
- timer
- I2C
- SPI
They utilize the GPIO pins and process the data the user gives them, and do the job faster then software.
*Instead of CPU doing everything, there are some very common functions was made into silicon magic, so they are ultra fast, and everyone uses it.*

### Heap management
In the world of ESP32, standard C `malloc` ~~will not work.~~*works but buns.*
Therefore the default `std::vector` and such will ~~not work too~~*be buns too*.
You have to use the ESP memory APIs to manage your own memory.
*Programs are all about manipulating memory.*

### Communication
Using the GPIO pins, for the ESP32 board to connect to the other components, such as screen and keyboards.
It consist of 3 parts:
- Panel - The handle or software manager class of the actual component (class representation of the actual part)
- IO - The point-to-point connection of a GPIO pin to the according pin of the actual hardware part.
- Bus - The manager of IOs, that defines a specific protocol, and toggle signals in that protocol according to the data you provided it.
Each part usually have a handle. it's the handle of the struct that stores the informations. and a config struct, which will be applied to a handle by a specific API.

# 2026-04-17

## 2026-04-17 17:45:22:<br>Category: Hardware Programming<br>Topic: GPIO Control
```cpp
#include "driver/gpio.h"
```

There are 2 sets of initialization for GPIO control:
- Direct Immediate functions
- Init-time baked configuration

### The system and the attributes
Just like many other big system *(im referencing OpnGL here)*, ESP-IDF's GPIO control system is controled by handles and attributes.
Each pin has it's own pin number. Sometimes it's bitmask, sometimes it's an index number. *The pin number is representing the IO number on the chip.*
And each pin have it's own configurations, which could be set using functions or a config class.
The available attributes are:
- mode: the IO mode: either input, output, or both way
  	```
  	GPIO_MODE_INPUT        // input
	GPIO_MODE_OUTPUT       // output
	GPIO_MODE_OUTPUT_OD    // open-drain
	GPIO_MODE_INPUT_OUTPUT // both way
	```
- enabling pull up / enabling pull down
- type of interruption *(see interrupt section below)*
	```
	GPIO_INTR_DISABLE    // disable
	GPIO_INTR_POSEDGE    // when become 1
	GPIO_INTR_NEGEDGE    // when become 0
	GPIO_INTR_ANYEDGE    // when state change
	GPIO_INTR_LOW_LEVEL  // when is 0 (repeative)
	GPIO_INTR_HIGH_LEVEL // when is 1 (repeative)
	```

#### Id system
IDs in GPIO controls are just integers. Defined by `GPIO_NUM_n` enum.
When involving multiple ids, it becomes bit mask:
`0b0001` <- this is GPIO 1.
`0b1000` <- this is GPIO 4.
*Usually you can do: `1uLL << n`.*

### Direct Immediate functions

#### configuration

`gpio_set_direction(id, value)`

Set the direction *(mode)* of the indicated pin.

`gpio_set_pull_mode(id, value)`

Set the pull mode for the indicated pin.
Option available:
```
GPIO_PULLUP_ONLY
GPIO_PULLDOWN_ONLY
GPIO_PULLUP_PULLDOWN
GPIO_FLOATING
```

#### direct access

`gpio_get_level(id)`

Get the indicated pin's current state.
Return 1 or 0.

`gpio_set_level(id, level)`

Set the indicated pin's current state.
`level`: 0 or 1

### Init-time baked configuration
*You set the config once, and it runs forever.*
```c
struct gpio_config_t {
    uint64_t pin_bit_mask; // bit masked pin numbers
    gpio_mode_t mode; // mode
    gpio_pullup_t pull_up_en; // boolean
    gpio_pulldown_t pull_down_en; // boolean
    gpio_int_type_t intr_type; // interrupt type (see interrupt section below)
};
```
You construct an object of this struct, configure it, and then pass it to `gpio_config()`, and then it's stored in the hardware register forever, and the GPIO pins you specified will run according to these rules.
*Look at these enums and functions and structs, back to WindowsAPI i guess... —— TX_Jerry*

`gpio_config(&config)`

Take in a pointer of a `gpio_config_t` object and set the configuration of the selected GPIO pins (defined in the struct).

#### Attributes

`pin_bit_mask`

The pins that you intend to apply this config to.
This value is bitmasked pin numbers (`1uLL << n`) combined together with bit or.
For example:
```cpp
pin_bit_mask = (1uLL << 1) | (1uLL << 2); // pin 1 and 2
```
or
```cpp
pin_bit_mask = 0b11 << 1; // pin 1 and 2
```

`mode`

The io mode of the pin.
Available options:
```cpp
GPIO_MODE_INPUT        // input
GPIO_MODE_OUTPUT       // output
GPIO_MODE_OUTPUT_OD    // open-drain
GPIO_MODE_INPUT_OUTPUT // both way
```

`pull_up_en` / `pull_down_en`

*It's just booleans. Ignore the enums, they are just int values.*
Enable or disable the pull up / pull down.

`intr_type`

*see interrupt section below.*

### Interrupt - The callback system

The interrupt feature is basically the callback system / event driven system of ESP-IDF.
*Similar to GLFW and Qt,* You can register a function pointer to be called when a selected state change happens.
So in summary (of the one sentence above), you have 2 things to configure: **what** function to call, and **when**

#### APIs

`gpio_install_isr_service(0)`

*Just call this at the init-time before using interrupt stuff.*
Initialization of interrupt and the entire callback system.

*In `gpio_config_t`:*
`intr_type`

Set the **When** attribute of the callback function for the pin selected.
Available attributes:
```cpp
GPIO_INTR_DISABLE    // disable
GPIO_INTR_POSEDGE    // when become 1
GPIO_INTR_NEGEDGE    // when become 0
GPIO_INTR_ANYEDGE    // when state change
GPIO_INTR_LOW_LEVEL  // when is 0 (repeative)
GPIO_INTR_HIGH_LEVEL // when is 1 (repeative)
```

`gpio_set_intr_type(id, value)` (Immediate)

Set the `intr_type` of a specific pin.

`gpio_isr_handler_add(id, funcPtr, nullptr)`

Register a callback function to a pin id.
`id`: the targeting pin id. just integer

#### **Important Note**

The function for call back often need to have `IRAM_ATTR` attribute:
```cpp
void IRAM_ATTR callback() { ... }
```
It makes ESP32 puts it in Instruction RAM instead of flash, avoiding crashes when flash memory is not available.

---
And also, notice that the callback system is not a software driven, but a hardware driven thing.
The callback function is running in restricted condition, so **don't put somthing like `malloc`.**

### Other APIs

`gpio_reset_pin(id)`

Reset the selected pin.

## 2026-04-17 19:57:06:<br>Category: Hardware Programming<br>Topic: Heap Management
```cpp
#include "esp_heap_caps.h"
```

*now we are talking! —— TX_Jerry*
Heap management in ESP32 have 2 major parts:
- Querying
- Allocating
***Notice that the standard `malloc` and `std::vector` still works, it's just not work that fine since you cannot set specifc attributes, and also: Fragmentation is a important thing now.***

### Caps

*Basically attributes of the buffer (yeah my brain is full of OpenGL concepts).*
Caps (Capability constraint) will tell the allocator the capabilities this memory mush have.
If there's no satisfied memory, it will just return `nullptr`.
Caps can be combined with bit or.
Available Options:
```cpp
MALLOC_CAP_8BIT      // normal data access
MALLOC_CAP_32BIT     // 32-bit aligned
MALLOC_CAP_INTERNAL  // internal RAM (fast)
MALLOC_CAP_SPIRAM    // external PSRAM
MALLOC_CAP_DMA       // usable by DMA
MALLOC_CAP_EXEC      // executable (IRAM)
```

### Querying

`heap_caps_get_free_size(caps)`

Get the current total free memory of certain constraints (caps).
`@return`: size_t

`heap_caps_get_largest_free_block(caps)`

Get the biggest continuous block of memory that satisfies the provided contraints.
`@return`: size_t

`heap_caps_check_integrity_all(print_errors)`

Check for memory corruptions and invalid writes.
`@return`: boolean -> good bit / bad bit

### Allocating

*Basically just standard C APIs but with `heap_caps_` prefix, and an additional parameter: `caps`.*
It supports most of the standard C allocation APIs:
```c
void* malloc(size_t size);
void free(void* ptr);
void* calloc(size_t n, size_t size);
void* realloc(void* ptr, size_t size);
```
but with prefix and caps:
```c
void* heap_caps_malloc(size_t size, uint32_t caps);
void heap_caps_free(void* ptr);
void* heap_caps_calloc(size_t n, size_t size, uint32_t caps);
void* heap_caps_realloc(void* ptr, size_t size, uint32_t caps);
```

### DMA

DMA(Direct Memory Access) allows Hardware Peripherals to directly fetch data from the RAM memory.
It makes a lot operations much faster, such as sending data to a bus, or use specific protocols.
But DMA would require:
- specifc region in RAM
- and continguous memory

# 2026-04-18

## 2026-04-18 13:02:10:<br>Category: Hardware Programming<br>Topic: Logging System
```cpp
#include "esp_log.h"
```

*Notice that normal `printf` and `cout` still works, but using the native ESP_LOG is just better practice for structure and stablity.*
The syntax of `ESP_LOG*` functions are fairly similar to the standard C `printf`, but they have a parameter before everything:
```cpp
ESP_LOGI("Main", ...); // there's one more parameter before the regular `printf` parameters
```
The new parameter is the "Tag" of this log message. It would be printed with the message in some default format.
The usual practice is define a `constexpr const char* tag` string and use that through out the process.

---
You probably noticed that I used `ESP_LOGx`. That means there are multiple function with the same parameter set and the `ESP_LOG` prefix.
**Each of these function is correspnding to a log level.**
There are 5 of these functions:
| Level         | Use                   |
| ------------- | --------------------- |
| `E` - Error   | something broke       |
| `W` - Warning | something suspicious  |
| `I` - Info    | normal important info |
| `D` - Debug   | debugging details     |
| `V` - Verbose | spam-level detail     |

## 2026-04-19 00:20:43:<br>Category: Development Report<br>Topic: The heap layout and Statistics experiment
*What i used to be told is that ESP32's heap is really fragmented, and very small. The usable space have only 140kb.*

Here's the test code:
```cpp
#include "esp_log.h"
#include "esp_heap_caps.h"


extern "C" void app_main(void) {
	size_t heapCapacity = heap_caps_get_free_size(MALLOC_CAP_8BIT),
	       internalCapacity = heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
	       dmaCapacity = heap_caps_get_free_size(MALLOC_CAP_DMA),
	       heapChunk = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
	       internalChunk = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
	       dmaChunk = heap_caps_get_largest_free_block(MALLOC_CAP_DMA);

	ESP_LOGI("Jerry", "Current Heap Capacity: %u", heapCapacity);
	ESP_LOGI("Jerry", "Current Internal Capacity: %u", internalCapacity);
	ESP_LOGI("Jerry", "Current DMA Capacity: %u", dmaCapacity);
	ESP_LOGI("Jerry", "Current Heap Chunk: %u", heapChunk);
	ESP_LOGI("Jerry", "Current Internal Chunk: %u", internalChunk);
	ESP_LOGI("Jerry", "Current DMA Chunk: %u", dmaChunk);
}
```
And here's the log:
```
I (260) main_task: Calling app_main()
I (260) Jerry: Current Heap Capacity: 401356
I (260) Jerry: Current Internal Capacity: 401356
I (260) Jerry: Current DMA Capacity: 393568
I (270) Jerry: Current Heap Chunk: 335872
I (270) Jerry: Current Internal Chunk: 335872
I (280) Jerry: Current DMA Chunk: 335872
I (280) main_task: Returned from app_main()
```
You can clearly see that indeed the biggest contiugous chunk is not same size as the total heap capacity, which indeed means that the heap is fragmented at stratup.
But, The one contiguous chunk is big enough, and way bigger then my old hypothesis. 356kb is a decent number of memory, in the context of embeded system.
Of course, this test is performed in the condition where things like wifi and LCD are not setted up, which means this is the pure and maximum memory it can possibly get.

## 2026-04-19 00:26:44:<br>Category: Personal Journal<br>Topic: The Lapse crash
Today, I had recorded 5 hours during afternoon, but when I tried to stop the recording, via pressing the stop button obviously, it didn't work: it purely refused to react, and nothing responded.
So I tried close and then reopen the browser after pressing the stop button a few times, and then I end up in the resume menu.
And then I clicked `submit`, and after that the progress bar came out, and it stoped at the `encryping session #1` stage, then a log poped out saying failure, something have to be smaller then 2GB.
And I also tried resume and then stop again using the stop button, but it appears the same error.
My hypothesis is that my lapse video is too big, since it's the longest one ive ever done: 5 hr. But I know other people that was doing more then 24 hours of lapsing, but still got away with it.
So... im confused and sad right now, because I lost 5 hour of recording....

# 2026-04-19

## 2026-04-19 12:33:04:<br>Category: Hardware Programming<br>Topic: The Graphics API - `ESP_LCD`
*`ESP_LCD` is a great example of the "Communication" component of the entire ESP32 general pipeline.*

`ESP_LCD` is a framework for hardware display, **It's not a render engine.** It only:
- Talk to the LCD driver
- Send framebuffer (raw pixel data, that is **composed by you**)
- manage bus communication
Anything higher then raw pixel data will **not** be managed by `ESP_LCD`.
*It's the next lower level then OpenGL.*
*The job `ESP_LCD` is doing is put the raw framebuffer to the screen, so the starting point of `ESP_LCD` API calls should be after framebuffer is constructed.*

### The 3 layer pipeline
There are 3 layers in the entire pipeline, each on top of each other.
- **Bus**
	The lowest layer, representing the actual lines of connections.
	It stores the metadata of pins, manages data transportation - basically just toggling the pins' on/off.
	It manages protocol of communication between `ESP32` and the LCD screen, essentially *the pattern to toggle the pins.*
- **IO**
	The intermetiate layer, representing the formatting of the data transferring.
- **Panel**
	The highest layer, representing the actual driver of the LCD screen.
	It's where you call all the top level API like draw call function and init funciton.

### The API pipeline
Each layer is a struct. It's inited with a function, where it *follows a standard C pattern, and* have you pass in the pointer to the object of that struct for the layer. In this case, the pointer parameter appears at the end of the function.
Each layer would require the previous layer as a parameter to init. *(except the lowest layer)*
After the Panel is inited, the layer below it is almost never touched again. All the operations will be done via the panel object.

### The Bus & IO Init and Terminate pipeline
**Init**
```cpp
esp_lcd_i80_bus_handle_t bus = nullptr;
esp_lcd_i80_bus_config_t config_bus = {};
esp_lcd_new_i80_bus(&config_bus, &bus);

esp_lcd_panel_io_handle_t io = nullptr;
esp_lcd_panel_io_i80_config_t config_io = {};
esp_lcd_new_panel_io_i80(bus, &config_io, &io);
```

**Terminate**
```cpp
esp_lcd_panel_io_del(io);
io = nullptr;
esp_lcd_del_i80_bus(bus);
bus = nullptr;
```
*well well well, look at the 2 delete functions, back to the inconsistent API hell... fortunately we don't have distinction between bind API and DSA API in ESP-IDF like what OpenGL has...*

#### The config_t parameters
**esp_lcd_i80_bus_config_t**
```cpp
struct esp_lcd_i80_bus_config_t {
    int dc_gpio_num;           // data/command pin
    int wr_gpio_num;           // write clock pin
    int clk_src;               // clock source (just write `LCD_CLK_SRC_DEFAULT`)

    int data_gpio_nums[16];    // parallel data pins (D0-D7 or D0-D15)
    int bus_width;             // `8` for 8-bit or `16` for 16-bit (Attension, it's the bus width, which is the data pin count, not the RGB protocol)

    size_t max_transfer_bytes; // DMA buffer limit (Unit: byte)
};
```

**esp_lcd_panel_io_i80_config_t**
```cpp
struct esp_lcd_panel_io_i80_config_t {
    int cs_gpio_num;                  // chip select (CS pin number)
    int pclk_hz;                      // pixel clock speed (usually 10 ~ 20)

    int trans_queue_depth;            // DMA queue size (usually 10 Mhz, which is `10 * 1000 * 1000`)

    int lcd_cmd_bits;                 // usually 8
    int lcd_param_bits;               // usually 8

    void (*on_color_trans_done)(...); // callback when finished
    void *user_ctx;                   // don't care about this
};
```

### The IO operations

`esp_lcd_panel_io_tx_param(io, command, parameter, parameterCount)`

Send a command and it's corrisponding parameters to the LCD screen.
*This function is the lower level function that `esp_lcd_panel_*` is using.*
It directly talk to the LCD screen, while using the protocol that the IO object is managing, so that you don't need to worry about it.
**Parameters**
`io`: The IO object
`command`: The command that's targeted to execute. type: `int`, it's defined by the driver type of the LCD that you are using. *(More of it in the future)*
`parameter`: The parameters about the command, type: `void*`. also defined by the driver type.
`parameterCount`: literally, parameter count.

`esp_lcd_panel_io_tx_color(io, command, data, dataLength)`
*The parameter signature of this function is exactly the same as `esp_lcd_panel_io_tx_param`, but they serve for different purposes.*
As if `esp_lcd_panel_io_tx_param` serves for command, this function serves for data transfer.
Specificly, **sending the raw pixel data.**
In this function, the `command` parameter is usually `0x2C`, which stands for **memory write**.

### The Panel
Because ESP_LCD don't support ILI9488 driver which is what i am using, so I decided to write my own driver...

## 2026-04-19 13:58:58:<br>Category: Design Report<br>Topic: The Screen Grid
The screen will have a width of 480px and height of 320px.
Since the project only need to display text, acting as a terminal, the entire screen can be dedicated for text rendering, and nothing else need to considered.
And since text are just retangles, abstract the entire screen into a grid system will be a good idea.

The entire screen is mapped into a `24x8` grid system, which each grid has size of `20x40px`.
Each grid is one ASCII character. It would have 4 pixels subtracted from width as horizontal margin (each side 2px), and 8 pixels from height for vertical margin (each side 4px), remaining a `16x32px` space for the actual text.
The height of one character will be split into 3 parts: top (9px), center (14px), bottom (9px).

*Or maybe move the bottom margin to the top? and maybe changet the parts to: top(8px), center(16px), bottom(8px)*

# 2026-04-20

## 2026-04-20 21:51:39:<br>Category: Hardware Programming<br>Topic: Making our own graphics driver - ILI9488 Documentation
### The begin of the problem
**The stupid ESP_LCD does not support ILI9488.**

### The Documentation - what save your life *(don't trust AI, always question)*
*If you want to build a project with screen size 480x320 and Parallel 8080, most likely you will use ILI9488, and most likely you will require to write your own driver.*
*And when it comes to hardware, you don't want to mess up, so the only reliable source of knowledge.*
*(well actually testing results is also a thing...)*

**Datasheet**
https://www.hpinfotech.ro/ILI9488.pdf
*You can literally find ILI9488 Data Sheet every where, this is just the one I am using.*

**How to read it**
*This is the main part.*

You will see a whole bunch of weird things, and you probably don't understand it. *(I don't either.)*
Well the best first step is turn to page 140, or search `5. Command`.
This will lead you directly to the command table, which is the most useful part *(I think)* of the entire datasheet.
You can find the usage and parameter descriptions here. More detailed information can be found after the table *(it's very long)*, in their respective sequence.

**How to read the command parameter description**

You see there are multiple rows, *(some of them may only have one, it's actually most of them have one)*, each row is a parameter, except the top row, which is the command data itself.
*If a command only have one row, then it means it don't have any parameters.*
The D1~D7 are the actual bit data of each pin. mostly you ignore that, but just read the hex value at the very right.

*Because each parameter might be bigger then a byte, for example, an `u16`, which is 2 bytes. then you need to do a bit to bit trick to put them into individual bits.*

# 2026-04-21

## 2026-04-21 19:41:53:<br>Category: Hardware Programming<br>Topic: Timer system
*There are multiple ways to either calculate duration or delay tasks. There's no perfect solution, each method serve for it's specifc purpose of usage. By the way, std::chrono might not work, or internally use them.*

### `vTaskDelay`
*Probably the simplest and most common used delay API in all methods.*
Function from FreeRTOS, operated and managed by the system.
The core logic of it is tell the schedualer that this thread / task is not running for a period of time.
It's not the most precise one - in fact, because it relies on FreeRTOS's tick system, it's pretty *inaccurate*, **in the microsecond world.**
It could be used for normal ms level delay, such as wait for a hardware to warm up, or force a sequence of execution.

```cpp
vTaskDelay(pdMS_TO_TICKS(500)); // 500 ms
```

`pdMS_TO_TICK(n)`

Convert ms to FreeRTOS's tick.

## 2026-04-21 23:17:25:<br>Category: Development Report<br>Topic: The Begin of TXCompute
This is almost the first actual progress of my project, **so far.**
In the pass time, I was all just learning. But today, once the first piece of code had reached from my brain to VSCode, I know, the development of TXCompute, had finally started.
April 8th, is when I started learning hardware. It took me 2 weeks to understand the hardware reality. And then, I have one month, to build something amazing.
And I, am prepared for that.

## 2026-04-21 23:21:55:<br>Category: Development Report<br>Topic: The ILI9488 Driver
*It's basically just a wrapper of the io system.*
Since ESP_LCD don't have a driver support for ILI9488, I have to write my own driver.
My driver is very tiny. It only compose of 2 component / functions:
- Initialization of the LCD screen;
- Write data to specific location of the LCD screen's framebuffer.
That's it. And that's all I need.
Call `ILI9488DriverPanel::init()` to initialize; Call `ILI9488DriverPanel::draw()` to write pixels.
*Yeah it's this simple.*

### Note
I potensially need to add gamma in initialization in the future too.

### The next step
Currently the `ILI9488DriverPanel` is just a static function collection. Stand alone it cannot do the job of rendering, it needs the IO.
Therefore, the next step will be a wrapper of the `ILI9488DriverPanel` driver and the IO, which will include the initialization, uninitialization, and forwarding of the IO and bus - so this will be a actual class object now.
And the goal of this wrapper will be **"Be able to draw content with just providing position, dimension, and data."**
I even have a name for it already:
**`FrameComposer`**

# 2026-04-22

## 2026-04-22 23:28:23:<br>Category: Development Report<br>Topic: TXCSL - `ExpressionEvaluator`
Since the Project of TXCompute had started, as the core logic handler of the project, TXCSL engine started becoming one of the considerations.
A math "programming language", definitely have to have a expression parser, and that's the first component I thought about the TXCSL engine.

---
`ExpressionEvaluator` is a string parser that handle 2 things:
- parse raw string expression into numbers and operations
- evaluate the expression according to the BEDMAS (order of evaluation) rule

### Key idea
`ExpressionEvaluator` will have  stages to evaluate a expression string:
1. structure the order of operation tree
   - compose brackets ranges - each bracket is an individual expression (recursive parsing logic), treated as one variable at their parent level
     - depth stack
     - parent / child data structure
   - compose Major operation unit tree - separate the expression by Minor operations (`plus` or `subtract`)
2. evaluate
   - evaluate each Major unit
   - evaluate minor operation recursively

## 2026-04-22 23:55:09:<br>Category: Development Report<br>Topic: `FrameComposer`
The wrapper around the `ILI9488Driver`, that manages the bus and io by RAII, so that the user only need to call `draw()` to render.

### Note
The `FrameComposer` deleted copy and move, it suppose to be only accessed by the singleton provided.

# 2026-04-23

## 2026-04-23 16:35:34:<br>Category: Development Report<br>Topic: `TextRenderer`
The application interface of the entire render pipeline.
Provide function: `draw(position, character)` to render the text bitmap.
The user still require to provide the bitmap data of the text / characters.

### Note
**The format of the `bitemapData` for text:**
Each character is required to have the same dimension.
Each character will occupy `std::ceil(width * height / 8)` bytes. Each bit (total of 8) in one byte will corrispond to one pixel (1 bpp / 1 Bit Per Pixel). If the pixel count is not exactly multiple of 8, the remaining of the last byte will be ignored.

## 2026-04-23 23:20:34:<br>Category: Development Report<br>Topic: TXCSL - `ExpressionEvaluator`: updated stucture
Instead of letting the user provide raw std::byte buffer, we ask user to provide a struct to me modified by the evaluator that contains all the necessary values to be evaluated:
```cpp
struct Expression {
	std::span<Command> commandQueue;
	std::span<num> constantBuffer;
	std::span<num> variableBuffer;
	tx::u32 registerCount;
};
```
And the stage from raw expression string to this expression data struct is called "compile". *(used to be called compose)*
After "compilation", the user will then pass the "compiled" result `Expression` struct object to `evaluate()` function, which will calculate the value of the expression.

### The reason of "compile"
Because I want to support function defining in the future, and since the "compilation" did most of the parsing job, is what's slow in the entire pipeline, "compiling" it into commands (aka instructions) is the method I think of to optimize it.

## 2026-04-23 23:27:36:<br>Category: Development Report<br>Topic: Project Outline: Things that I need to do before submit
- The Document
- The Shell design
  - Learn CAD
  - Design 3d model
- Physical connection design
  - KiCad work - schematic
  - not planning on using PCB, therefore it's just the schematic indicating the connections
  - Add USB port for external keyboard
- Terminal Engine
  - Converter from TTF font file to bitmap
    - Learn library
    - CLI
  - IO system
- Input system
  - Learn API
  - Build abstraction layer
- TXCSL
  - finish ExpressionEvaluator
  - variable system
  - function system
  - maybe branching?
- TXCompute
  - Connect everything together
  - memory manager



lights: backlights - brightness and power usage



Screen:
**480x320 - 10x5cm**
https://www.lcsc.com/product-detail/C19632787.html?s_z=n_q_480%2a320%25203.5%2520LCD%252C%2520OLED%252C%2520Graphic&spm=wm.ssy.bg.20.xh&lcsc_vid=RFUPX1FSFlRfAVUCTlhdX1MHT1ZaUwZURVINUVZXQFExVlNRQVRdX1BXQFRZUzsOAxUeFF5JWBYZEEoKFBINSQcJGk4dAgUUFAk%3D
**480x648 - 12x9cm**
https://www.lcsc.com/product-detail/C41416471.html?s_z=n_q_LCD%252C%2520OLED%252C%2520Graphic&spm=wm.ssy.bg.43.xh&lcsc_vid=RFUPX1FSFlRfAVUCTlhdX1MHT1ZaUwZURVINUVZXQFExVlNRQVRdXlVRQlBeVzsOAxUeFF5JWBYZEEoKFBINSQcJGk4dAgUUFAk%3D




## 2026-04-12 14:40:50:<br>Category: Personal Journal<br>Topic: The design idea struggle