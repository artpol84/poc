spinlock:

Invocation:
        :       48 8d 45 fc             lea    -0x4(%rbp),%rax                                                                                                                                                                                
        :       48 89 c7                mov    %rax,%rdi                                                                                                                                                                                      
        :       e8 8e 59 00 00          callq  407a20 <pthread_spin_lock>    

Function:
 acquire:       f0 ff 0f                lock decl (%rdi)
        :       75 0b                   jne    407a30 <pthread_spin_lock+0x10>
        :       31 c0                   xor    %eax,%eax
        :       c3                      retq
        :       0f 1f 84 00 00 00 00    nopl   0x0(%rax,%rax,1)
        :       00
   sleep:       f3 90                   pause
        :       83 3f 00                cmpl   $0x0,(%rdi)
        :       7f e9                   jg     sleep
        :       eb f7                   jmp    acquire

0000000000407a60 <pthread_spin_init>:
        :       c7 07 01 00 00 00       movl   $0x1,(%rdi)
        :       31 c0                   xor    %eax,%eax
        :       c3                      retq


mutex:

MUTEX lock

Invocation
        :       48 89 c7                mov    %rax,%rdi                                                                                                                                                                                      
        :       e8 8e 33 00 00          callq  405420 <__pthread_mutex_lock>



0000000000405420 <__pthread_mutex_lock>:

Fast path
  405476:       bf 01 00 00 00          mov    $0x1,%edi        // load 1 to EDI
  40547b:       31 c0                   xor    %eax,%eax        // Zero EAX
->40547d:       f0 41 0f b1 38          lock cmpxchg %edi,(%r8) // r8 contains the pointer to the mutex structure (__lock field)
                                                                // cmpxchg compares EAX with with second OP ($r8 => mutex->__lock) and if they are equal, sets second OP to the first OP (edi == 1)
                                                                // If exchange successful it sets ZF = 0, otherwise ZF = 1
  405482:       0f 85 24 01 00 00       jne    4055ac <_L_lock_812> // Goes here if exch fails (ZF=0), falls through if the lock acquired
  405488:       64 8b 04 25 d0 02 00    mov    %fs:0x2d0,%eax   //  place PID/TID of the locked process/thread to EAX
  40548f:       00
  405490:       41 89 40 08             mov    %eax,0x8(%r8)    // Move PID/TID to the offset 0x8 (mutex->__data.__owner)
  405494:       41 83 40 0c 01          addl   $0x1,0xc(%r8)    // Move 1 to mutex->nusers
  405499:       90                      nop
  40549a:       31 c0                   xor    %eax,%eax        // ret code to 0
  40549c:       c3                      retq                    // Exit


Slow path (fitst acquisition)
  405476:       bf 01 00 00 00          mov    $0x1,%edi        // load 1 to EDI
  40547b:       31 c0                   xor    %eax,%eax        // Zero EAX
->40547d:       f0 41 0f b1 38          lock cmpxchg %edi,(%r8) // r8 contains the pointer to the mutex structure (__lock field)
                                                                // cmpxchg compares EAX with with second OP ($r8 => mutex->__lock) and if they are equal, sets second OP to the first OP (edi == 1)
                                                                // If exchange successful it sets ZF = 0, otherwise ZF = 1
  405482:       0f 85 24 01 00 00       jne    4055ac <_L_lock_812> // Goes here if exch fails, falls through if the lock acquired

00000000004055ac <_L_lock_812>:
  4055ac:       49 8d 38                lea    (%r8),%rdi
  4055af:       48 81 ec 80 00 00 00    sub    $0x80,%rsp
  4055b6:       e8 15 44 00 00          callq  4099d0 <__lll_lock_wait>

00000000004099d0 <__lll_lock_wait>:
  4099d0:       41 52                   push   %r10
  4099d2:       52                      push   %rdx
  4099d3:       4d 31 d2                xor    %r10,%r10
  4099d6:       ba 02 00 00 00          mov    $0x2,%edx
  4099db:       81 f6 80 00 00 00       xor    $0x80,%esi
  4099e1:       39 d0                   cmp    %edx,%eax        // EAX contains value 0x1 because somebody else have taken the lock, (0x1 <> 0x2)
                                                                // Will skip the Futex call on the first iteration and jump to 4099ed
  4099e3:       75 08                   jne    4099ed <__lll_lock_wait+0x1d>
  4099e5:       90                      nop
  4099e6:       b8 ca 00 00 00          mov    $0xca,%eax   // #define __NR_futex 202
  4099eb:       0f 05                   syscall
  4099ed:       89 d0                   mov    %edx,%eax
  4099ef:       87 07                   xchg   %eax,(%rdi)  // Attempt to take the lock: exchange mutex->__data.__lock with 0x2
                                                            // ATOMICALLY: Note that xchng forces Bus locking even without LOCK prefix
  4099f1:       85 c0                   test   %eax,%eax    // if EAX is 0 then ZF = 1, else ZF = 0
  4099f3:       75 f0                   jne    4099e5 <__lll_lock_wait+0x15> // if EAX is 0 => mutex was unlocked and now we own the lock
                                                                             // So if ZF=0 we fall through with lock acquired, otherwise - reiterate with eax = 0
  4099f5:       5a                      pop    %rdx // Restore used values
  4099f6:       41 5a                   pop    %r10
  4099f8:       c3                      retq

