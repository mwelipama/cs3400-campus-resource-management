# CS3400 Mini Project — Campus Resource and Activity Management System

A C++ console application for the National University of Lesotho that manages
campus library resources, user requests and campus locations. The core data
structures are implemented from scratch rather than using the STL containers,
as required by the project.

## Features

- Resource management (add, remove, update, display) held in a linked list.
- Searching (linear and binary) and sorting (insertion, merge, quick) of resources and requests.
- Request handling with a First-In-First-Out queue.
- Undo of recent operations using a stack.
- Priority request handling using a binary min-heap priority queue.
- Hierarchical organisation and lookup using a balanced (AVL) binary search tree.
- Fast resource retrieval using a hash table with separate chaining.
- Campus navigation modelled as a graph with breadth-first and depth-first
  traversal, reachability, connected components and Dijkstra shortest path.
- File persistence: data is loaded on start-up and saved on exit
  (`data/resources.txt`, `data/requests.txt`, `data/locations.txt`).
- Complexity reference table and a large-dataset benchmark (1000 / 5000 / 10000 items).

## Build

Requires a C++17 compiler (for example g++).

    g++ -std=c++17 *.cpp -o campus_system

## Run

    ./campus_system        # Linux / macOS
    campus_system.exe      # Windows

Sample data in `data/` is loaded automatically when the program starts and
written back when you exit through the menu.

## Project structure

    *.h / *.cpp     the data structures, algorithms and system logic
    main.cpp        program entry point and menu loop
    data/           sample resources, requests and locations
    docs/           technical report and project brief

## Group members

- M.E.C. Pama — 201404452
- L.V. Soai — 202322577
- R. Mphatle — 201902287
