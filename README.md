# HarpindersMiniShell
This is a miniature shell I made in school.
TEST CASES USED:
-ls
-ls -l
-cat shell.c | grep include
cat chell.c | grep void | wc -l
-ls > output.txt
-grep main < shell.c > mainlines.txt
-sleep 5&
-sleep 10 > sleepOut.txt
-pwd
-cd ..
-pwd
-cd (whatever was printed abpve)
-help
-fakecommand
-exit


        The main loop acts as amy control loop. It initializes a commadn line array to store the users input with the macro MAX COMMAND LENGTH. Then it prints a welcome message. The "while (1)" loop indefinitely displaying the "mini_shell>" prompt until ended. We then read the input with "fgets" into the commandLine buffer. We also then removes the newline character from the end of the input for string processing. The input is first compared to the built in commands (explained later). They include exit, help, pwd, cd, and wait. If the input matches any of the built in commadns the correspinding program is ran. If it doesn't match any of our programs it is assumed that is external. It then checks whether the last argument is &. This measn that it should be running in the background. If so, isbackground gets set to 1 and removes the & from the arguments to prevent it from being passed to an external program. If it is meant to run in the backround, it forks a new process. The child process executes the command using execProg (again explained later), and the parent adds it to a list of background processes. For regular execution, we just call execProg direclty with the arguments and are later freed. It cotinues to loop until an exit is entered.
        For execProg, it is designed to to execute programs. It initliazs variables for tracking child proces status, coutning the number of pipes, ad file descriptors for input and output redirection. File descriptors are initially set to -1 to indicate that they are free to use. It then iterates over the array of command arguments to identify and handle special symbols <,>, and |. For output redirection (>) it opens specified files for writing or truncating. The file descriptors is stored in outFd. For input redirection (<) it opens the specified file for reading and storing the file descriptor in inFd. Each symbol of | found increments the num_pipes counter and then segments the command by null-terminating rthe argument list at the pipe symbol. This allows for execution of each segment as a separate command.Based on te number of pipes identified, an array of pipe file descriptors is initiated. Each entry represeting a pipe with two file descriptors one for reading and the other for writing. The loop iterates for each command segment (determined by the number of pipes plus one). For each iteration, a new child is forked. In that child process input redirection is set up if needed by duplicating the input descriptor (inFd to STDIN_FILENO). Output redirection is set up similarly for output file descriptors. For piped commands, it redirects the output of the curret command to the input of the next command by appropriately duplicating piep file descriptors onto STDIN or STDOUT_FILENO. After setting up redirections and closing unneeded file descriptors, it executes the command using execvp, whcih searches for the executable in the systems PATH if no explicit pth is provided. If execvp fails it prints an errorr. After execution, it closes any remainnig open file descriptors and waits for all child processes to finish for no zombie processes.
        runCd is pretty self explanatory. printPWD is the same, it just prints the current working directory with getcwd in POSIX. printHelp just gets called and then just prints everything.
        wait_for_background_processes is desgined as a helper function to manage and wait fot the completion of background processes. So it loops through a predifined array background_processes, whcih stores information about processes that have been started in the background. his arrai is os the macro size MAX BACKGROUND PROCESSES. Within the loop it checks the entries to see if there is an active process. This is done by checking the pid of the background aray. For each active background process, the function calls waitpid with the processe' pid, waiting for that specific process to finish. The function passes NULL as the second argument to waitpid becuase its not interested in the exit status of the chid process. The thirds argument is set to 0, indicating the wait pid shoudl wait for the child to terminate. Once waitpid returns,a messege gets printed and the pid field of the current slot of the array is set to 0.
        add_background_process is designed to ad a new backgorund process to the global array. The function iterates over background_processes array which is max 10.It finds an empty slot within the loop by checking every entry. And empty slot would be a 0 for the pid. Once an empty slot is found, it sets the pid to the pid of the new process and it copies the command associated with the new background process into the command filed of the slot using strncpy. The MAX COMMAND LENGTH constant makes sure the string is not too big which is the reason for buffer overlfow. The function returns 0 after a succesful addisition. If the array is full it returns a -1. typedef struct is just how you have to store the processes.

