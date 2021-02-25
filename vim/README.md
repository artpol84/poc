## VIM cheatsheet

### FILE MANAGER
* ":edit <dirname>" To open the file explorer
* ":bd" - delete current buffer to close it
* "ls" list buffers
* "b<number>" switch to a buffer

### EDITING
* Indentation:
  * Usee `==` to auto-indent the current line
  * OR: select with 'v' and use `==` with the selection.
* "<lineno> + Shift-g" Goto line
* "<number>u" - undo the last change 
* ":undolist" 
* Copy-past: "v" to start selecting; "d" to cut/"y" to copy; "P" to past before/"p" paste after cursor.
* "Ctrl-w" remove the whole word (in insert mode)
* Search/replace [[link](https://vim.fandom.com/wiki/Search_and_replace)]: `[%]s/search/replace/[flags]`
  * `%` search in all lines (search the current line otherwise)
  * flags:
    * `g` - global flag (change each occurence in the line)
    * `c` - ask for confirmation
    * `i` - case insensitive

### WINDOWS
* `:sp` - Split horisontaly
* `:vsp` - split vertically
* `CTRL-w, w` - switch between the windows
* `CTRL-w, <number>+` - INcrease the size by <number> lines
* `CTRL-w, <number>-` - DEcrease the size by <number> lines
* `CTRL-w, <number>{'<'|'>'}` vertical size adjustment
* `CTRL+w, =` - return to equalized split
* `CTRL+w, v` Opens a new vertical split
* `CTRL+w, c` Closes a window but keeps the buffer
* `CTRL+w, o` Closes other windows, keeps the active window only
* `CTRL+w, right arrow`: Moves the cursor to the window on the right
* `CTRL+w, r`: Moves the current window to the right

### SEARCH
* "/string" - forward search
* ?string - backward search
* "*" (fwd) / "#" (backwd) - search the word under cursor

### SUBSTITUTE
* :1,$s/colour/color/gi - Substite "colour" with "color" from line "1" till "$" (EOF)
* :1,$s/colour/color/gic - Same, but ask for confirmation

### SHELL
":!pwd" can run any command

### CODE NAVIGATION
#### ctags
* "!ctags -R . "
* ":tag <name>" 
* "Ctrl+]" search symbol under cursor ("Ctrl+t" - jump back)
* "Ctrl+x Ctrl+]" - autocomplete
#### cscope
* "cscope -Rb" To generate the database
* "Ctrl-\ s" To search for the symbols invocations
 
 ### CODE Tools
 #### Comment the block
for commenting lines 66-70: `:66,70s/^/#`
for uncommenting these lines: `:66,70s/^#/`

### Tips&Tricks
#### Vim freezes
* Ctrl + Q (see [[link](https://blog.marcinchwedczuk.pl/how-to-fix-vim-freezes#:~:text=To%20unfreeze%20program%20you%20must%20press%20Ctrl%2BQ%20.&text=It%20still%20happens%20from%20time,Q%20and%20continue%20my%20work)])