// Return to the _L_loc_812
  4055bb:       48 81 c4 80 00 00 00    add    $0x80,%rsp
  4055c2:       e9 c1 fe ff ff          jmpq   405488 <__pthread_mutex_lock+0x68>

// Return to the __pthread_mutex_lock
  405488:       64 8b 04 25 d0 02 00    mov    %fs:0x2d0,%eax   //  place PID/TID of the locked process/thread to EAX
  40548f:       00
  405490:       41 89 40 08             mov    %eax,0x8(%r8)    // Move PID/TID to the offset 0x8 (mutex->owner)
  405494:       41 83 40 0c 01          addl   $0x1,0xc(%r8)    // Move 1 to mutex->nusers
  405499:       90                      nop
  40549a:       31 c0                   xor    %eax,%eax        // ret code to 0
  40549c:       c3                      retq                    // Exit


MUTEX unlock


Fast path:

00000000004065d0 <__pthread_mutex_unlock>:
  4065d0:       8b 77 10                mov    0x10(%rdi),%esi
  4065d3:       48 89 fa                mov    %rdi,%rdx
... // Dealing with specific lock kinds
  4065ea:       c7 42 08 00 00 00 00    movl   $0x0,0x8(%rdx) // Overwrite PID in the mutex->__data.__owner
  4065f1:       83 6a 0c 01             subl   $0x1,0xc(%rdx) // Subtract 1 from number of users (in case of non-recursive it should be 1)
  4065f5:       81 e6 80 00 00 00       and    $0x80,%esi     // ??
  4065fb:       f0 ff 0a                lock decl (%rdx)        // Atomically decrement the value in mutex->__data.__lock
  4065fe:       75 7f                   jne    40667f <_L_unlock_713>   // If __lock became 0 - it is a fastpath, noone is waiting on the mutex, just exit.
  406600:       90                      nop
  406601:       44 89 c0                mov    %r8d,%eax
  406604:       c3                      retq


  4065d0:       8b 77 10                mov    0x10(%rdi),%esi
  4065d3:       48 89 fa                mov    %rdi,%rdx
... // Dealing with specific lock kinds
  4065ea:       c7 42 08 00 00 00 00    movl   $0x0,0x8(%rdx) // Overwrite PID in the mutex->__data.__owner
  4065f1:       83 6a 0c 01             subl   $0x1,0xc(%rdx) // Subtract 1 from number of users (in case of non-recursive it should be 1)
  4065f5:       81 e6 80 00 00 00       and    $0x80,%esi     // ??
  4065fb:       f0 ff 0a                lock decl (%rdx)        // Atomically decrement the value in mutex->__data.__lock
  4065fe:       75 7f                   jne    40667f <_L_unlock_713>   // Lock was 0x2 as someone is waiting for it, so jump to _L_unlock_713

000000000040667f <_L_unlock_713>:
  40667f:       48 8d 3a                lea    (%rdx),%rdi
  406682:       48 81 ec 80 00 00 00    sub    $0x80,%rsp
  406689:       e8 e2 33 00 00          callq  409a70 <__lll_unlock_wake>

0000000000409a70 <__lll_unlock_wake>:
  409a70:       56                      push   %rsi
  409a71:       52                      push   %rdx
  409a72:       c7 07 00 00 00 00       movl   $0x0,(%rdi)  // Write 0 to mutex->__data.__lock finally releasing the lock
  409a78:       81 f6 81 00 00 00       xor    $0x81,%esi
  409a7e:       ba 01 00 00 00          mov    $0x1,%edx    // Prepare for the Futex syscall
  409a83:       b8 ca 00 00 00          mov    $0xca,%eax   // #define __NR_futex 202
  409a88:       0f 05                   syscall             // Invoke the syscall
  409a8a:       5a                      pop    %rdx
  409a8b:       5e                      pop    %rsi
  409a8c:       c3                      retq
  409a8d:       0f 1f 00                nopl   (%rax)

// back to _L_unlock_713
  40668e:       48 81 c4 80 00 00 00    add    $0x80,%rsp
  406695:       e9 66 ff ff ff          jmpq   406600 <__pthread_mutex_unlock+0x30>
  40669a:       66 0f 1f 44 00 00       nopw   0x0(%rax,%rax,1)

// back to __pthread_mutex_unlock
  406600:       90                      nop
  406601:       44 89 c0                mov    %r8d,%eax
  406604:       c3                      retq
