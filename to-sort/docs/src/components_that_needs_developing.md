#<cldoc:Developers Guide::Components that needs to be developed>

A short description of what we need to develop.

# MVP Friday the 26th

Would require:


3. Networking inteface to deliver transactions / recieve
4. Block definition
5. Block queue
6. Blockchain definition                                   (write tonight)
7. Proof-of-work implemenation

Block and transaction definition could fall into same category
Body Hash
Type = 
Body
Proof


# Low level functionality

## VariantStack (done)
This implementation is done and can be found in the source.

## RandomAccessStack (done)
This implementation is done and can be found in the source.

## VersionedRandomAccessStack (done)
This implementation is done and can be found in the source.

## FileObject (in progress)



## Indexed Document Storage (not started)
Given an integer, this object allows you to read and write documents of
the hard drive.  This data types exists in two variants:  

1. Indexed Document Storage
2. Indexed Versioned Document Storage 

The implementation relies on FileObject to do the read and write.


## Hash Modifier (not started)

This is function that is supplied as argument to some of the following
data structures. It's main purpose is to define how we 

## Key-Value Storage / Versioned Key-Value Storage / Hashed Key-Value Storage / Hashed Versioned Key-Value Storage 
All of the previous

## Named Document Store /  Named Versioned Document Store /  Hashed Named Document Store /  Hashed Named Versioned Document Store 


# High level functionality

# Transaction Store

This is an unversioned document store that 

