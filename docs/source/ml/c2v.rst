# Code2Vec

# Input files
The content of the input file(s) is assumed to have the following syntax:
```
fct_1 sourceword_1a,path_1a,targetword_1a sourceword_1b,path_1b,targetword_1b ... 
fct_2 sourceword_2a,path_2a,targetword_2a sourceword_2b,path_2b,targetword_2b ...
```
where `fct_i` is the function name of the function `i`, and the triple (sourceword_ij,path_ij,targetword_ij) is the jth context of function i.
The context consists of a source word (start node of a path in the AST), path is a unique id/string indicating a path, targetword is the terminating word of a path in a ast.


# Data loading
The dataloader class loads in the data and prepares it such that it can be fed into the graph.
During training, a fixed number of contexts for a function has to be read in (`MAX_CONTEXTS`). If less contexts are available, dummy contexts are created.
(The elements of the dummy context get the index 0 in the path and word corpus)

The method `GetNext()` retrieves a pair of
```
(fct_i, (sourceword_tensor_ij, path_tensor_ij, targetword_tensor_ij))
```
where `fct_i` is the index of the function i, and the tensors have shape (MAX_CONTEXTS) holding the indices of the words/paths.


# Graph
In the following, we consider only one function per training loop is read in.
 
We have the following trainable tensors
 - SE: Word embedding (shared embedding for both, the source and target words), Dimension (WORD_VOCABULARY, EMBEDDING_SIZE)
 - PE: Path embedding, Dimension (PATH_VOCABULARY, EMBEDDING_SIZE)
 - FE: Function name embedding, Dimension (FUNCTION_VOCABULARY, EMBEDDING_SIZE)
 - FC: Weights of a fully connected layer (EMBEDDING_SIZE, EMBEDDING_SIZE)
 - AV: Attention vector (MAX_CONTEXTS)


Legend:
i denotes the index of the samples (i.e. i runs from 0 to MAX_CONTEXTS)
j denotes the index of a context embedding (i.e. j runs from 0 to 3*EMBEDDING_SIZE)
k denotes the index of an embedding (i.e. j runs from 0 to EMBEDDING_SIZE)
m denotes the index of the function name vocab
\vec SE_{i} (\vec PE_{i}, \vec FE_{i}) gives the representation of the word/path/.. of sample i


1. Data feed into the model gives us \vec SE_{i} (\vec PE_{i}, \vec FE_{i})

2. Context Vector: Concatenation of those
\vec CV_{i} = (\vec SE_{i}, \vec PE_{i}, \vec SE_{i})

3. Combined Context Vector: Transpose CV, multiply with FC, Applying Tanh
CC_{k, i} = tanh * (FC_{k, j} CV_{j, i}

4. Context with attention: Transposition of CC, Multiplication with AV
CA_{i,1} = \sum_{} CC_{i,k} AV_{k,1}

5. Attenion weights: Transposition, Softmax normalisation
AW_{1,i} = SoftMax(CA{1,i})

6. Code Vector: Transposition, multplication with CC
CV_{k,1} = CC_{k,i} * AW_{i,1}

7. Unnormalised prediction: Multiplication with FE
UP_{m,1} = FE_{m, k} CV_{k, 1}

8. Normalised prediction: Transpose, Softmax
PR_{1,m} = Softmax{UP_{1,m}}

9. Cross entropy loss
L = CrossEntropy(PR_{1,m}, GT_{1,m})


# Extensions: 
 - In the TF implemention, the SE, PE, FE are initialised with a variance_scaling_initializer. AV and FC with a glorot_uniform_initializer.
 - Adding a Softmax function to the fetch lib, which can normalise along any axis. (Currently it works only for axis 0, that's the reason for the many transposition operations)
 - In the data loader, the whole "corpus" (i.e. the umaps etc.) are created based on a c2v file. In the TF implementation, this corpus is created once, and then persisted, such that it can be reused next time (e.g. for model evaluation)
 - Bringing the AST extraction into fetch
 - Extension to "real" batch training, s.t. contexts of more than one function can be digested simultaneously. (I.e. it would require, that several tensors get a new dimension, and all operations can handly the extra dim) 

