

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

Since the measure of interest is the mean CPU time, we compute the standard error in the mean: &sigma;/*n*, where &sigma; denotes the standard deviation and *n* the number of repetitions. By this methodology, we can generate tables like the following, showing the mean CPU times for opcodes that do not depend on a variable or primitive.

### <a name="table1">Table 1</a>: CPU times for basic type-independent benchmarks
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

In the previous section, we found that some of the opcodes always appear in pairs. For example, arithmetic operations are almost always preceded by a PushVariable operation and isolated push operations are almost always followed by a Discard operation. As we will see later on, this also arises in more complex operations. Since it would be ideal to measure the CPU times for each opcode in isolation, we show one way to estimate these times here.

Let *A* be a matrix where each row corresponds to a benchmark and each element *A<sub>ij</sub>=k* if benchmark *i* uses opcode a total of *k* times. Let ***b*** be a vector where each element *b<sub>i</sub>* is the mean measured CPU time corresponding to benchmark *i*. Then the solution to ***x***<sup>\*</sup> = arg min(||*A****x*** - ***b***||<sup>2</sup>) provides an estimate of the individual opcode times, where each *x<sub>i</sub>* represents the CPU time required by opcode *i*.

For the benchmarks listed in [Table 1](#table1), we seek values in the vector ***x*** that make the following expression as close as possible to an equality.

<p align="center"><img src="https://lh3.googleusercontent.com/XEBuaslfkpGcO3el45UIoN88fF7uzpecNvc6UVzOlXrMYbE7EZAYfi_TVz4Vu1Q0_p_LCcQWJCI" alt="drawing" height="300" align="center"/></p>

In addition to minimizing ||*A****x*** - ***b***||<sup>2</sup>, we also need to constrain the individual opcode times to be positive (*x<sub>i</sub> > 0* for each *i*). Finally, since CPU times vary more for some benchmarks than others, we normalize the expression by the standard deviation and rewrite the expression as follows:

<p align="center"><img src="https://lh3.googleusercontent.com/kVZIT9dzirl7nqZAIkcgaZ5tPsSF8iOh8TnmR07EC2FvJPOxmQNHxS7ERGOOWH8AqrSzERzcHTA"/></p>

The solution of the above example is: 

<p align="center"><img src="https://lh3.googleusercontent.com/OQNoOtRSJ6kHStPb-xs-rvFCtAYVsXYalcQlRqEjEcFActzWZ5TbymZMjO-YZraDBEr46yQwZdU" alt="drawing" height="300" class="center"/></p>

where the values of **x** are given in nanoseconds and the corresponding opcodes are defined as {1: VariableDeclare, 4: PushFalse, 5: PushTrue, 7: PushConstant, 14: Discard, 15: Destruct, 18: Jump, 19: JumpIfFalse,  21: Return, 23: ForRangeInit, 24: ForRangeIterate, 25: ForRangeTerminate, 26: InvokeUserDefinedFreeFunction, 31: JumpIfFalseOrPop, 32: JumpIfTrueOrPop, 33: Not}.

We can then use these estimates to decouple the opcode times listed in [Table 1](#table1), which can then be subtracted wherever they appear in other benchmarks. Discrepancies between the Mean column and total estimated time reflect the fact that the least-squares fit is necessarily an approximation.

### <a name="table2">Table 2</a>: Basic benchmarks decomposed into estimated individual opcode times
| Benchmark (100 reps)   |   Mean (ns) |   Std. error (ns) | Opcodes**: estimated times (ns)                                                   |
|------------------------|-------------|-------------------|---------------------------------------------------------------------------------|
| Return                 |       28.27 |              0.16 | {**21**: 27.72}                                                                     |
| PushFalse              |       35.94 |              0.12 | {**4**: 5.45, **14**: 2.77, **21**: 27.72}                                                  |
| PushTrue               |       36.04 |              0.12 | {**5**: 5.55, **14**: 2.77, **21**: 27.72}                                                  |
| JumpIfFalse            |       36.36 |              0.13 | {**4**: 5.45, **19**: 2.69, **21**: 27.72}                                                  |
| Jump                   |       35.82 |              0.07 | {**4**: 5.45, **19**: 2.69, **18**: 0.0, **21**: 27.72}                                         |
| Not                    |       39.19 |              0.04 | {**5**: 5.55, **33**: 3.16, **14**: 2.77, **21**: 27.72}                                        |
| And                    |       45.05 |              0.08 | {**5**: 5.55, **31**: 3.46, **14**: 2.77, **21**: 27.72}                                        |
| Or                     |       45.01 |              0.07 | {**4**: 5.45, **32**: 3.53, **5**: 5.55, **14**: 2.77, **21**: 27.72}                               |
| ForLoop                |       66.17 |              0.13 | {**7**: 11.07, **23**: 5.52, **24**: 5.52, **18**: 0.0, **25**: 5.52, **21**: 27.72}                    |
| DestructBase           |       74.76 |              0.14 | {**1**: 7.75, **7**: 11.07, **23**: 5.52, **24**: 5.52, **18**: 0.0, **25**: 5.52, **21**: 27.72}           |
| Destruct               |       77.92 |              0.13 | {**7**: 11.07, **23**: 5.52, **24**: 5.52, **1**: 7.75, **15**: 3.76, **18**: 0.0, **25**: 5.52, **21**: 27.72} |
| Function               |       40.49 |              0.12 | {**26**: 12.77, **21**: 27.72}                                                          |
| VariableDeclareStr     |       35.31 |              0.13 | {**1**: 7.75, **21**: 27.72} 

See [Appendix](#app) for the list of all parameter-independent opcodes.

## Primitive opcodes

We tested the primitive opcodes for each of the different primitives to determine if there is any significant difference in CPU time. [Table 3](#table3) shows that the different integer types result only in very small variations in CPU time.

### <a name="table3">Table 3</a>: Mean CPU times for integer primitive opcodes (+/- std error)
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

In [Table 4](#table4), we see that certain operations are more expensive for fixed-point than floating point primitives, but there are only small differences between 32 and 64 bits within each primitive class.

### <a name="table4">Table 4</a>: Mean CPU times for floating and fixed point primitive opcodes (+/- std error)
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

Similarly, we tested the math function opcodes for the various primitives. [Table 5](#table5) shows that most math function opcodes take significantly longer for fixed point than floating point primitives.

### <a name="table5">Table 5</a>: Math function opcodes for fixed and floating point primitives
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

### <a name="table6">Table 6</a>: Least-squares linear fit parameters for string-based opcodes
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

<p align="center"><img src="https://lh3.googleusercontent.com/x_2cbgzSlwBvWMeHQitI_21loITfcWKTzq483LRI75a8dTbaFHImrY7nODcruph3Ydj92NLs4y8" width="600"/></p>
<p align="center"><img src="https://lh3.googleusercontent.com/uVMF38lLDQ556126BMMsxfONlATjW_0T1B049w0NrLUoPQ4gSzgz8E0attdGjGJxqQijD9UXetg" width="600"/></p>
<p align="center"><img src="https://lh3.googleusercontent.com/MIckMhq_X9IiNgrk3sWsJ-_kf0-Atz5A4jDMqpHaiq2AWal3w9i0VpdI0dy_PleNPCAk7idXzbM" width="600"/></p>
<p align="center"><img src="https://lh3.googleusercontent.com/yPCTvd5Osq2mEFfbhwcu9DCtnDRZjgVFb2ilPtEmFQeYIUguJypx5Vfp0qRnaF3bPwkaAhabIkI" width="600"/></p>
<p align="center"><img src="https://lh3.googleusercontent.com/JMGez-WoeUzB8-7TfSIaL7IqHc65DlyWusBKb7tbB3wDM_AwzwZ4jSEbQMpjEZE6TG3CAj12WKM" width="600"/></p>
<p align="center"><img src="https://lh3.googleusercontent.com/dmPd8p11ANh3q3nUV7-Dy9Bk00PP3C4XdHb6HDn6SVx2YIPbsdguNfZVny_R2QK6IuRXu554RUU" width="600"/></p>
<p align="center"><img src="https://lh3.googleusercontent.com/XtZHuq-gM385i0I2Sk56DgA8CQKujQR7dF_4wHOCxR-udr3rsk8QaquDFp1os7ux-0LOynTl1Fc" width="600"/></p>
<p align="center"><img src="https://lh3.googleusercontent.com/ULrd2GvoXDK7Zpa5-Gm1B-L99KMJTApA5sWxpg0wQ2kMhxGqSdtnROvzH4FawsGi9_-JI_USyBs" width="600"/></p>
<p align="center"><img src="https://lh3.googleusercontent.com/tjAvEXAplIo5tjCpUs9-7rJ8gGB_5wmhsdHuhMxtn1IoDI7gWhQcn13z6OYXWOhvitcT3YAPhMg" width="600"/></p>

## Array-based opcodes

We use a similar approach to profile array operations. As shown below, there is often a change in slope or behavior once the array length exceeds about 6000. For this reason, we compute the least-squares fits based only on arrays smaller than 6000. The change in behavior is likely a result of machine-dependent memory management, and memory storage will probably be included separately in transaction pricing.

### <a name="table7">Table 7</a>: Least-squares linear fit parameters for array-based opcodes
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

<p align="center"><img src="https://lh3.googleusercontent.com/UfiJ1SiH6oGEjyFpY40euuiSFIeKPikstWsH_DrBCv3wMj58-FSqUJN2wz_7fFNuf6WFbm311Tc" width="600"/></p>
<p align="center"><img src="https://lh3.googleusercontent.com/TO0q5ZNpeU5gbttuaDqskclm2vypVyPRq5ZhZJNlP0nalfIg1ZbUYh22RvFQ3B9Y32XOEKlovuA" width="600"/></p>
<p align="center"><img src="https://lh3.googleusercontent.com/iSK3mlbFRhqq7CkgdoUlvkglT1EH_HPEmc06l_LEQTWuNsGxayA30iFUeET0ptr15mV5uf3XMYE" width="600"/></p>
<p align="center"><img src="https://lh3.googleusercontent.com/wXoM1NIO199bHXSVHhqrS67SBZB7r_pOrtBP917xgTZK5fA96pa7mAQRO4Qx6Wzn48Te_AbwLk8" width="600"/></p>
<p align="center"><img src="https://lh3.googleusercontent.com/_4Mb9lC19WUurTI8bHrNBokSAbvzbs9YYrcpn36IIwVufWZ9y8V18G7sQqhdzKfg2PynZuoW5sM" width="600"/></p>
<p align="center"><img src="https://lh3.googleusercontent.com/AHZQ2UblQZxx75ag3Ptg0h7_IlX6_6PjO1wAWTh7KvoVednW_HW_vlPd74OBS2gSZhgGR1La74Y" width="600"/></p>

## Tensor-based opcodes

We profile tensor-based operations using a similar approach. To ensure that they do not depend on the dimension of the tensor, we tested the following dimensions independently: 2, 3 and 4. 

### <a name="table8">Table 8</a>: Least-squares linear fit parameters for tensor-based opcodes
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

<p align="center"><img src="https://lh3.googleusercontent.com/oJNMuMOKUsBiVlA8F7jreAgeq-yQ7lVb542ky3JoN7K6HOITzCD1FKWh_IklyRDvzEx9_fyFDv8" width="600"/></p>
<p align="center"><img src="https://lh3.googleusercontent.com/wU7uOueyDmyWTef0S0oBmrLqlw0IL_VTpciNQpPMv_LmzKPcd4scQG3AkB3uOIU458_Kcns4Ibw" width="600"/></p>
<p align="center"><img src="https://lh3.googleusercontent.com/Csu8HlTqcu9cYvpTaKQMKnHFKy26ICaZK8uTIxM9oof5pznGKvafkiyyW0dtxAnC05-PyKRPtSE" width="600"/></p>
<p align="center"><img src="https://lh3.googleusercontent.com/awDzFVNDZbiNk2avZTqJHfW_8wNZdsZE3eewJ4mWm3Fw83IG234LeDFqswvthmxiJLodtxgUjTY" width="600"/></p>

## Crypto-based opcodes

### <a name="table9">Table 9</a>: Least-squares linear fit parameters for crypto-based opcodes
| Benchmark (100 reps)   |   Slope (ns/char) |   Intercept (ns) |   Mean (ns) |   Std error (ns) | Net opcodes         |
|------------------------|-------------------|------------------|-------------|------------------|---------------------|
| Sha256UpdateStr        |             4.576 |           63.464 |   37633.507 |           23.537 | [8, 6, 235]         |
| Sha256DeclareBuf       |             0.012 |           31.005 |     150.590 |            1.061 | [7, 219]            |
| Sha256Final            |             0.000 |          501.583 |     511.842 |            0.418 | [8, 237, 14]        |
| Sha256Reset            |             0.000 |           29.546 |      36.277 |            0.387 | [8, 238]            |
| Sha256UpdateBuf        |            -0.000 |          202.819 |     316.383 |            1.077 | [233, 2, 8, 6, 235] |

<p align="center"><img src="https://lh3.googleusercontent.com/1VsZkBgZNNWg6G6RoTrFbiZKzADiKz3aDkYt0DyZgI_tR7ZSS68WnsPeeCNgEzkLOLW-npZSiPk" width="600"/></p>
<p align="center"><img src="https://lh3.googleusercontent.com/ZomW7d8kat5ZL7XxBPfboXujHe0yGAkuyVJnOs5RRsHW5ef08aEmNY6YkFIhU4xTqYAhd9rBLlA" width="600"/></p>

## <a name="app">Appendix</a>:  Parameter-independent opcode tables

|   Opcode (Base) | Name                          |   Estimated time (ns) |
|-----------------|-------------------------------|-----------------------|
|               1 | VariableDeclare               |                  7.75 |
|               4 | PushFalse                     |                  5.45 |
|               5 | PushTrue                      |                  5.55 |
|               7 | PushConstant                  |                 11.07 |
|              14 | Discard                       |                  2.77 |
|              15 | Destruct                      |                  3.76 |
|              18 | Jump                          |                  0.00 |
|              19 | JumpIfFalse                   |                  2.69 |
|              21 | Return                        |                 27.72 |
|              23 | ForRangeInit                  |                  5.52 |
|              24 | ForRangeIterate               |                  5.52 |
|              25 | ForRangeTerminate             |                  5.52 |
|              26 | InvokeUserDefinedFreeFunction |                 12.77 |
|              31 | JumpIfFalseOrPop              |                  3.46 |
|              32 | JumpIfTrueOrPop               |                  3.53 |
|              33 | Not                           |                  3.16 |

|   Opcode (Fixed32) | Name                             |   Estimated time (ns) |
|--------------------|----------------------------------|-----------------------|
|                  2 | VariableDeclareAssign            |                  0.00 |
|                  8 | PushVariable                     |                  4.96 |
|                  9 | PopToVariable                    |                  4.63 |
|                 22 | ReturnValue                      |                 22.62 |
|                 34 | PrimitiveEqual                   |                  9.40 |
|                 36 | PrimitiveNotEqual                |                  8.76 |
|                 38 | PrimitiveLessThan                |                 11.96 |
|                 40 | PrimitiveLessThanOrEqual         |                 11.03 |
|                 42 | PrimitiveGreaterThan             |                 13.16 |
|                 44 | PrimitiveGreaterThanOrEqual      |                 12.66 |
|                 46 | PrimitiveNegate                  |                  5.66 |
|                 48 | PrimitiveAdd                     |                  7.53 |
|                 52 | VariablePrimitiveInplaceAdd      |                  6.39 |
|                 55 | PrimitiveSubtract                |                  8.43 |
|                 59 | VariablePrimitiveInplaceSubtract |                  6.62 |
|                 62 | PrimitiveMultiply                |                 11.38 |
|                 66 | VariablePrimitiveInplaceMultiply |                  9.24 |
|                 69 | PrimitiveDivide                  |                 24.56 |
|                 73 | VariablePrimitiveInplaceDivide   |                 22.64 |
|                296 | ::abs^Fixed32^Fixed32            |                 12.06 |
|                301 | ::exp^Fixed32^Fixed32            |                109.14 |
|                321 | ::pow^Fixed32,Fixed32^Fixed32    |                311.74 |
|                334 | ::sqrt^Fixed32^Fixed32           |                117.19 |
|                363 | ::sin^Fixed32^Fixed32            |                134.60 |
|                364 | ::cos^Fixed32^Fixed32            |                139.75 |
|                365 | ::tan^Fixed32^Fixed32            |                110.24 |
|                366 | ::asin^Fixed32^Fixed32           |                148.40 |
|                367 | ::acos^Fixed32^Fixed32           |                162.24 |
|                368 | ::atan^Fixed32^Fixed32           |                 88.02 |
|                370 | ::sinh^Fixed32^Fixed32           |                236.63 |
|                371 | ::cosh^Fixed32^Fixed32           |                235.59 |
|                372 | ::tanh^Fixed32^Fixed32           |                256.53 |
|                373 | ::asinh^Fixed32^Fixed32          |                276.98 |
|                374 | ::acosh^Fixed32^Fixed32          |                275.08 |
|                375 | ::atanh^Fixed32^Fixed32          |                167.13 |

|   Opcode (Fixed64) | Name                             |   Estimated time (ns) |
|--------------------|----------------------------------|-----------------------|
|                  2 | VariableDeclareAssign            |                  0.00 |
|                  8 | PushVariable                     |                  5.46 |
|                  9 | PopToVariable                    |                  4.17 |
|                 22 | ReturnValue                      |                 22.65 |
|                 34 | PrimitiveEqual                   |                  7.92 |
|                 36 | PrimitiveNotEqual                |                  8.00 |
|                 38 | PrimitiveLessThan                |                 10.60 |
|                 40 | PrimitiveLessThanOrEqual         |                 11.41 |
|                 42 | PrimitiveGreaterThan             |                 12.03 |
|                 44 | PrimitiveGreaterThanOrEqual      |                 11.13 |
|                 46 | PrimitiveNegate                  |                  5.20 |
|                 48 | PrimitiveAdd                     |                  6.68 |
|                 52 | VariablePrimitiveInplaceAdd      |                  6.83 |
|                 55 | PrimitiveSubtract                |                  6.30 |
|                 59 | VariablePrimitiveInplaceSubtract |                  6.60 |
|                 62 | PrimitiveMultiply                |                  9.49 |
|                 66 | VariablePrimitiveInplaceMultiply |                  9.53 |
|                 69 | PrimitiveDivide                  |                 26.62 |
|                 73 | VariablePrimitiveInplaceDivide   |                 25.64 |
|                297 | ::abs^Fixed64^Fixed64            |                 13.77 |
|                302 | ::exp^Fixed64^Fixed64            |                140.83 |
|                322 | ::pow^Fixed64,Fixed64^Fixed64    |                460.48 |
|                335 | ::sqrt^Fixed64^Fixed64           |                186.90 |
|                376 | ::sin^Fixed64^Fixed64            |                183.61 |
|                377 | ::cos^Fixed64^Fixed64            |                176.09 |
|                378 | ::tan^Fixed64^Fixed64            |                132.83 |
|                379 | ::asin^Fixed64^Fixed64           |                190.70 |
|                380 | ::acos^Fixed64^Fixed64           |                200.17 |
|                381 | ::atan^Fixed64^Fixed64           |                102.77 |
|                383 | ::sinh^Fixed64^Fixed64           |                332.75 |
|                384 | ::cosh^Fixed64^Fixed64           |                330.50 |
|                385 | ::tanh^Fixed64^Fixed64           |                366.14 |
|                386 | ::asinh^Fixed64^Fixed64          |                388.16 |
|                387 | ::acosh^Fixed64^Fixed64          |                400.43 |
|                388 | ::atanh^Fixed64^Fixed64          |                269.30 |

|   Opcode (Fixed128) | Name                          |   Estimated time (ns) |
|---------------------|-------------------------------|-----------------------|
|                   2 | VariableDeclareAssign         |                  8.01 |
|                   8 | PushVariable                  |                  9.77 |
|                   9 | PopToVariable                 |                  3.89 |
|                  22 | ReturnValue                   |                 28.68 |
|                  35 | ObjectEqual                   |                  5.65 |
|                  37 | ObjectNotEqual                |                  7.67 |
|                  39 | ObjectLessThan                |                  8.48 |
|                  41 | ObjectLessThanOrEqual         |                 10.55 |
|                  43 | ObjectGreaterThan             |                  8.03 |
|                  45 | ObjectGreaterThanOrEqual      |                 10.31 |
|                  47 | ObjectNegate                  |                 20.66 |
|                  49 | ObjectAdd                     |                 42.86 |
|                  53 | VariableObjectInplaceAdd      |                 29.96 |
|                  56 | ObjectSubtract                |                 46.18 |
|                  60 | VariableObjectInplaceSubtract |                 33.03 |
|                  63 | ObjectMultiply                |                 81.01 |
|                  67 | VariableObjectInplaceMultiply |                 65.70 |
|                  70 | ObjectDivide                  |               1202.24 |
|                  74 | VariableObjectInplaceDivide   |               1183.90 |
|                  81 | PushLargeConstant             |                 22.21 |
|                 298 | ::abs^Fixed128^Fixed128       |                 93.70 |
|                 303 | ::exp^Fixed128^Fixed128       |               3984.08 |
|                 336 | ::sqrt^Fixed128^Fixed128      |               4533.79 |
|                 389 | ::sin^Fixed128^Fixed128       |               5727.26 |
|                 390 | ::cos^Fixed128^Fixed128       |               5498.05 |
|                 391 | ::tan^Fixed128^Fixed128       |               4169.56 |
|                 392 | ::asin^Fixed128^Fixed128      |               3272.88 |
|                 393 | ::acos^Fixed128^Fixed128      |               3368.53 |
|                 394 | ::atan^Fixed128^Fixed128      |               2485.70 |
|                 396 | ::sinh^Fixed128^Fixed128      |               9772.40 |
|                 397 | ::cosh^Fixed128^Fixed128      |               9756.85 |
|                 398 | ::tanh^Fixed128^Fixed128      |              11470.62 |
|                 399 | ::asinh^Fixed128^Fixed128     |              10920.12 |
|                 400 | ::acosh^Fixed128^Fixed128     |              10972.92 |
|                 401 | ::atanh^Fixed128^Fixed128     |               6950.12 |


|   Opcode (Float32) | Name                             |   Estimated time (ns) |
|--------------------|----------------------------------|-----------------------|
|                  2 | VariableDeclareAssign            |                  0.00 |
|                  8 | PushVariable                     |                  5.21 |
|                  9 | PopToVariable                    |                  4.51 |
|                 22 | ReturnValue                      |                 22.69 |
|                 34 | PrimitiveEqual                   |                  9.40 |
|                 36 | PrimitiveNotEqual                |                 10.45 |
|                 38 | PrimitiveLessThan                |                  9.20 |
|                 40 | PrimitiveLessThanOrEqual         |                 10.20 |
|                 42 | PrimitiveGreaterThan             |                  9.91 |
|                 44 | PrimitiveGreaterThanOrEqual      |                 11.42 |
|                 46 | PrimitiveNegate                  |                  4.85 |
|                 48 | PrimitiveAdd                     |                  6.86 |
|                 52 | VariablePrimitiveInplaceAdd      |                  3.44 |
|                 55 | PrimitiveSubtract                |                  6.58 |
|                 59 | VariablePrimitiveInplaceSubtract |                  3.63 |
|                 62 | PrimitiveMultiply                |                 10.63 |
|                 66 | VariablePrimitiveInplaceMultiply |                  6.15 |
|                 69 | PrimitiveDivide                  |                  8.05 |
|                 73 | VariablePrimitiveInplaceDivide   |                  6.98 |
|                294 | ::abs^Float32^Float32            |                 10.90 |
|                299 | ::exp^Float32^Float32            |                 22.46 |
|                319 | ::pow^Float32,Float32^Float32    |                 19.11 |
|                332 | ::sqrt^Float32^Float32           |                 13.00 |
|                337 | ::sin^Float32^Float32            |                 12.25 |
|                338 | ::cos^Float32^Float32            |                 14.19 |
|                339 | ::tan^Float32^Float32            |                 22.65 |
|                340 | ::asin^Float32^Float32           |                 19.68 |
|                341 | ::acos^Float32^Float32           |                 22.41 |
|                342 | ::atan^Float32^Float32           |                 24.93 |
|                344 | ::sinh^Float32^Float32           |                 36.33 |
|                345 | ::cosh^Float32^Float32           |                 18.53 |
|                346 | ::tanh^Float32^Float32           |                 35.54 |
|                347 | ::asinh^Float32^Float32          |                 38.62 |
|                348 | ::acosh^Float32^Float32          |                 34.77 |
|                349 | ::atanh^Float32^Float32          |                 34.07 |

|   Opcode (Float64) | Name                             |   Estimated time (ns) |
|--------------------|----------------------------------|-----------------------|
|                  2 | VariableDeclareAssign            |                  0.00 |
|                  8 | PushVariable                     |                  5.09 |
|                  9 | PopToVariable                    |                  4.31 |
|                 22 | ReturnValue                      |                 22.44 |
|                 34 | PrimitiveEqual                   |                  9.91 |
|                 36 | PrimitiveNotEqual                |                 11.25 |
|                 38 | PrimitiveLessThan                |                  9.58 |
|                 40 | PrimitiveLessThanOrEqual         |                 10.00 |
|                 42 | PrimitiveGreaterThan             |                  9.93 |
|                 44 | PrimitiveGreaterThanOrEqual      |                 11.15 |
|                 46 | PrimitiveNegate                  |                  4.73 |
|                 48 | PrimitiveAdd                     |                  6.93 |
|                 52 | VariablePrimitiveInplaceAdd      |                  3.59 |
|                 55 | PrimitiveSubtract                |                  6.80 |
|                 59 | VariablePrimitiveInplaceSubtract |                  3.60 |
|                 62 | PrimitiveMultiply                |                 11.49 |
|                 66 | VariablePrimitiveInplaceMultiply |                  7.22 |
|                 69 | PrimitiveDivide                  |                  8.38 |
|                 73 | VariablePrimitiveInplaceDivide   |                  6.85 |
|                295 | ::abs^Float64^Float64            |                 11.51 |
|                300 | ::exp^Float64^Float64            |                 25.63 |
|                320 | ::pow^Float64,Float64^Float64    |                 55.44 |
|                333 | ::sqrt^Float64^Float64           |                 10.60 |
|                350 | ::sin^Float64^Float64            |                 23.92 |
|                351 | ::cos^Float64^Float64            |                 21.89 |
|                352 | ::tan^Float64^Float64            |                 22.76 |
|                353 | ::asin^Float64^Float64           |                 23.90 |
|                354 | ::acos^Float64^Float64           |                 25.02 |
|                355 | ::atan^Float64^Float64           |                 20.86 |
|                357 | ::sinh^Float64^Float64           |                 37.54 |
|                358 | ::cosh^Float64^Float64           |                 27.71 |
|                359 | ::tanh^Float64^Float64           |                 36.03 |
|                360 | ::asinh^Float64^Float64          |                 39.16 |
|                361 | ::acosh^Float64^Float64          |                 36.36 |
|                362 | ::atanh^Float64^Float64          |                 34.53 |

|   Opcode (Int16) | Name                             |   Estimated time (ns) |
|------------------|----------------------------------|-----------------------|
|                2 | VariableDeclareAssign            |                  0.00 |
|                8 | PushVariable                     |                  5.17 |
|                9 | PopToVariable                    |                  4.50 |
|               22 | ReturnValue                      |                 22.82 |
|               27 | VariablePrefixInc                |                  6.77 |
|               28 | VariablePrefixDec                |                  7.31 |
|               29 | VariablePostfixInc               |                  7.26 |
|               30 | VariablePostfixDec               |                  6.98 |
|               34 | PrimitiveEqual                   |                 10.64 |
|               36 | PrimitiveNotEqual                |                 10.80 |
|               38 | PrimitiveLessThan                |                  9.39 |
|               40 | PrimitiveLessThanOrEqual         |                 10.80 |
|               42 | PrimitiveGreaterThan             |                 10.75 |
|               44 | PrimitiveGreaterThanOrEqual      |                 10.76 |
|               46 | PrimitiveNegate                  |                  4.97 |
|               48 | PrimitiveAdd                     |                  6.86 |
|               52 | VariablePrimitiveInplaceAdd      |                  3.97 |
|               55 | PrimitiveSubtract                |                  6.51 |
|               59 | VariablePrimitiveInplaceSubtract |                  3.71 |
|               62 | PrimitiveMultiply                |                 10.26 |
|               66 | VariablePrimitiveInplaceMultiply |                  6.14 |
|               69 | PrimitiveDivide                  |                 11.30 |
|               73 | VariablePrimitiveInplaceDivide   |                  8.87 |

|   Opcode (Int32) | Name                             |   Estimated time (ns) |
|------------------|----------------------------------|-----------------------|
|                2 | VariableDeclareAssign            |                  0.00 |
|                8 | PushVariable                     |                  5.45 |
|                9 | PopToVariable                    |                  4.34 |
|               22 | ReturnValue                      |                 22.44 |
|               27 | VariablePrefixInc                |                  6.96 |
|               28 | VariablePrefixDec                |                  6.71 |
|               29 | VariablePostfixInc               |                  5.98 |
|               30 | VariablePostfixDec               |                  6.04 |
|               34 | PrimitiveEqual                   |                 10.13 |
|               36 | PrimitiveNotEqual                |                 10.21 |
|               38 | PrimitiveLessThan                |                  8.45 |
|               40 | PrimitiveLessThanOrEqual         |                  9.26 |
|               42 | PrimitiveGreaterThan             |                 10.61 |
|               44 | PrimitiveGreaterThanOrEqual      |                 10.03 |
|               46 | PrimitiveNegate                  |                  4.52 |
|               48 | PrimitiveAdd                     |                  6.02 |
|               52 | VariablePrimitiveInplaceAdd      |                  3.95 |
|               55 | PrimitiveSubtract                |                  5.93 |
|               59 | VariablePrimitiveInplaceSubtract |                  3.41 |
|               62 | PrimitiveMultiply                |                 10.19 |
|               66 | VariablePrimitiveInplaceMultiply |                  6.23 |
|               69 | PrimitiveDivide                  |                 10.22 |
|               73 | VariablePrimitiveInplaceDivide   |                  8.71 |

|   Opcode (Int64) | Name                             |   Estimated time (ns) |
|------------------|----------------------------------|-----------------------|
|                2 | VariableDeclareAssign            |                  0.00 |
|                8 | PushVariable                     |                  5.08 |
|                9 | PopToVariable                    |                  4.70 |
|               22 | ReturnValue                      |                 22.51 |
|               27 | VariablePrefixInc                |                  6.10 |
|               28 | VariablePrefixDec                |                  6.76 |
|               29 | VariablePostfixInc               |                  5.96 |
|               30 | VariablePostfixDec               |                  6.00 |
|               34 | PrimitiveEqual                   |                 10.56 |
|               36 | PrimitiveNotEqual                |                 11.14 |
|               38 | PrimitiveLessThan                |                  9.13 |
|               40 | PrimitiveLessThanOrEqual         |                  9.96 |
|               42 | PrimitiveGreaterThan             |                 10.63 |
|               44 | PrimitiveGreaterThanOrEqual      |                 10.95 |
|               46 | PrimitiveNegate                  |                  4.93 |
|               48 | PrimitiveAdd                     |                  6.69 |
|               52 | VariablePrimitiveInplaceAdd      |                  4.40 |
|               55 | PrimitiveSubtract                |                  6.53 |
|               59 | VariablePrimitiveInplaceSubtract |                  3.87 |
|               62 | PrimitiveMultiply                |                 10.31 |
|               66 | VariablePrimitiveInplaceMultiply |                  6.31 |
|               69 | PrimitiveDivide                  |                 12.59 |
|               73 | VariablePrimitiveInplaceDivide   |                 12.00 |

|   Opcode (Int8) | Name                             |   Estimated time (ns) |
|-----------------|----------------------------------|-----------------------|
|               2 | VariableDeclareAssign            |                  0.00 |
|               8 | PushVariable                     |                  5.16 |
|               9 | PopToVariable                    |                  4.50 |
|              22 | ReturnValue                      |                 22.65 |
|              27 | VariablePrefixInc                |                  6.44 |
|              28 | VariablePrefixDec                |                  6.79 |
|              29 | VariablePostfixInc               |                  6.21 |
|              30 | VariablePostfixDec               |                  6.27 |
|              34 | PrimitiveEqual                   |                  8.75 |
|              36 | PrimitiveNotEqual                |                  9.99 |
|              38 | PrimitiveLessThan                |                  8.60 |
|              40 | PrimitiveLessThanOrEqual         |                  8.92 |
|              42 | PrimitiveGreaterThan             |                  9.71 |
|              44 | PrimitiveGreaterThanOrEqual      |                 10.42 |
|              46 | PrimitiveNegate                  |                  5.19 |
|              48 | PrimitiveAdd                     |                  6.79 |
|              52 | VariablePrimitiveInplaceAdd      |                  4.36 |
|              55 | PrimitiveSubtract                |                  6.77 |
|              59 | VariablePrimitiveInplaceSubtract |                  4.03 |
|              62 | PrimitiveMultiply                |                 10.13 |
|              66 | VariablePrimitiveInplaceMultiply |                  6.45 |
|              69 | PrimitiveDivide                  |                 10.24 |
|              73 | VariablePrimitiveInplaceDivide   |                  8.94 |

|   Opcode (UInt16) | Name                             |   Estimated time (ns) |
|-------------------|----------------------------------|-----------------------|
|                 2 | VariableDeclareAssign            |                  0.00 |
|                 8 | PushVariable                     |                  5.23 |
|                 9 | PopToVariable                    |                  4.40 |
|                22 | ReturnValue                      |                 22.86 |
|                27 | VariablePrefixInc                |                  6.81 |
|                28 | VariablePrefixDec                |                  7.35 |
|                29 | VariablePostfixInc               |                  7.28 |
|                30 | VariablePostfixDec               |                  7.87 |
|                34 | PrimitiveEqual                   |                 10.48 |
|                36 | PrimitiveNotEqual                |                 10.69 |
|                38 | PrimitiveLessThan                |                  9.83 |
|                40 | PrimitiveLessThanOrEqual         |                 10.87 |
|                42 | PrimitiveGreaterThan             |                 11.50 |
|                44 | PrimitiveGreaterThanOrEqual      |                 11.04 |
|                46 | PrimitiveNegate                  |                  5.19 |
|                48 | PrimitiveAdd                     |                  7.21 |
|                52 | VariablePrimitiveInplaceAdd      |                  4.08 |
|                55 | PrimitiveSubtract                |                  6.37 |
|                59 | VariablePrimitiveInplaceSubtract |                  3.54 |
|                62 | PrimitiveMultiply                |                  9.91 |
|                66 | VariablePrimitiveInplaceMultiply |                  5.96 |
|                69 | PrimitiveDivide                  |                 11.16 |
|                73 | VariablePrimitiveInplaceDivide   |                  9.01 |

|   Opcode (UInt32) | Name                             |   Estimated time (ns) |
|-------------------|----------------------------------|-----------------------|
|                 2 | VariableDeclareAssign            |                  0.00 |
|                 8 | PushVariable                     |                  5.41 |
|                 9 | PopToVariable                    |                  4.43 |
|                22 | ReturnValue                      |                 22.72 |
|                27 | VariablePrefixInc                |                  6.98 |
|                28 | VariablePrefixDec                |                  6.64 |
|                29 | VariablePostfixInc               |                  6.10 |
|                30 | VariablePostfixDec               |                  6.17 |
|                34 | PrimitiveEqual                   |                 10.11 |
|                36 | PrimitiveNotEqual                |                 10.83 |
|                38 | PrimitiveLessThan                |                  8.70 |
|                40 | PrimitiveLessThanOrEqual         |                 10.29 |
|                42 | PrimitiveGreaterThan             |                 10.88 |
|                44 | PrimitiveGreaterThanOrEqual      |                 10.21 |
|                46 | PrimitiveNegate                  |                  4.51 |
|                48 | PrimitiveAdd                     |                  6.14 |
|                52 | VariablePrimitiveInplaceAdd      |                  3.97 |
|                55 | PrimitiveSubtract                |                  5.83 |
|                59 | VariablePrimitiveInplaceSubtract |                  3.35 |
|                62 | PrimitiveMultiply                |                 10.37 |
|                66 | VariablePrimitiveInplaceMultiply |                  6.31 |
|                69 | PrimitiveDivide                  |                 10.84 |
|                73 | VariablePrimitiveInplaceDivide   |                  8.74 |

|   Opcode (UInt64) | Name                             |   Estimated time (ns) |
|-------------------|----------------------------------|-----------------------|
|                 2 | VariableDeclareAssign            |                  0.00 |
|                 8 | PushVariable                     |                  4.90 |
|                 9 | PopToVariable                    |                  5.07 |
|                22 | ReturnValue                      |                 22.48 |
|                27 | VariablePrefixInc                |                  5.99 |
|                28 | VariablePrefixDec                |                  6.70 |
|                29 | VariablePostfixInc               |                  5.87 |
|                30 | VariablePostfixDec               |                  5.89 |
|                34 | PrimitiveEqual                   |                 10.82 |
|                36 | PrimitiveNotEqual                |                 11.82 |
|                38 | PrimitiveLessThan                |                  9.41 |
|                40 | PrimitiveLessThanOrEqual         |                 11.52 |
|                42 | PrimitiveGreaterThan             |                 11.52 |
|                44 | PrimitiveGreaterThanOrEqual      |                 11.47 |
|                46 | PrimitiveNegate                  |                  4.65 |
|                48 | PrimitiveAdd                     |                  6.95 |
|                52 | VariablePrimitiveInplaceAdd      |                  4.55 |
|                55 | PrimitiveSubtract                |                  6.80 |
|                59 | VariablePrimitiveInplaceSubtract |                  3.94 |
|                62 | PrimitiveMultiply                |                 11.14 |
|                66 | VariablePrimitiveInplaceMultiply |                  6.34 |
|                69 | PrimitiveDivide                  |                 12.32 |
|                73 | VariablePrimitiveInplaceDivide   |                 11.37 |

|   Opcode (UInt8) | Name                             |   Estimated time (ns) |
|------------------|----------------------------------|-----------------------|
|                2 | VariableDeclareAssign            |                  0.00 |
|                8 | PushVariable                     |                  5.22 |
|                9 | PopToVariable                    |                  4.54 |
|               22 | ReturnValue                      |                 22.62 |
|               27 | VariablePrefixInc                |                  6.70 |
|               28 | VariablePrefixDec                |                  6.70 |
|               29 | VariablePostfixInc               |                  6.33 |
|               30 | VariablePostfixDec               |                  6.63 |
|               34 | PrimitiveEqual                   |                  8.63 |
|               36 | PrimitiveNotEqual                |                  9.95 |
|               38 | PrimitiveLessThan                |                  8.41 |
|               40 | PrimitiveLessThanOrEqual         |                  8.65 |
|               42 | PrimitiveGreaterThan             |                 10.11 |
|               44 | PrimitiveGreaterThanOrEqual      |                  9.69 |
|               46 | PrimitiveNegate                  |                  5.17 |
|               48 | PrimitiveAdd                     |                  6.61 |
|               52 | VariablePrimitiveInplaceAdd      |                  4.46 |
|               55 | PrimitiveSubtract                |                  6.54 |
|               59 | VariablePrimitiveInplaceSubtract |                  3.97 |
|               62 | PrimitiveMultiply                |                 10.03 |
|               66 | VariablePrimitiveInplaceMultiply |                  6.47 |
|               69 | PrimitiveDivide                  |                  9.69 |
|               73 | VariablePrimitiveInplaceDivide   |                  8.12 |
