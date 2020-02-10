# StringBuilder

StringBuilder is a heap-based dynamic buffer with many common string operations built in.
Despite the name, it is trivial to use StringBuilder for unsigned strings, and most of
the API is built to handle buffers safely without null-termination.

## Constraints

The class is built on a linked list that stores chunks of the full string represented by the class.
Whenever an operation is called that needs the string to be a single continuous buffer, the class
allocates double its string size from the heap and re-writes the data into the single buffer.

The collapsed string takes slightly less memory than the unified string, but the process of
"defragmenting" the string takes time and memory. It also changes the pointer used to hold the
unified string. Therefore, pointers returned by `string()` should only be considered valid as
long as the StringBuilder object remains unchanged.

StringBuilder's destructor will free any memory allocated by the class, which includes references
previously returned by `string()`.


## Usage example
