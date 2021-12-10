# shell
Shell that uses a command-line interface to use OS services

How to run on the command line:

In order to run Shell 2 on the command line, first navigate to the shell_2 directory in the
terminal. Here is the filepath: /home/new/course/cs0330/shell_2. Then, run "make" in order to
execute the Makefile which compiles the 33sh and 33noprompt executables. These files are virtually
identical (both dependent on sh.c and jobs.c). The only difference is 33sh prompts the user at the
command line and 33noprompt does not. After each file "makes", you can run either from terminal as
follows:

- for 33sh:          "./33sh"

- for 33noprompt:    "./33noprompt"

---------------------------------------------------------------------------------------------------

Valgrind:

I ran the valgrind tool to check for memory safety errors and leaks using the "-m" flag when
running the testing script, "cs0330_shell_2_test -s 33noprompt -m" and I did not encounter any
memory leak issues.

---------------------------------------------------------------------------------------------------

Explanation of Makefile:

I based the Makefile mainly off the Makefile we made in lab06. CFLAGS and PROMPT were provided, as
well as -D_GNU_SOURCE -std=gnu99 to compile our Shell 2. So, modeling after lab06 and my Makefile
from Shell 1, I created CC, EXECS, and DEPENDENCIES macros, setting them equal to gcc (our
compiler), 33sh 33noprompt (our executables), and sh.c job.c (what the executables depend on),
respectively. Then I declared the .PHONY targets: all clean, because they don't correspond to the
name of any build outputs. Finally, the all: and clean: targets are modeled after lab06, where all:
builds all the executables, and clean: removes all thees build outputs. Finally the targest 33sh:
and 33noprompt: are essentially the same except 33noprompt includes the -DPROMPT flag so no prompt
shows at execution. The dependences for each are just sh.c and jobs.c. Again, the syntax of the
shell command is modeled off of lab06, and the macro variables are used for the shell command when
appropriate again based off of lab06.

---------------------------------------------------------------------------------------------------

The structure of my program (sh.c):     (A lot of this comes from my Shell 1 readme.)

Running sh.c starts in the main() function and enters a REPL loop via while(1). This is because
the shell by design reads user input, executes the command, prints, and loops back for user input.
I have divided this into a couple sections for readability - signals/reaping, parsing/tokenizing,
background, redirection, the command array, shell 1 built-ins, shell 2 built-ins, and child process
from shell 1, and child process additions from Shell 2:

Signals/Reaping
In main(), we first declare a jobs list using init_job_list() and instantiate an integer job id.
The jobs list contains the job id, process id, command, and state of all jobs running in the
background. And I increment the job id int when I add a new job, so that it is different every
time. Then, I set SIGINT, SIGQUIT, SIGTSTP, and SIGTTOU to SIG_IGN, because we want our shell to
ignore these when no other process is running. Finally, I call my reap() helper on the jobs list.
reap() examines each child process using waitpid() by looping through the jobs list, and cleans
up any zombie processes if they have terminated, or updates the job if has changed state. In
waitpid(), we use all the flag options, WNOHANG | WUNTRACED | WCONTINUED, because we want to see
all the different cases for what could be happening in the child processes, and deal with them as
needed. For example, if WCONTINUED returns, then we know that job has restarted from a stopped
state and the jobs list must be updated to reflect that, and we also must print an informative
message.

Parsing/Tokenizing:
After reaping, main() declares a buffer char array to allocate memory for user input, prompts
the user, and reads into the buffer from the command line with the read() command. If read() does
not execute correcly, and error is thrown and the program exits. After checking for 'enter' (\n) or
ctrl-D, main() calls my count_tokens() helper to count the number of tokens in main. This is used
for instantiating a char** array to hold all of the tokens as separate string pointer, done in the
tokenize() helper. Before proceeding further, an error is checked in the case that there are
redirections but no commands.

Background Process:
Also, right after calling tokenize(), we check if the last token in the array is "&". If so, this
means that it is a background process, so the background_process flag is set to 1, I nullify that
space in the token array, and finally decrement the number of tokens, as we do not want to treat
the "&" as an argument further along. Using this "&" is what allows for multiple processes at once.

Redirection:
Then, I instantiate integer redirection flags and and a char** array to hold the filenames of the
redirections. Pointers to these variables are passed into check_redirects(), which iterates through
my token array, and sets redirection flags and the redirection char** array if any redirections are
encountered. check_redirects() also sets the addresses of the redirection token and associated
filename to NULL in the token array. There is also error checking involved, to make sure only one
input and output redirection happens per command maximum, and also that there is a redirection file
specified.

The Command Array:
After checking for redirection, the number of command line arguments (excluding redirections) is
found from the number of tokens and the redirection flags set (or left 0) by check_redirects(), and
this allows a char** array representing just the command line tokens to be instantiated, called
cmd_arg. A for loop iterates through the all-tokens array and puts all non-nullified elements into
cmd_arg. The last space (as the length of cmd_arg is the number of command tokens + 1) is filled
with a '\0' as this syntax is necessary for the execv() function with child processes. Again,
cmd_arg represents just the arguments of the command itself.

