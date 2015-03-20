#ifndef NDARRAYRING_H
#define NDARRAYRING_H

#include "NDArray.h"

class NDArrayRing
{
  public:

    // Creates a ring with pointers to buffers.
    NDArrayRing(int noOfBuffers);

    // Destructor.
    ~NDArrayRing();

    // Read the current size
    int size();

    // Add a new buffer reference to the end of the ring
    NDArray *addToEnd(NDArray *pArray);

    // Read out the first buffer reference from the ring
    NDArray *readFromStart();

    // Read out the next buffer reference from the ring
    NDArray *readNext();

    // Does the ring have any other data
    bool hasNext();

    // Removes all the elements from the ring.
    void clear();
      
  private:
    // Array of pointers to NDArrays
    NDArray** buffers_;

    // The size of the ring
    int  noOfBuffers_;

    // Index to read the next buffer
    int  readIndex_;

    // Index to write a new NDArray pointer into the ring
    int  writeIndex_;

    // Has the ring wrapped yet
    int  wrapped_;
};

#endif

