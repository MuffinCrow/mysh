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

cmd_Which- is a built in command function that replaces the normal "which" command to find the path to a program. This implementation searches the three directories("/usr/local/bin", "/usr/bin", "/bin") or if given a path, searches to see if that path exists, and returns the path to the program within the directories or the path given if it exists.
parseTokens - Is a function that parses through the list of tokens, removes white spaces, and separates special characters. It places each parsed Token into an array and returns this array as well as the amount of tokens that have been added to the array.
create_Node- is a function that utilizes parseTokens to create our struct based on the information given by the line after being parsed through. It contains a recursive call to itself to add the possibility of creating dynamic piping.
mode_Loop - is the input loop that is given a flag within main and utilizes this flag to choose whether to run through the loop for batch or interactive mode. It does this until it receieves an "exit" line command and closes out the program.
main - based on the argument, calls the input loop and passes it the flag that is utilized and any argument received along with it.
*****Testing*****


*****Errors*****
