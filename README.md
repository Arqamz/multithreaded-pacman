# MultithreadedPacman
A multi-threaded implementation of the classic Pac-Man game in C, showcasing advanced synchronization techniques for seamless gameplay.

Separate threads for the pacman, each of the ghosts, highscore check and for the win condition. These were synchronised without the use of explicit waiting mechanism (e.g. Thread – join or similar) to achieve concurrency/synchronization. Only synchronisation primitives such as semaphores and mutexes were used to achieve synchronicity.
