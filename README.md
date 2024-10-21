# Projects in Windows Graphics

## Project list

#### 2D
1. BouncingBalls
2. RotatingNet
3. GooglyEyes
4. Clock
5. **AsteroiDoom**

#### 3D
1. Tree
2. **Maze**

## Running

In order to run the projects, you have to compile the executable, and then run it. The second part is easy, but the first... oh boy.

The most convenient way I can get it to work is the following.

1. Be on Windows (obviously).
2. Install Visual Studio (don't worry, we won't use it).
   1. During installation, make sure to select the "Game development with C++" workload.
3. Open CLion (an IDE that actually isn't painful).
4. Open the repository folder.
5. Open Settings -> Build, Execution, Deployment -> Toolchains.
6. Add a Visual Studio toolchain and **move it to the top of the list**.
7. Now that CMake will actually find the DirectX libraries, select the CMake file of a project and load it.
8. Build and run the project; enjoy.

## Featured projects

### AsteroiDoom

My proudest creation is very simple, but is worth sharing because
- it actually does something interesting (and is able to remain bug-free!), and
- it's kind of fun.

Here's a showcase:

![AsteroiDoom](https://github.com/user-attachments/assets/5b838529-bce8-4e3c-ad4c-05b0a0a529d8)

The two things that I am proud of is:

#### 1. Looped playing field with seamless spawns

The playing field is perfectly looped, and when the player's ship or an asteroid crosses the edge, it appears on the other side. If it is in the middle of crossing the edge, it will be visible in both places at once (or maybe even four places, if in the corner!). This is achieved by spawning 9 copies of each object and displaying them all at the same time in 9 copies of the playing field arranged in a 3x3 grid. This way, any teleports are neatly hidden from the player.

But wait! What about asteroids that are just spawning in? Won't they look like they suddenly teleported in?

This is where another trick comes in, which is that objects are spawned as a single copy in a field that is slightly larger than the playing field. Then, as soon as they appear in their entirety on the screen, they are transferred and copied eight times to the 3x3 grid.

#### 2. Surprisingly simple, smooth acceleration and deceleration

The feeling of accelerating and deceleration may not be the most satisfying across all existing games, but it does feel somewhat intuitive and natural.

This was achieved by doing two things at once. Each frame, if the acceleration button (W) is pressed, the ship speeds up a fixed amount. However, at the same time, each frame, the ship slows down by a fixed factor. This means that the acceleration is linear, but the decelration is exponential, and they smoothly balance each other out.

### Maze

Also known as "Scene" in the project tree, this is just an interactive render of a 3D scene modeled in Blender, with baked graphics. There's a donut at the end of the maze.

![Screenshot 2024-10-21 154702](https://github.com/user-attachments/assets/f4f4fda4-a45a-403e-8fc8-da4110e7d096)
![Screenshot 2024-10-21 154606](https://github.com/user-attachments/assets/e591f148-5183-455c-909e-be5f93cc2ec1)
