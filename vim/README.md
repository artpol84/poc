## VIM cheatsheet

### FILE MANAGER
* ":edit <dirname>" To open the file explorer
* ":bd" - delete current buffer to close it
* "ls" list buffers
* "b<number>" switch to a buffer

### EDITING
* "<lineno> + Shift-g" Goto line
* "<number>u" - undo the last change 
* ":undolist" 
* Copy-past: "v" to start selecting; "d" to cut/"y" to copy; "P" to past before/"p" paste after cursor.
* "Ctrl-w" remove the whole word (in insert mode)

### WINDOWS
* ":sp" - Split horisontaly
* ":vsp" - split vertically
* "CTRL-w-w" - switch between the windows
* "CTRL-w <number>+" - INcrease the size by <number> lines
* "CTRL-w <number>-" - DEcrease the size by <number> lines
* "CTRL-w <number>{'<'|'>'}" vertical size adjustment
* "CTRL+w =" - return to equalized split

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
