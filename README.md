# **TXCompute**

TXCompute is an IDE device of the TXCSL language.
It consist of a terminal-like text editor.
It allow you to write, edit and execute TXCSL program.
The TXCompute project is targeting to be a more powerful calculator, that don't only rely on buttons, but can handle complicated logic.

---
This project consist of 4 main parts:
- The Hardware Assembly
- Fireware Infrastructure
- Terminal Engine
- TXCSL Languge Processor

# The Hardware Assembly

The TXCompute device, or the TXComputer consists of 5 parts:
- The CPU / MCU
- The LCD Screen
- The keyboard connection
- The power supply
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

## The Shell


# Software




# Terminal Engine



# TXCSL

TXCSL (TX Computational Scripting Language) is a bash-like, virtual machine based scripting language, which is capable of:
- Compute basic expressions
- Create variables
- Create functions

*TXCSL is targeting to be a DSL for computation. It is an enhancement of regular calculators, but not a turing complete programming language.*

TXCSL operates in the pipeline described below:
> **Every line of code will be *compiled* into instructions, then executed.**
Even the result appears instantly, it had been through the process of compilation.

## Interface Data Structs

### `Expression`
```cpp
struct Expression {
	std::span<Command> commandBuffer; // the commands / instructions
	std::span<num> constantBuffer; // constant values that are involved in the expression
	std::span<num> variableBuffer; // variable values that are involved in the expression
	tx::u32 registerCount; // the required registers when evaluating
	tx::u32 commandCount; // the count of commands / instructions
};
```
`Expression` is the **compiled binary instruction code** after compilation.
It will provide everything the `Evaluator` requires to calculate an output.
An object of this struct should be created before compilation, and all the buffers should be allocated with their respective size:
| Buffer Name      | Element Count |
| ---------------- | ------------- |
| `commandBuffer`  | 256           |
| `constantBuffer` | 64            |
| `variableBuffer` | 64            |

During compilation, the `commandBuffer` and `constantBuffer` will be populated.
But the `variableBuffer` will not be touched. The user need to provide values of the variables themself.

### `CompileResult`
```cpp
struct CompileResult {
	tx::u32 commandCount;
	tx::u32 constantCount;
	tx::u32 variableCount;
	std::vector<std::string_view> varibaleNames; // for the caller of compile() to fill in the variabe buffer
};
```

### How to populate `variableBuffer`

### Note for then span design
You can have a big buffer that's bigger then the buffer's required size for each buffer.
Then you can store multiple compiled expressions. (The function design)
And that's what the `*Count`s in `CompileResult` is for.

## Compilation

*There is the where the major processing logic of TXCSL is.*
Every line of code in TXCSL is treated as expression, and will be processed by the `tx::csl::Compiler`.

### The design pattern of the compilation
Each stage will have it's own class.
That class will be a temporary object, which will outputs a certain result of the stage.
The purpose of the class is to maintain it's own internal state (because they are all state machin design), and the temporary memory they allocate.

### Compilation pipeline
The compilation pipeline flow is:
1. Raw string
2. BracketParser - compose bracket structure
3. Tokenlizer    - tokenlize raw string. identify and convert value
4. Compiler      - transform tokens into instructions, flatten the operation tree
The process of nested expressions will be lazy, meaning that it will not be processed until necessary. 
When the Compiler encounters an unexpanded expression, it will call the Tokenlizer again, and tokenlize the unexpanded expression.

### APIs

`static CompileResult compile(std::string_view source, Expression& result)`

Compile the raw source string `source` into instruction code in `result`.
**Note:** the buffers in `result` should be allocated before calling this function. the buffers listed below will then be populated in the function.


# The story of TXCompute - Lower, lower, and lower....