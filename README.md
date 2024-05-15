# MultithreadedPacman
A multi-threaded implementation of the classic Pac-Man game in C, showcasing advanced synchronization techniques for seamless gameplay.

Separate threads for the pacman, each of the ghosts, highscore check and for the win condition. These were synchronised without the use of explicit waiting mechanism (e.g. Thread – join or similar) to achieve concurrency/synchronization. Only synchronisation primitives such as semaphores and mutexes were used to achieve synchronicity.


## Acknowledgements
This project is heavily based on [charlesrw1's Pacman repository](https://github.com/charlesrw1/Pacman/), which served as the foundation for my implementation. While the core logic for drawing and ghost movement was adapted from the referenced repository, I have made several modifications and additions to tailor the project to my specific needs.

It's important to acknowledge the contributions and inspiration from the original work. I express our gratitude to the contributors of the mentioned Pacman repository for providing a starting point for our project.
