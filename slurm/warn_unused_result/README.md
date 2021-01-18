### Slurm build errors if unused results warning

refs: [Bug10552](https://bugs.schedmd.com/show_bug.cgi?id=10552)

Gcc ignored unused resutls if `_FORTIFY_SOURCE` is not defined by default.
```bash
# if `_FORTIFY_SOURCE` undefiled:
$ gcc -Wall -Werror -O1 -U_FORTIFY_SOURCE pipe_warn.c 

$ gcc -Wall -Werror -O1 -D_FORTIFY_SOURCE=2 pipe_warn.c 
pipe_warn.c: In function ‘main’:
pipe_warn.c:12:5: error: ignoring return value of ‘pipe’, declared with attribute warn_unused_result [-Werror=unused-result]
     pipe(fd);
     ^~~~~~~~
cc1: all warnings being treated as errors
```
Links: [https://man7.org/linux/man-pages/man7/feature_test_macros.7.html](https://man7.org/linux/man-pages/man7/feature_test_macros.7.html)