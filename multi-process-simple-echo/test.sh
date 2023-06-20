#! bash/bin
# Usage: bash test.sh

# compile the file 
gcc -o server conc_tcp_srv_echo.c 
gcc -o client conc_tcp_cli_echo.c

# tar the file
tar -cvf conc_tcp_cs_echo.tar conc_tcp_srv_echo.c conc_tcp_cli_echo.c