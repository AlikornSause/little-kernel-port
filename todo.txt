Shit to do to maybe someday port k61v1_64_bsp to qin f21pro
    - fill in stuff in platform/mt6761/include/platform/mt_reg_base.h from shit like usb_sif@11210000
        - tried filling it in, we'll se what it does
    - fill in other stuff in platform/mt6761/include/platform/ such as platform/mt6761/include/platform/memory_layout.h 
    - get it to not crash :)
    - get uart logs working. 
        - added a raw memory write in kernel/main.c
          to print "H" to uart.
          I'll soon test if it does something
    - get lcd running someday

Done:
    - changed the dtb header to one extractedfrom an original lk dump.
    - done some work on the lcm driver usingthe kernel replacement attempt as help.
    - started using a "register marker" to see where the lk crashed.
    - realised that for some reason PROFILING_END() crashed.
    - oh... and dprintf() as well...
    - realised that the uart base address was wrong, because of me...
    - got uart output finally.
