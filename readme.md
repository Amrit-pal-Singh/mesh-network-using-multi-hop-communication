# Mesh network using multi-hop communication with ESP32(microcontrollers)

### All the theoretical information is written in the project PPT.<br>
ESP32 with **master_slave** code will send and receive the data.<br>
ESP32 with **slave** code will only receive the data.<br>

We will have layers in our network <br>
The 1st layer having only the root node is connected to the server. <br>
The **data transmission** is like one of the nodes in the nth layer send data to the (n-1)th layer
and which send ahead to next layer until it reaches the root node.

The root node receives all the data from all the layers.<br>
The root node initiates its hotspot with which any device can connect.<br>
After connecting to that hotspot just go to link 192.168.4.1 and you will receive the data.<br>





### To run this code all you need to do is -> 
Add slave.ino to any ESP32 which will be root node.<br>
Add master_slave.ino to all the other ESP32 and change-><br>
1. 183 line -> layer number<br>
2. 184 line -> which node in that layer.<br>
3. 255 and 315 line -> MAC address of ESP32 of next layer.<br>

![mesh](https://user-images.githubusercontent.com/41193564/51616754-c7b6c700-1f50-11e9-9819-c4d700d79b40.png)


This is the output
