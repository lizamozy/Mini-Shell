# `msh` - the `M`ini-`SH`ell

We are not implementing a [full, traditional shell](https://www.gnu.org/software/libc/manual/html_node/Implementing-a-Shell.html).
For example, we won't use process groups, and support full terminal support to run the likes of `nano`, `emacs`, `vim`, or `top`, nor are we implementing wildcards (`*`) and strings (`""` nor `''`).
Instead, we'll support the core of a shell: terminal input and output, pipelines, output redirection, and shell control over individual processes.

## Shell Design

There are many ways to implement a shell.
They are motivated by the requirements of

- Parsing inputs,
- Enabling program composition into pipelines,
- The need for the shell to `wait` (or `waitpid`) on children,
- The interactions between the shell and children via signals (i.e. terminating them),
- Enabling background computation,
- The interactions between the terminal and the shell (keyboard input and `cntl-c`/`cntl-z`), and
- The redirection of child output to files.


### Executing Pipelines

When the `execute` logic returns, there are no foreground tasks.
Either they have excited, or the foreground task was put into the background.
This means that either the shell decided to not block `wait`ing for the pipeline, or that the `wait` calls have returned, indicating that the foreground commands have exited.

The global `pipelines` data-structure tracks all active pipelines and is used to formulate output of the `jobs` command, and is accessed using the `fg` command.


### Foreground and Background Pipelines

Pipelines can be executed in the *foreground* or the *background*^[We will *not* support suspending commands (using `SIGSTOP`, and later waking using `SIGCONT`), and will instead only support tasks running in the foreground and background.].
Foreground pipelines are the default.
A foreground pipeline has three key properties:

1. it can be interacted with on the keyboard,
2. it prevents the shell prompt from being presented, and
3. it can be interacted with using keyboard shortcuts, for example, `cntl-c` to terminate the pipeline or `cntl-z` to put the pipeline into the background.

The core of how these are enabled is that the shell `wait`s (or `waitpid`s) for the foreground processes, thus enabling them to interact with the keyboard.

Background tasks are still executing (unlike in traditional shells that might suspend tasks with `cntl-z`), but can only be interacted with using a few builtin commands.

1. `jobs` which lists all of the background pipelines, and
2. `fg N` in which pipeline `N` is brought to the foreground.

As background tasks don't require terminal input, there can be multiple/many executing at any point in time.
A pipeline that terminates with a `&` is run in the background immediately.

### Exiting the Shell

- `exit` - builtin command to exit.
- an empty input must exit the shell (this is necessary for the automatic tests)

## Parsing and a Specification of Inputs

The string inputs on the command line include the following operators.
See `msh.h` for the description of the referenced errors.

- `;` - the sequence operator separates pipelines.
    The pipeline preceding the `;` must finish execution before the pipeline after is executed.
    Empty pipelines before or after the `;` are allowed (indicating "there is no command").
	The last pipeline is the one after the last `;`.
	When the last pipeline in a sequence finishes (all its commands have `exit`ed), the user has the ability to type more inputs.
	Errors include `MSH_ERR_SEQ_REDIR_OR_BACKGROUND_MISSING_CMD`.
- `|` -  the pipe operator to redirect output from the previous command to the input of the next.
    Errors include `MSH_ERR_PIPE_MISSING_CMD`, `MSH_ERR_TOO_MANY_CMDS`.
- `&` - the background operator is the last non-space character in a pipeline, and indicates that this pipeline should be run in the background.
    Pipelines run in the background enable the next pipeline in the sequence to execute before the previous one that executes in the background is finished, or if the background pipeline is the last, the shell returns to take additional user input.
	Errors include `MSH_ERR_MISUSED_BACKGROUND`.
- `1>`, `1>>`, `2>`, and `2>>` - are redirection operators requesting that the standard output, file descriptor `1`,  should be output to a file, or that the standard error, file descriptor `2`, should be output a file.
    The file name must follow these operators, and redirection operators must be last in a command aside from the optional `&` (they must follow the program and the arguments).
    The `>` variants will delete the specified file before outputting into it, while the `>>` variants will append to the file.
	In either case, if the file does not already exist, it will be created.
	Redirections of the standard output stream are *only allowed*
	Errors include `MSH_ERR_MULT_REDIRECTIONS`, `MSH_ERR_REDIRECTED_TO_TOO_MANY_FILES`, and `MSH_ERR_NO_REDIR_FILE`.

Each operator must be surrounded by spaces unless at the end of the input.
Each pipeline is a set of commands that includes a *program* to execute, and optionally:

1. a set of arguments to that program, and then, optionally,
2. redirection operators with their associated files.

Potential errors include `MSH_ERR_TOO_MANY_ARGS`, `MSH_ERR_NO_EXEC_PROG`.

### Assumptions

These are defined in `msh.h`.
Make sure to use the macros, not the constant values (i.e. use `MSH_MAXARGS`, not `16` in your code).
Given these assumptions, you might make your `struct msh_pipeline` hold a fixed array of `MSH_MAXCMNDS` `struct msh_command`s.
As such, these assumptions are meant to simply your allocations, should you choose to use internal array allocations.

## Code Organization

the tests are in `tests/`.
They are explained further in the specific milestone.
The `mshparse/` directory holds the files necessary, all of which will be built into a library.
You should not modify the `mshparse/msh_parse.h` nor `msh.h` files.

## Parsing Sequences, Pipelines, and Commands

This milestone will focus on parsing the shell's input.
The strings entered as input into the shell require pulling them apart into

1. different pipelines,
2. the commands in the pipelines, and
3. the commands and arguments.

You'll see in `mshparse/msh_parse.h` that there are corresponding structures:

1. `struct sequence` which is a set of pipelines (one to be executed after the other),
2. `struct pipeline` which is a set of commands whereby the output of the previous is piped to the input of the next, and
3. `struct command` which is a program or builtin command followed by the arguments to pass to that program/command.


### Examples

- `ls` - a single command run immediately in the foreground.
- `ls -l` - a command with an argument.
- `ls -1 | grep msh` - a pipeline with two commands.
- `ls | grep msh ; cd .. ; ls | grep msh` -
    A sequence of *three* pipelines, the first and the third having two commands.
	The `grep` commands have a single argument.


### Assumptions

- Whitespace will include only spaces (not tabs or newlines).



## Executing Commands and Pipelines

For this milestone, you'll implement a first version of the shell that supports pipes.
This means that you'll need to actually execute the commands, passing to them their arguments, and set up the pipes and descriptors to support pipelines.
Pipes must be set up as follows: the standard output the command on the left of the `|` goes to a `pipe`, and the standard input of the command on the right.

For example, the command `a b c d` would attempt to execute a program called `a`, passing it arguments `b`, `c`, and `d`.
The commands `a b | c d` execute programs `a` and `c` passing them `b` and `d`, respectively, and the output from `a` is a pipe that provides the input for `c`.
This is, of course, meant to strongly mimic how normal shells work.


- `cd` - change the current working directory.
	This takes a *single* argument, which is the directory to switch to.
    This must support the ability to change to a relative directory paths (e.g. `cd ..`, or `cd blah/`), absolute paths (e.g. `cd /home/gparmer`, or `cd /proc/`), and paths relative to our home directory (e.g. `cd ~` to switch to the home directory, and `cd ~/blahblah/` to switch into the `blahblah` directory in our home directory.)
	An example:

	```
	> pwd
	/home/gparmer/tmp
	> ls
	hw.c
	> cd ~
	> pwd
	/home/gparmer
	> ls
	tmp
	> cd ~/tmp
	> pwd
	/home/gparmer/tmp
	```

	We will *not* support the general use of `~` in arbitrary commands, instead only supporting it in `cd`.
- `exit` - exit from the shell.
	As this milestone does not support background computation, no pipelines should be executing when we exit from the shell.

To exit from the shell , simply type an empty command, or `cntl-d` (hold the control button, and press "d").
You *cannot* change this behavior.
The testing harness requires that an empty command or a `cntl-d` exits the shell.
This is the default behavior of `msh_main.c`.


## Job Control

This requires the use of signals to coordinate between the shell, user, and children, and a set of builtin commands to control pipelines (which are synonymous with "job").

When a pipeline is executing in the *foreground*, the shell is `wait`ing for it complete, thus will not receive additional inputs.
A pipeline is executed in the foreground by default.
If the `&` operator is provided at the end of a pipeline, it is executed in the background.
A foreground pipeline can be

1. *suspended* with `cntl-z` (that is, holding "control" and pressing "z") at which point the shell can receive input again, or
2. *terminated* with `cntl-c`.

A suspended pipeline can be placed into the background with the `bg` command or into the foreground with `fg`.

Signals are used to suspend, continue, and terminate pipelines.
Signals of interest include the following^[See `man 7 signal` for more information.]

- `SIGINT` - sent with `cntl-c` to the shell
- `SIGTSTP` - sent with `cntl-z` to the shell
- `SIGTERM` - sent to the pipeline processes, they will terminate

There are decent examples for [similar commands and signals in bash](https://superuser.com/questions/262942/whats-different-between-ctrlz-and-ctrlc-in-unix-command-line).

### Examples

- `sleep 10 &` - a single command, an argument, and to be run in the background.
- `sleep 10 & ; ls ; sleep 9` -
    Three commands, the first `sleep` runs in the background, thus the shell immediately executes the `ls`, then the last `sleep` which is run in the foreground, thus maintains the command line for `9` seconds.
- `sleep 10 ; ls ; sleep 9` -
    Since the first `sleep` is run in the foreground, the shell waits for `10` seconds before proceeding to the latter commands.
	If the user types `cntl-z` while the first sleep is executing, it will be moved to the background, thus the `ls` is immediately executed.


## File Redirection

This requires implementing the redirection operators introduced earlier.
After a program and its arguments, a command can include a redirection of standard error to a file, then either

- a redirection of standard output via a pipe,
- a redirection of standard output to a file, or
- no redirection for standard output, thus sending its output to the command line.

### Examples

- `echo hi 2> err.output | cat 2> caterrs.output` - This will run `echo`, redirecting its standard error to the `err.output` file, and redirect its standard output to the pipeline, and also redirect the errors for `cat` to a file.
    The standard output of the `cat` goes, as normal, to the terminal.
- `ls | grep msh 1>> output` - The last command in a pipeline can redirect its output to a file, in this case `grep`'s output is *appended* to output.

