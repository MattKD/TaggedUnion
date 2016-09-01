# TaggedUnion
Safe tagged union template class in C++14

This is only a basic implementation of a tagged union, and does not work in all cases. It doesn't allow move only types like unique_ptr, and always copies passed in value to ctor and reset. It is also left in an invalid state if a copy constructor throws. Performance is better than virtual functions, but worse than plain unions with switches in simple test cases.
