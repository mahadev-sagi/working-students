To start setting up, use homebrew installation manager

run "brew install pistache" 

then compile the server.cpp file and see if it works using this command. 

g++ server.cpp -std=c++17 -lpistache -pthread

or run this to keep executable in build folder

g++ src/server.cpp -std=c++17 -lpistache -pthread -o build/server


then run server using either "./server" or "./build/server" depending on the command you use

open this address to verify that "hello, world " has appeared, this is the server address

http://localhost:9080