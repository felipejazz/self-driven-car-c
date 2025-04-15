# Self-Driving Car Simulation

This project is a C++ simulation of self-driving cars using a neural network for decision-making and SFML for graphics visualization. The simulation includes features like AI-controlled cars, obstacles, road rendering, neural network visualization, and a basic game structure with menus and pause functionality.

## Features

* **Car Simulation:** Simulates car physics including speed, acceleration, friction, and turning.
* **AI Control:** Cars can be controlled by an AI powered by a simple feed-forward neural network.
* **Sensor Simulation:** Cars are equipped with ray sensors to detect road borders and obstacles. Sensor readings feed into the neural network.
* **Collision Detection:** Detects collisions between cars, obstacles, and road borders.
* **Neural Network:** Implements a basic neural network structure with customizable layers. Supports mutation for genetic algorithm-based training and saving/loading the network state.
* **Visualization:**
    * Renders the road, cars, and obstacles using SFML.
    * Visualizes the neural network structure and activation levels.
    * Includes graphs for tracking fitness and mutation rate over generations.
* **Game Structure:** Managed by a `Game` class, including states for Menu, Simulation, and Help.
* **Obstacle Generation:** Dynamically generates obstacles for the cars to navigate.
* **Genetic Algorithm Elements:**
    * Trains AI cars over multiple generations.
    * Calculates car fitness based on distance traveled, survival, lane changes, and obstacle avoidance.
    * Saves the best performing brain from each generation.
    * Applies mutation to subsequent generations based on the best brain.

## Key Components

* **`Game`**: Manages the overall application flow, views, game states, and simulation loop.
* **`Car`**: Represents a car with physics, controls, sensor, and optional neural network brain. Calculates fitness based on performance.
* **`Road`**: Defines the road geometry, including lanes and borders.
* **`Sensor`**: Simulates car sensors by casting rays and detecting intersections with borders and obstacles.
* **`Obstacle`**: Represents objects on the road that cars must avoid.
* **`Network`/`NeuralNetwork`**: Implements the neural network logic, including levels (layers), weights, biases, feed-forward, and mutation.
* **`Level`**: Represents a single layer within the neural network.
* **`Visualizer`**: Handles the drawing of the neural network and graphs.
* **`Controls`**: Manages the car's movement inputs (forward, reverse, left, right), supporting manual (`KEYS`) and AI control.
* **`Utils`**: Provides utility functions like linear interpolation (`lerp`), intersection calculations, and random number generation.

## Dependencies

* **SFML >= 3.0**: Required for graphics, windowing, and system functionalities. (Components: Graphics, Window, System)
* **CMake >= 3.10**: Used for building the project.
* **C++17 Compiler**: The project uses C++17 features.

## Building

1.  **Ensure Dependencies:** Make sure you have CMake and SFML 3+ development libraries installed on your system.
2.  **Navigate:** Open a terminal or command prompt in the project's root directory (where the main `CMakeLists.txt` is located).
3.  **Create Build Directory:** `mkdir build && cd build`
4.  **Configure:** `cmake ..`
5.  **Build:** `make` (or your chosen build system command, e.g., `ninja`)
6.  **Assets:** The build process attempts to copy the `assets` directory to the build folder. Ensure the `assets` folder exists in the project root.

## Running

After building, the executable `self_driving_car` will be located in the `build` directory. Run it from within the `build` directory to ensure it can find the copied `assets`:

```bash
./self_driving_car