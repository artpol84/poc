### Implementation descriptions

* **orig** - the version currently implemented in Slurm
* **xstrcatAT** the version provided by the [patch from Tim Wickberg](https://bugs.schedmd.com/attachment.cgi?id=12343&action=diff) in Slurm [bug 6932](https://bugs.schedmd.com/show_bug.cgi?id=6932) that enables the use of the new `xstrfmtcatat` function.
* **xstrcatAT/opt-order** - [patch from Tim Wickberg](https://bugs.schedmd.com/attachment.cgi?id=12343&action=diff) + optimized operation reordering, proposed in [patch from @artpol84](https://bugs.schedmd.com/attachment.cgi?id=10068) in [bug 6932](https://bugs.schedmd.com/show_bug.cgi?id=6932).
* **xstrcatAT/opt-order/no-strdup** same as **xstrcatAT/opt-order** + the fix from @karasevb (https://github.com/karasevb/slurm/commit/f40898b9ba65ae1b063c772d6616e747793f56e1) avoiding xstrdup function in `xstrfmtcatat`.
* **xstrcatAT/opt-order/no-strdup/no-memcpy** same as **xstrcatAT/opt-order/no-strdup** + the fix from @karasevb (https://github.com/karasevb/slurm/commit/ed7e0c59e11956ccca7b0246654521f675b93039) avoiding extra memcopy in `xstrfmtcatat` as in this particular implementation it is known in advance that the buffer is large enough to fit the whole string being generated (see explanation in https://github.com/artpol84/poc/blob/master/slurm/pmix_proc_map/main.c#L60:L86)
* **opt-order/my-strcat** implementation that corresponds to [patch from @artpol84](https://bugs.schedmd.com/attachment.cgi?id=10068) in [bug 6932](https://bugs.schedmd.com/show_bug.cgi?id=6932).

### Performance evaluation:

#### All modes described above: 

![All modes](https://github.com/artpol84/poc/blob/master/slurm/pmix_proc_map/imgs/all_curves.png)

It is clear from these measurements, that optimization of the order **opt-order** is the main bottleneck of the algorithm.

#### Modes with **opt-order** only:

![**opt-order** modes](https://github.com/artpol84/poc/blob/master/slurm/pmix_proc_map/imgs/opt_curves.png)

Here the disadvantages of `xstrfmtcatat` are highlighted
It is clear from these measurements, that optimization of the order **opt-order** is the main bottleneck of the algorithm.
* Compared to **opt-order/my-strcat** version, strdup has ~ 40% overhead
* Compared to **opt-order/my-strcat** version, unneeded memcopy contributes additional 10% overhead.

Result overhead from `xstrfmtcatat` is 50%.

