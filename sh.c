#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "jobs.h"

/*
 * count_tokens() - counts the number of tokens in buffer
 *
 * Parameters:
 *	- buffer: a char* to the start of the user input buffer
 *
 * Returns:
 *	- an integer representing the number of tokens in the buffer
 */
int count_tokens(char* buffer){
  /* if leading whitespace, buffer incremented */
  size_t lead_whitespace = strspn(buffer, "\t ");
  buffer += lead_whitespace;
  /* uses strcspn and strspn to cout tokens based delimiters in user input */
  int num_tok = 0;
  size_t i1 = 0;
  while(strcspn((buffer + i1), "\t ") != 0){
    size_t plus = strcspn((buffer + i1), "\t ");
    size_t num_delim = strspn((buffer + i1 + plus), "\t ");
    i1 += (plus + num_delim);
    num_tok++;
  }
  return num_tok;
}

/*
 * tokenize() - tokenizes the buffer by filling a string array of the buffer's tokens
 *
 * Parameters:
 *	- arr: an array of strings (char**) to hold all the tokens (including redirection)
 *         from the buffer
 *	- buffer: a char* to the start of the user input buffer
 *
 * Returns:
 *	- nothing (void) - the char** should be set
 */
void tokenize(char** arr, char* buffer){
  char* argument = strtok(buffer, "\t ");
  int i2 = 0;
  while (argument != NULL){
    arr[i2] = argument;
    argument = strtok(NULL, "\t ");
    i2++;
  }
}

/*
 * check_redirects() - checks the token (string) array for redirection and handles appropriately,
 *   if it is the first occurance of input or output and not the last token, then sets an integer
 *   flag corresponding to the redirection option, adds the next token to the redirection array,
 *   and sets appropriate locations in the token array to NULL
 *
 * Parameters:
 *  - num_tokens: an integer representing the number of tokens in the user input
 *  - redirect_input: an int* for the redirect input flag, 1 if true and 0 else
 *  - redirect_output: an int* for the redirect output flag, 1 if true and 0 else
 *  - redirect_output_append: an int* for the redirect append flag, 1 if true and 0 else
 *	- redirect_arr: an array of strings (char**) to hold all the tokens representing the location
 *                  of the redirection
 *	- alltok_arr: an array of strings (char**) holding all the tokens (including redirection)
 *                from the buffer
 *
 * Returns:
 *	- an integer, 1 if there was an error in parsing redirection, and 0 if redirects parsed
 *    correctly or there was no redirections
 */
int check_redirects(int num_tokens, int* redirect_input, int* redirect_output,
  int* redirect_output_append, char** redirect_arr, char** alltok_arr){
  for(int i = 0; i < num_tokens; i++){
    /* checks for "<" (i.e. red. input), checks first occurence, checks not last token, adds
       next element to redirect_arr, and sets locations i and i + 1 to NULL */
    if (!strcmp(alltok_arr[i], "<")){
      if (*redirect_input){
        fprintf(stderr, "ERROR - Can't have two input redirects on one line.\n");
        return 1;
      } else {
        if (i == num_tokens - 1) {
        fprintf(stderr, "ERROR - No redirection file specified.\n");
        return 1;
        }
        *redirect_input = 1;
        alltok_arr[i] = NULL;
        redirect_arr[0] = alltok_arr[i + 1];
        alltok_arr[i + 1] = NULL;
        i++;
        continue;
      }
    }
    /* checks for ">" (i.e. red. output), checks first occurence, checks not last token, adds
       next element to redirect_arr, and sets locations i and i + 1 to NULL */
    if (!strcmp(alltok_arr[i], ">")){
      if (*redirect_output || *redirect_output_append){
        fprintf(stderr, "ERROR - Can't have two output redirects on one line.\n");
        return 1;
      } else {
        if (i == num_tokens - 1) {
          fprintf(stderr, "ERROR - No redirection file specified.\n");
          return 1;
        }
        *redirect_output = 1;
        alltok_arr[i] = NULL;
        redirect_arr[1] = alltok_arr[i + 1];
        alltok_arr[i + 1] = NULL;
        i++;
        continue;
      }
    }
    /* checks for ">>" (i.e. red. append), checks first occurence, checks not last token, adds
       next element to redirect_arr, and sets locations i and i + 1 to NULL */
    if (!strcmp(alltok_arr[i], ">>")){
      if (*redirect_output || *redirect_output_append){
        fprintf(stderr, "ERROR - Can't have two output redirects on one line.\n");
        return 1;
      } else {
        if (i == num_tokens - 1) {
          fprintf(stderr, "ERROR - No redirection file specified.\n");
          return 1;
        }
        *redirect_output_append = 1;
        alltok_arr[i] = NULL;
        redirect_arr[2] = alltok_arr[i + 1];
        alltok_arr[i + 1] = NULL;
        i++;
        continue;
      }
    }
  }
  return 0;
}

