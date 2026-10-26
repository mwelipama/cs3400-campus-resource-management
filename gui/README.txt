CS3400 Campus Resource & Activity Management System — Desktop GUI
=================================================================

WHAT THIS IS
  An OPTIONAL Windows desktop interface (Qt 6 Widgets, C++17) for the CS3400
  mini project. The graded submission is the original CONSOLE application in the
  parent folder; this GUI is an extra demonstration layer. It does NOT change or
  replace any of the graded files — it links the same data-structure and
  algorithm source (linked list, AVL tree, hash table, FIFO queue, priority
  heap, stack/undo, graph, sorting and searching), unmodified.

HOW TO BUILD (on a PC that has Qt 6 with the MinGW kit installed)
  1. Double-click  build.bat   (in this gui folder).
     It finds your Qt kit, compiles, bundles the Qt + MinGW runtime with
     windeployqt, and creates the portable folder below.
  2. If build.bat cannot find Qt automatically, open the
     "Qt 6.x (MinGW 64-bit)" command prompt from the Start menu, cd into this
     gui folder, and run:  build.bat

THE PORTABLE APP (what you give the lecturer)
  After building, copy this whole folder:
     gui\release\CS3400_Campus_Resource_Manager\
  onto a USB drive or any Windows PC and double-click:
     CampusResourceManager.exe
  No Qt, no compiler and no installer are required on that PC — the Qt runtime
  DLLs, the platforms\qwindows.dll plugin and the MinGW runtime DLLs are bundled
  inside the folder.

WHERE DATA IS STORED
  The app reads and writes resources.txt, requests.txt and locations.txt in the
  "data" sub-folder next to the executable, using the same text format as the
  console app. Use the "Save to files" button on the Resources page to persist
  changes. The folder is self-contained and uses no absolute paths.

SCREENS
  Dashboard        live counts from the real structures
  Resources        add / edit / remove / undo / sort / search (linear,binary,AVL,hash)
  Requests         FIFO normal queue + priority heap, process / cancel / return
  AVL Tree         four traversals, height, search with comparison counts
  Hash Table       real djb2 + separate-chaining statistics
  Campus Map       BFS, DFS, reachability, connected components, Dijkstra
  DS & Algorithms  live 1k/5k/10k benchmark of the real sorts and lookups
