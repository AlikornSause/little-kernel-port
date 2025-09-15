<h1>Qin f21 Pro little kernel bootloader port</h1>

<h3>To build, install the needed stuff in dependencies.txt
and use the run.sh or runclean.sh scripts</h3>
<br>
<br>
<br>

<h3>Timeline I guess:</h3>
- changed the dtb header to one extractedfrom an original lk dump.
- done some work on the lcm driver usingthe kernel replacement attempt as help.
- started using a "register marker" to see where the lk crashed.
- realised that for some reason PROFILING_END() crashed.
- oh... and dprintf() as well...
- realised that the uart base address was wrong, because of me...
- got uart output finally.
- kernel now boots but doesnt boot android
- probably bad cmdline. check agains stock

- Fixed that! Now it properly boots linux kernel and android!

<h3>TODO and DONE:</h3>

TODO:
- fix lcd: logo not showing
- fix fastboot reboot 
- remove unnecessary code:
    - unnecessary apps (nandwrite, clocktests, pcitests, stringters etc)
    - unneeded libs etc
    - all of AVB functionality
- just clean up messy code (its bad)


DONE:
- fix fastboot
- fix usb connections
- fix serial number not loading in


<h3>LK booting process:</h3>

1. **arch/arm/crt0.S** in assembly, first thing thats executed
2. **kernel/main.c/kmain** main kernel function in C. It calls various init functions:
    - thread_init_early()
    - arch_early_init()
    - platform_early_init()
    - target_early_init()
    - call_constructors()
    - heap_init()
    - thread_init()
    - dpc_init()
    - timer_init()
    - bootstrap2()
3. **kernel/main.c/bootstrap2** function that takes on the init
    - print_stack_of_current_thread()
    - arch_init()
    - mboot_allocate_lk_scratch_from_mblock()
    - platform_init()
    - target_init()
    - apps_init()
4. **app/app.c/apps_init** function that calls different "apps". In our case its mt_boot
5. **app/mt_boot/mt_boot.c/mt_boot_init** the init function for that "app"
    - set_serial_num()

    Depending on the mode, it will go to either of these paths:
    1. LINUX
    - boot_linux_from_storage() 
    - boot_linux()

    2. FASTBOOT
    - target_fastboot_init()
    - mt_part_dump()
    - fastboot_init()
    - udc_start()
    