/*
 * run_command() - performs the built-in functions cd, ln, rm, exit, jobs, bg, and fg as instructed
 *                 in the pdf, also error checks for bad input or if system calls did not return
 *                 correctly
 *
 * Parameters:
 *  - num_args: the number of arguments in the user input (not including redirections)
 *	- cmd_arg: an array of strings (char**) to hold all the tokens representing the arguments to
 *             the command (again not including redirection)
 *  - j_list: a job_list_t representing the list of current background jobs, contatining job ID,
 *            process ID, command, and state
 *
 * Returns:
 *	- an integer, 1 no built-in was found in the argument array, and 0 if the built-in was executed
 *    or at least attempted (and threw an error)
 */
int run_command(int num_args, char** cmd_arg, job_list_t* j_list){
  /* handles cd built-in */
  if (!strcmp(cmd_arg[0], "cd")){
    if (num_args >= 2){
      int val1 = chdir(cmd_arg[1]);
      if (val1 == -1){
        perror("cd");
        cleanup_job_list(j_list);
        exit(1);
      }
      return 0;
    } else{
      fprintf(stderr, "cd: syntax error\n");
      return 0;
    }
  }
  /* handles ln built-in */
  if (!strcmp(cmd_arg[0], "ln")){
    if (num_args >= 3){
      int val2 = link(cmd_arg[1], cmd_arg[2]);
      if (val2 == -1){
        perror("ln");
        cleanup_job_list(j_list);
        exit(1);
      }
      return 0;
    } else{
      fprintf(stderr, "ln: syntax error\n");
      return 0;
    }
  }
  /* handles rm built-in */
  if (!strcmp(cmd_arg[0], "rm")){
    if (num_args >= 2){
      int val3 = unlink(cmd_arg[1]);
      if (val3 == -1){
        perror("rm");
        cleanup_job_list(j_list);
        exit(1);
      }
      return 0;
    } else{
      fprintf(stderr, "rm: syntax error\n");
      return 0;
    }
  }
  /* handles exit built-in */
  if (!strcmp(cmd_arg[0], "exit")){
    if (num_args >= 1){
      cleanup_job_list(j_list);
      exit(0);
    }
  }
  /* handles jobs built-in */
  if (!strcmp(cmd_arg[0], "jobs")){
    if (num_args >= 1){
      jobs(j_list);
      return 0;
    }
  }
  /* handles bg built-in */
  if (!strcmp(cmd_arg[0], "bg")){
    if (num_args >= 2){
      if(*cmd_arg[1] == '%'){
        /* checks if job exists */
        char* job_num_char = cmd_arg[1];
        job_num_char++;
        int job_num_int = atoi(job_num_char);
        pid_t pid = get_job_pid(j_list, job_num_int);
        if (pid == -1){
          fprintf(stderr, "job not found\n");
          return 0;
        } else {
          /* sends SIGCONT to all processes in process group −pid */
          if (kill(-pid, SIGCONT) == -1){
            perror("kill");
            cleanup_job_list(j_list);
            exit(1);
          }
          update_job_pid(j_list, pid, _STATE_RUNNING);
        }
      } else {
        fprintf(stderr, "bg: job input does not begin with %%\n");
        return 0;
      }
      return 0;
    } else {
      fprintf(stderr, "bg: syntax error\n");
      return 0;
    }
  }
  /* handles fg built-in */
  if (!strcmp(cmd_arg[0], "fg")){
    if (num_args >= 2){
      if(*cmd_arg[1] == '%'){
        /* checks if job exists */
        char* job_num_char = cmd_arg[1];
        job_num_char++;
        int job_num_int = atoi(job_num_char);
        pid_t pid = get_job_pid(j_list, job_num_int);
        if (pid == -1){
          fprintf(stderr, "job not found\n");
          return 0;
        } else {
          pid_t pid_shell = getpid();
          /* sets control of window to the child to recieve user input */
          if(tcsetpgrp(0, pid) == -1){
            perror("tcsetpgrp");
            cleanup_job_list(j_list);
            exit(1);
          }
          /* sends SIGCONT to all processes in process group −pid */
          if(kill(-pid, SIGCONT) == -1){
            perror("kill");
            cleanup_job_list(j_list);
            exit(1);
          }
          update_job_pid(j_list, pid, _STATE_RUNNING);
          /* calls waitpid on child process to examine change in state, and handle appropriately */
          int wret, status;
          wret = waitpid(pid, &status, WUNTRACED);
          if (wret == -1){
            fprintf(stderr, "ERROR - Child process did not execute properly.\n");
            cleanup_job_list(j_list);
            exit(1);
          }
          /* if child process terminates normally */
          if (WIFEXITED(status)){
            remove_job_jid(j_list, job_num_int);
          }
          /* if child process terminates with a signal */
          if (WIFSIGNALED(status)){
            int sig_exit_st = WTERMSIG(status);
            remove_job_jid(j_list, job_num_int);
            if (printf("[%d] (%d) terminated by signal %d\n", job_num_int, pid, sig_exit_st) < 0){
              fprintf(stderr, "ERROR - Message did not print successfully.\n");
              cleanup_job_list(j_list);
              exit(1);
            }
          }
          /* if child process stopped by a signal */
          if (WIFSTOPPED(status)){
            update_job_jid(j_list, job_num_int, _STATE_STOPPED);
            int signal_num = WSTOPSIG(status);
            if (printf("[%d] (%d) suspended by signal %d\n", job_num_int, pid, signal_num) < 0){
              fprintf(stderr, "ERROR - Message did not print successfully.\n");
              cleanup_job_list(j_list);
              exit(1);
            }
          }
          /* return control to the shell */
          if(tcsetpgrp(0, pid_shell) == -1){
            perror("tcsetpgrp");
            cleanup_job_list(j_list);
            exit(1);
          }
        }
      } else {
        fprintf(stderr, "fg: job input does not begin with %%\n");
        return 0;
      }
      return 0;
    } else {
      fprintf(stderr, "fg: syntax error\n");
      return 0;
    }
  }
  /* returns 1 only if first comand line argument is not an implemented built-in */
  return 1;
}

