<h1>Qin f21 Pro little kernel bootloader port<h1>

- changed the dtb header to one extractedfrom an original lk dump.\
- done some work on the lcm driver usingthe kernel replacement attempt as help.\
- started using a "register marker" to see where the lk crashed.\
- realised that for some reason PROFILING_END() crashed.\
- oh... and dprintf() as well...\
- realised that the uart base address was wrong, because of me...\
- got uart output finally.\
- kernel now boots but doesnt boot android\
- probably bad cmdline. check agains stock

- Fixed that! Now it properly boots linux kernel and android!\



TODO:
    - fix lcd: logo not showing
    - fix fastboot
    - fix usb connections
    - remove unnecessary code