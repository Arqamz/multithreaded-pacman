#include <unistd.h>
#include <SFML/Graphics.hpp>
#include <pthread.h>
#include <iostream> // For cout
#include <mutex> // For mutex

// Object struct
struct Object {
    float x;
    float y;
    sf::RectangleShape shape;
};

// Global variables to store direction
volatile int dir1 = 0; // Direction of object 1
volatile int dir2 = 0; // Direction of object 2

// Global mutex for synchronizing access to object positions
std::mutex positionMutex;

// Create objects
Object object1;
Object object2;

// Function to initialize objects
void initializeObjects() {
    object1.x = 100; // Initial x position of object 1
    object1.y = 300;
    object2.x = 500; // Initial x position of object 2
    object2.y = 300;

    // Set up shapes for objects
    object1.shape.setSize(sf::Vector2f(50, 50));
    object1.shape.setFillColor(sf::Color::Red);
    object1.shape.setPosition(object1.x, object1.y);

    object2.shape.setSize(sf::Vector2f(50, 50));
    object2.shape.setFillColor(sf::Color::Blue);
    object2.shape.setPosition(object2.x, object2.y);
}
// Function to move object 1
void* moveObject1(void* arg) {
    while (true) {
        // Lock the mutex before updating object1's position
        positionMutex.lock();
        // std::cout << "Thread 1: Position mutex locked" << std::endl;
        if (dir1 == 1) {
            // Move object 1 right
            object1.x += 0.0001;
            // std::cout << "Thread 1: Moving object 1 right. New position: " << object1.x << std::endl;
        } else if (dir1 == -1) {
            // Move object 1 left
            object1.x -= 0.0001;
            // std::cout << "Thread 1: Moving object 1 left. New position: " << object1.x << std::endl;
        }
        object1.shape.setPosition(object1.x, object1.y);
        // Unlock the mutex after updating object1's position
        positionMutex.unlock();
        // std::cout << "Thread 1: Position mutex unlocked" << std::endl;
        // Sleep for a short duration to avoid busy waiting
        // sleep(1);
    }
    return nullptr;
}

// Function to move object 2
void* moveObject2(void* arg) {
    while (true) {
        // Lock the mutex before updating object2's position
        positionMutex.lock();
        // std::cout << "Thread 2: Position mutex locked" << std::endl;
        if (dir2 == 1) {
            // Move object 2 right
            object2.x += 0.0001;
            // std::cout << "Thread 2: Moving object 2 right. New position: " << object2.x << std::endl;
        } else if (dir2 == -1) {
            // Move object 2 left
            object2.x -= 0.0001;
            // std::cout << "Thread 2: Moving object 2 left. New position: " << object2.x << std::endl;
        }
        object2.shape.setPosition(object2.x, object2.y);
        // Unlock the mutex after updating object2's position
        positionMutex.unlock();
        // std::cout << "Thread 2: Position mutex unlocked" << std::endl;
        // Sleep for a short duration to avoid busy waiting
        // sleep(1);
    }
    return nullptr;
}

int main() {
    // Create SFML window
    sf::RenderWindow window(sf::VideoMode(800, 600), "SFML Window");

    // Initialize objects
    initializeObjects();
    // std::cout << "Objects initialized" << std::endl;

    // Create threads
    pthread_t thread1, thread2;
    // std::cout << "Creating threads..." << std::endl;
    pthread_create(&thread1, nullptr, moveObject1, nullptr);
    pthread_create(&thread2, nullptr, moveObject2, nullptr);

    // Game loop
    while (window.isOpen()) {
        // sleep(1);
        // std::cout << "Entering game loop..." << std::endl;
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            // Handle other events
            // Check for key presses and update direction variables
            if (event.type == sf::Event::KeyPressed) {
                // std::cout << "Key pressed..." << std::endl;
                if (event.key.code == sf::Keyboard::A) {
                    // std::cout << "A key pressed: Moving object 1 left" << std::endl;
                    dir1 = -1; // Move object 1 left
                } else if (event.key.code == sf::Keyboard::D) {
                    // std::cout << "D key pressed: Moving object 1 right" << std::endl;
                    dir1 = 1; // Move object 1 right
                } else if (event.key.code == sf::Keyboard::J) {
                    // std::cout << "J key pressed: Moving object 2 left" << std::endl;
                    dir2 = -1; // Move object 2 left
                } else if (event.key.code == sf::Keyboard::L) {
                    // std::cout << "L key pressed: Moving object 2 right" << std::endl;
                    dir2 = 1; // Move object 2 right
                }
            }
            if (event.type == sf::Event::KeyReleased) {
                // std::cout << "Key released..." << std::endl;
                if (event.key.code == sf::Keyboard::A || event.key.code == sf::Keyboard::D) {
                    // std::cout << "A/D key released: Stopping object 1" << std::endl;
                    dir1 = 0; // Stop object 1
                }
                if (event.key.code == sf::Keyboard::J || event.key.code == sf::Keyboard::L) {
                    // std::cout << "J/L key released: Stopping object 2" << std::endl;
                    dir2 = 0; // Stop object 2
                }
            }
        }

        // Lock the mutex before accessing object positions for drawing
        positionMutex.lock();
        // std::cout << "Position mutex locked by main thread" << std::endl;

        // Clear window
        window.clear();

        // Draw objects
        window.draw(object1.shape);
        window.draw(object2.shape);

        // Unlock the mutex after accessing object positions for drawing
        positionMutex.unlock();
        // std::cout << "Position mutex unlocked by main thread" << std::endl;

        // Display window contents
        window.display();
    }

    // Join threads
    // std::cout << "Joining threads..." << std::endl;
    pthread_join(thread1, nullptr);
    pthread_join(thread2, nullptr);

    return 0;
}
