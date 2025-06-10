Modifies registry keys and launches the game from the same folder.

Build instructions:

Clone this repository:
![image](https://github.com/user-attachments/assets/e1067fa4-0e56-4552-955f-83349d6ccece)

Paste this in and click "Clone" :

https://github.com/danielharton/Reset-UserID-MvL.git

![image](https://github.com/user-attachments/assets/6fc2b32b-595b-4762-b1e2-77b4d83a0ddb)

Change the build to "Release" for more optimized machine code

![image](https://github.com/user-attachments/assets/954b1958-d9a9-4c49-88b5-426fbd7889f8)

(Already done) Go into Project Properties > Linker > System

SubSystem should be "Windows (/SUBSYSTEM:WINDOWS)" for the application when running to not bring a console window visible to the user. Then click "Apply"
![image](https://github.com/user-attachments/assets/72a0aedf-5d69-43f6-ab74-21a9574583b9)