/*
 * run_child_process() - forks the parent process into a child proceess in order to run an
 *                       command, checking for redirection and opening and closing i/o files as
 *                       needed, execues command, and returns back to parent process
 *
 * Parameters:
 *  - redirect_input: an int for the redirect input flag, 1 if true and 0 else
 *  - redirect_output: an int* for the redirect output flag, 1 if true and 0 else
 *  - redirect_output_append: an int* for the redirect append flag, 1 if true and 0 else
 *	- redirect_arr: an array of strings (char**) to hold all the tokens representing the location
 *                  of the redirection
 *	- cmd_arg: an array of strings (char**) to hold all the tokens representing the arguments to
 *             the command (again not including redirection)
 *  - background_process: an int flag for if its is a background process, 1 if true and 0 else
 *  - j_list: a job_list_t representing the list of current background jobs, contatining job ID,
 *            process ID, command, and state
 *  - jid: an int* representing the current job id, which gets incremented by 1 on each new job
 *
 * Returns:
 *	- nothing (void) - executes the child process and returns
 */
void run_child_process(int redirect_input, int redirect_output, int redirect_output_append,
  char** redirect_arr, char** cmd_arg, int background_process, job_list_t* j_list, int* jid){
  /* keeps pointer to full path, changes path pointer in command array to just the executable */
  char* full_path = cmd_arg[0];
  char* last_in_path = strrchr(cmd_arg[0], '/');
  if (last_in_path != '\0'){
    last_in_path++;
  }
  cmd_arg[0] = last_in_path;
  /* forks child process */
  pid_t pid_child;
  pid_t pid_parent = getpid();
  if ((pid_child = fork()) == 0){
    /* set's process group id to be that of the calling process, transfer control if not
       a background process */
    pid_t actual_pid_child = getpid();
    if (setpgid(actual_pid_child, actual_pid_child) == -1){
      perror("setpgid");
      cleanup_job_list(j_list);
      exit(1);
    }
    if (!background_process){
      if (tcsetpgrp(0, actual_pid_child) == -1){
        perror("tcsetpgrp");
        cleanup_job_list(j_list);
        exit(1);
      }
    }
    /* sets the signal ignores back to default handling in the child */
    if (signal(SIGINT, SIG_DFL) == SIG_ERR){
      perror("signal");
      cleanup_job_list(j_list);
      exit(1);
    }
    if (signal(SIGTSTP, SIG_DFL) == SIG_ERR){
      perror("signal");
      cleanup_job_list(j_list);
      exit(1);
    }
    if (signal(SIGQUIT, SIG_DFL) == SIG_ERR){
      perror("signal");
      cleanup_job_list(j_list);
      exit(1);
    }
    if (signal(SIGTTOU, SIG_DFL) == SIG_ERR){
      perror("signal");
      cleanup_job_list(j_list);
      exit(1);
    }
    /* checks if "<" used, handles appropriately */
    if(redirect_input){
       if (close(0) == -1){
         perror("close");
         cleanup_job_list(j_list);
         exit(1);
       }
       if (open(redirect_arr[0], O_RDONLY) == -1){
         perror("open");
         cleanup_job_list(j_list);
         exit(1);
       }
    }
    /* checks if ">" used, handles appropriately */
    if(redirect_output){
      if (close(1) == -1){
        perror("close");
        cleanup_job_list(j_list);
        exit(1);
      }
      if (open(redirect_arr[1], O_WRONLY | O_CREAT | O_TRUNC, 0666) == -1){
        perror("open");
        cleanup_job_list(j_list);
        exit(1);
      }
    }
    /* checks if ">>" used, handles appropriately */
    if(redirect_output_append){
      if (close(1) == -1){
        perror("close");
        cleanup_job_list(j_list);
        exit(1);
      }
      if (open(redirect_arr[2], O_WRONLY | O_CREAT | O_APPEND, 0666) == -1){
        perror("open");
        cleanup_job_list(j_list);
        exit(1);
      }
    }
    /* executes child process replacing old stack */
    execv(full_path, cmd_arg);
    /* we won't get here unless execv failed */
    perror("execv");
    cleanup_job_list(j_list);
    exit(1);
  }
  /* adds job to jobs list if background process and prints */
  if (background_process) {
    *jid = *jid + 1;
    add_job(j_list, *jid, pid_child, _STATE_RUNNING, full_path);
    if (printf("[%d] (%d)\n", *jid, pid_child) < 0){
      fprintf(stderr, "ERROR - Message did not print successfully.\n");
      cleanup_job_list(j_list);
      exit(1);
    }
    if (tcsetpgrp(0, pid_parent) == -1){
      perror("tcsetpgrp");
      cleanup_job_list(j_list);
      exit(1);
    }
  } else {
    /* if not waits for changes in status */
    int wret, status;
    wret = waitpid(pid_child, &status, WUNTRACED);
    if (wret == -1){
      fprintf(stderr, "ERROR - Child process did not execute properly.\n");
      cleanup_job_list(j_list);
      exit(1);
    }
    /* if process stopped by a signal */
    if (WIFSTOPPED(status)){
      int signal_num = WSTOPSIG(status);
      *jid = *jid + 1;
      add_job(j_list, *jid, pid_child, _STATE_STOPPED, full_path);
      if (printf("[%d] (%d) suspended by signal %d\n", *jid, pid_child, signal_num) < 0){
        fprintf(stderr, "ERROR - Message did not print successfully.\n");
        cleanup_job_list(j_list);
        exit(1);
      }
    }
    /* if process terminated with a signal */
    if (WIFSIGNALED(status)){
      int signal_num = WTERMSIG(status);
      *jid = *jid + 1;
      if (printf("[%d] (%d) terminated by signal %d\n", *jid, pid_child, signal_num) < 0){
        fprintf(stderr, "ERROR - Message did not print successfully.\n");
        cleanup_job_list(j_list);
        exit(1);
      }
    }
    /* transfer control back to shell */
    if (tcsetpgrp(0, pid_parent) == -1){
      perror("tcsetpgrp");
      cleanup_job_list(j_list);
      exit(1);
    }
  }
}

