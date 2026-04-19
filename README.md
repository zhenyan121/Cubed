## What's it
A cube game like Minecraft, using C++ and OpenGL.  

## Build Guide  
### Prerequisites  
- **CMake** (>= 3.24)
- **C++ 23** compatible compiler
### Step 
1. Clone the repository  
   ```bash
   git clone https://github.com/zhenyan121/Cubed.git && cd Cubed
   ```
2. Configure with CMake
   ```bash
   mkdir build && cd build
   cmake -G "Ninja" ..
   ```
3. Build the project  
   ```bash
   ninja
   ```