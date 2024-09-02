# remap
 break link between dll and it file on disk

break link between dll and it file on disk

we can do next:

- create section backed by the paging file.
- map this section at any address
- copy self image to this section
- call special func inside new mapped section
- this func unmap our image section and then map new section at this address 

possibilite that some another code allocate memory at this range after we unmap image and before map new section in practic zero
virtual alloc not allocate memory with this (dll/exe) range, if not specify exactly address

- return back to dll normal address range (now it already on paged section)
- unmap first (temporary) view of page section and close it

dll code still full functional (exceptions, cfg) all is work ok
we can delete dll from disk, or modify it code after this

one drawback of this method is if we debug the code - after we unmap image section - debugger get notify about it and usually no more support symbol/src code debugging of dll.
to solve this problem - we can create a thread and hide it from the debugger
and unmap image section from this thread. however this need only during code debug