/*
 * reap() - reaps the job list by iterating through jobs and checking for changes in
 *          state or termination
 *
 * Parameters:
 *  - j_list: a job_list_t representing the list of current background jobs, contatining job ID,
 *            process ID, command, and state
 *
 * Returns:
 *	- nothing (void) - reaps the job list and returns
 */
void reap(job_list_t* j_list){
  pid_t next_pid = get_next_pid(j_list);
  /* iterates through job list with while loop */
  while(next_pid != -1){
    /* waits for changes in status */
    int job_id = get_job_jid(j_list, next_pid);
    int wret, status;
    /* note: if wret = 0 then status has not changed, and we move onto next job */
    if ((wret = waitpid(next_pid, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0){
      /* if process exits normally */
      if (WIFEXITED(status)){
        int exit_st = WEXITSTATUS(status);
        remove_job_jid(j_list, job_id);
        if (printf("[%d] (%d) terminated with exit status %d\n", job_id, next_pid, exit_st) < 0){
          fprintf(stderr, "ERROR - Message did not print successfully.\n");
          cleanup_job_list(j_list);
          exit(1);
        }
      }
      /* if process terminated with a signal */
      if (WIFSIGNALED(status)){
        int sig_exit_status = WTERMSIG(status);
        remove_job_jid(j_list, job_id);
        if (printf("[%d] (%d) terminated by signal %d\n", job_id, next_pid, sig_exit_status) < 0){
          fprintf(stderr, "ERROR - Message did not print successfully.\n");
          cleanup_job_list(j_list);
          exit(1);
        }
      }
      /* if process stopped by a signal */
      if (WIFSTOPPED(status)){
        update_job_jid(j_list, job_id, _STATE_STOPPED);
        int signal_num = WSTOPSIG(status);
        if (printf("[%d] (%d) suspended by signal %d\n", job_id, next_pid, signal_num) < 0){
          fprintf(stderr, "ERROR - Message did not print successfully.\n");
          cleanup_job_list(j_list);
          exit(1);
        }
      }
      /* if process continued by a signal */
      if (WIFCONTINUED(status)){
        update_job_jid(j_list, job_id, _STATE_RUNNING);
        if (printf("[%d] (%d) resumed\n", job_id, next_pid) < 0){
          fprintf(stderr, "ERROR - Message did not print successfully.\n");
          cleanup_job_list(j_list);
          exit(1);
        }
      }
    } else {
      if (wret == -1){
        fprintf(stderr, "ERROR - Child process did not execute properly.\n");
        cleanup_job_list(j_list);
        exit(1);
      }
    }
    next_pid = get_next_pid(j_list);
  }
}

/* executes shell */
int main() {
  /* instantiates job list and job id */
  job_list_t* j_list = init_job_list();
  int jid = 0;
  /* ignore these signals in the shell */
  if (signal(SIGINT, SIG_IGN) == SIG_ERR){
    perror("signal");
    cleanup_job_list(j_list);
    exit(1);
  }
  if (signal(SIGTSTP, SIG_IGN) == SIG_ERR){
    perror("signal");
    cleanup_job_list(j_list);
    exit(1);
  }
  if (signal(SIGQUIT, SIG_IGN) == SIG_ERR){
    perror("signal");
    cleanup_job_list(j_list);
    exit(1);
  }
  if (signal(SIGTTOU, SIG_IGN) == SIG_ERR){
    perror("signal");
    cleanup_job_list(j_list);
    exit(1);
  }
  /* create REPL loop */
  while(1){
    /* reap the jobs list */
    reap(j_list);
    /* instantiates buffer */
    char buffer[1024];
    /* prompts user for input */
    #ifdef PROMPT
    if (printf("33sh> ") < 0){
      fprintf(stderr, "ERROR - Prompt did not print successfully.\n");
      cleanup_job_list(j_list);
      exit(1);
    }
    fflush(stdout);
    #endif
    /* reads in user input */
    ssize_t count = read(STDIN_FILENO, buffer, 1024);
    buffer[count - 1] = '\0';
    if (count < 0){
      fprintf(stderr, "ERROR - Input not read successfully.\n");
      cleanup_job_list(j_list);
      exit(1);
    }
    /* if user types 'enter', and then count is exactly 1 */
    if (count == 1){
      continue;
    }
    /* if user types ctrl-D */
    if (!count){
      cleanup_job_list(j_list);
      exit(0);
    }
    /* figures out number of tokens in input, if just whitespace restarts loop */
    int num_tokens = count_tokens(buffer);
    if (!num_tokens){
      continue;
    }
    /* stores all tokens in an array */
    char* alltok_arr[num_tokens];
    tokenize(alltok_arr, buffer);
    /* sets background flag if last token is & */
    int background_process = 0;
    if (!strcmp(alltok_arr[num_tokens-1], "&")){
      background_process = 1;
      alltok_arr[num_tokens-1] = '\0';
      num_tokens--;
    }
    /* error checking for no command */
    if ((*alltok_arr[0] == '<' || *alltok_arr[0] == '>') && num_tokens <= 2){
      fprintf(stderr, "ERROR - No command.\n");
      continue;
    }
    /* instantiates redirection int flags and array for filenames */
    int redirect_input = 0;
    int redirect_output = 0;
    int redirect_output_append = 0;
    char* redirect_arr[3];
    /* sets int flag pointers to pass into redirect function */
    int* r_i = &redirect_input;
    int* r_o = &redirect_output;
    int* r_oa = &redirect_output_append;
    /* checks for redirects */
    int ch_red_error = check_redirects(num_tokens, r_i, r_o, r_oa, redirect_arr, alltok_arr);
    if (ch_red_error){
      continue;
    }
    /* finds number of arguments without redirections */
    int num_args = num_tokens;
    if (redirect_input){
      num_args -= 2;
    }
    if (redirect_output || redirect_output_append){
      num_args -= 2;
    }
    /* create command arguments array */
    char* cmd_arg[num_args + 1];
    int j = 0;
    for(int i = 0; i < num_tokens; i++){
      if (alltok_arr[i] != NULL){
        cmd_arg[j] = alltok_arr[i];
        j++;
      }
    }
    cmd_arg[num_args] = '\0';
    /* parse for builtins and execute if exists */
    int val = run_command(num_args, cmd_arg, j_list);
    if (!val){
      continue;
    } else {
      /* no builtins, then try to execute shild process */
      run_child_process(redirect_input, redirect_output, redirect_output_append,
        redirect_arr, cmd_arg, background_process, j_list, &jid);
    }
  }
  return 0;
}
