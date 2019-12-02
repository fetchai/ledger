# Opcode Pricing

We have designed a [pricing mechanism](https://www.overleaf.com/read/qdgrnfdspjgd) to maximize transaction throughput in the fetch.ai ledger by incentivizing transactions to use a small number of resource lanes (or shards) and occupy the least congested lanes. Such a policy promotes efficient operation of an ideal ledger, but it is based on the assumption that transaction fees depend on lane usage alone. Ultimately, the price of transactions should also relate to the actual cost of maintaining the ledger, which in the case of digital tokens, is proportional to the resources consumed by the servers responsible for validating transactions. At a basic level, this can be expressed in terms of the expected number of CPU cycles and memory space needed to validate a transaction. Since transactions include smart contracts and are expressed in a programming language (Etch), they consist of a sequence of machine instructions. Estimates of the CPU time required by a transaction can therefore be decomposed into the times of the constituent instructions or opcodes.

In the fetch.ai distributed ledger, transactions are typically submitted via [Etch-code](https://docs.fetch.ai/etch-language/). Therefore, we estimate the CPU requirements of each opcode in the fetch.ai virtual machine by isolating it as much as possible in a compiled piece of Etch code. For example, if we want to compute the CPU requirements for adding two 32-bit integers, we can compile and time the following Etch code:
```c
	function PrimitiveAdd()  
	var x : Int32 = 1i32;  
	x + x;  
	endfunction
```
If we look at the opcodes that result from compiling this code, we get:

	7 PushConstant  
	2 VariableDeclareAssign  
	8 PushVariable  
	8 PushVariable  
	48 PrimitiveAdd  
	14 Discard  
	21 Return

Since we are interested in timing the add operation, we also time the following code for use as as a baseline:
```c
function PushVariable()  
var x : Int32 = 1i32;  
x;  
endfunction
```
This code runs the following opcodes:

	7 PushConstant  
	2 VariableDeclareAssign  
	8 PushVariable  
	14 Discard  
	21 Return

By subtracting the time of **PushVariable** from **PrimitiveAdd**, we obtain an estimate of the opcodes 8 (**PushVariable**) and 48 (**PrimitiveAdd**). Note that the primitive add operation requires pushing two variables to the stack. Since the arithmetic operations will almost always be executed after a push operation, we include both together in the benchmark timings. Similarly for the discard operation (14) after a push. However, we will show one way to estimate the isolated opcode times in the next section. Using the [Google benchmark library](https://github.com/google/benchmark) to run 100 repetitions of each compiled function, we obtain the following output:

	Run on (8 X 4000 MHz CPU s)  
	CPU Caches:  
	L1 Data 32K (x4)  
	L1 Instruction 32K (x4)  
	L2 Unified 256K (x4)  
	L3 Unified 8192K (x1)  
	------------------------------------------------------------------------------------
	Benchmark Time CPU Iterations
	------------------------------------------------------------------------------------
	OpcodeBenchmark/PushVariable_mean 53 ns 53 ns 13084724
	OpcodeBenchmark/PushVariable_median 53 ns 53 ns 13084724
	OpcodeBenchmark/PushVariable_stddev 0 ns 0 ns 13084724
	OpcodeBenchmark/PrimitiveAdd_mean 64 ns 64 ns 11084146
	OpcodeBenchmark/PrimitiveAdd_median 64 ns 64 ns 11084146
	OpcodeBenchmark/PrimitiveAdd_stddev 1 ns 1 ns 11084146

Since the measure of interest is the mean CPU time, we compute the standard error in the mean: $\sigma/n$, where denotes the standard deviation and n the number of repetitions. By this methodology, we can generate tables like the following, showing the mean CPU times for opcodes that do not depend on a variable or primitive.

### <a name="table1">Table 1</a>: CPU times for basic type-independent opcodes
| Benchmark (100 reps)   |   Mean (ns) |   Net mean (ns) |   Std error (ns) | Opcodes                           | Baseline     | Net opcodes            |
|------------------------|-------------|-----------------|------------------|-----------------------------------|--------------|------------------------|
| Return                 |       28.27 |            0.00 |             0.16 | [21]                              | Return       | []                     |
| PushFalse              |       35.94 |            7.67 |             0.12 | [4, 14, 21]                       | Return       | [4, 14]                |
| PushTrue               |       36.04 |            7.77 |             0.12 | [5, 14, 21]                       | Return       | [5, 14]                |
| JumpIfFalse            |       36.36 |            8.09 |             0.13 | [4, 19, 21]                       | Return       | [4, 19]                |
| Jump                   |       35.82 |           -0.54 |             0.07 | [4, 19, 18, 21]                   | JumpIfFalse  | [18]                   |
| Not                    |       39.19 |            3.16 |             0.04 | [5, 33, 14, 21]                   | PushTrue     | [33]                   |
| And                    |       45.05 |            9.01 |             0.08 | [5, 31, 5, 14, 21]                | PushTrue     | [31, 5]                |
| Or                     |       45.01 |            8.98 |             0.07 | [4, 32, 5, 14, 21]                | PushTrue     | [4, 32]                |
| ForLoop                |       66.17 |           37.90 |             0.13 | [7, 7, 23, 24, 18, 25, 21]        | Return       | [7, 7, 23, 24, 18, 25] |
| Break                  |       59.11 |           -7.06 |             0.10 | [7, 7, 23, 24, 16, 18, 25, 21]    | ForLoop      | [16]                   |
| Continue               |       65.90 |           -0.28 |             0.13 | [7, 7, 23, 24, 17, 18, 25, 21]    | ForLoop      | [17]                   |
| DestructBase           |       74.76 |            8.58 |             0.14 | [1, 7, 7, 23, 24, 18, 25, 21]     | ForLoop      | [1]                    |
| Destruct               |       77.92 |            3.16 |             0.13 | [7, 7, 23, 24, 1, 15, 18, 25, 21] | DestructBase | [15]                   |
| Function               |       40.49 |           12.22 |             0.12 | [26, 21]                          | Return       | [26]                   |
| VariableDeclareStr     |       35.31 |            7.04 |             0.13 | [1, 21]                           | Return       | [1]                    |

## Least-squares estimation of isolated opcode times

In the previous section, we found that some of the opcodes always appear in pairs. For example, arithmetic operations are almost always preceded by a PushVariable operation and isolated push operations are almost always followed by a Discard operation. As we will see later on, this also arises in more complex operations. Since it would be ideal to measure the cpu times for each opcode in isolation, we show one way to estimate these times here.

Let $A$ be a matrix where each row corresponds to a benchmark and each element $A_{ij}=k$ if benchmark $i$ uses opcode a total of $k$ times. Let $\mathbf{b}$ be a vector where each element $b_i$ is the mean cpu time to run benchmark $i$. Then the solution to $x = \argmin(||A\mathbf{x}-\mathbf{b}||^2)$ provides an estimate of the individual opcode times.

For the benchmarks listed in [Table 1](#table1), we seek values in the vector $\mathbf{x}$ (where each $x_i$ represents the cpu time required by opcode $i$) that make the following expression as close as possible to an equality.

$$
\begin{bmatrix}
  0 & 0 & 0 & 0 & 0 & 0 & 0 & 0 & 1 & 0 & 0 & 0 & 0 & 0 & 0 & 0\\
  0 & 1 & 0 & 0 & 1 & 0 & 0 & 0 & 1 & 0 & 0 & 0 & 0 & 0 & 0 & 0\\
  0 & 0 & 1 & 0 & 1 & 0 & 0 & 0 & 1 & 0 & 0 & 0 & 0 & 0 & 0 & 0\\
  0 & 1 & 0 & 0 & 0 & 0 & 0 & 1 & 1 & 0 & 0 & 0 & 0 & 0 & 0 & 0\\
  0 & 0 & 1 & 0 & 1 & 0 & 0 & 0 & 1 & 0 & 0 & 0 & 0 & 0 & 0 & 1\\
  0 & 0 & 2 & 0 & 1 & 0 & 0 & 0 & 1 & 0 & 0 & 0 & 0 & 1 & 0 & 0\\
  0 & 1 & 1 & 0 & 1 & 0 & 0 & 0 & 1 & 0 & 0 & 0 & 0 & 0 & 1 & 0\\
  0 & 0 & 0 & 2 & 0 & 0 & 1 & 0 & 1 & 1 & 1 & 1 & 0 & 0 & 0 & 0\\
  1 & 0 & 0 & 2 & 0 & 0 & 1 & 0 & 1 & 1 & 1 & 1 & 0 & 0 & 0 & 0\\
  1 & 0 & 0 & 2 & 0 & 1 & 1 & 0 & 1 & 1 & 1 & 1 & 0 & 0 & 0 & 0\\
  0 & 0 & 0 & 0 & 0 & 0 & 0 & 0 & 1 & 0 & 0 & 0 & 1 & 0 & 0 & 0\\
  1 & 0 & 0 & 0 & 0 & 0 & 0 & 0 & 1 & 0 & 0 & 0 & 0 & 0 & 0 & 0\\
\end{bmatrix}
\begin{bmatrix}
x_1\\ x_4\\  x_5\\  x_7\\  x_{14}\\ x_{15}\\  x_{18}\\ x_{19}\\  x_{21}\\
x_{23}\\ x_{24}\\  x_{25}\\  x_{26}\\  x_{31}\\ x_{32}\\ x_{33}\\
\end{bmatrix}\approx
\begin{bmatrix}
28.27\\35.94\\36.04\\36.36\\39.19\\45.05\\45.01\\66.17\\74.76\\
77.92\\40.49\\35.31\\
\end{bmatrix}
$$

In addition to minimizing $\argmin(||A\mathbf{x}-\mathbf{b}||^2)$, we also need to constrain the individual opcode times to be positive ($x_i>0$ for each $i$). Finally, since cpu times vary more for some benchmarks than others, we normalize the expression by the standard deviation $\sigma_i$ of each benchmark as follows: $\hat A := [\frac{a_{ij}}{\sigma_i}]_{m\times n}$ and $\hat\mathbf{b}:=[\frac{b_1}{\sigma_1},\dots,\frac{b_m}{\sigma_m}]$. This results in the following optimization:
$$
\mathbf{x}^* := \arg\min_{\mathbf{x}\geq 0}||\hat A\mathbf{x}-\hat\mathbf{b}||^2.
$$ 
The solution of the above example is: 
$$
\mathbf{x}^*  = 
\begin{bmatrix}
x_1\\ x_4\\  x_5\\  x_7\\  x_{14}\\ x_{15}\\  x_{18} \\x_{19} \\  x_{21}\\
x_{23}\\ x_{24}\\  x_{25}\\  x_{26}\\  x_{31}\\ x_{32}\\ x_{33}\\
\end{bmatrix} =
\begin{bmatrix}
7.81\\5.72\\5.82\\9.67\\2.33\\3.55\\4.83\\2.75\\27.88\\4.83\\4.83\\4.83\\
12.61\\3.19\\3.26\\3.16\\
\end{bmatrix}
$$
where the values of $\mathbf{x}$ are given in nanoseconds and the corresponding opcodes are defined as {1: VariableDeclare, 4: PushFalse, 5: PushTrue, 7: PushConstant, 14: Discard, 15: Destruct, 18: Jump, 19: JumpIfFalse,  21: Return, 23: ForRangeInit, 24: ForRangeIterate, 25: ForRangeTerminate, 26: InvokeUserDefinedFreeFunction, 31: JumpIfFalseOrPop, 32: JumpIfTrueOrPop, 33: Not}.

We can then use these estimates to decouple the opcode times listed in [Table 1](#table1), which can then be subtracted wherever they appear in other benchmarks. Discrepancies between the Mean column and total estimated time reflect the fact that the least-squares fit is necessarily an approximation.

| Benchmark (100 reps)   |   Mean (ns) |   Std. error (ns) | Opcodes: estimated times (ns)                                                   |
|------------------------|-------------|-------------------|---------------------------------------------------------------------------------|
| Return                 |       28.27 |              0.16 | {**21**: 27.72}                                                                     |
| PushFalse              |       35.94 |              0.12 | {**4**: 5.77, **14**: 2.44, **21**: 27.72}                                                  |
| PushTrue               |       36.04 |              0.12 | {**5**: 5.87, **14**: 2.44, **21**: 27.72}                                                  |
| JumpIfFalse            |       36.36 |              0.13 | {**4**: 5.77, **19**: 2.86, **21**: 27.72}                                                  |
| Not                    |       39.19 |              0.04 | {**5**: 5.87, **33**: 3.16, **14**: 2.44, **21**: 27.72}                                        |
| And                    |       45.05 |              0.08 | {**5**: 5.87, **31**: 3.14, **14**: 2.44, **21**: 27.72}                                        |
| Or                     |       45.01 |              0.07 | {**4**: 5.77, **32**: 3.2, **5**: 5.87, **14**: 2.44, 21: 27.72}                                |
| ForLoop                |       66.17 |              0.13 | {**7**: 9.67, **23**: 4.84, **24**: 4.84, **18**: 4.84, **25**: 4.84, **21**: 27.72}                    |
| DestructBase           |       74.76 |              0.14 | {**1**: 7.75, **7**: 9.67, **23**: 4.84, **24**: 4.84, **18**: 4.84, **25**: 4.84, **21**: 27.72}           |
| Destruct               |       77.92 |              0.13 | {**7**: 9.67, **23**: 4.84, **24**: 4.84, **1**: 7.75, **15**: 3.76, **18**: 4.84, **25**: 4.84, **21**: 27.72} |
| Function               |       40.49 |              0.12 | {**26**: 12.77, **21**: 27.72}                                                          |
| VariableDeclareStr     |       35.31 |              0.13 | {**1**: 7.75, **21**: 27.72}                                                            |

## Primitive opcodes

We tested the primitive opcodes for each of the different primitives to determine if there is any significant difference in CPU time. Table 3 shows that the different integer types result only in very small variations in CPU time.

### Table 3: Mean CPU times for integer primitive opcodes (+/- std error)
| Benchmark (100 reps)        | Int16 (ns)   | Int32 (ns)   | Int64 (ns)   | Int8 (ns)    | UInt16 (ns)   | UInt32 (ns)   | UInt64 (ns)   | UInt8 (ns)   |
|-----------------------------|--------------|--------------|--------------|--------------|---------------|---------------|---------------|--------------|
| PrimAdd                     | 12.01 ± 0.06 | 11.39 ± 0.09 | 11.79 ± 0.05 | 12.12 ± 0.06 | 12.44 ± 0.11  | 11.55 ± 0.09  | 11.85 ± 0.05  | 11.82 ± 0.05 |
| PrimDivide                  | 16.59 ± 0.12 | 15.56 ± 0.09 | 18.03 ± 0.05 | 16.09 ± 0.06 | 16.39 ± 0.08  | 16.26 ± 0.14  | 17.22 ± 0.06  | 14.91 ± 0.05 |
| PrimEqual                   | 17.08 ± 0.18 | 16.31 ± 0.14 | 16.45 ± 0.11 | 14.63 ± 0.49 | 15.71 ± 0.05  | 15.53 ± 0.09  | 15.73 ± 0.05  | 13.84 ± 0.06 |
| PrimGreaterThan             | 14.82 ± 0.07 | 15.15 ± 0.15 | 14.83 ± 0.05 | 14.72 ± 0.08 | 16.73 ± 0.06  | 16.29 ± 0.10  | 16.43 ± 0.05  | 15.32 ± 0.10 |
| PrimGreaterThanOrEqual      | 15.84 ± 0.08 | 15.32 ± 0.10 | 15.85 ± 0.07 | 15.82 ± 0.06 | 16.27 ± 0.11  | 15.62 ± 0.10  | 16.38 ± 0.12  | 14.91 ± 0.09 |
| PrimLessThan                | 14.50 ± 0.07 | 13.72 ± 0.09 | 14.21 ± 0.06 | 13.87 ± 0.05 | 15.06 ± 0.11  | 14.11 ± 0.10  | 14.32 ± 0.05  | 13.63 ± 0.07 |
| PrimLessThanOrEqual         | 14.80 ± 0.12 | 13.63 ± 0.09 | 14.18 ± 0.06 | 14.37 ± 0.07 | 16.10 ± 0.06  | 15.70 ± 0.09  | 16.43 ± 0.08  | 13.86 ± 0.06 |
| PrimMultiply                | 15.71 ± 0.06 | 15.48 ± 0.13 | 15.17 ± 0.06 | 15.35 ± 0.06 | 15.14 ± 0.06  | 15.79 ± 0.13  | 16.05 ± 0.11  | 15.25 ± 0.06 |
| PrimNegate                  | 4.96 ± 0.06  | 4.58 ± 0.10  | 5.15 ± 0.06  | 5.21 ± 0.05  | 5.19 ± 0.09   | 4.51 ± 0.09   | 4.65 ± 0.07   | 5.17 ± 0.05  |
| PrimNotEqual                | 16.02 ± 0.06 | 15.43 ± 0.09 | 16.00 ± 0.07 | 15.15 ± 0.09 | 15.92 ± 0.06  | 16.24 ± 0.11  | 16.72 ± 0.11  | 15.17 ± 0.09 |
| PrimPopToVariable           | 10.34 ± 0.05 | 10.39 ± 0.05 | 10.24 ± 0.04 | 10.28 ± 0.05 | 10.11 ± 0.05  | 10.16 ± 0.08  | 10.39 ± 0.09  | 10.26 ± 0.03 |
| PrimPushConst               | 7.97 ± 0.11  | 7.74 ± 0.11  | 7.86 ± 0.11  | 8.16 ± 0.12  | 7.91 ± 0.11   | 8.03 ± 0.12   | 7.95 ± 0.12   | 7.89 ± 0.11  |
| PrimPushVariable            | 10.42 ± 0.05 | 11.03 ± 0.09 | 10.65 ± 0.04 | 10.21 ± 0.05 | 10.60 ± 0.05  | 10.67 ± 0.09  | 10.25 ± 0.06  | 10.61 ± 0.04 |
| PrimReturnValue             | 5.60 ± 0.12  | 5.18 ± 0.11  | 5.34 ± 0.11  | 5.72 ± 0.12  | 5.66 ± 0.13   | 5.52 ± 0.12   | 5.28 ± 0.11   | 5.41 ± 0.11  |
| PrimSubtract                | 11.76 ± 0.06 | 11.90 ± 0.11 | 11.59 ± 0.05 | 12.70 ± 0.11 | 11.60 ± 0.06  | 11.25 ± 0.09  | 11.71 ± 0.05  | 11.75 ± 0.05 |
| PrimVariableDeclare         | 2.96 ± 0.11  | 3.33 ± 0.12  | 3.16 ± 0.11  | 3.01 ± 0.11  | 3.13 ± 0.11   | 3.17 ± 0.11   | 3.34 ± 0.12   | 3.07 ± 0.11  |
| PrimVariableDeclareAssign   | 9.89 ± 0.12  | 9.89 ± 0.11  | 10.03 ± 0.11 | 9.72 ± 0.12  | 10.03 ± 0.12  | 10.20 ± 0.12  | 10.10 ± 0.11  | 10.01 ± 0.11 |
| PrimVariableInplaceAdd      | 9.64 ± 0.05  | 10.06 ± 0.08 | 10.01 ± 0.10 | 10.23 ± 0.07 | 9.79 ± 0.06   | 9.70 ± 0.08   | 9.87 ± 0.08   | 10.18 ± 0.09 |
| PrimVariableInplaceDivide   | 14.48 ± 0.09 | 14.79 ± 0.05 | 18.10 ± 0.06 | 15.14 ± 0.06 | 14.73 ± 0.09  | 14.47 ± 0.07  | 16.69 ± 0.07  | 13.84 ± 0.09 |
| PrimVariableInplaceMultiply | 12.29 ± 0.05 | 12.28 ± 0.04 | 12.05 ± 0.04 | 12.32 ± 0.05 | 11.68 ± 0.04  | 12.04 ± 0.07  | 11.66 ± 0.03  | 12.19 ± 0.04 |
| PrimVariableInplaceSubtract | 9.98 ± 0.08  | 9.56 ± 0.04  | 9.59 ± 0.05  | 9.99 ± 0.07  | 9.26 ± 0.05   | 9.07 ± 0.07   | 9.26 ± 0.04   | 9.69 ± 0.06  |
| PrimVariablePostfixDec      | 10.36 ± 0.05 | 9.34 ± 0.05  | 9.35 ± 0.04  | 9.66 ± 0.04  | 11.13 ± 0.26  | 9.25 ± 0.07   | 9.07 ± 0.04   | 9.90 ± 0.05  |
| PrimVariablePostfixInc      | 10.65 ± 0.05 | 9.31 ± 0.04  | 9.75 ± 0.09  | 9.70 ± 0.04  | 10.53 ± 0.08  | 9.18 ± 0.07   | 9.06 ± 0.04   | 9.60 ± 0.04  |
| PrimVariablePrefixDec       | 10.60 ± 0.06 | 10.25 ± 0.07 | 10.15 ± 0.06 | 10.96 ± 0.08 | 10.61 ± 0.05  | 9.73 ± 0.07   | 9.89 ± 0.05   | 9.98 ± 0.04  |
| PrimVariablePrefixInc       | 10.14 ± 0.05 | 10.33 ± 0.08 | 9.64 ± 0.04  | 9.94 ± 0.05  | 10.06 ± 0.05  | 10.06 ± 0.08  | 9.18 ± 0.03   | 9.98 ± 0.08  |

In Table 4, we see that certain operations are more expensive for fixed-point than floating point primitives, but there are only small differences between 32 and 64 bits within each primitive class.

### Table 4: Mean CPU times for floating and fixed point primitive opcodes (+/- std error)
| Benchmark (100 reps)        | Fixed128 (ns)   | Fixed32 (ns)   | Fixed64 (ns)   | Float32 (ns)   | Float64 (ns)   |
|-----------------------------|-----------------|----------------|----------------|----------------|----------------|
| PrimAdd                     | 52.63 ± 0.12    | 12.49 ± 0.05   | 12.14 ± 0.06   | 12.07 ± 0.07   | 12.02 ± 0.06   |
| PrimDivide                  | 1212.01 ± 0.90  | 29.52 ± 0.06   | 32.08 ± 0.07   | 13.27 ± 0.06   | 13.46 ± 0.06   |
| PrimEqual                   | 15.42 ± 0.12    | 14.35 ± 0.10   | 13.38 ± 0.06   | 14.61 ± 0.07   | 15.00 ± 0.05   |
| PrimGreaterThan             | 17.80 ± 0.10    | 18.12 ± 0.07   | 17.49 ± 0.11   | 15.12 ± 0.07   | 15.02 ± 0.05   |
| PrimGreaterThanOrEqual      | 20.08 ± 0.12    | 17.62 ± 0.06   | 16.60 ± 0.06   | 16.64 ± 0.09   | 16.23 ± 0.08   |
| PrimLessThan                | 18.25 ± 0.12    | 16.92 ± 0.06   | 16.06 ± 0.08   | 14.41 ± 0.06   | 14.67 ± 0.06   |
| PrimLessThanOrEqual         | 20.31 ± 0.12    | 15.99 ± 0.06   | 16.88 ± 0.06   | 15.41 ± 0.10   | 15.09 ± 0.12   |
| PrimMultiply                | 90.78 ± 0.14    | 16.34 ± 0.07   | 14.95 ± 0.07   | 15.84 ± 0.12   | 16.58 ± 0.16   |
| PrimNegate                  | 20.66 ± 0.10    | 5.66 ± 0.05    | 5.20 ± 0.09    | 4.85 ± 0.06    | 4.73 ± 0.05    |
| PrimNotEqual                | 17.44 ± 0.16    | 13.72 ± 0.06   | 13.47 ± 0.08   | 15.66 ± 0.11   | 16.34 ± 0.17   |
| PrimPopToVariable           | 13.66 ± 0.06    | 9.79 ± 0.08    | 10.07 ± 0.07   | 10.59 ± 0.04   | 10.19 ± 0.05   |
| PrimPushConst               | 24.43 ± 0.12    | 7.76 ± 0.11    | 7.93 ± 0.11    | 8.31 ± 0.12    | 8.18 ± 0.12    |
| PrimPushVariable            | 17.78 ± 0.09    | 10.49 ± 0.04   | 10.82 ± 0.05   | 10.18 ± 0.06   | 10.19 ± 0.05   |
| PrimReturnValue             | 22.62 ± 0.11    | 5.42 ± 0.11    | 5.45 ± 0.12    | 5.49 ± 0.12    | 5.24 ± 0.11    |
| PrimSubtract                | 55.95 ± 0.15    | 13.39 ± 0.08   | 11.76 ± 0.06   | 11.80 ± 0.06   | 11.89 ± 0.08   |
| PrimVariableDeclare         | 6.87 ± 0.11     | 3.02 ± 0.11    | 3.53 ± 0.24    | 2.97 ± 0.11    | 2.88 ± 0.11    |
| PrimVariableDeclareAssign   | 29.68 ± 0.12    | 10.32 ± 0.13   | 10.08 ± 0.12   | 9.65 ± 0.12    | 9.73 ± 0.12    |
| PrimVariableInplaceAdd      | 39.73 ± 0.07    | 11.54 ± 0.08   | 12.72 ± 0.05   | 9.52 ± 0.06    | 9.46 ± 0.06    |
| PrimVariableInplaceDivide   | 1193.66 ± 0.48  | 27.79 ± 0.07   | 31.53 ± 0.06   | 13.06 ± 0.08   | 12.72 ± 0.05   |
| PrimVariableInplaceMultiply | 75.47 ± 0.15    | 14.39 ± 0.07   | 15.43 ± 0.06   | 12.23 ± 0.05   | 13.09 ± 0.09   |
| PrimVariableInplaceSubtract | 42.80 ± 0.11    | 11.78 ± 0.10   | 12.49 ± 0.07   | 9.71 ± 0.04    | 9.47 ± 0.05    |

## Math function opcodes

Similarly, we tested the math function opcodes for the various primitives. Table 5 shows that most math function opcodes take significantly longer for fixed point than floating point primitives.

### Table 5: Math function opcodes for fixed and floating point primitives
| Benchmark (100 reps)   | Fixed128 (ns)    | Fixed32 (ns)   | Fixed64 (ns)   | Float32 (ns)   | Float64 (ns)   |
|------------------------|------------------|----------------|----------------|----------------|----------------|
| MathAbs                | 93.70 ± 1.18     | 12.06 ± 0.06   | 13.77 ± 0.05   | 10.90 ± 0.08   | 11.51 ± 0.11   |
| MathAcos               | 3368.53 ± 1.62   | 162.24 ± 0.39  | 200.17 ± 0.16  | 22.41 ± 0.08   | 25.02 ± 0.07   |
| MathAcosh              | 10972.92 ± 5.51  | 275.08 ± 0.21  | 400.43 ± 0.14  | 34.77 ± 0.06   | 36.36 ± 0.09   |
| MathAsin               | 3272.88 ± 1.49   | 148.40 ± 0.08  | 190.70 ± 0.29  | 19.68 ± 0.09   | 23.90 ± 0.07   |
| MathAsinh              | 10920.12 ± 11.41 | 276.98 ± 0.19  | 388.16 ± 0.32  | 38.62 ± 0.07   | 39.16 ± 0.09   |
| MathAtan               | 2485.70 ± 2.51   | 88.02 ± 0.09   | 102.77 ± 0.09  | 24.93 ± 0.13   | 20.86 ± 0.06   |
| MathAtanh              | 6950.12 ± 4.52   | 167.13 ± 0.11  | 269.30 ± 0.11  | 34.07 ± 0.06   | 34.53 ± 0.07   |
| MathCos                | 5498.05 ± 2.65   | 139.75 ± 0.27  | 176.09 ± 0.12  | 14.19 ± 0.12   | 21.89 ± 0.09   |
| MathCosh               | 9756.85 ± 5.61   | 235.59 ± 0.16  | 330.50 ± 0.20  | 18.53 ± 0.08   | 27.71 ± 0.08   |
| MathExp                | 3984.08 ± 3.15   | 109.14 ± 0.10  | 140.83 ± 0.15  | 22.46 ± 0.07   | 25.63 ± 0.14   |
| MathPow                | --               | 316.70 ± 0.19  | 465.94 ± 0.23  | 24.32 ± 0.06   | 60.52 ± 0.09   |
| MathSin                | 5727.26 ± 5.58   | 134.60 ± 0.18  | 183.61 ± 0.43  | 12.25 ± 0.06   | 23.92 ± 0.08   |
| MathSinh               | 9772.40 ± 6.16   | 236.63 ± 0.17  | 332.75 ± 0.21  | 36.33 ± 0.06   | 37.54 ± 0.14   |
| MathSqrt               | 4533.79 ± 2.36   | 117.19 ± 0.32  | 186.90 ± 0.28  | 13.00 ± 0.14   | 10.60 ± 0.06   |
| MathTan                | 4169.56 ± 2.35   | 110.24 ± 0.09  | 132.83 ± 0.28  | 22.65 ± 0.07   | 22.76 ± 0.07   |
| MathTanh               | 11470.62 ± 9.54  | 256.53 ± 0.23  | 366.14 ± 0.07  | 35.54 ± 0.09   | 36.03 ± 0.07   |

# Parameter-dependent opcodes

Next, we profile the operations that may depend on variable size. When the relationship is linear, we report the least-squares linear fit, anchored to the first data point to ensure reliable estimates for small parameter values. For now, we do not attempt to do any nonlinear fitting.

## String-based opcodes

### Table 6: Least-squares linear fit parameters for string-based opcodes
| Benchmark (100 reps)           |   Slope (ns/char) |   Intercept (ns) |   Mean (ns) |   Std error (ns) | Net opcodes   |
|--------------------------------|-------------------|------------------|-------------|------------------|---------------|
| ObjectAddString                |             1.128 |           38.747 |   13742.782 |           14.243 | [8, 49]       |
| PushString                     |             0.545 |           44.120 |    4516.498 |            8.131 | [6, 14]       |
| VariableDeclareAssignString    |             0.543 |           51.653 |    4514.937 |            8.103 | [6, 2]        |
| ObjectGreaterThanString        |             0.017 |           13.284 |    4616.937 |            8.243 | [8, 43]       |
| ObjectEqualString              |             0.016 |           13.751 |    4601.948 |            8.227 | [8, 35]       |
| ObjectLessThanOrEqualString    |             0.014 |           15.059 |    4599.461 |            8.199 | [8, 41]       |
| ObjectLessThanString           |             0.013 |           14.206 |    4588.273 |            8.195 | [8, 39]       |
| ObjectGreaterThanOrEqualString |             0.013 |           13.627 |    4584.418 |            8.197 | [8, 45]       |
| ObjectNotEqualString           |             0.012 |           15.190 |    4582.888 |            8.191 | [8, 37]       |
| PushVariableString             |            -0.000 |            5.019 |    4471.781 |            8.101 | [8, 14]       |

![enter image description here](https://lh3.googleusercontent.com/x_2cbgzSlwBvWMeHQitI_21loITfcWKTzq483LRI75a8dTbaFHImrY7nODcruph3Ydj92NLs4y8)![enter image description here](https://lh3.googleusercontent.com/uVMF38lLDQ556126BMMsxfONlATjW_0T1B049w0NrLUoPQ4gSzgz8E0attdGjGJxqQijD9UXetg)
![enter image description here](https://lh3.googleusercontent.com/MIckMhq_X9IiNgrk3sWsJ-_kf0-Atz5A4jDMqpHaiq2AWal3w9i0VpdI0dy_PleNPCAk7idXzbM)
![enter image description here](https://lh3.googleusercontent.com/yPCTvd5Osq2mEFfbhwcu9DCtnDRZjgVFb2ilPtEmFQeYIUguJypx5Vfp0qRnaF3bPwkaAhabIkI)
![enter image description here](https://lh3.googleusercontent.com/JMGez-WoeUzB8-7TfSIaL7IqHc65DlyWusBKb7tbB3wDM_AwzwZ4jSEbQMpjEZE6TG3CAj12WKM)
![enter image description here](https://lh3.googleusercontent.com/dmPd8p11ANh3q3nUV7-Dy9Bk00PP3C4XdHb6HDn6SVx2YIPbsdguNfZVny_R2QK6IuRXu554RUU)
![enter image description here](https://lh3.googleusercontent.com/XtZHuq-gM385i0I2Sk56DgA8CQKujQR7dF_4wHOCxR-udr3rsk8QaquDFp1os7ux-0LOynTl1Fc)
![enter image description here](https://lh3.googleusercontent.com/ULrd2GvoXDK7Zpa5-Gm1B-L99KMJTApA5sWxpg0wQ2kMhxGqSdtnROvzH4FawsGi9_-JI_USyBs)
![enter image description here](https://lh3.googleusercontent.com/tjAvEXAplIo5tjCpUs9-7rJ8gGB_5wmhsdHuhMxtn1IoDI7gWhQcn13z6OYXWOhvitcT3YAPhMg)

## Array-based opcodes

We use a similar approach to profile array operations. As shown below, there is often a change in slope or behavior once the array length exceeds about 6000. For this reason, we compute the least-squares fits based only on arrays smaller than 6000. The change in behavior is likely a result of machine-dependent memory management, and memory storage will probably be included separately in transaction pricing.

### Table 7: Least-squares linear fit parameters for array-based opcodes
| Benchmark (100 reps)   |   Slope (ns/char) |   Intercept (ns) |   Mean (ns) |   Std error (ns) | Net opcodes          |
|------------------------|-------------------|------------------|-------------|------------------|----------------------|
| ReverseArray           |             0.277 |           13.673 |    2847.449 |            5.536 | [8, 107]             |
| ExtendArray            |             0.201 |           33.260 |    2967.516 |            6.909 | [8, 8, 102]          |
| DeclareTwoArray        |             0.126 |          114.496 |    1339.739 |            3.649 | [7, 98, 2, 7, 98, 2] |
| AppendArray            |             0.091 |           24.856 |    1351.099 |            3.797 | [8, 7, 99]           |
| DeclareArray           |             0.045 |           47.877 |     625.744 |            2.661 | [7, 98, 2]           |
| PopFrontArray          |             0.041 |           21.299 |    1058.931 |            3.596 | [8, 105, 14]         |
| AssignArray            |             0.003 |            7.606 |     600.269 |            2.648 | [8, 7, 7, 109]       |
| EraseArray             |             0.001 |            7.806 |     592.606 |            2.645 | [8, 7, 101]          |
| PopBackArray           |            -0.001 |           22.580 |     594.415 |            2.659 | [8, 103, 14]         |
| CountArray             |            -0.001 |           15.849 |     591.189 |            2.673 | [8, 100, 14]         |

![enter image description here](https://lh3.googleusercontent.com/UfiJ1SiH6oGEjyFpY40euuiSFIeKPikstWsH_DrBCv3wMj58-FSqUJN2wz_7fFNuf6WFbm311Tc)
![enter image description here](https://lh3.googleusercontent.com/TO0q5ZNpeU5gbttuaDqskclm2vypVyPRq5ZhZJNlP0nalfIg1ZbUYh22RvFQ3B9Y32XOEKlovuA)
![enter image description here](https://lh3.googleusercontent.com/iSK3mlbFRhqq7CkgdoUlvkglT1EH_HPEmc06l_LEQTWuNsGxayA30iFUeET0ptr15mV5uf3XMYE)
![enter image description here](https://lh3.googleusercontent.com/wXoM1NIO199bHXSVHhqrS67SBZB7r_pOrtBP917xgTZK5fA96pa7mAQRO4Qx6Wzn48Te_AbwLk8)
![enter image description here](https://lh3.googleusercontent.com/_4Mb9lC19WUurTI8bHrNBokSAbvzbs9YYrcpn36IIwVufWZ9y8V18G7sQqhdzKfg2PynZuoW5sM)
![enter image description here](https://lh3.googleusercontent.com/AHZQ2UblQZxx75ag3Ptg0h7_IlX6_6PjO1wAWTh7KvoVednW_HW_vlPd74OBS2gSZhgGR1La74Y)

## Tensor-based opcodes

We profile tensor-based operations using a similar approach. To ensure that they do not depend on the dimension of the tensor, we tested the following dimensions independently: 2, 3 and 4. 

### Table 7: Least-squares linear fit parameters for tensor-based opcodes
| Benchmark (100 reps)   |   Slope (ns/char) |   Intercept (ns) |    Mean (ns) |   Std error (ns) | Net opcodes                                                                   |
|------------------------|-------------------|------------------|--------------|------------------|-------------------------------------------------------------------------------|
| FromStrTensor_3        |            72.117 |          658.957 | 21459495.574 |          814.668 | [8, 6, 422]                                                                   |
| FromStrTensor_2        |            71.999 |          665.464 | 27505233.436 |          822.809 | [8, 6, 422]                                                                   |
| FromStrTensor_4        |            71.685 |          563.635 | 18579198.218 |          834.691 | [8, 6, 422]                                                                   |
| FillRandTensor_4       |            16.037 |           30.307 |  2099429.698 |          264.307 | [8, 416]                                                                      |
| FillRandTensor_3       |            16.025 |           30.315 |  2335978.656 |          256.090 | [8, 416]                                                                      |
| FillRandTensor_2       |            15.976 |           26.033 |  2954216.832 |          257.032 | [8, 416]                                                                      |
| FillTensor_3           |             1.077 |            7.318 |   183323.328 |           71.658 | [8, 7, 415]                                                                   |
| FillTensor_2           |             1.035 |           14.028 |   217779.776 |           69.744 | [8, 7, 415]                                                                   |
| FillTensor_4           |             1.008 |            6.782 |   157216.237 |           71.689 | [8, 7, 415]                                                                   |
| DeclareTensor_3        |             0.193 |          281.184 |    28487.991 |           27.967 | [7, 98, 2, 8, 7, 7, 109, 8, 7, 7, 109, 8, 7, 7, 109, 8, 402, 2]               |
| DeclareTensor_4        |             0.193 |          286.142 |    28007.845 |           28.103 | [7, 98, 2, 8, 7, 7, 109, 8, 7, 7, 109, 8, 7, 7, 109, 8, 7, 7, 109, 8, 402, 2] |
| DeclareTensor_2        |             0.154 |          277.480 |    28623.532 |           25.013 | [7, 98, 2, 8, 7, 7, 109, 8, 7, 7, 109, 8, 402, 2]                             |
| SizeTensor_4           |             0.002 |           15.864 |    27822.580 |           28.271 | [8, 419, 14]                                                                  |
| SizeTensor_2           |             0.002 |           26.582 |    28530.266 |           25.250 | [8, 419, 14]                                                                  |
| SizeTensor_3           |            -0.001 |           15.156 |    28087.432 |           27.902 | [8, 419, 14]                                                                  |

![enter image description here](https://lh3.googleusercontent.com/oJNMuMOKUsBiVlA8F7jreAgeq-yQ7lVb542ky3JoN7K6HOITzCD1FKWh_IklyRDvzEx9_fyFDv8)
![enter image description here](https://lh3.googleusercontent.com/wU7uOueyDmyWTef0S0oBmrLqlw0IL_VTpciNQpPMv_LmzKPcd4scQG3AkB3uOIU458_Kcns4Ibw)
![enter image description here](https://lh3.googleusercontent.com/Csu8HlTqcu9cYvpTaKQMKnHFKy26ICaZK8uTIxM9oof5pznGKvafkiyyW0dtxAnC05-PyKRPtSE)
![enter image description here](https://lh3.googleusercontent.com/awDzFVNDZbiNk2avZTqJHfW_8wNZdsZE3eewJ4mWm3Fw83IG234LeDFqswvthmxiJLodtxgUjTY)

## Crypto-based opcodes

### Table 6: Least-squares linear fit parameters for crypto-based opcodes
| Benchmark (100 reps)   |   Slope (ns/char) |   Intercept (ns) |   Mean (ns) |   Std error (ns) | Net opcodes         |
|------------------------|-------------------|------------------|-------------|------------------|---------------------|
| Sha256UpdateStr        |             4.576 |           63.464 |   37633.507 |           23.537 | [8, 6, 235]         |
| Sha256DeclareBuf       |             0.012 |           31.005 |     150.590 |            1.061 | [7, 219]            |
| Sha256Final            |             0.000 |          501.583 |     511.842 |            0.418 | [8, 237, 14]        |
| Sha256Reset            |             0.000 |           29.546 |      36.277 |            0.387 | [8, 238]            |
| Sha256UpdateBuf        |            -0.000 |          202.819 |     316.383 |            1.077 | [233, 2, 8, 6, 235] |

![enter image description here](https://lh3.googleusercontent.com/1VsZkBgZNNWg6G6RoTrFbiZKzADiKz3aDkYt0DyZgI_tR7ZSS68WnsPeeCNgEzkLOLW-npZSiPk)
![enter image description here](https://lh3.googleusercontent.com/ZomW7d8kat5ZL7XxBPfboXujHe0yGAkuyVJnOs5RRsHW5ef08aEmNY6YkFIhU4xTqYAhd9rBLlA)

