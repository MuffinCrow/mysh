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



*****Testing*****


*****Errors*****