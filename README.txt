Members:
Devon Botney - dcb200
Devin Tran - adt85

*****Methodology*****

For this assignment, the program was split over multiple functions. After checking for arguments in main,
the program would move to a function that would work for both batch and interactive mode, either reading
commands from a file or accepting them from the terminal. Once a command was entered, it is then sent
to a seperate function to parse through the input and sort it into a struct that holds info regarding
input, output, whether the command successfully executed, arguments, the command itself, etc. This struct
is then sent to new function that will decide whether or not piping was included in the input and would act
accordingly. When a command was to be executed, a call to the last main function would happen and the node/struct
would be input into commandExec where the command would be executed.

*****Functions******

cwdGrabber is a function that grabs the current working directory (cwd) and stores it in a global variable
called cwd which will then be used in commandExec to print. cwdGrabber is the built-in function replacement for
the terminal command pwd.

changeDirectory is a function that reads the arguments in the given struct and determines what path the user
wants to move to. Once it decides where the user wants to move, it then uses chdir to change the current directory.
changeDirectory is the built-in function replacement for the terminal command cd.

commandExec is the largest function in the program and while there are some issues that still need to be worked out,
it fulfills most requirements for normal usage. commandExec, when given a node, the function will decide what terminal
command needs to be executed and will handle things like redirection, then, and else when determining how to execute
a command.

pipeORexec is a function that is fed nodes and determines whether the user is attempting to pipe or not. If the user is
not piping, the function immediately sends the node to commandExec to be executed. If there is a request for piping,
pipe(), fork(), and dup2() are used in order to redirect the output of command 1 to the input of command 2. This
function works for the most part as the commands on their own execute but there are some issues which will be gone
over in the error section.

cmd_Which- is a built in command function that replaces the normal "which" command to find the path to a program. This implementation searches the three directories("/usr/local/bin", "/usr/bin", "/bin") or if given a path, searches to see if that path exists, and returns the path to the program within the directories or the path given if it exists.
parseTokens - Is a function that parses through the list of tokens, removes white spaces, and separates special characters. It places each parsed Token into an array and returns this array as well as the amount of tokens that have been added to the array.
create_Node- is a function that utilizes parseTokens to create our struct based on the information given by the line after being parsed through. It contains a recursive call to itself to add the possibility of creating dynamic piping.
mode_Loop - is the input loop that is given a flag within main and utilizes this flag to choose whether to run through the loop for batch or interactive mode. It does this until it receieves an "exit" line command and closes out the program.
main - based on the argument, calls the input loop and passes it the flag that is utilized and any argument received along with it.

*****Testing*****

cwdGrabber was straightforward to test. I would create test code in main that would run and make sure that
whatever was being put into cwd[] was the correct working directory. This was tested on more once changeDirectory
was created.

changeDirectory was tested in main() as well with dummy nodes that would run one after the other so that we can
both test to see if changeDirectory was actually changing the directory as well as if cwdGrabber was tracking
these changes correctly.

commandExec was tested through mostly external functions since the built-in testing was already done.
Commands like echo, ls, and cat were mainly used to test this code but an executable that prints "Hello
World!" was also included so that executing files in subdirectories was covered. Many bugs and issues were
found in this specific method and while most were fixed, the ones that were unable to be fixed are outlined
in the errors section.

pipeORexec was tested through executing commands. Testing for this method was more tedious as we found that
there was some issues with piping that will be outlined more in the errors section. We tested for most
basic commands and pipeORexec would correctly send them to commandExec to have them executed. As for testing
the piping itself, we tested to make sure that the first command executed but past that we were having errors
so testing came to a hault.

wildcards- Used asserts after creation of files to check if patterns were matched to find different files.
which - Tested cases checking specific paths and nonexistent paths as well as files such as ls that are located within usr/bin
create_Node- tested using for loops and print statements within them that are currently commented out, This also tested parseTokens as it allowed me to see that I was properly getting the argument list.
mode_Loop and main - tested by running the program using an argument multiple arguments and just ./mysh after running make. Also tested by using an argument of a text file that did not exist to cover that error case. Added print statements for reading empty lines that stated Empty command entered. Also tested by running commands that relied on each other to see if the linked list implementation of the struct worked.

*****Errors*****

The errors we are getting mainly stem from improper child handling, unknown issues with executing commands, and
potential issues redirecting for pipes. Most issues tend to come from commandExec on things like 'echo e l' or
frankly most commands with 2 arguments that aren't the command part of the input. We can't understand why these
won't execute sadly and it also creates issues with children. When an error occurs, the child for some reason never
ends so exit needs to be ran multiple times to exit mysh when an error occurs. This is most likely due to improper
forking but we were unable to figure out the source and cause for this imporper handling. The last problem is for
piping input and output for command 1 and 2 of a pipe. Whenever piping occurs, the terminal freezes like the second
command is waiting for input from the first command. We do not know if this is due to improper forks since we have
nested forks or if it is due to redirecting incorrectly. Either way, we did not have enough time to fully find where
the issue was and rectify it.

When an error occurs there is the chance for commands that would work, to stop working so there is the chance that the terminal
may have to be restarted in order to fully test the code for what works.
