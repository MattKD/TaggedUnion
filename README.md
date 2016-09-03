# TaggedUnion
Safe tagged union template class in C++14

This is my attempt at a safe tagged union in C++14. It passes all the included tests checking that the correct values are stored and returned, destructors are called, exceptions thrown when attempting to read a wrong value, and that it works with move only types. It is left in an invalid state if copy/move ctor throws in the reset method. Performance is better than virtual functions, but worse than plain unions with switches in simple test cases.
