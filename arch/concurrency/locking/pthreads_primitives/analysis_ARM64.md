

```
000000000000c85c <pthread_spin_lock>:
    c85c:       d10043ff        sub     sp, sp, #0x10
```

### Write 0x1 to the spinlock variable while in the exclusive mode

  * LDAXR:
    * Load-Acquire Exclusive Register derives an address from a base register value
    * Loads a 32-bit word or 64-bit doubleword from memory, and writes it to a register. 
    * The memory access is atomic. The PE marks the physical address being accessed as an exclusive access. This exclusive access mark is checked by Store Exclusive instructions.
    * The instruction also has memory ordering semantics as described in Load-Acquire, Store-Release in the ARMv8-A Architecture Reference Manual.
  * STXR Ws, Wt, [Xn|SP{,#0}] 
    * Ws:
      * 0 If the operation updates memory.
      * 1 If the operation fails to update memory.
    * Store Exclusive Register stores a 32-bit word or a 64-bit doubleword from a register to memory if the PE has exclusive access to the memory address
    * And returns a status value of 0 if the store was successful, or of 1 if no store was performed. 
```
    c860:       52800024        mov     w4, #0x1                        // #1
    c864:       885ffc01        ldaxr   w1, [x0]
    c868:       88027c04        stxr    w2, w4, [x0]
    c86c:       35ffffc2        cbnz    w2, c864 <pthread_spin_lock+0x8>
```
### Exit if the previous value was 0x0 (meaning that spinlock wasn't taken)
* Because PE is having a memory (cache line) in the exclusive mode this memory cannot be modified by others
* This means that no other concurrent PE can get 0 as well
* if w1 == 0 we acquired the lock - jump to the [exit_label]
```
    c870:       b90007e1        str     w1, [sp,#4]
    c874:       b94007e1        ldr     w1, [sp,#4]
    c878:       340002e1        cbz     w1, c8d4 <pthread_spin_lock+0x78>
```

### Set x3 = sp + 0xc = sp + #12
```
    c87c:       910033e3        add     x3, sp, #0xc
```

### [loop_label] Prepare to wait
* Check if w1 became 0 in the meantime
* if yes - skip waiting and try to acquire the lock going to [acquire_label]
```
    c880:       b9400001        ldr     w1, [x0]
    c884:       340000e1        cbz     w1, c8a0 <pthread_spin_lock+0x44>
```
### Initialize waiting list
* w1 is set to 1000 (iterations)
* then jump to the first list instruction
```
    c888:       52807d01        mov     w1, #0x3e8                      // #1000
    c88c:       14000002        b       c894 <pthread_spin_lock+0x38>
```

### [wait_label] Perform the loop
* Either do 1000 iterations
* or exit if the spinlock turns to 0
```
    c890:       54000080        b.eq    c8a0 <pthread_spin_lock+0x44>
    c894:       b9400002        ldr     w2, [x0]
    c898:       71000421        subs    w1, w1, #0x1
    c89c:       35ffffa2        cbnz    w2, c890 <pthread_spin_lock+0x34>
```

### [acquire_label] Attempt to change the spinlock to 0x1
```
    c8a0:       b9000fff        str     wzr, [sp,#12]
    c8a4:       b9400061        ldr     w1, [x3]        // Load 0 to w1
    c8a8:       885ffc02        ldaxr   w2, [x0]
    c8ac:       6b01005f        cmp     w2, w1
    c8b0:       54000061        b.ne    c8bc <pthread_spin_lock+0x60>
    c8b4:       88057c04        stxr    w5, w4, [x0]
    c8b8:       35ffff85        cbnz    w5, c8a8 <pthread_spin_lock+0x4c>
```
### if `cmp w2, w1` returned true (if w2 == 0) - jump to the exit, lock obtained
```
    c8bc:       54000040        b.eq    c8c4 <pthread_spin_lock+0x68>
```
### if not 
* store the value of w2 by address in x3
* do some unclear manipulations with w1 (but [sp,#12] is the same as [x3], see how x3 is being set)
* jump to the [loop_label]
```
    c8c0:       b9000062        str     w2, [x3]
    c8c4:       b9400fe1        ldr     w1, [sp,#12]
    c8c8:       b9000be1        str     w1, [sp,#8]
    c8cc:       b9400be1        ldr     w1, [sp,#8]
    c8d0:       35fffd81        cbnz    w1, c880 <pthread_spin_lock+0x24>
```

### [exit_label] the lock (obtained)
```
    c8d4:       52800000        mov     w0, #0x0                        // #0
    c8d8:       910043ff        add     sp, sp, #0x10
    c8dc:       d65f03c0        ret
```
