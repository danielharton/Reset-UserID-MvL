Modifies registry keys and launches the game from the same folder.

Build instructions:

Clone this repository:
![image](https://github.com/user-attachments/assets/e1067fa4-0e56-4552-955f-83349d6ccece)

Paste github repo in and click "Clone" :

Change the build to "Release" for more optimized machine code


(Already done) Go into Project Properties > Linker > System

SubSystem should be "Windows (/SUBSYSTEM:WINDOWS)" for the application when running to not bring a console window visible to the user. Then click "Apply"

