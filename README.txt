Courtney Beames
V00805419

ReadMe

///FOR P2A:////

1. How do you design and implement your RDP header and header fields? Do you use any additional header fields?

Used a struct for the header fields, with all integers except the "CSC361" (magic), type, and data.
I made a function that turns the header to a string, and one that does the opposite.
I just strcat the header with the filedata and send it to the receiver. For the receiver, i just send the header back.

2. How do you design and implement the connection management using SYN, FIN and RST packets? How to choose the initial sequence number?

I chose a random number for the initial sequence number.
For the SYN packet, I sent it before entering the infinite for loop. The FIN packet I sent when the amount of data sent to the receiver was the same as the initial file size.
For RST, I will terminate the connection with error handling.

3. How do you design and implement the flow control using window size? How to choose the initial window size and adjust the size?
How to read and write the file and how to manage the buffer at the sender and receiver side, respectively?

I chose 10240 as the initial window size. Everytime I read in bytes, I subtracted those bytes from the window size and sent it back to the sender.
I read from the file using fread, and wrote to the file using fputs, so that I could put string by string into the file.
The buffer will increase and decrease based on what is being sent and received.

4. How do you design and implement the error detection, notification and recovery?
How to use timer? How many timers do you use? How to respond to the events at the sender and receiver side, respectively? How to ensure reliable data transfer?

I will see if sender receives an ack, if they dont, then I will resend the packet. I will keep the packets in a queue and remove them from the queue when they are acknowledged.
I will use a timer when I send the packet from the sender and if the timer runs out, I will resend the packet.
To ensure reliable data transfer, I will have error control checking the ack and seq numbers.

5. Any additional design and implementation considerations you want to get feedback from your lab instructor?

Not as of now.

///FOR P2B:///

For p2b, I added to my header struct and added in a timestamp value. I made this a string because that was the easiest way to do it. I would convert my string to a double when I needed to compare it to other times.

I was originally going to do Go Back N for packet loss, but the way I ended up implementing my code made it easier to selectively resend packets.
I only sent 10 packets at a time, and only resent 5 packets at a time. I kept track of how many packets I sent on the sender side and kept track of a window on the receiver side.
The window on the receiver side is originally 10, for 10 packets, and decrements when a packet is received. I then write to the file right away.

I used a timer on the sender and the receiver side. On the sender side, I had a timer for each packet. I started the timer when I sent the packet, and checked if it had timed out during the execution. If it timed out, I resent the packet and changed the timer value. On the receiver side, I start a timer once I send my final ack. I then wait for one round trip time and then quit the program. 

I store each sent packet in an array of header structs, and each time one is acknowledged, I remove it from the list. Once I send the FIN, I check if every packet has been acknowledged, and then I quit the program. 

