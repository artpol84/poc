## Spin lock
```
000000000000c85c <pthread_spin_lock>:
    c85c:       d10043ff        sub     sp, sp, #0x10
```

### Write 0x1 to the spinlock variable while in the exclusive mode
* This is a logical SWAP operation: we read the old value to w1 and write a new value from w4
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

## Spin try lock

```
000000000000c8e0 <pthread_spin_trylock>:
    c8e0:       d10043ff        sub     sp, sp, #0x10
```
### Write 0x1 to the spinlock and get the old value (swap)
* Note
```
    c8e4:       52800022        mov     w2, #0x1                        // #1
    c8e8:       885ffc01        ldaxr   w1, [x0]
    c8ec:       88037c02        stxr    w3, w2, [x0]
    c8f0:       35ffffc3        cbnz    w3, c8e8 <pthread_spin_trylock+0x8>
```
### Compare obtained old value with zero (wzr)
```
    c8f4:       b9000fe1        str     w1, [sp,#12]
    c8f8:       b9400fe0        ldr     w0, [sp,#12]
    c8fc:       6b1f001f        cmp     w0, wzr
```
### Move 0x16 (Device or resource busy) error code to w0 
```
    c900:       52800200        mov     w0, #0x10                       // #16
```
### Set the ultimate error code
* CSEL Wd, Wn, Wm, cond
  * Arguments
    * Wd: the 32-bit name of the general-purpose destination register.
    * Wn: the 32-bit name of the first general-purpose source register.
    * Wm: the 32-bit name of the second general-purpose source register.
  * Conditional Select returns, in the destination register, the value of the first source register if the condition is TRUE, and otherwise returns the value of the second source register.
* in this case we set w0 to 
  * w0 (0x16) if the result of comparison in 0xc8fc is `ne` = Not Equal
  * wzr (0) otherwise (if we took the lock)
```
    c904:       1a9f1000        csel    w0, w0, wzr, ne
```
### Exit sequence
```
    c908:       910043ff        add     sp, sp, #0x10
    c90c:       d65f03c0        ret
```

## Mutex logic

### Mutex startup code
```
0000000000009a68 <__pthread_mutex_lock>:
    9a68:       a9bd7bfd        stp     x29, x30, [sp,#-48]!
    9a6c:       910003fd        mov     x29, sp
...
```

### Perform compare-and-swap (CAS) of the flag variable
* Expect to have 0 value there in order to get the lock
```
    9a9c:       52800020        mov     w0, #0x1                        // #1
    9aa0:       885ffe61        ldaxr   w1, [x19]
    9aa4:       6b1f003f        cmp     w1, wzr
    9aa8:       54000061        b.ne    9ab4 <__pthread_mutex_lock+0x4c>
    9aac:       88027e60        stxr    w2, w0, [x19]
    9ab0:       35ffff82        cbnz    w2, 9aa0 <__pthread_mutex_lock+0x38>
```
### Controlled by comparison from line ???? (cmp    w1, wzr)
* If (w1 != 0) - goes through the slow path
* If (w1 == 0) - fall thru to the fastpath exit
```
    9ab4:       540001c1        b.ne    9aec <__pthread_mutex_lock+0x84>
```

### Perform fastpath cleanup and return
```
    9ab8:       b94023a0        ldr     w0, [x29,#32]
    9abc:       35000420        cbnz    w0, 9b40 <__pthread_mutex_lock+0xd8> 
    9ac0:       b9400e61        ldr     w1, [x19,#12]
    9ac4:       d11bc294        sub     x20, x20, #0x6f0
    9ac8:       b940d280        ldr     w0, [x20,#208]
    9acc:       11000421        add     w1, w1, #0x1
    9ad0:       b9000a60        str     w0, [x19,#8]
    9ad4:       b9000e61        str     w1, [x19,#12]
    9ad8:       d503201f        nop
    9adc:       52800000        mov     w0, #0x0                        // #0
    9ae0:       a94153f3        ldp     x19, x20, [sp,#16]
    9ae4:       a8c37bfd        ldp     x29, x30, [sp],#48
    9ae8:       d65f03c0        ret
```

### Slow path - __lll_lock_wait
* Read atomic flag value to see if it is equal to 0x2
* if it is 0x2 someone already waiting for it so it jumps into the futex (similar to x86)
* otherwise it first replaces 0x1 with 0x2 and then goes into the futex
```
    ef8c:       b9400000        ldr     w0, [x0]
    ef90:       7100081f        cmp     w0, #0x2
    ef94:       54000260        b.eq    efe0 <__lll_lock_wait+0x6c>

```
### Prepare for futex
* See http://man7.org/linux/man-pages/man2/futex.2.html
* 0x80 is (FUTEX_WAIT | FUTEX_PRIVATE_FLAG) - in this case mutex is private, not shared between procs
```
    ef98:       521902b5        eor     w21, w21, #0x80
    ef9c:       52800054        mov     w20, #0x2                       // #2
    efa0:       93407eb5        sxtw    x21, w21  
    efa4:       14000007        b       efc0 <__lll_lock_wait+0x4c>
```
### attempt to switch value to 0x2 (atomic swap)
* If the old value is non-zero - jump to a futex sequence
* Fall-thru otherwise
```
    efc0:       885ffe60        ldaxr   w0, [x19]
    efc4:       88017e74        stxr    w1, w20, [x19]
    efc8:       35ffffc1        cbnz    w1, efc0 <__lll_lock_wait+0x4c>
    efcc:       35fffee0        cbnz    w0, efa8 <__lll_lock_wait+0x34>
```
### Fall-thru - lock acquired
```
    efd0:       a94053f3        ldp     x19, x20, [sp]
    efd4:       f9400bf5        ldr     x21, [sp,#16]
    efd8:       910083ff        add     sp, sp, #0x20
    efdc:       d65f03c0        ret
```

### Futex sequence
* x0 = x19 - address of the mutex variable
* x1 = 0x80 - (FUTEX_WAIT | FUTEX_PRIVATE_FLAG) operation
* x2 = "0x02" - expected value that should be found by the address (if cahnged - futex won't sleep)
* x3 = "0x00" - timeout - sleep indefinitely

```
    efa8:       aa1303e0        mov     x0, x19
    efac:       aa1503e1        mov     x1, x21 
    efb0:       d2800042        mov     x2, #0x2                        // #2
    efb4:       d2800003        mov     x3, #0x0                        // #0
    efb8:       d2800c48        mov     x8, #0x62                       // #98
    efbc:       d4000001        svc     #0x0
``` 

### Futex sequence if futex address already contains 0x02
    efe0:       52190021        eor     w1, w1, #0x80
    efe4:       aa1303e0        mov     x0, x19
    efe8:       93407c21        sxtw    x1, w1
    efec:       d2800042        mov     x2, #0x2                        // #2
    eff0:       d2800003        mov     x3, #0x0                        // #0
    eff4:       d2800c48        mov     x8, #0x62                       // #98
    eff8:       d4000001        svc     #0x0
    effc:       17ffffe7        b       ef98 <__lll_lock_wait+0x24>
```
