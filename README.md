# TcpFileTransferForLinux

北邮计网大作业实验二，需要的同学自取即可。

A little program that should work well on linux, written in purely C.
This program is used for educational purposes, so dont take it seriously. I just code for fun:)
It includes two files, client1.c & server1.c

Before your compiling, dont forget to change the sharefolder variable in server1.c to your own path.
After compiling these two files, run ./server to start the server process.
run ./client \<serverip\> \<filename\> \<savepath\> to start the client process. 

The client sends the filename to server, then server transfer the corresponding file in the sharefolder to the client.Ez to understand, right?

That's all, for any bugs or suggestions, dont hesitate to contact me. I'll fix them if available.
