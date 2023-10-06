## Description
This is a desktop application that simulates simple robots. The simulator uses threads and allows multiple robots to connect to it, with each robot running as its own process. The robots move forward or turn in fixed increments each time, and there is an environment containing up to a maximum number of robots. The line drawn through the center of each robot indicates what direction it is facing. On collision with the walls or other robots, the robots seek an alternative path that avoids subsequent collisions. Made with C.

## Run Instructions and General Information
Run the make file. \
Run the environmentServer executable to initiate the server. \
Run up to 20 robotClient executables in different terminals to add robots to the screen. \
The line on each robot indicates what direction they are moving in.

## Demo
![Robot-Collision-Simulation](https://user-images.githubusercontent.com/51683551/200935670-e896deb0-3d45-461b-b5be-67952d9fd1c7.gif)