Shell 1 Built-ins (cd, ln, rm, exit):
There are four built-ins we are implementing here: cd, ln, rm, and exit, handled in run_command().
run_command() checks if the first argument in cmd_arg is "cd", "ln", "rm", or "exit", and if
so executes the correct system call (chdir(), link(), unlink() for cd, ln, and rm respectively)
passing in elements from cmd_arg as needed (whose pointer is passed into the helper). In
run_command() in each case, error checking is done for the correct number of arguments, and also
to make sure that the system calls are executed correctly. If something goes wrong, perror() prints
and informative message. Importantly, run_command() returns 1 only if none of these built-ins are
specified, and 0 otherwise (i.e. correct built-in execution or an error occurred in the process).

Shell 2 Built-ins (jobs, fg, bg):
There are three more built-ins we implement for shell 2 in run_command(): jobs, bg, and fg.
run_command() checks if the first argument in cmd_arg is "jobs", "bg", "fg". If it is jobs, then
jobs() is called from jobs.c, which prints the jobs list. For "bg", we first error check bad user
input so that the command is in the form "bg %d" where d is an integer corresponding to a valid
job id in the jobs list. If so, we send the SIGCONT command with kill() to -pid (i.e. all child
processes with the job id -pid) and update the job list. This continues the processes in the group,
which is the intention of bg. Finally, for "fg" we again error bad user input to make sure the
command is in the form "fg %d" where d is an integer corresponding to a valid job id in the jobs
list. First, we set control of window to the child to receive user input. Then, like with bg, we
use kill() to sends SIGCONT to all processes in process group −pid and update the jobs list. But,
since the process is now in control of the window receiving using input and in foreground, we must
call waitpit() like in reaping to wait for changes in status, including from using input in the
command line like stopping or terminating signals, and handle the different cases appropriately,
again, in similar fashion as in reaping explained above. If the process exits normally, for
example, nothing is printed since it is a foreground process, and the job is removed from the jobs
list. Again, run_command() returns 1 only if none of these built-ins are specified, and 0 otherwise
(i.e. correct built-in execution or an error occurred in the process).

Child Processes from Shell 1:
Back in main(), an if statement checks whether or not a built-in was encountered depending on what
is returned by run_command(). And if no built-ins were hit, then we execute the run_child_process()
helper. This method first changes the cmd_arg slightly, keeping a pointer to the full filepath,
found in cmd_arg[0], but using strrchr() finds the name after the last "\" in the filepath (i.e the
executable itself) and sets that as the new cmd_arg[0]. Again, this is done to conform to execv(),
which takes two arguments, a pointer to the full filepath, and cmd_arg as it is now. Next, the
parent process is forked into a child process with fork() (as shown in lecture). In the child
process, first the redirection flags are checked. If the input flag is true, then we close(0),
closing the standard input, and open redirect_arr[0], which stores the name of the input file set
in check_redirects(). We also set option to O_RDONLY (as we just read from this file). And since
the lowest integer filedescriptor, 0, was just closed by close(0), open() sets the input file from
redirect_arr[0] as the new "0" standard input filedescriptor, as "<" specifies. The same logic is
generalized for output, which is switched to redirect_arr[1] or redirect_arr[2] depending on if
">" (output) or ">>" (output append) was used. Options and the mode are also set as needed
depending on if ">" or ">>" were called. For example, O_TRUNC deletes all previous contents of the
file (i.e. for ">") while O_APPEND appends output onto the previous contents of the file (i.e. for
">>"). Finally, after the appropriate files are opened and closed, execv() is called with the full
path and the new cmd_arg array. If there is an error then perror() prints an informative message
and the process exits. If all goes smoothly, since execv() replaces the entire child stack with the
new program, then after execv() executes we simply return out to the parent process, which has been
waiting for it to terminate via calling wait() outside of the if statement. wait() is error-checked
and if there are no issues, run_child_process() returns to main(), which is now at the end of our
while(1) loop. And next, we simply loop of our loop in the REPL to read the next input from the
user.

Child Processes additions from Shell 2:
Although the main code is the same from shell 1 in the run_child_process() helper, I also had to
add a few things to handle fg/bg processes, jobs, signaling, etc. in shell 2. The first addition is
after forking the child process. Here, we get the pid of the child and with setpgid(), we set the
process group id (and all of its children) to be that of the current child process. Then, if the
background flag not set (from way back in tokenizing) we transfer control to the child process.
This allows us to support multiple processes, because if the background flag IS set, then control
of the window and user input never leaves the shell, meaning that we can run another job right
away. And unless that next job is a foreground job, we can continue running more jobs in back.
Finally, we set all the signals (SIGINT, SIGTSTP, SIGQUIT, and SIGTTOU) which we were ignoring back
to default, as we do not want to ignore these in the child process. Next, things proceed as before
until we return out of the fork. Now, if the job is a background process, we adds it with all the
relevant information to jobs list and print an informative message. Otherwise, like with reaping
and fg, we use waitpid() to see if either the foreground child process was stopped or terminated by
a signal (with the WUNTRACED flag), and did not just exit normally. If it stopped, that the child
process gets added to the jobs list. In both cases, an informative message is printed out. And
finally control of the window is transfer back from the child to the parent (i.e. the shell) to
again loop back to prompt the user and accept user input.
